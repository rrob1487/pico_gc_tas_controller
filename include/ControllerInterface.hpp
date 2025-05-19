#pragma once

#include <cstdint>
#include <cstddef>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "InputReader.hpp"
#include "RKGReader.hpp"
#include "LUAReader.hpp"
#include "DTMReader.hpp"
#include "my_pio.pio.h"

// Constants
constexpr uint32_t GC_DATA_PIN = 28;
constexpr uint32_t MICROSECONDS_IN_SECOND = 1000000;
constexpr float FRAME_RATE = 59.94f;
constexpr float MICROSECONDS_PER_FRAME = MICROSECONDS_IN_SECOND / FRAME_RATE;

class ControllerInterface {

public:
    ControllerInterface(InputReader* reader);
    ~ControllerInterface();
    void runLoop();

private:
    void init();
    void convertToPio(const uint8_t *command, uint32_t len, uint32_t *result, uint32_t &resultLen);
    GCPadStatus GetGCPadStatus();
    void sendData(uint32_t *result, uint32_t resultLen);
    void probe();
    void origin();
    void poll();
    void fail();

    InputReader* m_reader;
    PIO m_pio = pio0;
    uint32_t m_offset;
    pio_sm_config m_config;
    uint16_t m_frame;
    uint32_t m_time;
};