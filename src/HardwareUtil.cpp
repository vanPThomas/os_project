#include "HardwareUtil.h"

// Busy-wait delay in milliseconds using the hardware timer (µs resolution)
void HardwareUtil::my_sleep_ms(uint32_t millisec)
{
    uint64_t start_us  = get_current_us();               // current time in microseconds (_us means microseconds)
    uint64_t delay_us  = (uint64_t)millisec * 1000ULL;   // convert milliseconds → microseconds
    uint64_t target_us = start_us + delay_us;

    // Spin until current time reaches or passes target
    // (64-bit math naturally handles wraparound after ~584k years)
    while (get_current_us() < target_us){}
}


// Returns microseconds since reset/power-on (bare-metal replacement for time_us_64())
uint64_t HardwareUtil::get_current_us()
{
    // Pointers to the timer registers (volatile = must read fresh from hardware every time)
    volatile uint32_t *timera_wh = (volatile uint32_t *)(TIMER_BASE_ME + TIMER_TIMERAWH_OFFSET_ME);
    volatile uint32_t *timera_wl = (volatile uint32_t *)(TIMER_BASE_ME + TIMER_TIMERAWL_OFFSET_ME);

    uint32_t high1 = *timera_wh;   // upper 32 bits first
    uint32_t low   = *timera_wl;   // lower 32 bits
    uint32_t high2 = *timera_wh;   // upper again to detect rollover

    // If high changed between reads → low might be inconsistent → re-read low
    if (high1 != high2)
    {
        high1 = high2;
        low   = *timera_wl;
    }

    // Combine into full 64-bit microseconds
    return ((uint64_t)high1 << 32) | low;
}

// boot timer in hardware doesn't start at 0
uint64_t HardwareUtil::get_boot_offset_us()
{
    static uint64_t offset = 0;
    if (offset == 0) {
        offset = get_current_us();
    }
    return offset;
}

void HardwareUtil::set_pin_function_sio(uint32_t pin)
{
    // Each GPIO has a CTRL register at offset 0x0c + 8*pin
    // FUNCSEL bits [4:0] = 5 → SIO control
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)(IO_BANK0_BASE_ME + 0x0c + 8 * pin);
    *ctrl_reg = 5;  // FUNCSEL = 5 (b101)
}

// make a pico pin as input pin
void HardwareUtil::set_pin_as_input(uint32_t pin)
{
    // Clear output-enable bit → pin becomes high-Z input
    *(volatile uint32_t *)(SIO_BASE_ME + SIO_GPIO_OE_CLR_ME) = GPIO_BIT(pin);
}


// void HardwareUtil::set_pin_pullup_enabled(uint32_t pin)
// {
//     volatile uint32_t *pad_reg = (volatile uint32_t *)(PADS_BANK0_BASE_ME + 0x04 + 4 * pin);

//     uint32_t current = *pad_reg;

//     // Clear PU (bit 3) and PD (bit 2)
//     current &= ~((1u << 2) | (1u << 3));

//     // Set IE=1 (bit 7), PU=1 (bit 3), and optionally IS=1 (bit 5)
//     current |= (1u << 7) | (1u << 3) | (1u << 5);

//     *pad_reg = current;
// }

void HardwareUtil::init_input_pin_with_pullup(uint32_t pin)
{
    set_pin_function_sio(pin);
    set_pin_as_input(pin);
    set_pin_pullup_enabled(pin);
}

// PIN FUNCTIONS

void HardwareUtil::set_pin_function_i2c(uint32_t pin)
{
    if (pin > 29) return;

    // GPIOx_CTRL register: IO_BANK0_BASE + 0x04 + 8*pin
    // FUNCSEL = 3 for I2C (see RP2040 datasheet 2.19.6.1)
    volatile uint32_t *gpio_ctrl = (volatile uint32_t *)
        (IO_BANK0_BASE + 0x04u + (pin * 8u));

    *gpio_ctrl = 3u;   // bits [4:0] = FUNCSEL = 3 (I2C0 or I2C1 depending on pin)
}

void HardwareUtil::set_pin_pullup_enabled(uint32_t pin)
{
    if (pin > 29) return;

    // PADS_BANK0_GPIOx register
    volatile uint32_t *pads_reg = (volatile uint32_t *)
        (PADS_BANK0_BASE + 0x04u + (pin * 4u));

    *pads_reg |= (1u << 3);   // PUE = 1  (pull-up enable)
    *pads_reg &= ~(1u << 2);  // PDN = 0  (pull-down disable)
}

