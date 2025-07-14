#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <pthread.h>

// pthread_xxx   pre C++
// std::thread, pthread 
namespace sylar {

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name); 
    ~Thread();
    typedef pid_t tid_t;  // or pthread_t if you prefer

    tid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void join();
    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;
private:
    tid_t m_id = -1;           // Thread ID
    pthread_t m_thread = 0;   // POSIX thread handle
    std::function<void()> m_cb;
    std::string m_name;
};

}

#endif