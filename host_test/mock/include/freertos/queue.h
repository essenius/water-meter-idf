#pragma once

#include <cstdint>
#include <queue>
#include <functional>
#include <cstdio>
#include <memory>
#include <cstring>
#include <mutex>
#include "freeRTOS.h"
#include "../esp_log.h"

// Mock FreeRTOS types and functions
using QueueHandle_t = std::queue<std::shared_ptr<std::vector<uint8_t>>>*;

static int m_itemSize = 0;
static std::mutex queueMutex; // Mutex for synchronizing access to the queue


inline QueueHandle_t xQueueCreate(int queueLength, int itemSize) {
    m_itemSize = itemSize;
    return new std::queue<std::shared_ptr<std::vector<uint8_t>>>();
}

inline void vQueueDelete(QueueHandle_t& queue) {
    delete queue;
}

inline bool isAligned(const void* const ptr, const size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
}


// Function to display the contents of copied_item
inline void displayCopiedItem(const std::shared_ptr<std::vector<uint8_t>>& copiedItem) {
    ESP_LOGI( "displayCopiedItem", "Contents of copied_item:");
    for (const auto& byte : *copiedItem) {
        printf("%02x ", byte);
    }
    printf("\n");
}

inline bool xQueueSend(QueueHandle_t& queue, const void* const item, int) {
    constexpr auto kTag = "xQueueSend";
    std::lock_guard<std::mutex> lock(queueMutex);

    if (m_itemSize <= 0) {
        ESP_LOGE(kTag, "Invalid item size");
        return false;
    }

        // Check alignment
    if (!isAligned(item, alignof(uint8_t))) {
        ESP_LOGE(kTag, "Misaligned item pointer");
        return false;
    }

    try {
        auto realItem = static_cast<const uint8_t*>(item);
        const auto copiedItem = std::make_shared<std::vector<uint8_t>>(realItem, realItem + m_itemSize);
        // displayCopiedItem(copied_item);
        queue->push(copiedItem);
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
    std::lock_guard<std::mutex> lock(queueMutex);

    if (queue->empty()) return false;
    const auto copiedItem = queue->front();
    std::memcpy(item, copiedItem->data(), copiedItem->size());
    queue->pop();
    return true;
}

inline UBaseType_t uxQueueMessagesWaiting(QueueHandle_t queue) {
    return queue->size();
}