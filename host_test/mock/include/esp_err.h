#pragma once

#include <unity.h>

typedef int esp_err_t;

#define ESP_OK 0       
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103;

#define ESP_ERROR_CHECK(x) do { if ((x) != ESP_OK) { printf("\033[1;31mESP_ERROR_CHECK failed: %s\033[0m\n", #x); TEST_ABORT(); } } while (0)

inline const char *esp_err_to_name(esp_err_t) {
    return "test";
}