#include "fiber.h"

namespace sylar {

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    
};

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