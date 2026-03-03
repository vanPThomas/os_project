#pragma once
#include <stdio.h>
#include "Oled.h"
#include "IrRemote.h"
#include "Font.h"
#include <cstdint>
#include "pico/stdlib.h"

// Peripheral register base address for the RP2040 hardware timer
#define TIMER_BASE_ME          0x40054000UL
// Offset to lower 32 bits of the 64-bit free-running microsecond counter
#define TIMER_TIMERAWL_OFFSET_ME  0x28UL   // bits  0–31  (changes fast)
// Offset to upper 32 bits of the 64-bit free-running microsecond counter
#define TIMER_TIMERAWH_OFFSET_ME  0x2cUL   // bits 32–63  (changes slowly)

class MenuApp
{
public:
    MenuApp();

    void run();

private:
    Oled display;
    IrRemote remote;

    
    int cursorLocation = 1;           // start at first option
    int previousCursor = 0;           // to only redraw changed parts
    bool menuNeedsRedraw = true;      // flag for initial draw or major changes
    uint8_t contrast = 0xCF;

    uint64_t boot_offset_us = 0;
    
    int itemCount = 7;
    const char* menuItems[7] =
    {
        "Menu",     
        "Option 1",
        "Uptime",
        "Contrast",
        "Option 4",
        "Option 5",
        "Option 6"
    };

    void printContrast();
    void printUptime();
    void drawMenu();
    void bootSequence();
    void okButtonPress();

    uint64_t get_current_us();
    void my_sleep_ms(uint32_t millisec);
    void init_timer_offset();
};