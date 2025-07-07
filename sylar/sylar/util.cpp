#include "util.h"

namespace sylar {
pid_t GetThreadId() {
    return gettid();
}

uint32_t GetFiberId() {
    return 0;
}
}