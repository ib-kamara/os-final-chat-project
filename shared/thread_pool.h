#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <condition_variable>

class ThreadPool
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()> > tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(int threadCount)
    {
        stop = false;

        for (int i = 0; i < threadCount; i++)
        {
            workers.push_back(std::thread([this]()
            {
                while (true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex);

                        condition.wait(lock, [this]()
                        {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty())
                        {
                            return;
                        }

                        task = tasks.front();
                        tasks.pop();
                    }

                    task();
                }
            }));
        }
    }

    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(task);
        }

        condition.notify_one();
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }

        condition.notify_all();

        for (int i = 0; i < (int)workers.size(); i++)
        {
            workers[i].join();
        }
    }
};

#endif