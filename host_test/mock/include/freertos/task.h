#pragma once

#include <functional>
#include <thread>
#include <unordered_map>
#include <memory>
#include <cstdio>
#include <ranges>

#include "freeRTOS.h"
#include "esp_log.h"

typedef void (* TaskFunction_t)( void * );

using TaskHandle_t = std::shared_ptr<struct TaskControlBlock>;

constexpr auto kTaskTag = "task";

struct TaskControlBlock {
    std::thread thread;
};

static std::unordered_map<std::thread::id, TaskHandle_t> taskThreads;


inline BaseType_t xTaskCreate(const TaskFunction_t& task, const char* name, int, void* param, int, TaskHandle_t* taskHandle) {
    if (!taskHandle) {
        ESP_LOGW(kTaskTag, "Task handle cannot be null");
        return pdFAIL;
    }
    auto tcb = std::make_shared<TaskControlBlock>();
    tcb->thread = std::thread([task, param, tcb]() {
        task(param);
        ESP_LOGI(kTaskTag, "Task terminated");
    });
    taskThreads[tcb->thread.get_id()] = tcb;

    *taskHandle = tcb;
    ESP_LOGI(kTaskTag, "Created task %s (%p)", name, tcb.get());
    return pdPASS;
}

inline void vTaskDelay(int ticks) {
    // just sleep for 1 ms to switch context, this is a mock after all
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

// Yield the processor to other threads
inline void taskYIELD() {
    std::this_thread::yield();
}

// delete a task by handle. Expects the handle to be valid. Internal use only.
inline void deleteTask(const TaskHandle_t& taskHandle) {
    ESP_LOGD(kTaskTag, "Deleting task %p", taskHandle.get());
    if (taskHandle->thread.joinable()) {
        if (taskHandle->thread.get_id() == std::this_thread::get_id()) {
            ESP_LOGW(kTaskTag, "Detaching the current task to avoid deadlock");
            taskHandle->thread.detach();
        } else {
            ESP_LOGI(kTaskTag, "Thread is joinable, attempting to join");
            try {
                taskHandle->thread.join();
                ESP_LOGI(kTaskTag, "Thread joined successfully");
            }
            catch (const std::system_error& e) {
                ESP_LOGE(kTaskTag, "Failed to join thread: %s, %s", e.what(), e.code().message().c_str());
            }
        }
    }
    taskThreads.erase(taskHandle->thread.get_id());
}

inline void vTaskDelete(const TaskHandle_t& taskHandle) {
    if (taskHandle == nullptr) {
        // Delete the current task
        if (const auto it = taskThreads.find(std::this_thread::get_id()); it != taskThreads.end()) {
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
    for (auto& tcb : taskThreads | std::views::values) {
        deleteTask(tcb);
    }
    taskThreads.clear();
}