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
        ESP_LOGI(STAG, "Taking semaphore");
        semaphore->lock();
        return true;
    } else {
        ESP_LOGI(STAG, "Taking semaphore with timeout: %d", timeout);
        auto timeout_duration = std::chrono::milliseconds(timeout * portTICK_PERIOD_MS);
        if (semaphore->try_lock_for(timeout_duration)) {
            ESP_LOGI(STAG, "Semaphore taken (timeout %d)", timeout);
            return true;
        } else {
            ESP_LOGI(STAG, "Semaphore not taken (timeout %d)", timeout);
            return false;
        }
    }
}

inline void xSemaphoreGive(SemaphoreHandle_t& semaphore) {
    semaphore->unlock();
    ESP_LOGI(STAG, "Semaphore given");
}