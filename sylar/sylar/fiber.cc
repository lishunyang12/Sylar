#include "fiber.h"

namespace sylar {
Fiber(std::function<void()> cb, size_t stacksize = 0);

~Fiber();  


void reset(std::function<void()> cb);

void swapIn();

void swapOut();


static Fiber::ptr GetThis();

static void YieldToReady();

static void YieldToHold();

static uint64_t TotalFibers();

static void MainFunc();
}