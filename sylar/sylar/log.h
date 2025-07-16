#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

/**
 * @def SYLAR_LOG_LEVEL(logger, level)
 * @brief Base logging macro for specified level
 * 
 * Usage: SYLAR_LOG_LEVEL(logger_ptr, level) << "message";
 * 
 * Only evaluates if logger's level threshold permits
 */
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
          __FILE__, __LINE__, 0, sylar::GetThreadId(), \
            sylar::GetFiberId(), time(0)))).getSS()

// Convenience macros for standard levels
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level)	\
		sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
			__FILE__, __LINE__, 0, sylar::GetThreadId(), \
		sylar::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @def SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)
 * @brief Formatted logging macro for specified level
 * 
 * Usage: SYLAR_LOG_FMT_LEVEL(logger, level, "format %s", arg)
 */
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)   SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)   SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

// Root logger access  
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name);

namespace sylar {

class Logger;
class LoggerManager;

/**
 * @enum LogLevel::level
 * @brief Log severity levels
 */
class LogLevel {
public:
    enum level {
        UNKNOWN = 0,
        DEBUG = 1,  ///< Debug-level messages
        INFO = 2,   ///< Informational messages
        WARN = 3,   ///< Warning conditions
        ERROR = 4,  ///< Error conditions
        FATAL = 5   ///< Fatal/critical conditions  
    };

    /// Convert level enum to string representation
    static const char* ToString(LogLevel::level level);
    static LogLevel::level FromString(const std::string str);
};

/**
 * @class LogEvent
 * @brief Contains all data for a single log event
 * 
 * Captures:
 * - Source location (file/line)
 * - Timing information
 * - Thread/fiber context
 * - Message content
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    
    /**
     * @brief Construct a new LogEvent
     * @param logger Owning logger
     * @param level Severity level
     * @param file Source file
     * @param line Source line
     * @param elapse Milliseconds since program start
     * @param thread_id OS thread ID
     * @param fiber_id Application fiber ID
     * @param time Unix timestamp
     */
    LogEvent(std::shared_ptr<Logger> Logger, LogLevel::level level, const char* file, int32_t m_line, uint32_t elapse, 
            uint32_t thread_id, uint32_t fiber_id, uint64_t time)
        : m_file(file), 
          m_line(m_line),
          m_elapse(elapse),
          m_threadId(thread_id),
          m_fiberId(fiber_id),
          m_time(time),
		  m_Logger(Logger),
		  m_level(level) {}

    const char* getFile() const { return m_file ? m_file : ""; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }

    std::string getContent() const { return m_ss.str(); }
	std::shared_ptr<Logger> getLogger() const { return m_Logger; }
	LogLevel::level getLevel() const { return m_level; }

    std::stringstream& getSS() { return m_ss; }
	void format(const char* fmt, ...);
	void format(const char* fmt, va_list al);

private:
    const char* m_file = nullptr; // file name 
    int32_t m_line = 0;          // line number
    uint32_t m_elapse = 0;       // the period of time in milliseconds from program start 
    uint32_t m_threadId = 0;     // thread id
    uint32_t m_fiberId = 0;      // fiber id
    uint64_t m_time = 0;         // time stamp    
    std::stringstream m_ss;   

	std::shared_ptr<Logger> m_Logger;  
	LogLevel::level m_level;
};

/**
 * @class LogEventWrap
 * @brief RAII wrapper for log events
 * 
 * Ensures log events are flushed when scope exits
 */
class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    
    std::stringstream& getSS();
    LogEvent::ptr getEvent() const;
    
private:
    LogEvent::ptr m_event;
};


/**
 * @class LogFormatter
 * @brief Formats log messages according to a specified pattern string
 *
 * The formatter converts log events into human-readable strings using
 * a pattern syntax similar to log4j/printf. Patterns contain literal
 * text and format specifiers that get replaced with log event data.
 */
class LogFormatter {
public:
    /// Shared pointer type for safe memory management
    typedef std::shared_ptr<LogFormatter> ptr;

