#include "communication_protocols/joybus.hpp"

#include "global.hpp"
#include "inputs.hpp"

#include "hardware/gpio.h"

#include "my_pio.pio.h"

constexpr std::array<input, 2677> InputInst::inputs;
int frame;

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
    GCReport ret = defaultGcReport;
    InputInst inputInst;

    if (!inputInst.frameValid(frame/2))
        return ret;
    
    ret.a = (uint8_t) inputInst.getButton(frame/2, BUTTON_A);
    ret.b = (uint8_t) inputInst.getButton(frame/2, BUTTON_B);
    ret.l = (uint8_t) inputInst.getButton(frame/2, BUTTON_L);

    ret.xStick = (uint8_t) inputInst.getStick(frame/2, STICK_X);
    ret.yStick = (uint8_t) inputInst.getStick(frame/2, STICK_Y);

    ret.dLeft = (uint8_t) inputInst.getDPadLeft(frame/2);
    ret.dRight = (uint8_t) inputInst.getDPadRight(frame/2);
    ret.dUp = (uint8_t) inputInst.getDPadUp(frame/2);
    ret.dDown = (uint8_t) inputInst.getDPadDown(frame/2);

    frame++;

    return ret;
}

namespace CommunicationProtocols
{
namespace Joybus
{

void resetState(const uint& offset, const pio_sm_config& config, bool bWrite, uint32_t* result, uint32_t resultLen) {
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_init(pio0, 0, offset+save_offset_outmode, &config);
    pio_sm_set_enabled(pio0, 0, true);

    if (bWrite) {
        for (int i = 0; i<resultLen; i++) pio_sm_put_blocking(pio0, 0, result[i]);
    }
}

void probe(const uint& offset, const pio_sm_config& config) {
    uint8_t probeResponse[3] = { 0x09, 0x00, 0x03 };
    uint32_t result[2];
    int resultLen;
    convertToPio(probeResponse, 3, result, resultLen);
    sleep_us(6); // 3.75us into the bit before end bit => 6.25 to wait if the end-bit is 5us long

    resetState(offset, config, true, result, resultLen);
}

void origin(const uint& offset, const pio_sm_config& config) {
    uint8_t originResponse[10] = { 0x00, 0x80, 128, 128, 128, 128, 0, 0, 0, 0 };
    uint32_t result[6];
    int resultLen;
    convertToPio(originResponse, 10, result, resultLen);
    // Here we don't wait because convertToPio takes time

    resetState(offset, config, true, result, resultLen);
}

void poll(const uint& offset, const pio_sm_config& config) {
    uint8_t buffer;
    buffer = pio_sm_get_blocking(pio0, 0);
    buffer = pio_sm_get_blocking(pio0, 0);
    gpio_put(rumblePin, buffer & 1);

    GCReport gcReport = getReport();

    uint32_t result[5];
    int resultLen;
    convertToPio((uint8_t*)(&gcReport), 8, result, resultLen);

    resetState(offset, config, true, result, resultLen);
}

void fail(const uint& offset, const pio_sm_config& config) {
    pio_sm_set_enabled(pio0, 0, false);
    sleep_us(400);
    pio_sm_init(pio0, 0, offset+save_offset_inmode, &config);
    pio_sm_set_enabled(pio0, 0, true);
}

void enterMode(int dataPin) {
    frame = 0;
    gpio_init(dataPin);
    gpio_set_dir(dataPin, GPIO_IN);
    gpio_pull_up(dataPin);

    gpio_init(rumblePin);
    gpio_set_dir(rumblePin, GPIO_OUT);

    sleep_us(100); // Stabilize voltages

    pio_gpio_init(pio0, dataPin);
    uint offset = pio_add_program(pio0, &save_program);

    pio_sm_config config = save_program_get_default_config(offset);
    sm_config_set_in_pins(&config, dataPin);
    sm_config_set_out_pins(&config, dataPin, 1);
    sm_config_set_set_pins(&config, dataPin, 1);
    sm_config_set_clkdiv(&config, 5);
    sm_config_set_out_shift(&config, true, false, 32);
    sm_config_set_in_shift(&config, false, true, 8);
    
    pio_sm_init(pio0, 0, offset, &config);
    pio_sm_set_enabled(pio0, 0, true);
    
    while (true) {
        uint8_t buffer = pio_sm_get_blocking(pio0, 0);
        void (*func)(const uint& offset, const pio_sm_config&);
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

        (*func)(offset, config);
    }
}

}
}
