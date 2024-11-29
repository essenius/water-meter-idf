// implement the mock of freertos/semphr.h using standard C++ features
#pragma once

#include <memory>
#include <mutex>
#include <chrono>
#include "freeRTOS.h"
#include <cstdio>
#include "esp_log.h"

constexpr const char* STAG = "semphr";
// Mock FreeRTOS types and functions
using SemaphoreHandle_t = std::shared_ptr<std::timed_mutex>;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
    return std::make_shared<std::timed_mutex>();
}

inline bool xSemaphoreTake(SemaphoreHandle_t& semaphore, uint32_t timeout) {
    if (timeout == portMAX_DELAY) {
        semaphore->lock();
        return true;
    } else {
        auto timeout_duration = std::chrono::milliseconds(timeout * portTICK_PERIOD_MS);
        return semaphore->try_lock_for(timeout_duration);
    }
}

inline bool xSemaphoreGive(SemaphoreHandle_t& semaphore) {
    semaphore->unlock();
    return true;
}