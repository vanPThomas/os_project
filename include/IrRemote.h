// IrRemote.h
#pragma once

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include <cstdint>

enum class IrButton : uint8_t {
    NONE       = 0,
    BUTTON_0   = 0x10,
    BUTTON_1   = 0x11,
    BUTTON_2   = 0x12,
    BUTTON_3   = 0x13,
    BUTTON_4   = 0x14,
    BUTTON_5   = 0x15,
    BUTTON_6   = 0x16,
    BUTTON_7   = 0x17,
    BUTTON_8   = 0x18,
    BUTTON_9   = 0x19,
    BUTTON_UP    = 0x40,
    BUTTON_DOWN  = 0x41,
    BUTTON_LEFT  = 0x07,
    BUTTON_RIGHT = 0x06,
    BUTTON_OK    = 0x44,
    BUTTON_LIST  = 0x53,
    BUTTON_BACK  = 0x28,
    UNKNOWN    = 0xFF
};

class IrRemote {
public:
    explicit IrRemote(uint pin = 15, uint timeout_ms = 800);
    ~IrRemote() = default;

    // Returns NONE if no valid new press or timeout/repeat
    IrButton getButton();

private:
    uint _pin;
    uint _timeout_ms;

    uint8_t readRawButtonId();
};