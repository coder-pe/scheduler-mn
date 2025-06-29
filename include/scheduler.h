#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <coroutine>
#include <iostream>

using Task = std::function<void()>;

class Scheduler {
public:
    Scheduler(size_t worker_count = std::thread::hardware_concurrency());
    ~Scheduler();
    
    void schedule(Task task);
    void wait_all();

private:
    std::vector<std::thread> workers;
    std::vector<std::deque<Task>> task_queues;
    std::mutex global_mutex;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
    std::atomic<size_t> active_tasks{0};
    
    void worker_loop(size_t worker_id);
    bool try_steal_task(size_t worker_id, Task& task);
};

