// Oled.h
#pragma once

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <cstdint>
#include <cstring>

struct Oled {
    static constexpr uint8_t ADDR          = 0x3C;  // or 0x3D
    static constexpr uint     SDA_PIN      = 4;
    static constexpr uint     SCL_PIN      = 5;
    static constexpr uint     SPEED_HZ     = 400000;
    static constexpr uint8_t  SCREEN_COLS  = 132;
    static constexpr uint8_t  PAGE_COUNT   = 8;

    static void init_i2c_and_pins() {
        i2c_init(i2c0, SPEED_HZ);
        gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(SDA_PIN);
        gpio_pull_up(SCL_PIN);
        sleep_ms(200);
    }

    static void cmd(uint8_t cmd) {
        uint8_t buf[2] = {0x00, cmd};
        i2c_write_blocking(i2c0, ADDR, buf, 2, false);
    }

    static void data(const uint8_t* buf, size_t len) {
        uint8_t header[len + 1];
        header[0] = 0x40;
        std::memcpy(header + 1, buf, len);
        i2c_write_blocking(i2c0, ADDR, header, len + 1, false);
    }

    static void init_display() {
        cmd(0xAE);
        cmd(0xD5); cmd(0x80);
        cmd(0xA8); cmd(0x3F);
        cmd(0xD3); cmd(0x00);
        cmd(0x40);
        cmd(0x8D); cmd(0x14);
        cmd(0x20); cmd(0x00);
        cmd(0xA1);
        cmd(0xC8);
        cmd(0xDA); cmd(0x12);
        cmd(0x81); cmd(0xCF);
        cmd(0xD9); cmd(0xF1);
        cmd(0xDB); cmd(0x40);
        cmd(0xA4);
        cmd(0xA6);
        cmd(0xAF);
    }

    static void clear() {
        for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
            cmd(0xB0 + page);
            cmd(0x02);
            cmd(0x10);
            uint8_t zeros[SCREEN_COLS] = {};
            data(zeros, SCREEN_COLS);
        }
    }
};