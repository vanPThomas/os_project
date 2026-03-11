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
