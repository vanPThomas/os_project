#include "pico/stdlib.h"
#include "Oled.h"
#include "Font.h"
#include <stdio.h>
#include "IrRemote.h"

int main() {

    stdio_init_all();

    // Create the display object with your current parameters
    Oled display(i2c0,          // i2c instance
        4,             // SDA
        5,             // SCL
        400000,        // speed Hz
        0x3C,          // address
        128,           // visible width
        64,            // visible height
        132);          // internal RAM width (SH1106)
        
        // Initialize it
    if (!display.init()) {
        while (true) tight_loop_contents();
    }

    display.clear();

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
    Font::center_print(display, 3, "v0.1.5 - 2026");
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
                        display.clear();
                        uint64_t uptime_us = time_us_64();
                        uint32_t seconds = uptime_us / 1000000ULL;
                        uint32_t minutes = seconds / 60;
                        seconds %= 60;
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Uptime: %02u:%02u", minutes, seconds);
                        Font::center_print(display, 2, buf);
                        Font::center_print(display, 4, "Press any key");
                        // Wait for any button press to return
                        while (remote.getButton() == IrButton::NONE) {
                            sleep_ms(50);
                        }
                        menuNeedsRedraw = true;
                        break;
                    }
                    case 3: {
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
        if (menuNeedsRedraw || cursorMoved) {
            if (menuNeedsRedraw) {
                display.clear();

                Font::print(display, 3, 0, "Menu");
                Font::print(display, 3, 1, "Option 1");
                Font::print(display, 3, 2, "Uptime");
                Font::print(display, 3, 3, "Contrast");
                Font::print(display, 3, 4, "Option 4");
                Font::print(display, 3, 5, "Option 5");
                Font::print(display, 3, 6, "Option 6");
            }

            // Erase old cursor
            if (cursorMoved && !menuNeedsRedraw) {
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