#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

template <typename T>
class safe_queue {
public:
    safe_queue() {}

    void push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(value);
        condition_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });

        T value = queue_.front();
        queue_.pop();
        return value;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

class thread_pool {
public:
    thread_pool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this] { worker_thread(); });
        }
    }

    ~thread_pool() {
        stop = true;
        for (auto& thread : threads) {
            thread.join();
        }
    }

    void submit(std::function<void()> task) {
        queue.push(task);
    }

private:
    void worker_thread() {
        while (!stop) {
            std::function<void()> task = queue.pop();
            if (task) {
                task();
            }
        }
    }

    safe_queue<std::function<void()>> queue;
    std::vector<std::thread> threads;
    std::atomic<bool> stop;
};

int main() {
    size_t num_threads = std::thread::hardware_concurrency();
    thread_pool pool(num_threads);

    auto task1 = [] { std::cout << "Task 1 executed\n"; };
    auto task2 = [] { std::cout << "Task 2 executed\n"; };

    pool.submit(task1);
    pool.submit(task2);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
