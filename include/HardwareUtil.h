#pragma once
#include <cstdint>
#include <cstring>

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

// ----
// I2C0 / I2C1 Registers

// #define I2C0_BASE               0x40044000UL
// #define I2C1_BASE               0x40048000UL

// // Common I2C register offsets
// #define IC_CON                  0x00
// #define IC_TAR                  0x04
// #define IC_ENABLE               0x06
// #define IC_DATA_CMD             0x10
// #define IC_RAW_INTR_STAT        0x34
// #define IC_CLR_INTR             0x40
// #define IC_STATUS               0x08
// #define IC_TXFLR                0x18
// #define IC_RXFLR                0x1c
// #define IC_TXABRT_SOURCE        0x80
// #define IC_CLR_TX_ABRT          0x84

// // IC_CON bits
// #define IC_CON_MASTER_MODE      (1u << 0)
// #define IC_CON_SPEED_FAST       (1u << 1)     // Fast Mode (400kHz)
// #define IC_CON_RESTART_EN       (1u << 5)
// #define IC_CON_7BIT_ADDR        (0u << 4)     // 7-bit addressing

// // IC_STATUS bits
// #define IC_STATUS_TFNF          (1u << 2)     // TX FIFO Not Full
// #define IC_STATUS_TFE           (1u << 3)     // TX FIFO Empty

// // IC_DATA_CMD bits
// #define IC_DATA_CMD_STOP        (1u << 9)

// #define RESETS_BASE             0x4000c000UL
// #define RESETS_RESET            0x0
// #define RESETS_RESET_DONE       0x8

#define I2C0_BASE        0x40044000
#define RESETS_BASE      0x4000C000
#define RESETS_RESET     0x00
#define RESETS_RESET_DONE 0x08

#define IO_BANK0_BASE    0x40014000
#define PADS_BANK0_BASE  0x4001C000

// -----

namespace HardwareUtil
{
    // Time functions
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

    int i2c_bare_write(uint8_t addr, const uint8_t* buf, size_t len, bool nostop = false);
    void i2c_bare_init(uint32_t speed_hz);
}