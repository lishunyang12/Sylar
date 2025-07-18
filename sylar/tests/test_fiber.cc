#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

// The coroutine function that will execute in a non-root fiber
void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";  // [3] First output when fiber starts
    sylar::Fiber::YieldToHold();  // [4] Yield execution back to root fiber (main)
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";    // [6] Resumes and prints before terminating
    // [7] Implicit return here will automatically switch back to root fiber
    //     due to uc_link pointing to root_fiber's context
}

void test_fiber() {
SYLAR_LOG_INFO(g_logger) << "main begin -1";  // [2] First output
    {
        sylar::Fiber::GetCurrentFiber();
        // Create a new fiber with run_in_fiber as its entry function
        // This fiber will have uc_link pointing to root_fiber's context
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        
        // [First swapIn]
        // - Saves root_fiber's context (main's execution point here)
        // - Switches to fiber's context (starts run_in_fiber)
        fiber->swapIn();  
        
        // [5] Execution resumes here after first YieldToHold()
        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        
        // [Second swapIn]
        // - Switches back to fiber's context (resumes after YieldToHold)
        fiber->swapIn();
    }
    // [8] Execution resumes here after fiber terminates
    SYLAR_LOG_INFO(g_logger) << "main end";
}

int main(int argc, char** argv) {
    // [1] Initialize the root fiber for main thread
    // This captures main's execution context into t_rootFiber
    sylar::Thread::SetName("main");
    
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(sylar::Thread::ptr(new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    for(auto i : thrs) {
        i->join();
    }

    return 0;
}