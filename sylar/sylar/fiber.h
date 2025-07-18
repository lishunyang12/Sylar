#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <ucontext.h>  // For context switching
#include "thread.h"
#include <functional>

namespace sylar {

/**
 * Lightweight user-space thread (Fiber) implementation 
 * using ucontext for context switching.
 * Inherits from enable_shared_from_this for safe shared_ptr handling.
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;  // Self reference

    /**
     * Fiber state machine:
     * - INIT:  Freshly created, not yet scheduled
     * - HOLD:  Paused, can be resumed later
     * - EXEC:  Currently executing
     * - TERM:  Finished execution
     * - READY: Ready to be scheduled
     */
    enum class State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };



private:
    Fiber();  // Private default constructor

public:
    /**
     * Create a fiber with execution callback
     * @param cb        Callback function to execute
     * @param stacksize Custom stack size (0=default)
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    
    ~Fiber();  // Releases allocated stack memory

    /**
     * Reset fiber with new callback (only valid in INIT/TERM states)
     * @param cb New callback function
     */
    void reset(std::function<void()> cb);

    /// Switch execution to this fiber
    void swapIn();

    /// Yield execution back to scheduler
    void swapOut();

    uint64_t getId() const { return m_id; }

public:
    static void SetCurrentFiber(Fiber* f);
    friend Fiber::ptr GetCurrentFiber();

    /// Yield execution and set state to READY
    static void YieldToReady();

    /// Yield execution and set state to HOLD
    static void YieldToHold();

    /// Get total alive fiber count
    static uint64_t TotalFibers();

    /// Internal entry point that wraps m_cb execution
    static void MainFunc();

    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;          // Unique fiber ID
    uint32_t m_stacksize = 0;   // Stack size in bytes
    State m_state = State::INIT;       // Current state

    ucontext_t m_ctx;           // Execution context
    void* m_stack = nullptr;    // Allocated stack memory

    std::function<void()> m_cb; // Callback to execute
};

} // namespace sylar

#endif

// [START]
// ↓
// new Fiber(callback) → allocates stack → sets up ucontext → INIT
// ↓
// reset(cb) → READY
// ↓
// swapIn() → EXEC → executes m_cb
// ↙️               ↓                ↘️
// YieldToHold()   m_cb completes   YieldToReady()
// ↓                    ↓                  ↓
// HOLD                TERM              READY
// ↓                    ↙️                ↖️
// swapIn() ←←←←←← reset(cb) ←←←←←←←←←←←←←┘
// ↓
// EXEC (repeats cycle)
// ↓
// [TERM without reset] → ~Fiber() → destroys stack → [END]