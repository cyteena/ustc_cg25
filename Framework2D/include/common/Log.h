#ifndef LOG_H
#define LOG_H

#include <iostream>

// 定义日志等级
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Off // 关闭日志
};

// 日志类
class Logger {
public:
    // 构造函数
    Logger(LogLevel level = LogLevel::Info) : log_level_(level) {}

    // 设置日志等级
    void setLogLevel(LogLevel level) {
        log_level_ = level;
    }

    // 获取当前日志等级
    LogLevel getLogLevel() const {
        return log_level_;
    }

    // 输出 Trace 级别日志
    std::ostream& trace() {
        if (log_level_ <= LogLevel::Trace) {
            return stream("Trace: ");
        } else {
            return discard_stream_;
        }
    }

    // 输出 Debug 级别日志
    std::ostream& debug() {
        if (log_level_ <= LogLevel::Debug) {
            return stream("Debug: ");
        } else {
            return discard_stream_;
        }
    }

    // 输出 Info 级别日志
    std::ostream& info() {
        if (log_level_ <= LogLevel::Info) {
            return stream("Info: ");
        } else {
            return discard_stream_;
        }
    }

    // 输出 Warning 级别日志
    std::ostream& warning() {
        if (log_level_ <= LogLevel::Warning) {
            return stream("Warning: ");
        } else {
            return discard_stream_;
        }
    }

    // 输出 Error 级别日志
    std::ostream& error() {
        if (log_level_ <= LogLevel::Error) {
            return stream("Error: ");
        } else {
            return discard_stream_;
        }
    }

private:
    // 用于丢弃日志的空ostream
    class DiscardStream : public std::ostream {
    public:
        DiscardStream() : std::ostream(nullptr) {}
    };

    // 输出带前缀的日志
    std::ostream& stream(const std::string& prefix) {
        return std::cout << prefix;
    }

private:
    LogLevel log_level_;
    DiscardStream discard_stream_;
};

// 全局日志对象
extern Logger logger;

#endif