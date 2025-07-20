#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_current_scheduler = nullptr;
static thread_local Fiber* t_root_fiber = nullptr;

Scheduler::Scheduler(size_t worker_thread_count, bool use_caller_thread, const std::string& name) 
    :m_name(name), 
     m_threadCount(worker_thread_count),
     m_rootThread(-1) 
    {
   
    SYLAR_ASSERT2(worker_thread_count > 0, "Worker thread count should be positive");

    if(use_caller_thread) {
        SYLAR_LOG_INFO(g_logger) << "choose use call mode";
       initializeCallerThreadContext();  
    } else {
        SYLAR_LOG_INFO(g_logger) << "choose non use call mode";
    }
}

void Scheduler::initializeCallerThreadContext() {
    sylar::Fiber::GetCurrentFiber();
    --m_threadCount;  // 使用成员变量更清晰

    SYLAR_ASSERT2(GetCurrentScheduler() == nullptr, "Current scheduler should be null");
    t_current_scheduler = this;

    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this)));
    sylar::Thread::SetName(m_name);

    t_root_fiber = m_rootFiber.get();
    m_rootThread = sylar::GetThreadId();
    SYLAR_LOG_INFO(g_logger) << "root thread is set id: " + m_rootThread 
                            << "current root fiber is set";

    m_threadIds.push_back(m_rootThread);
}



