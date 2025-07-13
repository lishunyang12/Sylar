#include "log.h"
#include "map"
#include "functional"
#include "iostream"

namespace sylar {

const char* LogLevel::ToString(LogLevel::level level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
    default:
        return "UNKNOWN";
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr m) 
    :m_event(m){

}
LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}

LogEvent::ptr LogEventWrap::getEvent() const {
	return m_event;
}

LogFormatter::FormatItem::FormatItem(const std::string& fmt){}

class MessageFormatItem: public LogFormatter::FormatItem {
	public:
        MessageFormatItem(const std::string str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
};

class LevelFormatItem: public LogFormatter::FormatItem {
	public:    
        LevelFormatItem(const std::string str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }
};

class ElapseFormatItem: public LogFormatter::FormatItem {
    public:
        ElapseFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }
};

class NameFormatItem: public LogFormatter::FormatItem {
    public:
        NameFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << event->getLogger()->getName();
        }
};


class ThreadIdFormatItem: public LogFormatter::FormatItem {
    public:
        ThreadIdFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
                os << event->getThreadId();
        }
};


class FiberFormatItem: public LogFormatter::FormatItem {
    public:
        FiberFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
};

class DateTimeFormatItem: public LogFormatter::FormatItem {
    public:
        DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
            :m_format{format} {
                if(m_format.empty()) {
                    m_format = "%Y-%m-%d %H:%M:%S";
                } 
            }
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
                struct tm tm;
                time_t time = event->getTime();
                localtime_r(&time, &tm);
                char buf[64];
                strftime(buf, sizeof(buf), m_format.c_str(), &tm);
                os << buf;
        }
    private:
        std::string m_format;
};


class LineFormatItem: public LogFormatter::FormatItem {
    public:
        LineFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << event->getLine();
        }
};

class FileFormatItem: public LogFormatter::FormatItem {
    public:
        FileFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
                os << event->getFile();
        }
};

class StringFormatItem: public LogFormatter::FormatItem {
    public:
        StringFormatItem(const std::string &str)
            :FormatItem(str), m_string(str) {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
            os << m_string;
        }
    private:
        std::string m_string;
};


class NewLineFormatItem: public LogFormatter::FormatItem {
    public:
        NewLineFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
                os << "\n";
        }
};

class TabFormatItem: public LogFormatter::FormatItem {
    public:
        TabFormatItem(const std::string str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) override {
                os << "\t";
        }
};

Logger::Logger(const std::string& name) 
    : m_name(name) 
    , m_level(LogLevel::DEBUG){
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}
//%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T%[p]%T%[%c]%T%f:%l%T%m%n

void Logger::addAppender(LogAppender::ptr appender) {
    if(!appender->getFormatter()){
            appender->setFormatter(m_formatter);
    }
    Logger::m_appenders.push_back(appender); 
};
void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = Logger::m_appenders.begin(); it != Logger::m_appenders.end(); ++it) {  
        if(*it == appender) {
                m_appenders.erase(it);
                break;
        }   
    }
}

