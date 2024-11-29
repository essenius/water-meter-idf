#pragma once

#include <cstdint>
#include "esp_err.h"

namespace i2c_master_test {
    void clear();
    void setOutput(const uint8_t* buffer, size_t size);
}

#define CONFIG_I2C_SDA 22
#define CONFIG_I2C_SCL 23

#define I2C_CLK_SRC_DEFAULT 4

typedef int i2c_port_num_t;
typedef int i2c_clock_source_t;
typedef int i2c_master_bus_t;
typedef int i2c_master_dev_t;

typedef enum {
    I2C_ADDR_BIT_LEN_7 = 0
} i2c_addr_bit_len_t;

typedef i2c_master_bus_t* i2c_master_bus_handle_t;
typedef i2c_master_dev_t* i2c_master_dev_handle_t;

typedef enum {
    I2C_NUM_0 = 0,
#if SOC_HP_I2C_NUM >= 2
    I2C_NUM_1,                  /*!< I2C port 1 */
#endif /* SOC_HP_I2C_NUM >= 2 */
#if SOC_LP_I2C_NUM >= 1
    LP_I2C_NUM_0,               /*< LP_I2C port 0 */
#endif /* SOC_LP_I2C_NUM >= 1 */
    I2C_NUM_MAX,                /*!< I2C port max */
} i2c_port_t;

typedef enum {
    GPIO_NUM_NC = -1,   
    GPIO_NUM_0 = 0,     
    GPIO_NUM_1 = 1,     
    GPIO_NUM_2 = 2,     
    GPIO_NUM_3 = 3,     
    GPIO_NUM_4 = 4,     
    GPIO_NUM_5 = 5,     
    GPIO_NUM_6 = 6,     
    GPIO_NUM_7 = 7,     
    GPIO_NUM_8 = 8,     
    GPIO_NUM_9 = 9,     
    GPIO_NUM_10 = 10,   
    GPIO_NUM_11 = 11,   
    GPIO_NUM_12 = 12,   
    GPIO_NUM_13 = 13,   
    GPIO_NUM_14 = 14,   
    GPIO_NUM_15 = 15,
    GPIO_NUM_16 = 16,
    GPIO_NUM_17 = 17,
    GPIO_NUM_18 = 18,
    GPIO_NUM_19 = 19,
    GPIO_NUM_20 = 20,
    GPIO_NUM_21 = 21,
    GPIO_NUM_22 = 22,
    GPIO_NUM_23 = 23,
    GPIO_NUM_25 = 25,
    GPIO_NUM_26 = 26,
    GPIO_NUM_27 = 27,
    GPIO_NUM_28 = 28,
    GPIO_NUM_29 = 29,
    GPIO_NUM_30 = 30,
    GPIO_NUM_31 = 31,
    GPIO_NUM_32 = 32,
    GPIO_NUM_33 = 33,
    GPIO_NUM_34 = 34,
    GPIO_NUM_35 = 35,
    GPIO_NUM_36 = 36,
    GPIO_NUM_37 = 37,
    GPIO_NUM_38 = 38,
    GPIO_NUM_39 = 39,
    GPIO_NUM_MAX,
} gpio_num_t;

typedef struct {
    i2c_addr_bit_len_t dev_addr_length;         /*!< Select the address length of the slave device. */
    uint16_t device_address;                    /*!< I2C device raw address. (The 7/10 bit address without read/write bit) */
    uint32_t scl_speed_hz;                      /*!< I2C SCL line frequency. */
    uint32_t scl_wait_us;                      /*!< Timeout value. (unit: us). Please note this value should not be so small that it can handle stretch/disturbance properly. If 0 is set, that means use the default reg value*/
    struct {
        uint32_t disable_ack_check:      1;     /*!< Disable ACK check. If this is set false, that means ack check is enabled, the transaction will be stopped and API returns error when nack is detected. */
    } flags;                                    /*!< I2C device config flags */
} i2c_device_config_t;

typedef struct {
    i2c_port_num_t i2c_port;              /*!< I2C port number, `-1` for auto selecting, (not include LP I2C instance) */
    gpio_num_t sda_io_num;                /*!< GPIO number of I2C SDA signal, pulled-up internally */
    gpio_num_t scl_io_num;                /*!< GPIO number of I2C SCL signal, pulled-up internally */
    union {
        i2c_clock_source_t clk_source;        /*!< Clock source of I2C master bus */
    };
    uint8_t glitch_ignore_cnt;            /*!< If the glitch period on the line is less than this value, it can be filtered out, typically value is 7 (unit: I2C module clock cycle)*/
    int intr_priority;                    /*!< I2C interrupt priority, if set to 0, driver will select the default priority (1,2,3). */
    size_t trans_queue_depth;             /*!< Depth of internal transfer queue, increase this value can support more transfers pending in the background, only valid in asynchronous transaction. (Typically max_device_num * per_transaction)*/
    struct {
        uint32_t enable_internal_pullup: 1;  /*!< Enable internal pullups. Note: This is not strong enough to pullup buses under high-speed frequency. Recommend proper external pull-up if possible */
    } flags;                              /*!< I2C master config flags */
} i2c_master_bus_config_t;

inline esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus_handle) { return ESP_OK; }
inline esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *bus_config, i2c_master_bus_handle_t *ret_bus_handle) { *ret_bus_handle = (i2c_master_bus_handle_t)33; return ESP_OK; }
inline esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus_handle, const i2c_device_config_t *dev_config, i2c_master_dev_handle_t *ret_handle) { *ret_handle = (i2c_master_dev_handle_t)27; return ESP_OK; }
inline esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t i2c_dev) { return ESP_OK; }
inline esp_err_t i2c_master_probe(i2c_master_bus_handle_t bus_handle, uint16_t address, int xfer_timeout_ms) { return address == 0x1e ? ESP_OK : ESP_FAIL; }
inline esp_err_t i2c_master_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms) { return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms);
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms);


