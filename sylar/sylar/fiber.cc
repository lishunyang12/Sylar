#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>
#include "scheduler.h"
#include "log.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_currentFiber = nullptr;          // observe Current fiber
static thread_local Fiber::ptr t_rootFiber = nullptr;         // Own Root fiber

static ConfigVar<uint32_t>::ptr g_default_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024* 1024, "fiber stack size");


class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_currentFiber) {
        return t_currentFiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = State::EXEC;
    SetCurrentFiber(this);

    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize) 
    :m_id(++s_fiber_id) 
    ,m_cb(cb) {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_default_fiber_stack_size->getValue();
    
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    if(this == t_rootFiber.get()) {
        m_ctx.uc_link = nullptr;  
    } else {
        m_ctx.uc_link = t_rootFiber->getContext();
    }
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id" << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        // non-root fiber
        SYLAR_ASSERT(m_state != State::EXEC);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        // root fiber
        Fiber* cur = t_currentFiber;
        if(cur == this) {
            SetCurrentFiber(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;
} 

// INIT TERM
void Fiber::reset(const std::function<void()>& cb) {
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == State::TERM 
            || m_state == State::INIT
            || m_state == State::EXCEPT);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    if(this == t_rootFiber.get()) {
        m_ctx.uc_link = nullptr;  
    } else {
        m_ctx.uc_link = t_rootFiber->getContext();
    }

    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = State::INIT;
}

void Fiber::call() {
    m_state = State::EXEC;
    if(swapcontext(&(t_rootFiber->m_ctx), &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapIn() {
    SetCurrentFiber(this);
    SYLAR_ASSERT(m_state != State::EXEC);
    m_state = State::EXEC;

    if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext"); 
    }
}

void Fiber::swapOut() {
    if(t_currentFiber != Scheduler::GetMainFiber()) {
        SetCurrentFiber(Scheduler::GetMainFiber());
        if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        } 
    } else {
        SetCurrentFiber(t_rootFiber.get());
        if(swapcontext(&m_ctx, &t_rootFiber->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
}


Fiber::ptr Fiber::GetCurrentFiber() {
    if(t_currentFiber) {
        return t_currentFiber->shared_from_this();
    }

    // Lazy Initialization， create main fiber with emptry callback
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_currentFiber = main_fiber.get());
    t_rootFiber = main_fiber;
    return main_fiber;
}

void Fiber::SetCurrentFiber(Fiber* f) {
    t_currentFiber = f;
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = GetCurrentFiber();
    cur->m_state = State::READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = GetCurrentFiber();
    cur->m_state = State::HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetCurrentFiber();
    SYLAR_ASSERT(cur);
    try {
        if(cur->m_cb) {
            cur->m_cb();
        } else {
            SYLAR_LOG_WARN(g_logger) << "Fiber with empty callback, id=" << cur->getId();
        }
        cur->m_cb = nullptr;
        cur->m_state = State::TERM;
    } catch (std::exception& ex) {
        cur->m_state = State::EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Execpt: " << ex.what()
                    << " fiber_id=" << cur->getId()
                    << std::endl
                    << sylar::BacktraceToString();
    } catch(...) {
        cur->m_state = State::EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Execpt"
         << " fiber_id=" << cur->getId()
         << std::endl
         << sylar::BacktraceToString();
    }
    
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
}

}
// 1. t_fiber (Thread-Local Storage)
//     What it is: A thread-local pointer (thread_local Fiber* t_fiber)
//     Purpose: Tracks the currently executing fiber for each thread
//     Behavior:
//         When a regular fiber runs: t_fiber points to that fiber
//         When no fiber is active: t_fiber points to the thread's root fiber
//     Key invariant: Always non-null (at minimum, points to the root fiber)
// 2. Root Fiber
//     What it is: A special fiber representing the thread's default execution context
//     Characteristics:
//         Has no stack allocation (uses the thread's original stack)
//         Has no callback function (m_cb = nullptr)
//         Always in EXEC state
//         Created implicitly when Fiber::GetThis() is first called on a thread
//     Purpose:
//         Provides a fallback context when fibers yield
//         Ensures every thread always has a valid context to return to