#include "driver/i2c_master.h"
#include "esp_log.h"
#include <vector>

namespace i2c_master_test {
    uint8_t registerValue[10] = {10,11,12,13,14,15,16,17,18,19};
    uint8_t writeBuffer[10];
    uint8_t readBuffer[10] = {0,1,2,3,4,5,6,7,8,9};
    std::vector<uint8_t> m_writeBuffer;
    std::vector<uint8_t> m_readBuffer;
    unsigned short m_readIndex = 0;

    void clear() {
        m_writeBuffer.clear();
        m_readBuffer.clear();
        m_readIndex = 0;
    }

    void setOutput(const uint8_t* buffer, size_t size) {
        m_readBuffer.clear();
        m_readBuffer.insert(m_readBuffer.begin(), buffer, buffer + size);
    }

    void writeByte(uint8_t value) {
        m_writeBuffer.push_back(value);
    }

    bool readByte(uint8_t& value) {
        if (m_readBuffer.empty()) return false;
        if (m_readIndex >= m_readBuffer.size()) m_readIndex = 0;
        value = m_readBuffer[m_readIndex++];
        return true;
    }
}

esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms) { 
    if (write_size >= 10 || read_size >= 10) return ESP_FAIL;
    for (int i = 0; i < write_size; i++) {
        i2c_master_test::writeByte(write_buffer[i]);
    }
    if (read_size == 1) {
        read_buffer[0] = i2c_master_test::registerValue[write_buffer[0]];
        return ESP_OK;
    }

    for (int i = 0; i < read_size; i++) {
        if (i2c_master_test::readByte(read_buffer[i]) == ESP_FAIL) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms) {
    if (write_size >= 10) return ESP_FAIL;
    for (int i = 0; i < write_size; i++) {
        i2c_master_test::writeByte(write_buffer[i]);
    }
    if (write_size == 2) {
        i2c_master_test::registerValue[write_buffer[0]] = write_buffer[1];
    }
    return ESP_OK;
}
