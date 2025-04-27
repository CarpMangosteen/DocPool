#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <thread>
#include <future>

class ThreadPoll
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks; // 任务队列
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPoll(size_t threads) : stop(false)
    {
        for (size_t i = 0; i < threads; ++i)
        {
            workers.emplace_back([this]
                                 {
                for(;;){
                    std::function<void()>task;
                    {
                        std::unique_lock<std::mutex>lock(this->queue_mutex);
                        //等待任务队列不为空或线程池停止,lock是可以先解锁，唤醒后自动获得锁
                        this->condition.wait(lock,[this]{return this->stop||!this->tasks.empty();});
                        if(this->stop&&this->tasks.empty()){
                            return;
                        }
                        task=std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                } });
        }
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        // 创建打包任务
        auto task = std::make_shared<std::packaged_task<return_type>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_loc<std::mutex> lock(queue_mutex);

            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPoll");
            }
            tasks.emplace([task]()
                          { (*task)(); });
        }
        condition.notify_one();
        return res;
    }
    ~ThreadPoll()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
        {
            worker.join();
        }
    }
};