    /**
     * @brief Construct a new LogFormatter with the specified pattern
     * @param pattern Format pattern string (e.g. "%d [%p] %m%n")
     *
     * Pattern syntax:
     *   %m - Message content
     *   %p - Log level (DEBUG/INFO/etc)
     *   %r - Elapsed time since program start
     *   %c - Logger name
     *   %t - Thread ID
     *   %n - Newline
     *   %d - Date/time (with optional format: %d{format})
     *   %f - Filename
     *   %l - Line number
     *   %F - Fiber ID
     *   %% - Percent sign
     */
    explicit LogFormatter(const std::string& pattern);

    /**
     * @brief Format a log event into a string according to the pattern
     * @param logger Source logger
     * @param level Log severity level
     * @param event Log event data
     * @return Formatted log message string
     */
    std::string format(std::shared_ptr<Logger> logger, 
                      LogLevel::level level, 
                      LogEvent::ptr event);

    /**
     * @class FormatItem
     * @brief Abstract base class for pattern format items
     *
     * Each format specifier (%d, %p etc) has a corresponding FormatItem
     * subclass that knows how to render that piece of log data.
     */
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        
        /**
         * @brief Construct a new FormatItem
         * @param fmt Optional format string for complex items (like date formatting)
         */
        explicit FormatItem(const std::string& fmt = "");
        
        virtual ~FormatItem() = default;
        
        /**
         * @brief Format this item's data into the output stream
         * @param os Output stream to write to
         * @param logger Source logger
         * @param level Log severity level
         * @param event Log event data
         */
        virtual void format(std::ostream& os,
                           std::shared_ptr<Logger> logger,
                           LogLevel::level level,
                           LogEvent::ptr event) = 0;
    };

    /**
     * @brief Initialize the formatter by parsing the pattern
     * @throws std::runtime_error if pattern is invalid
     *
     * Splits the pattern into literal text and format items.
     * Called automatically during construction.
     */
    void init();

    bool isError() const { return m_error; }
    const std::string getPattern() const { return m_pattern; }
private:
    std::string m_pattern;              ///< Original pattern string
    std::vector<FormatItem::ptr> m_items; ///< Parsed format items
    bool m_error = false;
};

/**
 * @class LogAppender
 * @brief Abstract base class for all log output destinations
 *
 * Defines the interface for log appenders that handle:
 * - Writing log messages to specific outputs (console, file, network, etc)
 * - Filtering by log level
 * - Formatting log events using a LogFormatter
 */
class LogAppender {
public:
    /// Shared pointer type for safe memory management
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Mutex MutexType;

    /// Virtual destructor for polymorphic deletion
    virtual ~LogAppender() = default;

    /**
     * @brief Set the formatter for this appender
     * @param val Formatter to use for converting log events to strings
     *
     * The formatter controls the textual representation of log messages.
     * Can be shared between multiple appenders.
     */
    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);

    /**
     * @brief Get the current formatter
     * @return Current log formatter
     */
    LogFormatter::ptr getFormatter();

    /**
     * @brief Get the minimum log level for this appender
     * @return Current log level threshold
     *
     * Only messages at or above this level will be processed.
     */
    LogLevel::level getLevel() { return m_level; }

    /**
     * @brief Set the minimum log level for this appender
     * @param val New log level threshold
     *
     * Example:
     *   appender->setLevel(LogLevel::WARN); // Only log warnings and errors
     */
    void setLevel(LogLevel::level val) { m_level = val; }

    /**
     * @brief Process a log event (pure virtual)
     * @param logger Source logger that generated the event
     * @param level Severity level of the log event
     * @param event The log event data to process
     *
     * Derived classes must implement this to:
     * 1. Apply level filtering (if not already done)
     * 2. Format the message (using m_formatter)
     * 3. Write to the appropriate output destination
     */
    virtual void log(std::shared_ptr<Logger> logger, 
                    LogLevel::level level, 
                    LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

protected:
    friend class Logger;
    /// Minimum log level for this appender (default: DEBUG)
    LogLevel::level m_level = LogLevel::DEBUG;
    bool m_hasFormatter = false;
    MutexType m_mutex;
    /// Formatter used to convert log events to strings
    LogFormatter::ptr m_formatter;
};

