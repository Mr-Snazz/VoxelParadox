// Arquivo: Engine/src/engine/thread_pool.h
// Papel: declara o pool de threads fixo usado pela geração assíncrona.
// Fluxo: centraliza jobs de background para evitar explosão de `std::thread` no streaming de chunks.
// Dependências principais: `std::thread`, `std::mutex` e `std::condition_variable`.
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

inline size_t defaultThreadPoolSize() {
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads == 0) {
        return 2;
    }
    if (hardwareThreads <= 2) {
        return 1;
    }
    if (hardwareThreads <= 4) {
        return 2;
    }
    return static_cast<size_t>(std::min(6u, hardwareThreads - 2));
}

// ThreadPool: Standard Concurrency Queue
// Prevents the OS from being overloaded with thousands of raw std::threads
// Creates a fixed number of CPU workers that pull Generation tasks symmetrically,
// keeping the FPS stable when a new dimension triggers massive chunk loading.
class ThreadPool {
public:
    ThreadPool(size_t threads = defaultThreadPoolSize()) : stop(false) {
        if (threads == 0) {
            threads = 1;
        }
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                [this] {
                    for(;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                [this]{ return this->stop || !this->tasks.empty(); });
                            if(this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                }
            );
    }

    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if(stop) return;
            tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
        condition.notify_one();
    }

    size_t threadCount() const {
        return workers.size();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};
