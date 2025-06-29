#include "scheduler.h"

Scheduler::Scheduler(size_t worker_count) {
    task_queues.resize(worker_count);
    
    for (size_t i = 0; i < worker_count; ++i) {
        workers.emplace_back(&Scheduler::worker_loop, this, i);
    }
}

Scheduler::~Scheduler() {
    stop = true;
    cv.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

void Scheduler::schedule(Task task) {
    active_tasks++;
    
    // Intentar balancear la carga
    static thread_local size_t last_worker = 0;
    size_t current_worker = last_worker++ % workers.size();
    
    {
        std::lock_guard<std::mutex> lock(global_mutex);
        task_queues[current_worker].push_back(std::move(task));
    }
    
    cv.notify_one();
}

void Scheduler::worker_loop(size_t worker_id) {
    while (!stop) {
        Task task;
        bool has_task = false;
        
        // Verificar cola local
        {
            std::unique_lock<std::mutex> lock(global_mutex);
            if (!task_queues[worker_id].empty()) {
                task = std::move(task_queues[worker_id].front());
                task_queues[worker_id].pop_front();
                has_task = true;
            }
        }
        
        // Work stealing si no hay tareas locales
        if (!has_task) {
            has_task = try_steal_task(worker_id, task);
        }
        
        // Esperar si no hay tareas
        if (!has_task) {
            std::unique_lock<std::mutex> lock(global_mutex);
            if (active_tasks == 0) continue;
            
            cv.wait(lock, [this, worker_id] {
                return stop || !task_queues[worker_id].empty();
            });
            continue;
        }
        
        // Ejecutar la tarea
        task();
        active_tasks--;
    }
}

bool Scheduler::try_steal_task(size_t worker_id, Task& task) {
    const size_t num_queues = task_queues.size();
    
    for (size_t i = 1; i < num_queues; ++i) {
        size_t victim = (worker_id + i) % num_queues;
        
        std::unique_lock<std::mutex> lock(global_mutex);
        if (!task_queues[victim].empty()) {
            task = std::move(task_queues[victim].back());
            task_queues[victim].pop_back();
            return true;
        }
    }
    
    return false;
}

void Scheduler::wait_all() {
    while (active_tasks > 0) {
        std::this_thread::yield();
    }
}

