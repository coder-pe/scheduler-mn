#include "scheduler.h"

int main() {
    Scheduler scheduler;
    std::atomic<int> counter{0};
    constexpr int NUM_TASKS = 100000;
    
    for (int i = 0; i < NUM_TASKS; ++i) {
        scheduler.schedule([&counter, i] {
            std::cout << "Task " << i << " executed on thread " 
                      << std::this_thread::get_id() << "\n";
            counter++;
        });
    }
    
    scheduler.wait_all();
    std::cout << "All tasks completed. Counter: " << counter << "\n";
    
    return 0;
}
