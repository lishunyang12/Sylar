#include "log.h"
#include "map"
#include "functional"
#include "iostream"
#include "config.h"

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

LogLevel::level LogLevel::FromString(const std::string str) {
#define XX(name) \
    if(str == #name) { \
        return LogLevel::name; \
    }
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
    return LogLevel::UNKNOWN;
#undef XX
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
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));   // default formatter for appenders if their formatter is null

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

void Logger::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
}
void Logger::setFormatter(const std::string& val) {
    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name
                << " value=" << val << " invalid formatter" 
                << std::endl;
        return;
    }
    m_formatter = new_val;
}

LogFormatter::ptr Logger::getFormatter() {
    return m_formatter;
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

std::string FileLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["file"] = m_filename;
    node["level"] = LogLevel::ToString(m_level);
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::level level, LogEvent::ptr event) {
    if (level >= m_level) {
        std::string str = m_formatter->format(logger, level, event);
        std::cout << str << std::endl;
    }
}

std::string StdoutLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["level"] = LogLevel::ToString(m_level);
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
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

// This pattern is useful for log rotation or when you need to reopen a log file after it might have been moved/deleted
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
  // m_pattern = "%d [%p] %f %m %n“ -> liternal(string) -> tokens(tuple) -> m_items 

//   m_pattern (string)  
//     → tokens (parsed elements)  
//     → m_items (executable FormatItems)  
//     → formatted output
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
        m_error = true;
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
                m_error = true;
            } else {
                m_items.emplace_back(it->second(std::get<1>(i)));
            }
        }
        //std::cout << "{"<< std::get<0>(i) << "} - {" << std::get<1>(i) << "} - {" << std::get<2>(i) << "}"<< std::endl;
    }
    std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

    m_loggers[m_root->m_name] = m_root;
    init();
}

std::string Logger::ToYamlString() {
    YAML::Node node;
    node["name"] = m_name;
    node["level"] = LogLevel::ToString(m_level);
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString())); 
    }

    std::stringstream ss;
    ss << node;;
    return ss.str();
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
    int type = 0; // 1 File, 2 Stdout
    LogLevel::level level = LogLevel::UNKNOWN;
    std::string formatter;
    std::string file;

     bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::level level = LogLevel::UNKNOWN;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

   bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
   }

   bool operator<(const LogDefine& oth) const {
        return name < oth.name;
   }
};

// Template specialization for converting std::string to std::set<LogDefine>
// string -> node -> std::set<LogDefine>
template<>
class LexicalCast<std::string, std::set<LogDefine>> {
public:
    std::set<LogDefine> operator() (const std::string& v) {
        // Parse the input string into a YAML node structure
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;  

        // Iterate through each element in the YAML sequence
        for(size_t i = 0; i < node.size(); ++i) {
            auto n = node[i];  
            
            if(!n["name"].IsDefined()) {
                std::cout << "Log config error: name is null, " << n
                          << std::endl; 
                continue;  
            }

            LogDefine ld; 
            
            // Extract required fields with appropriate conversions
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(
                n["level"].IsDefined() ? n["level"].as<std::string>() : ""
            );
            
            // Optional formatter field
            if(n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            // Process appenders if they exist
            if(n["appenders"].IsDefined()) {
                for(size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];  
                    
                    if(!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, "
                                << a << std::endl;
                        continue;
                    }
                    
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;  
                    
                    if(type == "FileLogAppender") {
                        lad.type = 1;  
                        
                        if(!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender file is null"
                                      << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        
                        if(n["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if(type == "StdoutLogAppender") {
                        lad.type = 2;  
                    } else {
                        std::cout << "log config error: appender type is invalid"
                                  << std::endl;
                        continue;
                    }
                    
                    ld.appenders.emplace_back(lad);
                }
            }
            vec.insert(ld);
        }
        return vec;
    }
};

// Template specialization for converting std::set<LogDefine> to std::string
template<>
class LexicalCast<std::set<LogDefine>, std::string> {
public:
    std::string operator() (const std::set<LogDefine>& v) {
        YAML::Node node;  // Create root YAML node
        
        // Convert each LogDefine to a YAML node
        for(auto& i : v) {
            YAML::Node n;  
            n["name"] = i.name;  
            
            // NOTE: Currently missing other fields (level, formatter, appenders)
            // Should be expanded similar to the deserialization logic
            n["level"] = LogLevel::ToString(i.level);
            n["formatter"] = i.formatter;
            
            for(auto& a : i.appenders) {
                YAML::Node na;
                if(a.type == 1) {
                    na["type"] = "FileLogAppenders";
                    na["file"] = a.file;
                } else if(a.type == 2) {
                    na["type"] = "StdoutLogAppenders";
                }
                na["level"] = LogLevel::ToString(a.level);
                if(!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);            
            }
            node.push_back(n); 
        }

        // Convert the YAML node tree to a string
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines = 
    sylar::Config::Lookup("logs", std::set<LogDefine>{}, "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value){
            // 新增
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "On logger ref has been changed";
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增logger
                    logger = SYLAR_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改logger
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                }
                logger->setLevel(i.level);
                if(!(i.formatter.empty())) {
                    logger->setFormatter(i.formatter);
                }
                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    sylar::LogAppender::ptr ap;
                    if(a.type == 1) {   
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new StdoutLogAppender);   
                    }
                    ap->setLevel(a.level);            // ?? appender.formatter ??
                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::level)100);
                    logger->clearAppenders();
                }
            } 
        });
    }
};

