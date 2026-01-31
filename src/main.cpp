#include "pico/stdlib.h"
#include "Oled.h"
#include "Font.h"
#include <stdio.h>
#include "IrRemote.h"
#include "pico/cyw43_arch.h"

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
        
        display.clear();  // now using instance
        
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
    Font::center_print(display, 3, "v0.1.3 - 2026");
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
            sleep_ms(200);  // longer debounce when button pressed

            // Handle selection (OK)
            if (btn == IrButton::BUTTON_OK) {
                const char* selected = "Selected!";
                switch (cursorLocation) {
                    case 1: selected = "Option 1 chosen"; break;
                    case 2: selected = "Option 2 chosen"; break;
                    
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
                Font::print(display, 3, 2, "Option 2");
                Font::print(display, 3, 3, "Option 3");
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