Scheduler::~Scheduler() {
    // SYLAR_ASSERT2(stopping(), 
    //     "Scheduler must be fully stopped before destruction (threads running or tasks pending)");
    if(GetCurrentScheduler() == this) {
        if(m_rootFiber && m_rootFiber->getState() == Fiber::State::EXEC) {
            m_rootFiber->setState(Fiber::State::TERM);
        }
        t_current_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetCurrentScheduler() {
    return t_current_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_root_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    
    if (isRunning()) {
        return;
    }
    
    prepareWorkerThreads();
    lock.unlock();
    startRootFiberIfNeeded();
}

bool Scheduler::isRunning() const {
    return !m_stopping;
}

void Scheduler::prepareWorkerThreads() {
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.reserve(m_threadCount);  // 预分配空间更高效
    for (size_t i = 0; i < m_threadCount; ++i) {
        createAndAddWorkerThread(i);
    }
}

void Scheduler::createAndAddWorkerThread(size_t index) {
    m_threads.emplace_back(
        new Thread(std::bind(&Scheduler::run, this),
                  m_name + "_" + std::to_string(index)));
    m_threadIds.push_back(m_threads.back()->getId());
}

void Scheduler::startRootFiberIfNeeded() {
    if (m_rootFiber) {
        m_rootFiber->call();
        SYLAR_LOG_INFO(g_logger) << "call out";
    }
}


void Scheduler::stop() {
    m_autoStop = true;
    
    if (canStopImmediately()) {
        return;
    }

    validateStopConditions();
    signalAllThreadsToStop();

    while(!stopping()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    cleanupAfterStop();
}

bool Scheduler::canStopImmediately() const {
    return m_rootFiber && 
           m_threadCount == 0 && 
           (m_rootFiber->getState() == Fiber::State::TERM || 
            m_rootFiber->getState() == Fiber::State::INIT);
}

void Scheduler::validateStopConditions() {
    if (m_rootThread != -1) {
        SYLAR_ASSERT(GetCurrentScheduler() == this);
    } else {
        SYLAR_ASSERT(GetCurrentScheduler() != this);
    }
}

void Scheduler::signalAllThreadsToStop() {
    m_stopping = true;
    
    // 通知工作线程
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    
    // 通知根fiber
    if (m_rootFiber) {
        tickle();
    }
}

void Scheduler::cleanupAfterStop() {
    if (stopping()) {
        return;
    }

    clearPendingTasks();
    joinAllWorkerThreads();
}

void Scheduler::clearPendingTasks() {
    MutexType::Lock lock(m_mutex);
    m_fibers.clear();
}

void Scheduler::joinAllWorkerThreads() {
    for (auto& t : m_threads) {
        t->join();
    }
    m_threads.clear();
}



void Scheduler::setCurrentScheduler() {
    t_current_scheduler = this;
}



void Scheduler::run() {
    SYLAR_LOG_INFO(g_logger) << "run";
    // Initialize scheduler context for current thread
    initializeSchedulerContext();

    // Prepare idle fiber and callback fiber
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    SYLAR_LOG_INFO(g_logger) << "idle fiber created";
    // idle fiber created
    Fiber::ptr cb_fiber;

    while (true) {
        // Try to fetch a task from queue
        SYLAR_LOG_INFO(g_logger) << "start to fetch task....";
        FiberAndThread task = fetchTask();

        // Handle different task types
        if (task.fiber) {
            SYLAR_LOG_INFO(g_logger) << "fiber to be scheduled";
            handleFiberTask(task.fiber);
        } else if (task.cb) {
            SYLAR_LOG_INFO(g_logger) << "function to be scheduled";
            handleCallbackTask(task.cb, cb_fiber);
        } else {
            // No tasks available, run idle logic
            if (checkIdleTermination(idle_fiber)) {
                break;
            }
            runIdleLogic(idle_fiber);
        }
    }
}


void Scheduler::initializeSchedulerContext() {
    setCurrentScheduler();
    if (sylar::GetThreadId() != m_rootThread) {
        t_root_fiber = Fiber::GetCurrentFiber().get();
    }
}


Scheduler::FiberAndThread Scheduler::fetchTask() {
    FiberAndThread task;
    bool need_notify = false;

    {
        MutexType::Lock lock(m_mutex);
        SYLAR_LOG_INFO(g_logger) << "Current queue size: " << m_fibers.size();  // 打印队列大小
        for (auto it = m_fibers.begin(); it != m_fibers.end(); ) {
            // Skip tasks not assigned to current thread
            if (shouldSkipTask(*it, need_notify)) {
                ++it;
                continue;
            }

            // Found executable task
            task = *it;
            m_fibers.erase(it);
            SYLAR_LOG_INFO(g_logger) << "Task fetched from queue";
            break;
        }
    }

    if (need_notify) {
        tickle();
    }

    return task;
}

bool Scheduler::shouldSkipTask(const FiberAndThread& ft, bool& need_notify) const {
    if (ft.thread != -1 && ft.thread != sylar::GetThreadId()) {
        need_notify = true;  // 任务绑定到其他线程，需要通知
        return true;
    }
    if (ft.fiber && ft.fiber->getState() == Fiber::State::EXEC) {
        need_notify = false;  // 任务已在执行，无需通知
        return true;
    }
    return false;  // 不跳过任务XEC);
}

void Scheduler::handleFiberTask(Fiber::ptr fiber) {
    SYLAR_ASSERT(fiber);
    
    ++m_activeThreadCount;
    fiber->swapIn();
    --m_activeThreadCount;

    // Handle post-execution state
    switch (fiber->getState()) {
        case Fiber::State::READY:
            schedule(fiber);  // Reschedule if fiber yields with READY
            break;
        case Fiber::State::TERM:
        case Fiber::State::EXCEPT:
            break;  // Fiber completed
        default:
            fiber->setState(Fiber::State::HOLD);  // Pause fiber
    }
}

void Scheduler::handleCallbackTask(std::function<void()>& cb, Fiber::ptr& cb_fiber) {
    // Reuse or create callback fiber
    if (!cb_fiber) {
        cb_fiber.reset(new Fiber(cb));
    } else {
        cb_fiber->reset(cb);
    }

    ++m_activeThreadCount;
    cb_fiber->swapIn();
    --m_activeThreadCount;

    // Handle post-execution state
    switch (cb_fiber->getState()) {
        case Fiber::State::READY:
            schedule(cb_fiber);  // Reschedule
            cb_fiber.reset();
            break;
        case Fiber::State::TERM:
        case Fiber::State::EXCEPT:
            cb_fiber->reset(nullptr);  // Clean up
            break;
        default:
            cb_fiber->setState(Fiber::State::HOLD);
            cb_fiber.reset();
    }
}

bool Scheduler::checkIdleTermination(const Fiber::ptr& idle_fiber) const {
    if (idle_fiber->getState() == Fiber::State::TERM) {
        SYLAR_LOG_INFO(g_logger) << "idle fiber terminated";
        return true;
    }
    SYLAR_LOG_INFO(g_logger) << "idle fiber not terminated";
    return false;
}

void Scheduler::runIdleLogic(Fiber::ptr& idle_fiber) {
    ++m_idleThreadCount;
    idle_fiber->swapIn();
    --m_idleThreadCount;

    if (idle_fiber->getState() != Fiber::State::TERM && 
        idle_fiber->getState() != Fiber::State::EXCEPT) {
        idle_fiber->setState(Fiber::State::HOLD);
    }
}


void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
            && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
}


}

// [Initialization Phase]
// Main Thread 
//   → Creates Scheduler(3) 
//     → Initializes m_rootFiber (EXEC state) 
//     → Sets current_fiber = m_rootFiber
//     → Prepares 3 worker threads (not yet started)

// [Startup Phase]
// Main Thread 
//   → start() 
//     → Spawns Worker Threads 1-3 
//       → Each worker:
//         → Creates thread-local main fiber 
//         → Enters run() loop
//     → Main Thread executes m_rootFiber->call() 
//       → Switches to scheduler context

// [Task Scheduling Phase]
// Main Thread 
//   → schedule(test_fiber) 
//     → (Lock) Adds task to m_fibers queue 
//     → (Unlock) 
//     → If queue was empty: → tickle() → Wakes idle workers

// [Task Execution Phase]
// Worker Thread (any) 
//   → run() loop iteration
//     → (Lock) Finds eligible task → (Unlock)
//     → For fiber task:
//       → INIT → EXEC (swapIn) 
//         → Executes test_fiber() 
//         → TERM (swapOut) → Returns to scheduler
//     → For callback task: 
//       → Creates temp fiber → Same flow as above

// [Shutdown Phase]
// Main Thread 
//   → stop()
//     → Sets m_stopping=true
//     → tickle() all workers
//     → Waits for:
//       → m_fibers.empty() 
//       → m_activeThreadCount==0
//     → Workers terminate run() loops
//     → Cleans up all fibers

// [Key State Transitions]
// Fiber Lifecycle:
// INIT → READY → EXEC → (TERM|EXCEPT|HOLD)
//         ↑_____________|

// Thread States:
// CREATED → RUNNING → (EXECUTING) → SHUTTING_DOWN → TERMINATED