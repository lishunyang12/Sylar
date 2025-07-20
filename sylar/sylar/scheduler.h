#ifndef __SYLAR_SCHEDULER_H__  
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"
#include "fiber.h"
#include "log.h"



namespace sylar {

static sylar::Logger::ptr g_logger_hello = SYLAR_LOG_NAME("system");

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();
    
    const std::string& GetName() const { return m_name; }

    static Scheduler* GetCurrentScheduler();
    static Fiber* GetMainFiber();

    void start();
    bool isRunning() const;
    void prepareWorkerThreads();
    void createAndAddWorkerThread(size_t index);
    void startRootFiberIfNeeded();
    
    void stop();
    bool canStopImmediately() const;
    void validateStopConditions();
    void signalAllThreadsToStop();
    void cleanupAfterStop();
    void clearPendingTasks();
    void joinAllWorkerThreads();


    template<class FiberOrCb> 
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        
        if(need_tickle) {
            tickle();
        }
    }
    
    template<class InputIterator> 
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }

    void initializeCallerThreadContext();

private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f), thread(thr) {}
        
        FiberAndThread(Fiber::ptr* f, int thr) 
            :thread(thr) {
                fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr) 
            :cb(f), thread(thr) {   
        }

        FiberAndThread(std::function<void()>* f, int thr)
            :thread(thr) {
                cb.swap(*f);
        }

        FiberAndThread() 
            :thread(-1) {
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }

        bool isFiberTask() const { return fiber != nullptr; }
        bool isCallbackTask() const { return cb != nullptr; }
    };

protected:
    void run();
    // Task fetching and handling
    FiberAndThread fetchTask();
    // Initialize scheduler context for current thread
    bool shouldSkipTask(const FiberAndThread& ft, bool& need_notify)const;
    void handleFiberTask(Fiber::ptr fiber);
    void handleCallbackTask(std::function<void()>& cb, Fiber::ptr& cb_fiber);
    bool checkIdleTermination(const Fiber::ptr& idle_fiber) const;
    void runIdleLogic(Fiber::ptr& idle_fiber);

    // Initialization
    void initializeSchedulerContext();
    void setCurrentScheduler();

    // Virtual methods
    virtual void tickle();
    virtual bool stopping();
    virtual void idle();


private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        SYLAR_LOG_INFO(g_logger_hello) << "schedule task starting...";
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        SYLAR_LOG_INFO(g_logger_hello) << "the number of fibers to be scheduled: " << m_fibers.size();
        return need_tickle;
    }



private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;
    std::string m_name;

protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true;
    bool m_autoStop = false;
    int m_rootThread = 0;
};



}

#endif