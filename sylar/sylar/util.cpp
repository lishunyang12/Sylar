#include "util.h"
#include <execinfo.h>
#include "log.h"

namespace sylar {

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() {
    return gettid();
}

uint32_t GetFiberId() {
    return 0;
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** callstack = (void**)malloc((sizeof(void*) * size));
    size_t frames = ::backtrace(callstack, size);
    
    char** strings = backtrace_symbols(callstack, frames);
    if(strings == NULL) {
        SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }

    for(size_t i = skip; i < frames; ++i) {
        bt.push_back(strings[i]);
    }
    free(strings);
    free(callstack);
}

std::string BacktraceToString(int size, int skip);

}
