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


void HardwareUtil::set_pin_pullup_enabled(uint32_t pin)
{
    volatile uint32_t *pad_reg = (volatile uint32_t *)(PADS_BANK0_BASE_ME + 0x04 + 4 * pin);

    uint32_t current = *pad_reg;

    // Clear PU (bit 3) and PD (bit 2)
    current &= ~((1u << 2) | (1u << 3));

    // Set IE=1 (bit 7), PU=1 (bit 3), and optionally IS=1 (bit 5)
    current |= (1u << 7) | (1u << 3) | (1u << 5);

    *pad_reg = current;
}

void HardwareUtil::init_input_pin_with_pullup(uint32_t pin)
{
    set_pin_function_sio(pin);
    set_pin_as_input(pin);
    set_pin_pullup_enabled(pin);
}

void HardwareUtil::set_pin_function_i2c(uint32_t pin)
{
    // Set GPIO function select to I2C mode
    // FUNCSEL = 3 for most pins on i2c0 / i2c1 (see RP2040 datasheet Table 84 / 2.19.2)
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)(IO_BANK0_BASE_ME + 0x0c + 8 * pin);
    *ctrl_reg = 3;  // b011 = I2C function
}

// ===================================================================
// I2C Bare-metal Init (400 kHz)
// ===================================================================
void HardwareUtil::i2c_bare_init(uint32_t speed_hz)
{
    const uint32_t base = I2C0_BASE;

    // 1. Release I2C0 from reset
    volatile uint32_t *reset_reg = (volatile uint32_t *)(RESETS_BASE + RESETS_RESET);
    *reset_reg &= ~(1u << 5);                    // clear i2c0 reset bit

    // Wait for reset to complete
    volatile uint32_t *reset_done = (volatile uint32_t *)(RESETS_BASE + RESETS_RESET_DONE);
    while ((*reset_done & (1u << 5)) == 0) {}

    // 2. Disable I2C before configuration
    *(volatile uint32_t *)(base + IC_ENABLE) = 0;

    // 3. Configure IC_CON
    *(volatile uint32_t *)(base + IC_CON) =
        IC_CON_MASTER_MODE |
        IC_CON_SPEED_FAST |
        IC_CON_RESTART_EN |
        IC_CON_7BIT_ADDR;

    // 4. Set baudrate for Fast Mode (~400 kHz)
    uint32_t clk_hz = 125000000UL;
    uint32_t period = clk_hz / speed_hz;

    uint32_t hcnt = (period * 40) / 100;   // ~40% high time
    uint32_t lcnt = period - hcnt;

    if (hcnt < 8) hcnt = 8;
    if (lcnt < 8) lcnt = 8;

    *(volatile uint32_t *)(base + 0x1C) = hcnt;   // IC_FS_SCL_HCNT
    *(volatile uint32_t *)(base + 0x20) = lcnt;   // IC_FS_SCL_LCNT

    // 5. Enable I2C
    *(volatile uint32_t *)(base + IC_ENABLE) = 1;

    my_sleep_ms(2);   // stabilization delay
}

// ===================================================================
// I2C Bare-metal Write
// ===================================================================
bool HardwareUtil::i2c_bare_write(uint8_t addr, const uint8_t* buf, size_t len, bool nostop)
{
    const uint32_t base = I2C0_BASE;

    // Clear previous errors
    *(volatile uint32_t *)(base + IC_CLR_INTR) = 0xFFFFFFFF;
    *(volatile uint32_t *)(base + IC_CLR_TX_ABRT) = 1;

    // Set target address
    *(volatile uint32_t *)(base + IC_TAR) = addr & 0x7F;

    bool success = true;

    for (size_t i = 0; i < len; ++i)
    {
        uint32_t data_cmd = (uint32_t)buf[i]
                          | (i == len - 1 && !nostop ? IC_DATA_CMD_STOP : 0);

        // Wait for TX FIFO not full
        while ((*(volatile uint32_t *)(base + IC_STATUS) & IC_STATUS_TFNF) == 0) {}

        *(volatile uint32_t *)(base + IC_DATA_CMD) = data_cmd;

        // After last byte, wait until TX FIFO is empty
        if (i == len - 1)
        {
            while ((*(volatile uint32_t *)(base + IC_STATUS) & IC_STATUS_TFE) == 0) {}
        }

        // Check for NACK / abort
        if (*(volatile uint32_t *)(base + IC_RAW_INTR_STAT) & (1u << 3))  // TX_ABRT
        {
            success = false;
            *(volatile uint32_t *)(base + IC_CLR_TX_ABRT) = 1;
            break;
        }
    }

    // Wait for STOP condition if we sent one
    if (!nostop)
    {
        while ((*(volatile uint32_t *)(base + IC_RAW_INTR_STAT) & (1u << 5)) == 0) {} // STOP_DET
        *(volatile uint32_t *)(base + IC_CLR_INTR) = 1;
    }

    return success;
}