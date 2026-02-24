#include "pico/stdlib.h"
#include "MenuApp.h"

int main()
{
    stdio_init_all();
    MenuApp menuApp;
    menuApp.run();
}