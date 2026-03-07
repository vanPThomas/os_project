# Pico OS

A tiny "operating system" experiment for the **Raspberry Pi Pico W** written in low-level C++.

The goal is to build a minimal, self-contained system from scratch using no libraries.  
It currently features:

- 128×64 SSD1306/SH1106 OLED costum display driver (I²C)
- Custom bitmap font rendering
- Polling-based NEC IR remote control decoder
- Button mapping for a typical cheap 21-key IR remote
- Simple boot animation and press-to-interact screen

Very much a work-in-progress hobby project to explore bare-metal programming on microcontrollers.

### Current Hardware Setup

- **Board**: Raspberry Pi Pico W
- **Display**: 0.96" or 1.3" I²C OLED (128×64, SSD1306 or SH1106 controller)
  - SDA → GP4
  - SCL → GP5
  - VCC → 3.3V
  - GND → GND
- **IR Receiver**: VS1838B / TSOP4838 / HX1838 (38 kHz)
  - OUT → GP15
  - VCC → 3.3V
  - GND → GND
- **Remote**: Generic IR remote (NEC protocol variant)

### Building & Flashing


1. `mkdir build && cd build`
2. `cmake -DPICO_BOARD=pico_w ..`
3. `make -j`
4. Hold BOOTSEL button → drag&drop `pico_os.uf2` to the RPI-RP2 drive or use picotools

### Features (so far)

- Animated boot sequence
- Welcome screen
- IR remote button detection → shows button name on OLED
- Clean C++ class structure for display & IR handling

### Planned / Future Ideas

- Simple menu system with cursor navigation
- Multiple screens / states
- Basic "apps" (clock, text viewer, mini games…)
- Maybe UART shell later
- Networking

Feedback and ideas welcome!