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

#define SYLAR_LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
		sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
		  __FILE__, __LINE__, 0, sylar::GetThreadId(), \
			sylar::GetFiberId(), time(0)))).getSS()

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
	
#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)   SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)   SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar {

class Logger;
class LoggerManager;

// log level
class LogLevel {
public:
	enum level {
		UNKNOWN = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5	
	};

	static const char* ToString(LogLevel::level level);
	static LogLevel::level FromString(const std::string& str);
};

class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
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

class LogEventWrap {
public:
	LogEventWrap(LogEvent::ptr m);
	~LogEventWrap();

	std::stringstream& getSS();
	LogEvent::ptr getEvent() const;
private:
	LogEvent::ptr m_event;
};

// logformatter
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	LogFormatter(const std::string& pattern);

	// %t	%thread_Id %m%n
	std::string format(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event);
public:
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		FormatItem(const std::string& fmt = "");
		virtual ~FormatItem() {}
		virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) = 0;
	};

	void init();

	bool isError() {
		return m_error;
	}

private:
	bool m_error = false;
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;
};

// logger destination 
class LogAppender {
public:
	typedef std::shared_ptr<LogAppender> ptr;
	virtual ~LogAppender() {};
	
	void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
	LogFormatter::ptr getFormatter() const { return m_formatter; }

	LogLevel::level getLevel() { return m_level; }
	void setLevel(LogLevel::level val) { m_level = val; }
	virtual void log(std::shared_ptr<Logger> Logger, LogLevel::level sslevel, LogEvent::ptr event) = 0;

protected:
	LogLevel::level m_level = LogLevel::DEBUG; //debug
	LogFormatter::ptr m_formatter;  
};

// logger
class Logger :public std::enable_shared_from_this<Logger> {
public:
	typedef std::shared_ptr<Logger> ptr;

	Logger(const std::string& name = "root");	
	void log(LogLevel::level, LogEvent::ptr event);

	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);

	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
	void clearAppenders();
	LogLevel::level getLevel() const { return m_level; }
	void setLevel(LogLevel::level val) { m_level = val; }

	void setFormatter(LogFormatter::ptr val);
	void setFormatter(const std::string& val);
	LogFormatter::ptr getFormatter();

	const std::string& getName() { return m_name; }



private:
	friend class LogManager;
	std::string m_name;	 			 				// logger's name
	LogLevel::level m_level; 			 			// log level  debug
	std::list<LogAppender::ptr> m_appenders;        // appender list
	LogFormatter::ptr m_formatter;					// share with appender in appenders of it doesn't have formatter
	Logger::ptr m_root;
};

// Appender that sends output to console
class StdoutLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	StdoutLogAppender() {
        // 默认格式：时间+线程+文件名+行号+消息
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S} [%t] %f:%l %m%n"));
    }
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override;
};

// Appender that sends output to file
class FileLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<FileLogAppender> ptr;
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override;
	FileLogAppender(const std::string& filename);

	// reopen file, return true if successful, false if failed
	bool reopen();

private:
	std::string m_filename;			// filename
	std::ofstream m_filestream;		// file stream
};

class LogManager {
public:
	Logger::ptr getLogger(const std::string& name);
	void init();
	Logger::ptr getRoot() const { return m_root; }
private:
	friend class sylar::Singleton<LogManager>;

	std::map<std::string, Logger::ptr> m_loggers;
	Logger::ptr m_root;

	LogManager();
	~LogManager() = default;
};

typedef sylar::Singleton<LogManager> LoggerMgr;

};

#endif