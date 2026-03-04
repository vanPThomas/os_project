#pragma once
#include <cstdint>

// Peripheral register base address for the RP2040 hardware timer
#define TIMER_BASE_ME          0x40054000UL
// Offset to lower 32 bits of the 64-bit free-running microsecond counter
#define TIMER_TIMERAWL_OFFSET_ME  0x28UL   // bits  0–31  (changes fast)
// Offset to upper 32 bits of the 64-bit free-running microsecond counter
#define TIMER_TIMERAWH_OFFSET_ME  0x2cUL   // bits 32–63  (changes slowly)

namespace HardwareUtil
{
    uint64_t get_current_us();
    void my_sleep_ms(uint32_t millisec);
    uint64_t init_timer_offset();
}