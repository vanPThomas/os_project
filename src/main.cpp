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
        // Optional: handle error (e.g. blink LED or printf)
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
    Font::center_print(display, 3, "v0.1 - 2026");
    Font::center_print(display, 5, "PRESS ANY KEY");

    sleep_ms(2000);
    display.clear();


    
    IrRemote remote(15);

    while (true) {
        IrButton btn = remote.getButton();

        if (btn != IrButton::NONE) {
            display.clear();

            const char* label = "Unknown";

            switch (btn) {
                case IrButton::BUTTON_0:   label = "0";     break;
                case IrButton::BUTTON_1:   label = "1";     break;
                case IrButton::BUTTON_2:   label = "2";     break;
                case IrButton::BUTTON_3:   label = "3";     break;
                case IrButton::BUTTON_4:   label = "4";     break;
                case IrButton::BUTTON_5:   label = "5";     break;
                case IrButton::BUTTON_6:   label = "6";     break;
                case IrButton::BUTTON_7:   label = "7";     break;
                case IrButton::BUTTON_8:   label = "8";     break;
                case IrButton::BUTTON_9:   label = "9";     break;
                case IrButton::BUTTON_UP:    label = "Up";    break;
                case IrButton::BUTTON_DOWN:  label = "Down";  break;
                case IrButton::BUTTON_LEFT:  label = "Left";  break;
                case IrButton::BUTTON_RIGHT: label = "Right"; break;
                case IrButton::BUTTON_OK:    label = "OK";    break;
                case IrButton::BUTTON_LIST:  label = "List";  break;
                case IrButton::BUTTON_BACK:  label = "Back";  break;
                case IrButton::UNKNOWN:      label = "???";   break;
                default:                     label = "Other"; break;
            }

            Font::center_print(display, 2, label);
            // if (btn == IrButton::BUTTON_UP)    { menuUp();    }
            // if (btn == IrButton::BUTTON_OK)    { selectItem(); }
            // if (btn == IrButton::BUTTON_BACK)  { goBack();     }
            // etc.

            sleep_ms(350);  // simple debounce / visual feedback delay
        }

        sleep_ms(25);  // light polling — adjust as needed
    }

    // unreachable
    return 0;
}