static LogIniter __log_init;

std::string LoggerManager::toYamlString() {
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->ToYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LoggerManager::init() {

}


};


// Phase 1: Logger Initialization (Setup)
// 1. LoggerManager Initialization
//    │
//    ├─ 1.1. Create root logger ("root")
//    │    ├─ Sets default log level (e.g., DEBUG)
//    │    └─ Assigns default formatter (e.g., "%d [%p] %m%n")
//    │
//    ├─ 1.2. Register appenders (optional)
//    │    ├─ StdoutAppender (console)
//    │    └─ FileAppender ("app.log")
//    │
//    └─ 1.3. Hierarchical logger creation
//         ├─ e.g., getLogger("network.http") creates:
//         │    ├─ Logger("network")
//         │    └─ Logger("network.http")
//         └─ Child loggers inherit parent appenders/level unless overridden





// Phase 2: Pattern Parsing (LogFormatter Init)
// 2. LogFormatter Initialization (when setFormatter() is called)
//    │
//    ├─ 2.1. Parse m_pattern into tokens
//    │    ├─ State machine processes:
//    │    │    ├─ Literals (e.g., " [" → StringFormatItem)
//    │    │    └─ Specifiers (e.g., "%d" → DateTimeFormatItem)
//    │    └─ Handles nested formats (e.g., "%d{%Y-%m-%d}")
//    │
//    ├─ 2.2. Compile tokens into FormatItems
//    │    ├─ Uses s_format_items map to resolve specifiers:
//    │    │    ├─ e.g., "d" → DateTimeFormatItem(fmt_str)
//    │    │    └─ Unknown specifiers → Error StringFormatItem
//    │    └─ Stores compiled items in m_items vector
//    │
//    └─ 2.3. Validation
//         ├─ Invalid patterns fail fast (e.g., unclosed "%d{")
//         └─ Debuggable token stream (for development)





// Phase 3: Logging Execution (Runtime)
// ├── 3.1.1 Macro Expansion
// │    ├── Level Check: logger->getLevel() <= log_level
// │    └── Construct LogEvent: Captures (metadata + message)
// │
// ├── 3.1.2 LogEventWrap (RAII)
// │    └── On destruction: Invokes logger->log()
// │
// └── 3.1.3 Logger Processing
//      ├── Routing Logic:
//      │    ├── Primary: Forward to appenders where appender->getLevel() <= log_level
//      │    └── Fallback: Route to root logger if no appenders available
//      │
//      └── Appender Pipeline
//           ├── 3.1.3.1 Formatter Selection
//           │    ├── Priority: appender->m_formatter
//           │    └── Default: logger->m_formatter
//           │
//           ├── 3.1.3.2 Message Formatting
//           │    ├── Process FormatItems:
//           │    │    └── Each item appends to stringstream (e.g., %d → datetime)
//           │    └── Example: "%d [%p] %m" → "2023-01-01 14:30 [INFO] Hello"
//           │
//           └── 3.1.3.3 Output Dispatch
//                ├── StdoutAppender: std::cout
//                ├── FileAppender: ofstream (with rotation)
//                └── CustomAppenders:
//                     ├── Network: Socket transmission
//                     ├── Database: SQL/NoSQL write
//                     └── Syslog: System logging




// Phase 4: Output Examples
// 4.1. Console Output (StdoutAppender)
//      │
//      ├─ Formatted: "2023-01-01 [INFO] main.cpp:42 User connected"
//      └─ Written to stdout (colored if enabled)

// 4.2. File Output (FileAppender)
//      │
//      ├─ Formatted: "2023-01-01T12:00:00 [INFO] thread=1234 file=main.cpp:42 msg='User connected'"
//      └─ Appended to "app.log" (with rotation)




// Smart Pointers
// reset()   decrements the counter in the control block, Destroys the object and Deletes the control block if refcount hits 0                   
// reset(SYLAR_LOG_NAME.get())	Bypasses refcounting	Unsafe (double-free)	Never use this!
// SYLAR_LOG_NAME()	            Proper refcount++	    Safe	                Normal global logger access
// reset(new Logger)	        Starts new refcount=1	Safe (but isolated)	    Special-case independent loggers


// auto n = node[i];
// cannot reference to a Lvalue with short lifetime and without name