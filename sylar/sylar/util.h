#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

namespace sylar {
pid_t GetThreadId();

uint32_t GetFiberId();
}

#endif