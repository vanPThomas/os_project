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

void HardwareUtil::i2c_bare_init(uint32_t speed_hz)
{
    // Use I2C0 – change to I2C1_BASE if using i2c1
    const uint32_t base = I2C0_BASE;

    // Step 1: Release I2C from reset (RESETS register)
    volatile uint32_t *resets = (volatile uint32_t *)0x4000c000UL;
    *resets &= ~(1u << 5);   // clear reset for i2c0 (bit 5)

    // Step 2: Disable I2C before config
    *(volatile uint32_t *)(base + IC_ENABLE) = 0;

    // Step 3: Configure IC_CON (control)
    volatile uint32_t *ic_con = (volatile uint32_t *)(base + IC_CON);
    *ic_con = IC_CON_MASTER_MODE
            | IC_CON_SPEED_FAST          // Fast mode (400 kHz)
            | IC_CON_RESTART_EN
            | (1u << 3);                 // 7-bit address mode

    // Step 4: Set baudrate (for 400 kHz at typical 125 MHz sysclk)
    // Formula from datasheet §4.3.3.1
    uint32_t clk_hz = 125000000;  // Change if your clk_sys is different
    uint32_t min_scl_period = clk_hz / speed_hz;
    uint32_t hcnt = min_scl_period / 2;     // Rough 50% duty
    uint32_t lcnt = min_scl_period - hcnt;

    *(volatile uint32_t *)(base + 0x1C) = hcnt;  // IC_FS_SCL_HCNT
    *(volatile uint32_t *)(base + 0x20) = lcnt;  // IC_FS_SCL_LCNT

    // Optional: FIFO thresholds (default 0 is fine for small transfers)
    // *(volatile uint32_t *)(base + 0x1c) = 0; // IC_TX_TL
    // *(volatile uint32_t *)(base + 0x20) = 0; // IC_RX_TL

    // Step 5: Enable I2C
    *(volatile uint32_t *)(base + IC_ENABLE) = 1;

    // Small delay for peripheral to stabilize
    my_sleep_ms(1);
}

bool HardwareUtil::i2c_bare_write(uint8_t addr, const uint8_t* buf, size_t len, bool nostop)
{
    const uint32_t base = I2C0_BASE;

    // Clear any previous abort/interrupt
    *(volatile uint32_t *)(base + IC_CLR_INTR) = 1;
    *(volatile uint32_t *)(base + IC_CLR_TX_ABRT) = 1;

    // Set target address (7-bit)
    *(volatile uint32_t *)(base + IC_TAR) = addr & 0x7F;

    bool ok = true;

    for (size_t i = 0; i < len; ++i) {
        uint32_t cmd = buf[i]
                     | IC_DATA_CMD_CMD_WRITE
                     | (i == len - 1 && !nostop ? (1u << 9) : 0);  // STOP on last if !nostop

        // Wait until TX FIFO has space
        while ((*(volatile uint32_t *)(base + IC_STATUS) & IC_STATUS_TFNF) == 0) {}

        *(volatile uint32_t *)(base + IC_DATA_CMD) = cmd;

        // Optional: wait for ACK on each byte (poll TX abort)
        // Better: wait until TX FIFO empty after last byte
        if (i == len - 1) {
            while ((*(volatile uint32_t *)(base + IC_STATUS) & IC_STATUS_TFE) == 0) {}
        }

        // Check for abort (NACK, etc.)
        if (*(volatile uint32_t *)(base + IC_RAW_INTR_STAT) & (1u << 3)) {  // TX_ABRT
            ok = false;
            *(volatile uint32_t *)(base + IC_CLR_TX_ABRT) = 1;  // clear
            break;
        }
    }

    // Wait for STOP complete if sent
    if (!nostop) {
        while ((*(volatile uint32_t *)(base + IC_RAW_INTR_STAT) & (1u << 5)) == 0) {}  // STOP_DET
        *(volatile uint32_t *)(base + IC_CLR_INTR) = 1;
    }

    return ok;
}