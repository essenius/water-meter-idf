// implement the mock of freertos/semphr.h using standard C++ features
#pragma once

#include <memory>
#include <mutex>
#include <chrono>
#include "freeRTOS.h"

constexpr auto kStag = "semphr";
// Mock FreeRTOS types and functions
using SemaphoreHandle_t = std::shared_ptr<std::timed_mutex>;

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
    return std::make_shared<std::timed_mutex>();
}

inline bool xSemaphoreTake(const SemaphoreHandle_t& semaphore, const uint32_t timeout) {
    if (timeout == portMAX_DELAY) {
        semaphore->lock();
        return true;
    }
    const auto timeoutDuration = std::chrono::milliseconds(timeout * portTICK_PERIOD_MS);
    return semaphore->try_lock_for(timeoutDuration);
}

inline bool xSemaphoreGive(const SemaphoreHandle_t& semaphore) {
    semaphore->unlock();
    return true;
}

inline void vSemaphoreDelete(SemaphoreHandle_t& semaphore) {
    semaphore.reset();
}