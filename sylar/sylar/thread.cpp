#include "thread.h"

namespace sylar {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOWN";

Thread* Thread::GetThis() {
    return t_thread;
}

static const std::string& GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    } 
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name) {

}

Thread::~Thread(){

}

void join() {

}

}