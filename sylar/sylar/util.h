#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace sylar {
pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size, int skip);
std::string BacktraceToString(int size, int skip, const std::string& prefix = "");

}

#endif
