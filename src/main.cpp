#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "my_pio.pio.h"

#include "global.hpp"
#include "inputs.hpp"

constexpr std::array<input, 5161> InputInst::inputs;
int frame;
uint64_t time;
InputInst inputInst;
pio_sm_config config;
uint offset;

// PIO Shifts to the right by default
// In: pushes batches of 8 shifted left, i.e we get [0x40, 0x03, rumble (the end bit is never pushed)]
// Out: We push commands for a right shift with an enable pin, ie 5 (101) would be 0b11'10'11
// So in doesn't need post processing but out does
void convertToPio(const uint8_t* command, const int len, uint32_t* result, int& resultLen) {
    if (len == 0) {
        resultLen = 0;
        return;
    }
    resultLen = len/2 + 1;
    int i;
    for (i = 0; i < resultLen; i++) {
        result[i] = 0;
    }
    for (i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            result[i / 2] += 1 << (2 * (8 * (i % 2) + j) + 1);
            result[i / 2] += (!!(command[i] & (0x80u >> j))) << (2 * (8 * (i % 2) + j));
        }
    }
    // End bit
    result[len / 2] += 3 << (2 * (8 * (len % 2)));
}

GCReport getReport() {
    if (time == 0) {
        frame = 0;
        time = time_us_64();
    }
    else {
        // Time diff
        uint64_t timeDiff = time_us_64() - time;
        frame = (int)((double)timeDiff / 16683.35);
    }
    
    return inputInst.getReport(frame);
}

void resetState(uint32_t* result, uint32_t resultLen) {
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_init(pio0, 0, offset+save_offset_outmode, &config);
    pio_sm_set_enabled(pio0, 0, true);

    for (int i = 0; i<resultLen; i++) pio_sm_put_blocking(pio0, 0, result[i]);
}

void probe() {
    uint8_t probeResponse[3] = { 0x09, 0x00, 0x03 };
    uint32_t result[2];
    int resultLen;
    convertToPio(probeResponse, 3, result, resultLen);
    sleep_us(6); // 3.75us into the bit before end bit => 6.25 to wait if the end-bit is 5us long

    resetState(result, resultLen);
}

void origin() {
    uint8_t originResponse[10] = { 0x00, 0x80, 128, 128, 128, 128, 0, 0, 0, 0 };
    uint32_t result[6];
    int resultLen;
    convertToPio(originResponse, 10, result, resultLen);
    // Here we don't wait because convertToPio takes time

    resetState(result, resultLen);
}

void poll() {
    uint8_t buffer;
    buffer = pio_sm_get_blocking(pio0, 0);
    buffer = pio_sm_get_blocking(pio0, 0);
    gpio_put(rumblePin, buffer & 1);

    GCReport gcReport = getReport();

    uint32_t result[5];
    int resultLen;
    convertToPio((uint8_t*)(&gcReport), 8, result, resultLen);

    resetState(result, resultLen);
}

void fail() {
    pio_sm_set_enabled(pio0, 0, false);
    sleep_us(400);
    pio_sm_init(pio0, 0, offset+save_offset_inmode, &config);
    pio_sm_set_enabled(pio0, 0, true);
}

void init() {
    frame = 0;
    time = 0;
    gpio_init(gcDataPin);
    gpio_set_dir(gcDataPin, GPIO_IN);
    gpio_pull_up(gcDataPin);

    gpio_init(rumblePin);
    gpio_set_dir(rumblePin, GPIO_OUT);

    sleep_us(100); // Stabilize voltages

    pio_gpio_init(pio0, gcDataPin);
    offset = pio_add_program(pio0, &save_program);

    config = save_program_get_default_config(offset);
    sm_config_set_in_pins(&config, gcDataPin);
    sm_config_set_out_pins(&config, gcDataPin, 1);
    sm_config_set_set_pins(&config, gcDataPin, 1);
    sm_config_set_clkdiv(&config, 5);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_shift(&config, false, true, 8);
    
    pio_sm_init(pio0, 0, offset, &config);
    pio_sm_set_enabled(pio0, 0, true);
}

int main() {
    init();
    
    while (true) {
        uint8_t buffer = pio_sm_get_blocking(pio0, 0);
        void (*func)();
        switch(buffer) {
        case 0x00:
            func = probe;
            break;
        case 0x41:
            func = origin;
            break;
        case 0x40:
            func = poll;
            break;
        default:
            func = fail;
        }

        (*func)();
    }
}