/**
 * Central logging class that handles message processing and routing.
 * Supports hierarchical naming, level filtering, and multiple output destinations.
 */
class Logger : public std::enable_shared_from_this<Logger> {
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Mutex MutexType;

    explicit Logger(const std::string& name = "root");
    
    void log(LogLevel::level level, LogEvent::ptr event);

    // Log methods for standard levels
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event); 
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    // Appender management
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders() { m_appenders.clear(); }
    // Level configuration
    LogLevel::level getLevel() const { return m_level; }
    void setLevel(LogLevel::level val) { m_level = val; }

    const std::string& getName() { return m_name; }

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);

    LogFormatter::ptr getFormatter();

    std::string ToYamlString();
private:
    friend class LoggerManager;
    std::string m_name;                 // Hierarchical name (e.g. "system.network")
    LogLevel::level m_level = LogLevel::DEBUG;
    MutexType m_mutex;
    std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;      // Default format for appenders without their own
    Logger::ptr m_root;                 // Fallback logger when no appenders are configured
};

/**
 * @class StdoutLogAppender
 * @brief Log appender that outputs to standard console stream
 *
 * Formats log messages and writes them to stdout/stderr
 * based on log level. Thread-safe for concurrent logging.
 */
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    
    /**
     * @brief Process and output a log event
     * @param logger Source logger instance
     * @param level Log severity level
     * @param event Log event data
     *
     * Writes formatted message to:
     * - stdout for DEBUG/INFO/WARN  
     * - stderr for ERROR/FATAL
     */
    virtual void log(std::shared_ptr<Logger> logger, 
                   LogLevel::level level,
                   LogEvent::ptr event) override;
    std::string toYamlString() override;
};

/**
 * Thread-safe file logging with rotation support.
 * Automatically flushes writes to disk.
 */
class FileLogAppender : public LogAppender {
public:
    using ptr = std::shared_ptr<FileLogAppender>;
    
    /**
     * Opens specified log file for writing.
     * @throws std::ios_base::failure if file cannot be opened
     */
    explicit FileLogAppender(const std::string& filename);
    
    virtual void log(std::shared_ptr<Logger> logger,
            LogLevel::level level,
            LogEvent::ptr event) override;

    /**
     * Reopens log file, typically after rotation.
     * @return false if file couldn't be reopened
     */
    bool reopen();
    std::string toYamlString() override;

private:
    std::string m_filename;
    std::ofstream m_filestream;
};

/**
 * @class LoggerManager
 * @brief Central registry and factory for logger instances
 *
 * Implements:
 * - Logger singleton management
 * - Hierarchical logger creation
 * - Default configuration
 */
class LoggerManager {
public:
    typedef Mutex Mutextype;
    /**
     * @brief Get or create named logger
     * @param name Logger name (dot-separated hierarchy)
     * @return Shared pointer to logger instance
     * 
     * Creates parent loggers automatically if needed.
     * Example: getLogger("system.network.tcp") creates 3 loggers.
     */
    Logger::ptr getLogger(const std::string& name);
    
    /**
     * @brief Initialize logging system
     *
     * Sets up:
     * - Root logger with default appenders  
     * - Default formatters
     * - System-wide log level
     */
    void init();
    
    /**
     * @brief Get root logger instance
     * @return Root logger shared pointer
     */
    Logger::ptr getRoot() const { return m_root; }

    std::string toYamlString();
private:
    friend class sylar::Singleton<LoggerManager>;

    Mutextype m_mutex;
    LoggerManager();  ///< Private constructor for singleton
    ~LoggerManager() = default;
    
    std::map<std::string, Logger::ptr> m_loggers; ///< Logger registry
    Logger::ptr m_root;                           ///< Root logger instance
};

/// Global singleton accessor for LoggerManager
typedef sylar::Singleton<LoggerManager> LoggerMgr;

};

#endif

