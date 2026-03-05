#pragma once

// #include "pico/stdlib.h"
// #include "hardware/gpio.h"
#include <cstdint>
#include "HardwareUtil.h"

// SIO (Single-cycle IO) base – GPIO input/output registers
#define SIO_BASE_ME             0xd0000000UL

// Useful SIO offsets
#define SIO_GPIO_IN_ME             0x0004UL   // RO – 32-bit input levels (one bit per GPIO)
#define SIO_GPIO_OE_ME             0x0020UL   // RW – output enable (1 = output)
#define SIO_GPIO_OE_SET_ME         0x0024UL   // WO – set output enable bits
#define SIO_GPIO_OE_CLR_ME         0x0028UL   // WO – clear output enable bits

// IO_BANK0 base – function select, overrides, etc.
#define IO_BANK0_BASE_ME           0x40014000UL

// PADS_BANK0 base – pad controls (pull-up, pull-down, drive strength, etc.)
#define PADS_BANK0_BASE_ME         0x4001c000UL

// Helper macros (bit operations)
#define GPIO_BIT(pin)           (1UL << (pin))

enum class IrButton : uint8_t
{
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

class IrRemote
{
public:
    explicit IrRemote(uint32_t pin = 15, uint32_t timeout_ms = 800);
    ~IrRemote() = default;

    // Returns NONE if no valid new press or timeout/repeat
    IrButton getButton();

private:
    uint32_t _pin;
    uint32_t _timeout_ms;

    uint8_t readRawButtonId();


    // Inline helper
    inline bool read_gpio(uint32_t pin)
    {
        return (*(volatile uint32_t *)(SIO_BASE_ME + SIO_GPIO_IN_ME) & GPIO_BIT(pin)) != 0;
    }
};