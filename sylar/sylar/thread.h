#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>

// pthread_xxx   C
// std::thread, pthread 
namespace sylar {

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    
    // Constructor: takes a callback function and thread name
    Thread(std::function<void()> cb, const std::string& name); 

    ~Thread();

    // Get thread ID
    pid_t getId() const { return m_id; }
    
    // Use getName() when you're managing threads from "outside" and need to query a specific thread's name
    const std::string& getName() const { return m_name; }

    // Join the thread
    void join();
    
    // Static method to get current thread pointer
    static Thread* GetThis();
    
    // Static method to get current thread name (for debugging)
    static const std::string& GetName();    
    
    // Static method to set thread name
    static void SetName(const std::string& name);
    
private:
    // Disable copy constructors and assignment operator
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    // Static thread entry function
    static void* run(void* arg);
    
private:
    pid_t m_id = -1;           // Thread ID
    pthread_t m_thread = 0;    // POSIX thread handle
    std::function<void()> m_cb; // Thread callback function
    std::string m_name;        // Thread name
};

}

#endif