#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>

namespace sylar {
static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024* 1024*, "fiber stack size");


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

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize = 0) 
    :m_id(++s_fiber_id) 
    ,m_cb(cb) {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    
    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        SYLAR_ASSERT(m_state == TERM || m_state == INIT );

        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
} 

void reset(std::function<void()> cb);

void swapIn();

void swapOut();


static Fiber::ptr GetThis();

void SetThis(Fiber* f);

static void YieldToReady();

static void YieldToHold();

static uint64_t TotalFibers();

static void MainFunc();
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