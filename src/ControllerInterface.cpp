#include "ControllerInterface.hpp"

ControllerInterface::ControllerInterface(InputReader* reader)
    : m_reader(reader), m_frame(0), m_time(0) { init(); }

ControllerInterface::~ControllerInterface() {
    delete m_reader;
    pio_sm_set_enabled(m_pio, 0, false);
    sleep_us(400);
    pio_sm_restart(m_pio, 0);
    pio_sm_clear_fifos(m_pio, 0);
}


// PIO Shifts to the right by default
// In: pushes batches of 8 shifted left, i.e we get [0x40, 0x03, rumble (the end bit is never
// pushed)] Out: We push commands for a right shift with an enable pin, ie 5 (101) would be
// 0b11'10'11 So in doesn't need post processing but out does
void ControllerInterface::convertToPio(const uint8_t *command, const uint32_t len, uint32_t *result,
        uint32_t &resultLen) {
    if (len == 0) {
        resultLen = 0;
        return;
    }

    resultLen = len / 2 + 1;

    for (uint32_t i = 0; i < resultLen; i++) {
        result[i] = 0;
    }
    for (uint32_t i = 0; i < len; i++) {
        for (uint32_t j = 0; j < 8; j++) {
            result[i / 2] += 1 << (2 * (8 * (i % 2) + j) + 1);
            result[i / 2] += (!!(command[i] & (0x80u >> j))) << (2 * (8 * (i % 2) + j));
        }
    }
    // End bit
    result[len / 2] += 3 << (2 * (8 * (len % 2)));
}

GCPadStatus ControllerInterface::GetGCPadStatus() {
    if (m_time == 0) {
        m_frame = 0;
        m_time = time_us_32();
    } else {
        uint32_t timeDiff = time_us_32() - m_time;
        m_frame = static_cast<uint16_t>(timeDiff / MICROSECONDS_PER_FRAME);
    }

    return m_reader->CalcFrame(m_frame);
}

void ControllerInterface::sendData(uint32_t *result, uint32_t resultLen) {
    pio_sm_set_enabled(m_pio, 0, false);
    pio_sm_init(m_pio, 0, m_offset + save_offset_outmode, &m_config);
    pio_sm_set_enabled(m_pio, 0, true);

    for (uint32_t i = 0; i < resultLen; i++) {
        pio_sm_put_blocking(m_pio, 0, result[i]);
    }
}

void ControllerInterface::probe() {
    uint8_t probeResponse[3] = {0x09, 0x00, 0x03};
    uint32_t result[2];
    uint32_t resultLen;
    convertToPio(probeResponse, 3, result, resultLen);
    sleep_us(6); // 3.75us into the bit before end bit => 6.25 to wait if the end-bit is 5us long

    sendData(result, resultLen);
}

void ControllerInterface::origin() {
    uint8_t originResponse[10] = {0x00, 0x80, 128, 128, 128, 128, 0, 0, 0, 0};
    uint32_t result[6];
    uint32_t resultLen;
    convertToPio(originResponse, 10, result, resultLen);
    // Here we don't wait because convertToPio takes time

    sendData(result, resultLen);
}

void ControllerInterface::poll() {
    uint8_t buffer;
    buffer = pio_sm_get_blocking(m_pio, 0);
    buffer = pio_sm_get_blocking(m_pio, 0);

    GCPadStatus pad = GetGCPadStatus();
//    printGCPadStatus(pad);

    uint32_t result[5];
    uint32_t resultLen;
    convertToPio(reinterpret_cast<uint8_t *>(&pad), sizeof(GCPadStatus), result, resultLen);

    sendData(result, resultLen);
}

void ControllerInterface::fail() {
    pio_sm_set_enabled(m_pio, 0, false);
    sleep_us(400);
    pio_sm_init(m_pio, 0, m_offset + save_offset_inmode, &m_config);
    pio_sm_set_enabled(m_pio, 0, true);
    printf("fuck");
}

void ControllerInterface::init() {
    printf("Starting Controller Interface Init\n");
    gpio_init(GC_DATA_PIN);
    gpio_set_dir(GC_DATA_PIN, GPIO_IN);
    gpio_pull_up(GC_DATA_PIN);

    sleep_us(100); // Stabilize voltages

    pio_gpio_init(m_pio, GC_DATA_PIN);

    // Clear and reload PIO program
    pio_sm_set_enabled(m_pio, 0, false);
    pio_sm_restart(m_pio, 0);
    pio_sm_clear_fifos(m_pio, 0);
    pio_remove_program(m_pio, &save_program, m_offset);
    m_offset = pio_add_program(m_pio, &save_program);

    m_config = save_program_get_default_config(m_offset);

    sm_config_set_in_pins(&m_config, GC_DATA_PIN);
    sm_config_set_out_pins(&m_config, GC_DATA_PIN, 1);
    sm_config_set_set_pins(&m_config, GC_DATA_PIN, 1);
    sm_config_set_clkdiv(&m_config, 5);
    sm_config_set_out_shift(&m_config, true, false, 32);
    sm_config_set_in_shift(&m_config, false, true, 8);

    pio_sm_init(m_pio, 0, m_offset, &m_config);
    pio_sm_set_enabled(m_pio, 0, true);

    printf("Controller Interface Init Complete\n");
}

void ControllerInterface::runLoop(){
    while (!m_reader->Done()) {
        //printf("Waiting for buffer...\n");
        uint8_t buffer = pio_sm_get_blocking(m_pio, 0);
        //printf("Got buffer: 0x%02X\n", buffer);
        switch (buffer) {
        case 0x00:
            probe();
            break;
        case 0x41:
            origin();
            break;
        case 0x40:
            poll();
            break;
        default:
            fail();
        }
    }
}