#pragma once

#include <cstdint>
#include <queue>
#include <functional>
#include <cstdio>
#include <memory>
#include <cstring>
#include <mutex>
#include "FreeRTOS.h"
#include "../esp_log.h"

// Mock FreeRTOS types and functions
using QueueHandle_t = std::queue<std::shared_ptr<std::vector<uint8_t>>>*;

static int ItemSize = 0;
static std::mutex queue_mutex; // Mutex for synchronizing access to the queue


inline QueueHandle_t xQueueCreate(int queueLength, int itemSize) {
    ItemSize = itemSize;
    return new std::queue<std::shared_ptr<std::vector<uint8_t>>>();
}

inline void vQueueDelete(QueueHandle_t& queue) {
    delete queue;
}

inline bool isAligned(const void* const ptr, const size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
}


// Function to display the contents of copied_item
inline void displayCopiedItem(const std::shared_ptr<std::vector<uint8_t>>& copied_item) {
    ESP_LOGI( "displayCopiedItem", "Contents of copied_item:");
    for (const auto& byte : *copied_item) {
        printf("%02x ", byte);
    }
    printf("\n");
}

inline bool xQueueSend(QueueHandle_t& queue, const void* const item, int) {
    constexpr const char* kTag = "xQueueSend";
    std::lock_guard<std::mutex> lock(queue_mutex);

    if (ItemSize <= 0) {
        ESP_LOGE(kTag, "Invalid item size");
        return false;
    }

        // Check alignment
    if (!isAligned(item, alignof(uint8_t))) {
        ESP_LOGE(kTag, "Misaligned item pointer");
        return false;
    }

    try {
        auto real_item = static_cast<const uint8_t*>(item);
        auto copied_item = std::make_shared<std::vector<uint8_t>>(real_item, real_item + ItemSize);
        // displayCopiedItem(copied_item);
        queue->push(copied_item);
    } catch (const std::bad_alloc& e) {
        ESP_LOGE(kTag, "Bad allocation Exception in xQueueSend: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        ESP_LOGE(kTag, "Exception in xQueueSend: %s", e.what());
        return false;
    } catch (...) {
        ESP_LOGE(kTag, "Unknown exception in xQueueSend");
        return false;
    }
    return true;
}

inline bool xQueueReceive(QueueHandle_t& queue, void* item, int) {
    std::lock_guard<std::mutex> lock(queue_mutex);

    if (queue->empty()) return false;
    auto copied_item = queue->front();
    std::memcpy(item, copied_item->data(), copied_item->size());
    queue->pop();
    return true;
}

inline UBaseType_t uxQueueMessagesWaiting(QueueHandle_t queue) {
    return queue->size();
}