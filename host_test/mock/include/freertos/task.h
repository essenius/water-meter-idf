#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <cstdio>
#include "freeRTOS.h"
#include "esp_log.h"

using TaskFunction_t = std::function<void(void*)>;
using TaskHandle_t = std::shared_ptr<struct TaskControlBlock>;

constexpr const char* kTaskTag = "task";

struct TaskControlBlock {
    std::thread thread;
};

static std::unordered_map<std::thread::id, TaskHandle_t> task_threads;


inline BaseType_t xTaskCreate(TaskFunction_t task, const char* name, int, void* param, int, TaskHandle_t* taskHandle) {
    auto tcb = std::make_shared<TaskControlBlock>();
    tcb->thread = std::thread([task, param, tcb]() {
        task(param);
        ESP_LOGI(kTaskTag, "Task terminated");
    });
    task_threads[tcb->thread.get_id()] = tcb;
    if (!taskHandle) {
        ESP_LOGW(kTaskTag, "Task handle is null");
        return pdFAIL;
    }
    *taskHandle = tcb;
    ESP_LOGI(kTaskTag, "Created task %s (%p)", name, tcb.get());
    return pdPASS;
}

inline void vTaskDelay(int ticks) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ticks * portTICK_PERIOD_MS));
}

// delete a task by handle. Expects the handle to be valid. Internal use only.
inline void deleteTask(TaskHandle_t taskHandle) {
    ESP_LOGD(kTaskTag, "Deleting task %p", taskHandle.get());
    if (taskHandle->thread.joinable()) {
        taskHandle->thread.join();
    }
    task_threads.erase(taskHandle->thread.get_id());
}

inline void vTaskDelete(TaskHandle_t taskHandle) {
        if (taskHandle == nullptr) {
        // Delete the current task
        auto it = task_threads.find(std::this_thread::get_id());
        if (it != task_threads.end()) {
            deleteTask(it->second);
        }
    } else {
        // Delete the specified task
        deleteTask(taskHandle);
    }
}

// Function to join all threads (to be called at the end of the test)
// note: make sure that all tasks are terminated before calling this function
inline void deleteAllTasks() {
    for (auto& [id, tcb] : task_threads) {
        deleteTask(tcb);
    }
    task_threads.clear();
}