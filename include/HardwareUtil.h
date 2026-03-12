#pragma once
#include <cstdint>

// Peripheral register base address for the RP2040 hardware timer
#define TIMER_BASE_ME          0x40054000UL
// Offset to lower 32 bits of the 64-bit free-running microsecond counter
#define TIMER_TIMERAWL_OFFSET_ME  0x28UL   // bits  0–31  (changes fast)
// Offset to upper 32 bits of the 64-bit free-running microsecond counter
#define TIMER_TIMERAWH_OFFSET_ME  0x2cUL   // bits 32–63  (changes slowly)

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

namespace HardwareUtil
{
    uint64_t get_current_us();
    void my_sleep_ms(uint32_t millisec);
    uint64_t get_boot_offset_us();

    // Sets the GPIO function select to SIO (software GPIO control)
    void set_pin_function_sio(uint32_t pin);

    // replacement for ppio_set_function from pico-sdk where you set pin to i2c
    void set_pin_function_i2c(uint32_t pin);

    // Configures pin as input (disables output driver)
    void set_pin_as_input(uint32_t pin);

    // Enables internal pull-up resistor + input buffer (replacement for pico sdk function gpio_pull_up)
    void set_pin_pullup_enabled(uint32_t pin);

    // Convenience: configure pin as input with pull-up (most common for buttons/IR)
    void init_input_pin_with_pullup(uint32_t pin);

    inline bool read_gpio(uint32_t pin)
    {
        return (*(volatile uint32_t *)(SIO_BASE_ME + SIO_GPIO_IN_ME) & GPIO_BIT(pin)) != 0;
    }


}