void Logger::log(LogLevel::level level, LogEvent::ptr event) {
    if(level >= m_level) {
        auto self = shared_from_this();
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if(m_root){
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}


void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) {
    if (level >= m_level) {
        std::string str = m_formatter->format(logger, level, event);
        std::cout << str << std::endl;
    }
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) {
    if (level >= m_level) {
        m_filestream << m_formatter->format(logger, level, event);
    }
}

FileLogAppender::FileLogAppender(const std::string& filename) 
    : m_filename(filename) {
    if (!reopen()) {  
        throw std::runtime_error("Failed to create log file: " + filename);
    }
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

LogFormatter::LogFormatter(const std::string& pattern)
    :m_pattern{pattern} {
        init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}


// Log format parser state machine
  // m_pattern = "%d [%p] %f %m %n“ -> liternal(string) -> tokens(tuple) -> m_items  What is the size of m_items? 
void LogFormatter::init() {
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> tokens;
    std::string literal_buf; // Buffer for literal characters between format specifiers
    
    // State machine states
    enum class ParserState {
        LITERAL,        // Processing literal characters
        SPECIFIER,      // Found %, parsing specifier name
        FORMAT_START,   // Found {, parsing format string
    };

    ParserState state = ParserState::LITERAL;
    size_t spec_start = 0;  // Start position of current specifier
    size_t fmt_start = 0;   // Start position of format string
    std::string spec_name;  // Buffer for specifier name
    std::string fmt_str;    // Buffer for format string

    for (size_t i = 0; i < m_pattern.size(); ++i) {
            char c = m_pattern[i];

            switch (state) {
                //-----------------------------------------
                // State 1: Processing literal characters
                //-----------------------------------------
                case ParserState::LITERAL:
                    if (c == '%') {
                        // Handle %% escape sequence
                        if (i + 1 < m_pattern.size() && m_pattern[i+1] == '%') {
                            literal_buf += '%';
                            i++;    // Skip next %
                            break;  
                        }

                        // Tm_itemsransition to specifier parsing
                        state = ParserState::SPECIFIER;
                        spec_start = i + 1; // Mark start of specifier

                        // Push any accumulated literal text
                        if (!literal_buf.empty()) {
                            tokens.emplace_back(literal_buf, "", 0);
                            literal_buf.clear();
                        }
                    } else { 
                        literal_buf += c; // Accumulate literal characters
                    }
                    break;

                //-----------------------------------------
                // State 2: Parsing specifier name (after %)
                //-----------------------------------------
                case ParserState::SPECIFIER:
                    
                    if(!isalpha(c) && c != '{' && c != '}') {
                        // End of specifier without format
                        spec_name = m_pattern.substr(spec_start, i - spec_start);
                        tokens.emplace_back(spec_name, "", 1); 
                        if(c == '%') {
                            spec_start = i + 1; // Mark start of new specifier
                            break;
                        }
                        literal_buf += c;
                        
                        state = ParserState::LITERAL;
                    }
                    else if (c == '{') {
                        // Extract specifier name between % and {
                        spec_name = m_pattern.substr(spec_start, i - spec_start);
                        state = ParserState::FORMAT_START;
                        fmt_start = i + 1;  // Mark start of format
                    } 
                    break;

                //-----------------------------------------
                // State 3: Parsing format string (between {})
                //------------TabFormatItem-----------------------------
                case ParserState::FORMAT_START:
                    if (c == '}') {
                        // Extract format string between { and }
                        fmt_str = m_pattern.substr(fmt_start, i - fmt_start);
                        tokens.emplace_back(spec_name, fmt_str, 1);
                        state = ParserState::LITERAL;
                    }
                    // else if (isspace(c)) {
                    //     // Error Unclosed format specifier
                    //     tokens.emplace_back("<<pattern_error>>", "", 0);
                    //     state = ParserState::LITERAL;
                    // }
                    break;
                
                // Should never reach here
                default:
                    break;
            }
    }

    // ----------------------------------------- 
    // Final state cleanupcout
    // -----------------------------------------
    switch (state)
    {
    case ParserState::SPECIFIER:
        // Handle specifier at end of string
        spec_name = m_pattern.substr(spec_start);
        tokens.emplace_back(spec_name, "", 1);
        break;
    case ParserState::FORMAT_START:
        // Handle unclosed format specifier at end
        tokens.emplace_back("<<pattern_error>>", "", 0);
        break;
    case ParserState::LITERAL:
        // Push any remaining literal text
        if (!literal_buf.empty()) {
            tokens.emplace_back(literal_buf, "", 0);
        }    
    default:
        break;
    }
    /**
     * @brief A static map associating log format specifiers with their corresponding factory functions
     * 
     * Maps format specifiers (e.g., "%m") to factory functions that create FormatItem objects.
     * Each factory function takes a format string and returns a smart pointer to a FormatItem.
     */
    static const std::map<std::string, std::function<FormatItem::ptr(const std::string&)>> s_format_items = 
    {
        // Define a macro to reduce boilerplate for map entries
        // Parameters:
        //   specifier: The format specifier character (e.g., 'm' for "%m")
        //   formatter: The FormatItem class to instantiate
        #define FORMAT_ITEM(specifier, formatter) \
            {#specifier, [](const std::string& fmt) {return FormatItem::ptr(new formatter(fmt));}} 

        // Register all supported format specifiers
        FORMAT_ITEM(m, MessageFormatItem),    // %m: Log message content
        FORMAT_ITEM(p, LevelFormatItem),      // %p: Log level (INFO, DEBUG, etc.)
        FORMAT_ITEM(r, ElapseFormatItem),     // %r: Milliseconds since program start
        FORMAT_ITEM(c, NameFormatItem),       // %c: Logger name
        FORMAT_ITEM(t, ThreadIdFormatItem),   // %t: Thread ID
        FORMAT_ITEM(n, NewLineFormatItem),    // %n: Newline character
        FORMAT_ITEM(d, DateTimeFormatItem),   // %d: Timestamp
        FORMAT_ITEM(f, FileFormatItem),       // %f: Source file name
        FORMAT_ITEM(l, LineFormatItem),       // %l: Line number
        FORMAT_ITEM(T, TabFormatItem),        // %T  Tab character  
        FORMAT_ITEM(F, FiberFormatItem)       // %F  Fiber ID

        // Clean up the macro to avoid pollution
        #undef FORMAT_ITEM
    };
    // Documentation of format specifiers:
    //   %m → Message body (the actual log content)
    //   %p → Log level (e.g., INFO, WARN, ERROR)
    //   %r → Time elapsed since program startup (milliseconds)
    //   %c → Logger name (e.g., "root" or a custom logger name)
    //   %t → Thread ID (identifies which thread logged the message)
    //   %n → Newline (platform-appropriate line ending, \r\n or \n)
    //   %d → Timestamp (formatted date/time)
    //   %f → File name (source file where the log was emitted)
    //   %l → Line number (in the source file)

    for(auto& i : tokens) {
        if(std::get<2>(i) == 0) {
            m_items.emplace_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.emplace_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        //std::cout << "{"<< std::get<0>(i) << "} - {" << std::get<1>(i) << "} - {" << std::get<2>(i) << "}"<< std::endl;
    }
    std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) return it->second;

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {

};

struct LogDefine {

};

void LoggerManager::init() {

}


};