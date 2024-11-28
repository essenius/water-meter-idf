#pragma once

#include <cstdio>

#define ESP_LOGI(tag, format, ...) printf("\033[1;92m[%s] INFO: " format "\033[0m\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) printf("\033[1;31m[%s] ERROR: " format "\033[0m\n", tag, ##__VA_ARGS__)
#define ESP_ERROR_CHECK(x) do { if (!(x)) { printf("\033[1;31mESP_ERROR_CHECK failed: %s\033[0m\n", #x); } } while (0)