// I2C INIT
void HardwareUtil::i2c_bare_init(uint32_t speed_hz)
{
    const uint32_t base = I2C0_BASE;        // GPIO 4/5 → I2C0
    const uint32_t clk_hz = 125000000UL;

    // 1. Release I2C0 from reset
    *(volatile uint32_t*)(RESETS_BASE + RESETS_RESET) &= ~(1u << 5);  // bit 5 = i2c0
    while ((*(volatile uint32_t*)(RESETS_BASE + RESETS_RESET_DONE) & (1u << 5)) == 0) {}

    // 2. Disable I2C
    *(volatile uint32_t*)(base + 0x40) = 0;   // IC_ENABLE offset = 0x40

    // 3. Configure IC_CON (Master, Fast mode, 7-bit, restart enabled)
    *(volatile uint32_t*)(base + 0x00) = 
        (1u << 0) |     // MASTER_MODE
        (2u << 1) |     // SPEED = 2 (Fast Mode)
        (1u << 5) |     // IC_RESTART_EN
        (0u << 6);      // 7-bit address

    // 4. Baudrate for ~400 kHz
    uint32_t hcnt, lcnt;
    if (speed_hz >= 400000) {
        hcnt = (clk_hz / (speed_hz * 2)) - 3;
        lcnt = (clk_hz / (speed_hz * 2)) - 1;
    } else {
        hcnt = (clk_hz / (speed_hz * 2)) - 7;
        lcnt = (clk_hz / (speed_hz * 2)) - 5;
    }
    if (hcnt < 8) hcnt = 8;
    if (lcnt < 8) lcnt = 8;

    *(volatile uint32_t*)(base + 0x1C) = hcnt;   // IC_FS_SCL_HCNT
    *(volatile uint32_t*)(base + 0x20) = lcnt;   // IC_FS_SCL_LCNT

    // Optional: SDA hold time (helps with some OLED displays)
    *(volatile uint32_t*)(base + 0x3C) = 1;      // IC_SDA_HOLD (offset 0x3C)

    // 5. Enable I2C
    *(volatile uint32_t*)(base + 0x40) = 1;      // IC_ENABLE
    my_sleep_ms(10);
}

// I2C WRITE
int HardwareUtil::i2c_bare_write(uint8_t addr, const uint8_t* buf, size_t len, bool nostop)
{
    if (len == 0) return 0;

    const uint32_t base = I2C0_BASE;

    // Clear previous abort and stop
    *(volatile uint32_t*)(base + 0x54) = 1;   // IC_CLR_TX_ABRT   offset 0x54
    *(volatile uint32_t*)(base + 0x5C) = 1;   // IC_CLR_STOP_DET  offset 0x5C

    // Set target address
    *(volatile uint32_t*)(base + 0x04) = addr & 0x7F;   // IC_TAR offset 0x04

    size_t i = 0;
    for (; i < len; ++i)
    {
        uint32_t data_cmd = buf[i];

        // Send STOP with the last byte (if not nostop)
        if (i == len - 1 && !nostop)
            data_cmd |= (1u << 9);   // STOP bit

        // Wait for TX FIFO not full (TFNF bit 1)
        while ((*(volatile uint32_t*)(base + 0x70) & (1u << 1)) == 0) {}

        *(volatile uint32_t*)(base + 0x10) = data_cmd;   // IC_DATA_CMD offset 0x10

        // Check for abort (NACK etc.)
        if (*(volatile uint32_t*)(base + 0x58) & (1u << 3))   // RAW_INTR_STAT TX_ABRT bit 3
        {
            *(volatile uint32_t*)(base + 0x54) = 1;   // clear TX_ABRT
            return (int)i;   // return bytes successfully queued
        }
    }

    // Wait until TX FIFO is completely empty
    while ((*(volatile uint32_t*)(base + 0x70) & (1u << 2)) == 0) {}  // TFE bit 2

    if (!nostop)
    {
        // Wait for STOP condition
        while ((*(volatile uint32_t*)(base + 0x58) & (1u << 5)) == 0) {}  // STOP_DET bit 5
        *(volatile uint32_t*)(base + 0x5C) = 1;   // clear STOP_DET
    }

    return (int)len;
}