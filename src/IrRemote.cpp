#include "IrRemote.h"

IrRemote::IrRemote(uint pin, uint timeout_ms)
    : _pin(pin), _timeout_ms(timeout_ms)
{
    gpio_init(_pin);
    gpio_set_dir(_pin, GPIO_IN);
    gpio_pull_up(_pin);
}

IrButton IrRemote::getButton()
{
    uint8_t raw = readRawButtonId();
    if (raw == 0) {
        return IrButton::NONE;
    }

    // Direct cast + lookup — very efficient
    switch (raw) {
        case static_cast<uint8_t>(IrButton::BUTTON_0):   return IrButton::BUTTON_0;
        case static_cast<uint8_t>(IrButton::BUTTON_1):   return IrButton::BUTTON_1;
        case static_cast<uint8_t>(IrButton::BUTTON_2):   return IrButton::BUTTON_2;
        case static_cast<uint8_t>(IrButton::BUTTON_3):   return IrButton::BUTTON_3;
        case static_cast<uint8_t>(IrButton::BUTTON_4):   return IrButton::BUTTON_4;
        case static_cast<uint8_t>(IrButton::BUTTON_5):   return IrButton::BUTTON_5;
        case static_cast<uint8_t>(IrButton::BUTTON_6):   return IrButton::BUTTON_6;
        case static_cast<uint8_t>(IrButton::BUTTON_7):   return IrButton::BUTTON_7;
        case static_cast<uint8_t>(IrButton::BUTTON_8):   return IrButton::BUTTON_8;
        case static_cast<uint8_t>(IrButton::BUTTON_9):   return IrButton::BUTTON_9;
        case static_cast<uint8_t>(IrButton::BUTTON_UP):    return IrButton::BUTTON_UP;
        case static_cast<uint8_t>(IrButton::BUTTON_DOWN):  return IrButton::BUTTON_DOWN;
        case static_cast<uint8_t>(IrButton::BUTTON_LEFT):  return IrButton::BUTTON_LEFT;
        case static_cast<uint8_t>(IrButton::BUTTON_RIGHT): return IrButton::BUTTON_RIGHT;
        case static_cast<uint8_t>(IrButton::BUTTON_OK):    return IrButton::BUTTON_OK;
        case static_cast<uint8_t>(IrButton::BUTTON_LIST):  return IrButton::BUTTON_LIST;
        case static_cast<uint8_t>(IrButton::BUTTON_BACK):  return IrButton::BUTTON_BACK;
        default:                                           return IrButton::UNKNOWN;
    }
}

uint8_t IrRemote::readRawButtonId()
{
    uint64_t now = time_us_64();
    uint64_t start;
    uint32_t code = 0;

    // Wait for start falling edge
    while (gpio_get(_pin) && (time_us_64() - now < _timeout_ms * 1000ULL)) {}
    if (gpio_get(_pin)) return 0;

    // Header low (~9 ms)
    start = time_us_64();
    while (!gpio_get(_pin)) {}
    uint64_t header_low = time_us_64() - start;
    if (header_low < 7500 || header_low > 11000) return 0;

    // Header high (~4.5 ms)
    start = time_us_64();
    while (gpio_get(_pin)) {}
    uint64_t header_high = time_us_64() - start;
    if (header_high < 3500 || header_high > 5500) return 0;

    // Read 32 bits
    for (int i = 0; i < 32; ++i) {
        start = time_us_64();
        while (!gpio_get(_pin)) {}
        uint64_t bit_low = time_us_64() - start;
        if (bit_low < 350 || bit_low > 850) return 0;

        start = time_us_64();
        while (gpio_get(_pin)) {}
        uint64_t bit_high = time_us_64() - start;

        code >>= 1;
        if (bit_high > 1200) code |= 0x80000000;
    }

    if (code == 0xFFFFFFFF) return 0;  // repeat

    uint8_t addr  = (code >> 24) & 0xFF;
    uint8_t naddr = (code >> 16) & 0xFF;
    uint8_t cmd   = (code >>  8) & 0xFF;
    uint8_t ncmd  =  code        & 0xFF;

    // Validate NEC checksum
    if (addr == (uint8_t)~naddr && cmd == (uint8_t)~ncmd) {
        return naddr;
    }

    return 0;
}