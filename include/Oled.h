// Oled.h
#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <cstdint>
#include <cstring>

class Oled {
public:
    Oled(i2c_inst_t* i2c_inst = i2c0,
         uint sda_pin = 4,
         uint scl_pin = 5,
         uint speed_hz = 400000,
         uint8_t addr = 0x3C,
         uint16_t width = 128,
         uint16_t height = 64,
         uint16_t ram_width = 132);  // SH1106 needs 132 columns internally

    ~Oled() = default;

    bool init();                    // returns true on success
    void clear();
    void set_contrast(uint8_t level);
    void invert(bool on);
    void display_on(bool on);

    // Core low-level methods
    bool cmd(uint8_t cmd);
    bool data(const uint8_t* buf, size_t len);

    // basic drawing helpers
    void set_cursor(uint8_t col, uint8_t page);
    void draw_byte(uint8_t col, uint8_t page, uint8_t byte); // single column

    // Getters
    uint16_t get_width()  const { return width_;  }
    uint16_t get_height() const { return height_; }

private:
    i2c_inst_t* i2c_;
    uint sda_pin_;
    uint scl_pin_;
    uint speed_hz_;
    uint8_t addr_;
    uint16_t width_;       // visible width (usually 128)
    uint16_t height_;      // visible height (64)
    uint16_t ram_width_;   // internal RAM columns (128 or 132)
    uint8_t  pages_;       // height / 8

    bool write_blocking(const uint8_t* buf, size_t len);
    void init_sequence();
};