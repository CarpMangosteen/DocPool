#pragma once

#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "AsyncBuffer.hpp"

namespace mylog
{
    enum class AsyncType
    {
        ASYNC_SAFE,
        ASYNC_UNSAFE
    };
    using functor = std::function<void(Buffer &)>;
    class AsyncWorker
    {
    private:
        AsyncType async_type_;
        std::atomic<bool> stop_; // 控制异步工作器
        std::mutex mtx_;
        mylog::Buffer buffer_productor_;
        mylog::Buffer buffer_consumer_;
        std::condition_variable cond_productor_;
        std::condition_variable cond_consumer_;
        std::thread thread_;

        functor callback_;

    private:
        void ThreadEntry()
        {
            while (1)
            {
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    if (buffer_productor_.IsEmpty() && stop_)
                    {
                        cond_consumer_.wait(lock, [&]()
                                            { return stop_ || !buffer_productor_.IsEmpty(); });
                    }
                    buffer_productor_.Swap(buffer_consumer_);
                    if (async_type_ == AsyncType::ASYNC_SAFE)
                    {
                        cond_productor_.notify_one();
                    }
                }
                callback_(buffer_consumer_);
                buffer_consumer_.Reset();
                if (stop_ && buffer_productor_.IsEmpty())
                    return;
            }
        }

    public:
        using ptr = std::shared_ptr<AsyncWorker>;
        AsyncWorker(const functor &cb, AsyncType async_type = AsyncType::ASYNC_SAFE) : async_type_(async_type),
                                                                                       callback_(cb),
                                                                                       stop_(false),
                                                                                       thread_(std::thread(&AsyncWorker::ThreadEntry, this)) {}
        ~AsyncWorker() { Stop(); }
        void Stop()
        {
            stop_ = true;
            cond_consumer_.notify_all();
            thread_.join();
        }
        void Push(const char *data, size_t len)
        {
            // 如果生产者队列不足以写下len长度数据，并且缓冲区是固定大小，那么阻塞
            std::unique_lock<std::mutex> lock(mtx_);
            if (AsyncType::ASYNC_SAFE == async_type_)
            {
                cond_productor_.wait(lock, [&]()
                                     { return len <= buffer_productor_.WriteableSize(); });
            }
            buffer_productor_.Push(data, len);
            cond_consumer_.notify_one();
        }
    };
} // namespace mylog
