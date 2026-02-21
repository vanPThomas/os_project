#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "gspi.pio.h"

// PIO program for gSPI (half-duplex, 32-bit aligned transfers)
static const uint16_t cyw_gspi_pio_program[] = {
    // .program spi_cpha0_sample1
    0x6001, // out pins, 1   side 0
    0x0300, // jmp x-- lp    side 1  (lp is offset 0)
    0xc030, // set pindirs, 0 side 0  (switch to input)
    0xa001, // in pins, 1    side 1
    0x0303, // jmp y-- lp2   side 0  (lp2 is offset 3)
    // wrap
};

class Cyw43439 {
private:
    PIO pio_;
    uint sm_;
    uint offset_;
    int dma_tx_;
    int dma_rx_;
    uint32_t freq_ = 12000000;  // Start at 12 MHz, can increase after init

public:
    Cyw43439(PIO pio = pio0) : pio_(pio), sm_(-1), offset_(-1), dma_tx_(-1), dma_rx_(-1) {}

    bool init() {
        // Pins setup
        gpio_init(29);  // WL_REG_ON
        gpio_set_dir(29, GPIO_OUT);
        gpio_put(29, 0);  // Low to reset
        sleep_ms(20);
        gpio_put(29, 1);  // High to power on
        sleep_ms(50);   // Wait for stabilization

        gpio_set_function(23, GPIO_FUNC_PIO0);  // CLK
        gpio_set_function(24, GPIO_FUNC_PIO0);  // DATA out/in
        gpio_set_function(25, GPIO_FUNC_GPIO);  // CS (manual control)
        gpio_set_dir(25, GPIO_OUT);
        gpio_put(25, 1);  // High (inactive)

        // Load PIO program
        offset_ = pio_add_program(pio_, &cyw_gspi_pio_program Instructions);  // Error: this is pseudo; use pio_add_program with struct
        // Correct way: Define pio_program_t
        pio_program_t prog = {
            .instructions = cyw_gspi_pio_program,
            .length = sizeof(cyw_gspi_pio_program) / sizeof(uint16_t),
            .origin = -1
        };
        offset_ = pio_add_program(pio_, &prog);

        // Claim state machine
        sm_ = pio_claim_unused_sm(pio_, true);
        if (sm_ < 0) return false;

        // Config PIO SM
        pio_sm_config cfg = pio_get_default_sm_config();
        sm_config_set_wrap(&cfg, offset_ + 0, offset_ + 4);  // Wrap from end to start
        sm_config_set_sideset(&cfg, 1, false, false);  // 1 side-set pin (CLK)
        sm_config_set_sideset_pins(&cfg, 23);  // CLK on GPIO23
        sm_config_set_out_pins(&cfg, 24, 1);  // DATA out on GPIO24
        sm_config_set_in_pins(&cfg, 24, 1);  // DATA in on GPIO24
        sm_config_set_out_shift(&cfg, false, true, 32);  // Autobush, MSR first
        sm_config_set_in_shift(&cfg, false, true, 32);  // Autobush, MSR first
        sm_config_set_clkdiv(&cfg, (float)clock_get_hz(clk_sys) / (2 * freq_));  // Divider for desired freq
        pio_sm_init(pio_, sm_, offset_, &cfg);

        // Enable SM
        pio_sm_set_enabled(pio_, sm_, true);

        // Claim DMA channels
        dma_tx_ = dma_claim_unused_channel(true);
        dma_rx_ = dma_claim_unused_channel(true);

        return true;
    }

    void deinit() {
        if (sm_ >= 0) {
            pio_sm_set_enabled(pio_, sm_, false);
            pio_remove_program(pio_, &cyw_gspi_pio_program, offset_);
            pio_sm_unclaim(pio_, sm_);
        }
        if (dma_tx_ >= 0) dma_channel_unclaim(dma_tx_);
        if (dma_rx_ >= 0) dma_channel_unclaim(dma_rx_);
    }

    // Simple register read (32-bit)
    uint32_t reg_read(uint32_t addr, uint32_t len) {
        uint32_t cmd = (len << 0) | (addr << 11) | (0 << 28) | (1 << 29) | (0 << 30) | (0 << 31);  // Read, func 0, incr, len in bytes
        uint32_t buf[2] = {cmd, 0};  // Cmd + padding for status
        uint32_t rx = 0;

        gpio_put(25, 0);  // CS low

        // Setup DMA for TX
        dma_channel_config cfg_tx = dma_channel_get_default_config(dma_tx_);
        channel_config_set_transfer_data_size(&cfg_tx, DMA_SIZE_32);
        channel_config_set_dreq(&cfg_tx, pio_get_dreq(pio_, sm_, true));
        dma_channel_configure(dma_tx_, &cfg_tx, &pio_->txf[sm_], buf, 2, false);

        // Setup DMA for RX
        dma_channel_config cfg_rx = dma_channel_get_default_config(dma_rx_);
        channel_config_set_transfer_data_size(&cfg_rx, DMA_SIZE_32);
        channel_config_set_dreq(&cfg_rx, pio_get_dreq(pio_, sm_, false));
        dma_channel_configure(dma_rx_, &cfg_rx, &rx, &pio_->rxf[sm_], 1, false);

        // Start PIO for transfer
        pio_sm_clear_fifos(pio_, sm_);
        pio_sm_put(pio_, sm_, 31);  // Bits -1 for 32 bits TX
        pio_sm_put(pio_, sm_, 31);  // For RX
        pio_sm_exec(pio_, sm_, pio_encode_jmp(offset_));  // Start PIO

        dma_start_channel_mask((1u << dma_tx_) | (1u << dma_rx_));
        dma_wait_for_finish_blocking(dma_rx_);

        gpio_put(25, 1);  // CS high

        return rx;
    }

    bool test_chip() {
        uint32_t val = reg_read(0x14, 4);  // Test RO register
        if (val == 0xFEEDBEAD) {
            printf("Chip test passed: 0x%08x\n", val);
            return true;
        }
        printf("Chip test failed: 0x%08x\n", val);
        return false;
    }
};