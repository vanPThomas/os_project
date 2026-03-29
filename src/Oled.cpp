#include "Oled.h"

Oled::Oled(uint32_t sda_pin,
           uint32_t scl_pin,
           uint32_t speed_hz,
           uint8_t addr,
           uint16_t width,
           uint16_t height,
           uint16_t ram_width)
    : sda_pin_(sda_pin),
      scl_pin_(scl_pin),
      speed_hz_(speed_hz),
      addr_(addr),
      width_(width),
      height_(height),
      ram_width_(ram_width),
      pages_(height / 8)
{}

bool Oled::init()
{
    // I2C setup
    // i2c_init(i2c_, speed_hz_);
    HardwareUtil::i2c_bare_init(speed_hz_);
    HardwareUtil::set_pin_function_i2c(sda_pin_);
    HardwareUtil::set_pin_function_i2c(scl_pin_);
    HardwareUtil::set_pin_pullup_enabled(sda_pin_);
    HardwareUtil::set_pin_pullup_enabled(scl_pin_);
    HardwareUtil::my_sleep_ms(200);

    // Run controller init sequence
    init_sequence();

    // Test clear to confirm it works
    clear();
    return true;
}

void Oled::init_sequence()
{
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

void Oled::set_cursor(uint8_t col, uint8_t page)
{
    cmd(0xB0 + page);                  // page
    cmd(0x00 + (col & 0x0F));          // low nibble
    cmd(0x10 + (col >> 4));            // high nibble
}

bool Oled::cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    // return i2c_write_blocking(i2c_, addr_, buf, 2, false) == 2;
    return HardwareUtil::i2c_bare_write(addr_, buf, 2, false) == 2;
}

bool Oled::data(const uint8_t* buf, size_t len)
{
    uint8_t header[len + 1];
    header[0] = 0x40;
    std::memcpy(header + 1, buf, len);
    // return i2c_write_blocking(i2c_, addr_, header, len + 1, false) == (int)(len + 1);
    return HardwareUtil::i2c_bare_write(addr_, header, len + 1, false) == (int)(len + 1);
}

void Oled::clear()
{
    for (uint8_t page = 0; page < pages_; ++page) {
        set_cursor(0, page);
        uint8_t zeros[ram_width_] = {};
        data(zeros, ram_width_);
    }
}

void Oled::set_contrast(uint8_t level)
{
    cmd(0x81);      // set contrast control
    cmd(level);     // level 0–255
}