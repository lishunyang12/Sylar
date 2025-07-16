#include "sylar/sylar.h"
#include <unistd.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
sylar::RWMutex s_mutex;

volatile int count = 0;

void func1() {
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id " << sylar::Thread::GetThis()->getId(); 
    for(int i = 0; i < 1000000; ++i) {
        sylar::RWMutex::WriteLock lock(s_mutex);
        ++count;
    }
}

void func2() {

}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "Thread test begin";
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i) {
        sylar::Thread::ptr thr(new sylar::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "Thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;
    return 0;
}