#include <iostream>
#include <string>
#include <ctime>
#include <fstream>
#include <memory>

// 日志级别枚举
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// 日志格式化器
class Formatter {
public:
    std::string format(LogLevel level, const std::string& message) {
        std::time_t now = std::time(nullptr);
        std::string time_str = std::ctime(&now);
        time_str.pop_back(); // 去掉换行符

        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO: level_str = "INFO"; break;
            case LogLevel::WARN: level_str = "WARN"; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
            case LogLevel::FATAL: level_str = "FATAL"; break;
        }

        return "[" + time_str + "] [" + level_str + "] " + message;
    }
};

// 日志输出器
class Flush {
public:
    virtual void flush(const std::string& formatted_log) = 0;
    virtual ~Flush() = default;
};

// 控制台输出器
class ConsoleFlush : public Flush {
public:
    void flush(const std::string& formatted_log) override {
        std::cout << formatted_log << std::endl;
    }
};

// 文件输出器
class FileFlush : public Flush {
public:
    FileFlush(const std::string& filename) : file_(filename, std::ios::app) {}

    void flush(const std::string& formatted_log) override {
        if (file_.is_open()) {
            file_ << formatted_log << std::endl;
        }
    }

    ~FileFlush() {
        if (file_.is_open()) {
            file_.close();
        }
    }

private:
    std::ofstream file_;
};

// 日志记录器
class Logger {
public:
    Logger(LogLevel level) : level_(level), formatter_(std::make_unique<Formatter>()) {
        console_Flush_ = std::make_unique<ConsoleFlush>();
        file_Flush_ = std::make_unique<FileFlush>("app.log");
    }

    void log(LogLevel level, const std::string& message) {
        if (level >= level_) {
            std::string formatted_log = formatter_->format(level, message);
            console_Flush_->flush(formatted_log);
            file_Flush_->flush(formatted_log);
        }
    }

    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void warn(const std::string& message) {
        log(LogLevel::WARN, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

    void fatal(const std::string& message) {
        log(LogLevel::FATAL, message);
    }

private:
    LogLevel level_;
    std::unique_ptr<Formatter> formatter_;
    std::unique_ptr<Flush> console_Flush_;
    std::unique_ptr<Flush> file_Flush_;
};