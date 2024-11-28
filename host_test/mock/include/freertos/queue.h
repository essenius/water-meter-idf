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


/* #include <windows.h>
#include <psapi.h>
#include <iostream>

inline void printMemoryInfo() {
    // Get the memory status
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        std::cout << "Total physical memory: " << memStatus.ullTotalPhys / 1024 / 1024 << " MB" << std::endl;
        std::cout << "Available physical memory: " << memStatus.ullAvailPhys / 1024 / 1024 << " MB" << std::endl;
        std::cout << "Total virtual memory: " << memStatus.ullTotalVirtual / 1024 / 1024 << " MB" << std::endl;
        std::cout << "Available virtual memory: " << memStatus.ullAvailVirtual / 1024 / 1024 << " MB" << std::endl;
    } else {
        std::cerr << "Failed to get memory status" << std::endl;
    }

    // Get the process memory info
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        std::cout << "Working set size: " << pmc.WorkingSetSize / 1024 / 1024 << " MB" << std::endl;
        std::cout << "Private usage: " << pmc.PrivateUsage / 1024 / 1024 << " MB" << std::endl;
    } else {
        std::cerr << "Failed to get process memory info" << std::endl;
    }
}

inline void printStackInfo() {
    // This is a simple example to get the current stack pointer
    // Note: This does not provide the full stack usage information
    void* stackPointer;
#if defined(__i386__)
    asm volatile ("movl %%esp, %0" : "=r" (stackPointer));
#elif defined(__x86_64__)
    asm volatile ("movq %%rsp, %0" : "=r" (stackPointer));
#else
    #error "Unsupported architecture"
#endif
    std::cout << "Current stack pointer: " << stackPointer << std::endl;
} */


// Mock FreeRTOS types and functions
using QueueHandle_t = std::queue<std::shared_ptr<std::vector<uint8_t>>>*;

constexpr const char* QTAG = "queue";
static int ItemSize = 0;
static std::mutex queue_mutex; // Mutex for synchronizing access to the queue


inline QueueHandle_t xQueueCreate(int queueLength, int itemSize) {
    ItemSize = itemSize;
    return new std::queue<std::shared_ptr<std::vector<uint8_t>>>();
}

inline void vQueueDelete(QueueHandle_t& queue) {
    delete queue;
}

inline bool isAligned(void* ptr, size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
}


// Function to display the contents of copied_item
inline void displayCopiedItem(const std::shared_ptr<std::vector<uint8_t>>& copied_item) {
    ESP_LOGI( QTAG, "Contents of copied_item:");
    for (const auto& byte : *copied_item) {
        printf("%02x ", byte);
    }
    printf("\n");
}
inline bool xQueueSend(QueueHandle_t& queue, void* item, int) {
    std::lock_guard<std::mutex> lock(queue_mutex);

    if (ItemSize <= 0) {
        ESP_LOGE(QTAG, "Invalid item size");
        return false;
    }

        // Check alignment
    if (!isAligned(item, alignof(uint8_t))) {
        ESP_LOGE(QTAG, "Misaligned item pointer");
        return false;
    }

    try {
        auto real_item = static_cast<uint8_t*>(item);
        ESP_LOGI(QTAG, "Creating std::vector<uint8_t>");
        auto copied_item = std::make_shared<std::vector<uint8_t>>(real_item, real_item + ItemSize);
        ESP_LOGI(QTAG, "std::vector<uint8_t> created and copied successfully");
        displayCopiedItem(copied_item);
        queue->push(copied_item);
        ESP_LOGI(QTAG, "done xQueueSend");
    } catch (const std::bad_alloc& e) {
        ESP_LOGE(QTAG, "Bad allocation Exception in xQueueSend: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        ESP_LOGE(QTAG, "Exception in xQueueSend: %s", e.what());
        return false;
    } catch (...) {
        ESP_LOGE(QTAG, "Unknown exception in xQueueSend");
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
    ESP_LOGI(QTAG, "done xQueueReceive");
    return true;
}

inline UBaseType_t uxQueueMessagesWaiting(QueueHandle_t queue) {
    return queue->size();
}