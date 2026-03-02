#pragma once
#include <stdio.h>
#include "Oled.h"
#include "IrRemote.h"
#include "Font.h"
#include <cstdint>
#include "pico/stdlib.h"


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
    
    int itemCount = 7;
    const char* menuItems[itemCount] =
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
};