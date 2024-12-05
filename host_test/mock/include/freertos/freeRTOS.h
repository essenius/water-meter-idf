#pragma once

#include <cstdint>

// mock for testing

#define pdTRUE 1
#define pdPASS 1
#define pdFALSE 0
#define pdFAIL 0
#define portMAX_DELAY 0xFFFFFFFF
#define portTICK_PERIOD_MS 10
#define configTICK_RATE_HZ 100

using UBaseType_t = unsigned int;
using BaseType_t = int;

typedef uint32_t TickType_t;

#define pdMS_TO_TICKS(xTimeInMs) ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / ( TickType_t ) 1000U ) )

