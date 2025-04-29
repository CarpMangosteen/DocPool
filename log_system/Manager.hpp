#include <unordered_map>
#include "AsyncLogger.hpp"
#include <mutex>

namespace mylog
{
    class LoggerManager
    {
    private:
        LoggerManager(){
            std::unique_ptr<LoggerBuilder>builder(new LoggerBuilder);
            builder->BuildLoggerName("default");
            default_logger_=builder->Build();
            loggers_.insert(std::make_pair("default",default_logger_));
        }

    private:
        std::mutex mtx_;
        AsyncLogger::ptr default_logger_;
        std::unordered_map<std::string,AsyncLogger::ptr>loggers_;
    public:
        static LoggerManager &GetInstance(){
            static LoggerManager eton;
            return eton;
        }

        bool LoggerExist(const std::string &name){
            std::unique_lock<std::mutex>lock(mtx_);
            auto it=loggers_.find(name);
            if(it==loggers_.end())
                return false;
            return true;
        }

        void AddLogger(const AsyncLogger::ptr &&AsyncLogger){
            if(LoggerExist(AsyncLogger->Name()))
                return;
            std::unique_lock<std::mutex>lock(mtx_);
            loggers_.insert(std::make_pair(AsyncLogger->Name(),AsyncLogger));
        }

        AsyncLogger::ptr GetLogger(const std::string &name){
            std::unique_lock<std::mutex> lock(mtx_);
            auto it=loggers_.find(name);
            if(it==loggers_.end())
                return AsyncLogger::ptr();
            return it->second;
        }

        AsyncLogger::ptr DefaultLogger(){return default_logger_;}

    };
    
    
    
}
 // namespace mylog
