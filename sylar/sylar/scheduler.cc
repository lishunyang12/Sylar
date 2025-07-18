#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar {
static sylar::Logger::ptr g_logger = SYSLAR_LOG_NAME("system");

static thread_local Scheduler* current_scheduler = nullptr;
static thread_local Fiber* current_fiber = nullptr;

Scheduler::Scheduler(size_t worker_thread_count, bool use_caller_thread, const std::string& name) 
    :m_name(name) {
    SYLAR_ASSERT(worker_thread_count > 0);

    if(use_caller_thread) {
        sylar::Fiber::GetCurrentFiber();
        --worker_thread_count;

        SYLAR_ASSERT(GetCurrentScheduler() == nullptr);
        current_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this)));
        sylar::Thread::SetName(m_name);

        current_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = worker_thread_count;
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if(GetCurrentScheduler() == this) {
        current_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetCurrentScheduler() {
    return current_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return current_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;

    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                                        , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber 
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::State::TERM)
                || m_rootFiber->getState() == Fiber::State::INIT) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    bool exit_on_this_fiber = false;
    if(m_rootThread != -1) {
        SYLAR_ASSERT(GetCurrentScheduler() == this);   
    } else {
        SYLAR_ASSERT(GetCurrentScheduler() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }

    if(stopping()) {
        return;
    }
 
    // if(exit_on_this_fiber) {

    // }

}

void Scheduler::setCurrentScheduler() {
    current_scheduler = this;
}

void Scheduler::run() {
    setCurrentScheduler();
    if(sylar::GetThreadId() != m_rootThread) {
        current_fiber = Fiber::GetCurrentFiber().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                if(it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::State::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
            }
        }
        if(tickle_me) {
            tickle();
        }

        if(ft.fiber && ft.fiber->getState() != Fiber::State::TERM
                        && ft.fiber->getState() != Fiber::State::EXCEPT) {
            ++m_activeThreadCount;
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::State::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::State::TERM
                    && ft.fiber->getState() != Fiber::State::EXCEPT) {
                ft.fiber->setState(Fiber::State::HOLD);
            }
            ft.reset();
        } else if (ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            ++m_activeThreadCount;
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::State::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::State::EXCEPT
                    || cb_fiber->getState() == Fiber::State::TERM) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->setState(Fiber::State::HOLD);
                cb_fiber.reset();
            }
        } else {
            if(idle_fiber->getState() == Fiber::State::TERM) {
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::State::TERM
                    || idle_fiber->getState() != Fiber::State::EXCEPT) {
                idle_fiber->setState(Fiber::State::HOLD);
            }

        }
    }
}


}

/**
 * Scheduler Modes Comparison:
 * 
 * | Feature                | use_caller_thread=true          | use_caller_thread=false         |
 * |------------------------|---------------------------------|---------------------------------|
 * | Calling Thread         | Becomes a worker                | Only manages scheduler          |
 * | m_rootFiber            | Exists (runs scheduler loop)    | Does not exist                  |
 * | Thread Hierarchy       | Main thread + workers           | All threads equal               |
 * | Task Coordination      | Main thread orchestrates        | Threads compete freely          |
 * | Best Use Case          | Low-latency/mixed workloads     | Pure parallelism/simplicity     |
 * 
 * Note: Both modes use tickle() to wake idle threads on new tasks.
 */