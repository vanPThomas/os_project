#include "IrRemote.h"

// IrRemote::IrRemote(uint pin, uint timeout_ms)
//     : _pin(pin), _timeout_ms(timeout_ms)
// {
//     gpio_init(_pin);
//     gpio_set_dir(_pin, GPIO_IN);
//     gpio_pull_up(_pin);
// }

IrRemote::IrRemote(uint32_t pin, uint32_t timeout_ms)
    : _pin(pin), _timeout_ms(timeout_ms)
{
    // 1. Make sure pin is controlled by SIO (not PIO/UART/I2C/etc.)
    //    GPIO function = 5 (SIO) = b101
    volatile uint32_t *io_bank_gpio_ctrl = (volatile uint32_t *)(IO_BANK0_BASE_ME + 0x0c + 8 * pin);
    *io_bank_gpio_ctrl = 5;                     // FUNCSEL = 5 (SIO)

    // 2. Disable output enable → pin becomes input
    *(volatile uint32_t *)(SIO_BASE_ME + SIO_GPIO_OE_CLR_ME) = GPIO_BIT(pin);

    // 3. Enable pull-up resistor
    volatile uint32_t *pad_gpio = (volatile uint32_t *)(PADS_BANK0_BASE_ME + 0x04 + 4 * pin);
    *pad_gpio = (*pad_gpio & ~(0x3 << 2)) | (1 << 3);  // IE=1 (input enable), PU=1 (pull-up)
}

IrButton IrRemote::getButton()
{
    uint8_t raw = readRawButtonId();
    if (raw == 0) {
        return IrButton::NONE;
    }

    // Direct cast + lookup — very efficient
    switch (raw) {
        case static_cast<uint8_t>(IrButton::BUTTON_0):   return IrButton::BUTTON_0;
        case static_cast<uint8_t>(IrButton::BUTTON_1):   return IrButton::BUTTON_1;
        case static_cast<uint8_t>(IrButton::BUTTON_2):   return IrButton::BUTTON_2;
        case static_cast<uint8_t>(IrButton::BUTTON_3):   return IrButton::BUTTON_3;
        case static_cast<uint8_t>(IrButton::BUTTON_4):   return IrButton::BUTTON_4;
        case static_cast<uint8_t>(IrButton::BUTTON_5):   return IrButton::BUTTON_5;
        case static_cast<uint8_t>(IrButton::BUTTON_6):   return IrButton::BUTTON_6;
        case static_cast<uint8_t>(IrButton::BUTTON_7):   return IrButton::BUTTON_7;
        case static_cast<uint8_t>(IrButton::BUTTON_8):   return IrButton::BUTTON_8;
        case static_cast<uint8_t>(IrButton::BUTTON_9):   return IrButton::BUTTON_9;
        case static_cast<uint8_t>(IrButton::BUTTON_UP):    return IrButton::BUTTON_UP;
        case static_cast<uint8_t>(IrButton::BUTTON_DOWN):  return IrButton::BUTTON_DOWN;
        case static_cast<uint8_t>(IrButton::BUTTON_LEFT):  return IrButton::BUTTON_LEFT;
        case static_cast<uint8_t>(IrButton::BUTTON_RIGHT): return IrButton::BUTTON_RIGHT;
        case static_cast<uint8_t>(IrButton::BUTTON_OK):    return IrButton::BUTTON_OK;
        case static_cast<uint8_t>(IrButton::BUTTON_LIST):  return IrButton::BUTTON_LIST;
        case static_cast<uint8_t>(IrButton::BUTTON_BACK):  return IrButton::BUTTON_BACK;
        default:                                           return IrButton::UNKNOWN;
    }
}

uint8_t IrRemote::readRawButtonId()
{
    uint64_t now = HardwareUtil::get_current_us();
    uint64_t start;
    uint32_t code = 0;

    // Wait for start falling edge
    while (read_gpio(_pin) && (HardwareUtil::get_current_us() - now < _timeout_ms * 1000ULL)) {}
    if (read_gpio(_pin)) return 0;

    // Header low (~9 ms)
    start = HardwareUtil::get_current_us();
    while (!read_gpio(_pin)) {}
    uint64_t header_low = HardwareUtil::get_current_us() - start;
    if (header_low < 7500 || header_low > 11000) return 0;

    // Header high (~4.5 ms)
    start = HardwareUtil::get_current_us();
    while (read_gpio(_pin)) {}
    uint64_t header_high = HardwareUtil::get_current_us() - start;
    if (header_high < 3500 || header_high > 5500) return 0;

    // Read 32 bits
    for (int i = 0; i < 32; ++i) {
        start = HardwareUtil::get_current_us();
        while (!read_gpio(_pin)) {}
        uint64_t bit_low = HardwareUtil::get_current_us() - start;
        if (bit_low < 350 || bit_low > 850) return 0;

        start = HardwareUtil::get_current_us();
        while (read_gpio(_pin)) {}
        uint64_t bit_high = HardwareUtil::get_current_us() - start;

        code >>= 1;
        if (bit_high > 1200) code |= 0x80000000;
    }

    if (code == 0xFFFFFFFF) return 0;  // repeat

    uint8_t addr  = (code >> 24) & 0xFF;
    uint8_t naddr = (code >> 16) & 0xFF;
    uint8_t cmd   = (code >>  8) & 0xFF;
    uint8_t ncmd  =  code        & 0xFF;

    // Validate NEC checksum
    if (addr == (uint8_t)~naddr && cmd == (uint8_t)~ncmd) {
        return naddr;
    }

    return 0;
}