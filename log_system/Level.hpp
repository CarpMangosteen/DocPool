#pragma once
#include<string>

namespace mylog{
class LogLevel
{

public:
    enum class value{DEBUG,INFO,WARN,ERROR,FATAL};

    static const char* ToString(value level){//静态函数可以直接通过类名调用（如 LogLevel::ToString(...)），无需创建类的对象。
        switch (level) {
            case value::DEBUG:
                return "DEBUG";
            case value::INFO:
                return "INFO";
            case value::WARN:
                return "WARN";
            case value::ERROR:
                return "ERROR";
            case value::FATAL:
                return "FATAL";
            default:
                return "UNKNOW";
        }
        return "UNKNOW";
    }
};




} // namespace mylog
