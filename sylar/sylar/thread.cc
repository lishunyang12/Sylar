#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

// Thread-local storage for current thread pointer
static thread_local Thread* t_thread = nullptr;

// Thread-local storage for current thread name
static thread_local std::string t_thread_name = "UNKNOWN";

// Logger instance for thread-related messages
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

// Get current thread object
Thread* Thread::GetThis() {
    return t_thread;
}

// Get current thread name
const std::string& Thread::GetName() {
    return t_thread_name;
}

// Set thread name
void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    } 
    t_thread_name = name;
}

// Thread constructor
Thread::Thread(std::function<void()> cb, const std::string& name) 
    : m_cb(cb), m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    // Create POSIX thread
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt    
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
}

// Thread destructor
Thread::~Thread(){
    if(m_thread) {
        // Detach thread if not joined
        pthread_detach(m_thread);
    }
}

// Join thread
void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt    
                << " name=" << m_name;
            throw std::logic_error("pthread_create error");
        }
        m_thread = 0;
    }
}

// Static thread entry function
void* Thread::run(void *arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;  // Set thread-local thread pointer
    
    // Get and set thread ID
    thread->m_id = sylar::GetThreadId();
    
    // Set thread name (limited to 15 characters)
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    t_thread_name = thread->m_name;
    // Swap the callback to local variable to ensure it's called only once
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    // Execute the callback
    cb();
    
    return 0; 
}

} 