#include "Oled.h"

Oled::Oled(i2c_inst_t* i2c_inst, uint sda, uint scl, uint speed, uint8_t addr,
           uint16_t w, uint16_t h, uint16_t ram_w)
    : i2c_(i2c_inst),
      sda_pin_(sda), scl_pin_(scl), speed_hz_(speed), addr_(addr),
      width_(w), height_(h), ram_width_(ram_w),
      pages_(h / 8)
{
    // nothing here — real init in init()
}

bool Oled::init() {
    // I2C setup
    i2c_init(i2c_, speed_hz_);
    gpio_set_function(sda_pin_, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin_, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin_);
    gpio_pull_up(scl_pin_);
    sleep_ms(200);

    // Run controller init sequence
    init_sequence();

    // Test clear to confirm it works
    clear();
    return true;  // later: return false if I²C fails
}

void Oled::init_sequence() {
    cmd(0xAE);             // off
    cmd(0xD5); cmd(0x80);  // clock
    cmd(0xA8); cmd(height_ - 1);  // multiplex ratio (0x3F = 63 for 64 rows)
    cmd(0xD3); cmd(0x00);  // no offset
    cmd(0x40);             // start line
    cmd(0x8D); cmd(0x14);  // charge pump
    cmd(0x20); cmd(0x02);  // page addressing mode (better for SH1106)
    cmd(0xA1);             // segment remap (try 0xA0 if mirrored)
    cmd(0xC8);             // COM scan dir (try 0xC0 if upside down)
    cmd(0xDA); cmd(0x12);  // COM pins (may differ for other sizes)
    cmd(0x81); cmd(0xCF);  // contrast
    cmd(0xD9); cmd(0xF1);  // precharge
    cmd(0xDB); cmd(0x40);  // VCOMH
    cmd(0xA4);             // display from RAM
    cmd(0xA6);             // normal (not inverted)
    cmd(0xAF);             // on
}

void Oled::set_cursor(uint8_t col, uint8_t page) {
    cmd(0xB0 + page);                  // page
    cmd(0x00 + (col & 0x0F));          // low nibble
    cmd(0x10 + (col >> 4));            // high nibble
}

bool Oled::cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return i2c_write_blocking(i2c_, addr_, buf, 2, false) == 2;
}

bool Oled::data(const uint8_t* buf, size_t len) {
    uint8_t header[len + 1];
    header[0] = 0x40;
    std::memcpy(header + 1, buf, len);
    return i2c_write_blocking(i2c_, addr_, header, len + 1, false) == (int)(len + 1);
}

void Oled::clear() {
    for (uint8_t page = 0; page < pages_; ++page) {
        set_cursor(0, page);
        uint8_t zeros[ram_width_] = {};
        data(zeros, ram_width_);
    }
}

void Oled::set_contrast(uint8_t level) {
    cmd(0x81);      // set contrast control
    cmd(level);     // level 0–255
}