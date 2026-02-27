// MenuApp.cpp
#include "MenuApp.h"

MenuApp::MenuApp()
    : display(i2c0, 4, 5, 400000, 0x3C, 128, 64, 132),
      remote(15)
{
    Oled display(
        i2c0,          // i2c instance
        4,             // SDA
        5,             // SCL
        400000,        // speed Hz
        0x3C,          // address
        128,           // visible width
        64,            // visible height
        132);          // internal RAM width (SH1106)

    if (!display.init())
    {
        while (true) tight_loop_contents();
    }

    display.clear();

    bootSequence();
    display.clear();

    IrRemote remote(15);
}

void MenuApp::run()
{
    while (true)
    {
        IrButton btn = remote.getButton();

        // Handle navigation
        bool cursorMoved = false;
        if (btn != IrButton::NONE)
        {
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
                okButtonPress();
            }
        }

        // Only redraw when needed
        if (menuNeedsRedraw || cursorMoved)
        {
            if (menuNeedsRedraw)
            {
                drawMenu();
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
}

void MenuApp::printContrast()
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
        if (btn == IrButton::BUTTON_UP)
        {
            if (contrast < 255) contrast += 5;
            display.set_contrast(contrast);
        }
        if (btn == IrButton::BUTTON_DOWN)
        {
            if (contrast > 5) contrast -= 5;
            display.set_contrast(contrast);
        }
        if (btn == IrButton::BUTTON_OK || btn == IrButton::BUTTON_BACK)
        {
            menuNeedsRedraw = true;
            break;
        }
        sleep_ms(50);
    }
}

void MenuApp::printUptime()
{
    display.clear();
    char buf[32];
    uint64_t now = time_us_64();
    uint64_t uptime_us = now;
    uint32_t seconds = uptime_us / 1000000ULL;
    uint32_t minutes = seconds / 60;
    uint32_t hours   = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, seconds);
    Font::center_print(display, 1, "Uptime");
    Font::center_print(display, 3, buf);
    Font::center_print(display, 5, "Any key to exit");

    uint64_t last_update = time_us_64();
    while (true)
    {
        // Update every 5 seconds
        now = time_us_64();
        if (now - last_update >= 5000000ULL)   // 5 seconds in µs
        {
            uptime_us = now;
            seconds = uptime_us / 1000000ULL;
            minutes = seconds / 60;
            hours   = minutes / 60;
            seconds %= 60;
            minutes %= 60;
            snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, seconds);
            display.clear();
            Font::center_print(display, 1, "Uptime");
            Font::center_print(display, 3, buf);
            Font::center_print(display, 5, "Any key to exit");


            last_update = now;
        }

        // Exit on any button press
        if (remote.getButton() != IrButton::NONE)
        {
            menuNeedsRedraw = true;
            break;
        }

        sleep_ms(50);  // light sleep to not burn CPU
    }
}

void MenuApp::drawMenu()
{
    display.clear();
    for (int i = 0; i < itemCount; ++i) {
        Font::print(display, 3, i, menuItems[i]);
    }
    Font::print(display, 1, cursorLocation, ">");
}

void MenuApp::bootSequence()
{
    // Boot animation - pass display to Font functions
    for (uint8_t i = 0; i < 4; ++i)
    {
        Font::print(display, 1, 4, "Booting.");
        sleep_ms(200);
        Font::print(display, 1, 4, "Booting..");
        sleep_ms(200);
        Font::print(display, 1, 4, "Booting...");
        sleep_ms(200);
        display.clear();
    }

    Font::center_print(display, 1, "PICO OS");
    Font::center_print(display, 3, PROJECT_VERSION);
    Font::center_print(display, 5, "PRESS ANY KEY");

    sleep_ms(2000);
}

void MenuApp::okButtonPress()
{
    const char* selected = "Selected!";
    switch (cursorLocation)
    {
        case 1: selected = "Option 1 chosen"; break;
        case 2:
        {
            printUptime();
            break;
        }
        case 3:
        {
            printContrast();
            break;
        }
        
        default: break;
    }
    display.clear();
    Font::center_print(display, 3, selected);
    sleep_ms(1500);               // show for 1.5 sec
    menuNeedsRedraw = true;       // force redraw menu after
}