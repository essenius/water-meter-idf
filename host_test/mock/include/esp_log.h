#pragma once

#include <cstdio>
#include <mutex>

extern std::mutex logMutex; // in esp_log.cpp

#define LOG_HELPER(color_code, level, tag, format, ...) \
    do { \
        std::lock_guard<std::mutex> lock(logMutex); \
        printf(color_code "[%s] " level ": " format "\033[0m\n", tag, ##__VA_ARGS__); \
    } while (0)

#define ESP_LOGI(tag, format, ...) LOG_HELPER("\033[1;92m", "INFO", tag, format, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) LOG_HELPER("\033[1;31m", "ERROR", tag, format, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) LOG_HELPER("\033[1;33m", "WARNING", tag, format, ##__VA_ARGS__)    
#define ESP_LOGD(tag, format, ...) LOG_HELPER("\033[1;94m", "DEBUG", tag, format, ##__VA_ARGS__)