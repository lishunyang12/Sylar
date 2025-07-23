## 1.Logging Module
Supports both stream-style and printf-style logging，Customizable log formats, log levels, and log separation.
Stream-style usage：SYLAR_LOG_INFO(g_logger) << "this is a log";
Formatted usage：SYLAR_LOG_FMT_INFO(g_logger, "%s", "this is a log");
Configurable fields: timestamp, thread ID, thread name, log level, logger name, filename, line number

## 2.Configuration Module
Convention-over-configuration design
YAML-based configuration with change notifications
Supports hierarchical data types and STL containers (vector, list, set, map)),
Custom type support through serialization/deserialization
```cpp
static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
	sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
```
A TCP connection timeout parameter has been defined. You can directly use g_tcp_connect_timeout->getValue()
to obtain the parameter's value. When the configuration is modified and reloaded, 
this value will be automatically updated.

The configuration format is as follows:
```
tcp:
    connect:
            timeout: 10000
```
## 3. Thread Module
The thread module encapsulates some commonly used functionalities from pthread, 
including objects such as Thread, Semaphore, Mutex, RWMutex,
and Spinlock, facilitating daily thread operations in development.

Why not use C++11's std::thread?
Although this framework is developed using C++11, 
we chose not to use std::thread because it is ultimately implemented based on pthread. 
Additionally, C++11 does not provide certain synchronization primitives like read-write mutexes 
(RWMutex) and Spinlock, which are essential in high-concurrency scenarios. 
Therefore, we opted to encapsulate pthread ourselves to meet these requirements.

## 4. Coroutine
Coroutines are user-mode threads, akin to "threads within threads," 
but much more lightweight. By integrating with socket hooking, 
complex asynchronous calls can be encapsulated into synchronous operations,
significantly reducing the complexity of business logic implementation.

-Current Implementation:
The current coroutine implementation is based on ucontext_t.

Future Support
In the future, we will also support implementation using fcontext_t from
Boost.Context, providing higher performance and flexibility.
