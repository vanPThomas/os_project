#include "pico/stdlib.h"
#include "Oled.h"
#include "Font.h"
#include <stdio.h>
#include "IrRemote.h"
#include "hardware/uart.h"

void printUptime(Oled display, bool menuNeedsRedraw, IrRemote remote);
void printContrast(Oled display, bool menuNeedsRedraw, IrRemote remote);
void drawMenu(Oled& display, int cursorPos, const char* const* items, int itemCount);

int main() {

    stdio_init_all();

    // Create the display object with your current parameters
    Oled display(
        i2c0,          // i2c instance
        4,             // SDA
        5,             // SCL
        400000,        // speed Hz
        0x3C,          // address
        128,           // visible width
        64,            // visible height
        132);          // internal RAM width (SH1106)

    static const char* menuItems[7] = {
        "Menu",     
        "Option 1",
        "Uptime",
        "Contrast",
        "Option 4",
        "Option 5",
        "Option 6"
    };

    if (!display.init())
    {
        while (true) tight_loop_contents();
    }

    display.clear();  // ready for boot animation

    // Boot animation - pass display to Font functions
    for (uint8_t i = 0; i < 4; ++i) {
        Font::print(display, 1, 4, "Booting.");
        sleep_ms(200);
        Font::print(display, 1, 4, "Booting..");
        sleep_ms(200);
        Font::print(display, 1, 4, "Booting...");
        sleep_ms(200);
        display.clear();
    }

    Font::center_print(display, 1, "PICO OS");
    Font::center_print(display, 3, "v0.1.6 - 2026");
    Font::center_print(display, 5, "PRESS ANY KEY");

    sleep_ms(2000);
    display.clear();

    IrRemote remote(15);

    static int cursorLocation = 1;           // start at first option
    static int previousCursor = 0;           // to only redraw changed parts
    static bool menuNeedsRedraw = true;      // flag for initial draw or major changes

    while (true) {
        IrButton btn = remote.getButton();

        // Handle navigation

        bool cursorMoved = false;
        if (btn != IrButton::NONE) {
            if (btn == IrButton::BUTTON_DOWN) {
                cursorLocation++;
                cursorMoved = true;
            }
            if (btn == IrButton::BUTTON_UP) {
                cursorLocation--;
                cursorMoved = true;
            }

            // Wrap around (1 to 6)
            if (cursorLocation < 1) cursorLocation = 6;
            if (cursorLocation > 6) cursorLocation = 1;

            // Simple debounce + visual delay
            sleep_ms(200);

            // Handle selection (OK)
            if (btn == IrButton::BUTTON_OK) {
                const char* selected = "Selected!";
                switch (cursorLocation) {
                    case 1: selected = "Option 1 chosen"; break;
                    case 2:
                    {
                        printUptime(display, menuNeedsRedraw, remote);
                        break;
                    }
                    case 3:
                    {
                        printContrast(display, menuNeedsRedraw, remote);
                        break;
                    }
                    
                    default: break;
                }
                display.clear();
                Font::center_print(display, 3, selected);
                sleep_ms(1500);               // show for 1.5 sec
                menuNeedsRedraw = true;       // force redraw menu after
            }
        }

        // Only redraw when needed
        if (menuNeedsRedraw || cursorMoved)
        {
            if (menuNeedsRedraw)
            {
                drawMenu(display, cursorLocation, menuItems, 7);
            }

            // Erase old cursor
            if (cursorMoved && !menuNeedsRedraw)
            {
                Font::print(display, 1, previousCursor, " ");  // blank
            }

            // Draw new cursor
            Font::print(display, 1, cursorLocation, ">");

            previousCursor = cursorLocation;
            menuNeedsRedraw = false;
        }

        // Light sleep when idle — prevents 100% CPU and flicker
        sleep_ms(50);
    }

    return 0;
}

void printContrast(Oled display, bool menuNeedsRedraw, IrRemote remote)
{
    static uint8_t contrast = 0xCF;  // default
    display.clear();
    Font::center_print(display, 1, "Contrast");
    char buf[16];
    snprintf(buf, sizeof(buf), "Level: %d", contrast);
    Font::center_print(display, 3, buf);
    Font::center_print(display, 5, "Up/Down to adjust");
    
    while (true) {
        IrButton btn = remote.getButton();
        if (btn == IrButton::BUTTON_UP) {
            if (contrast < 255) contrast += 5;
            display.set_contrast(contrast);
        }
        if (btn == IrButton::BUTTON_DOWN) {
            if (contrast > 5) contrast -= 5;
            display.set_contrast(contrast);
        }
        if (btn == IrButton::BUTTON_OK || btn == IrButton::BUTTON_BACK) {
            menuNeedsRedraw = true;
            break;
        }
        sleep_ms(50);
    }
}

void printUptime(Oled display, bool menuNeedsRedraw, IrRemote remote)
{
    display.clear();
    Font::center_print(display, 1, "Uptime");
    Font::center_print(display, 5, "Any key to exit");

    uint64_t last_update = time_us_64();
    while (true) {
        // Update every 5 seconds
        uint64_t now = time_us_64();
        if (now - last_update >= 5000000ULL) {  // 5 seconds in µs
            uint64_t uptime_us = now;
            uint32_t seconds = uptime_us / 1000000ULL;
            uint32_t minutes = seconds / 60;
            uint32_t hours   = minutes / 60;
            seconds %= 60;
            minutes %= 60;

            char buf[32];
            snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, seconds);
            Font::center_print(display, 3, buf);

            last_update = now;
        }

        // Exit on any button press
        if (remote.getButton() != IrButton::NONE) {
            menuNeedsRedraw = true;
            break;
        }

        sleep_ms(50);  // light sleep to not burn CPU
    }
}

void drawMenu(Oled& display, int cursorPos, const char* const* items, int itemCount)
{
    display.clear();
    for (int i = 0; i < itemCount; ++i) {
        Font::print(display, 3, i, items[i]);
    }
    Font::print(display, 1, cursorPos, ">");
}
