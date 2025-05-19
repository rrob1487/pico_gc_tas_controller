#include "DTMReader.hpp"

#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cstdio>

// DTM header is always 256 bytes
static constexpr size_t DTM_HEADER_SIZE = 0x100;
static constexpr size_t OFFSET_CONTROLLERS = 0x0A;

DTMReader::DTMReader(uint8_t* pData, size_t size)
    : m_inputData(pData), m_inputSize(size), m_inputOffset(DTM_HEADER_SIZE)
{
    // Default to 1 controller if something goes wrong
    if (size >= OFFSET_CONTROLLERS + 1) {
        uint8_t controllers_byte = pData[OFFSET_CONTROLLERS];
        // Lower 4 bits = GC controllers 1-4
        m_numControllers = 0;
        for (int i = 0; i < 4; ++i) {
            if (controllers_byte & (1 << i)) {
                ++m_numControllers;
            }
        }
    }

    //m_bytesPerFrame = m_numControllers * 8;

    // Guess total number of frames based on file size
    if (m_inputOffset < m_inputSize && m_bytesPerFrame != 0) {
        m_totalFrames = (m_inputSize - m_inputOffset) / m_bytesPerFrame;

        // Allocate and copy all frame data
        m_frameData = new uint8_t[m_totalFrames * m_bytesPerFrame];
        memcpy(m_frameData, m_inputData + m_inputOffset, m_totalFrames * m_bytesPerFrame);
    } else {
        m_totalFrames = 0;
    }
}

DTMReader::~DTMReader() {
    delete[] m_frameData;
}

size_t DTMReader::GetInputOffset(uint16_t frame) const {
    return m_inputOffset + frame * m_bytesPerFrame;
}

void debug_controller(uint8_t controllers_byte, uint16_t buttons, uint8_t stickX, uint8_t stickY, uint8_t cStickX, uint8_t cStickY) {
    printf("Controller byte: 0x%02X\n", controllers_byte);
    printf("Buttons: 0x%04X\n", buttons);
    printf("Stick X: %d, Stick Y: %d\n", stickX, stickY);
    printf("C-Stick X: %d, C-Stick Y: %d\n", cStickX, cStickY);
}

void print_data_array(const uint8_t* data) {
    printf("Frame raw bytes: ");
    for (int i = 0; i < 8; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

GCPadStatus DTMReader::CalcFrame(uint16_t frame) {
    GCPadStatus status = s_defaultGCPadStatus;

    size_t offset = frame * m_bytesPerFrame;
    if (!m_frameData || offset + m_bytesPerFrame > m_totalFrames * m_bytesPerFrame) {
        printf("File Compled Runing\n");
        this->done = true;
        return status;
    }

    const uint8_t* data = &m_frameData[offset];

    uint16_t buttons = (data[0] << 8) | data[1];
    uint8_t alogL = data[2];
    uint8_t alogR = data[3];
    uint8_t stickX = data[4];
    uint8_t stickY = data[5];
    uint8_t cStickX = data[6];
    uint8_t cStickY = data[7];


    // Debugging output
    //debug_controller(data[0], buttons, stickX, stickY, cStickX, cStickY);

    // Basic button mappings
    status.a = buttons & 0x0100;
    status.b = buttons & 0x0200;
    status.x = buttons & 0x0400;
    status.y = buttons & 0x0800;
    status.z = buttons & 0x1000;
    status.start = buttons & 0x0080;

    status.l = buttons & 0x0020;
    status.r = buttons & 0x0010;

    // Stick positions (already analog)
    status.xStick = stickX;
    status.yStick = stickY;

    // You could also use C-stick if needed:
    status.cxStick = cStickX;
    status.cyStick = cStickY;

    // DPad mapping from buttons
    status.dUp    = buttons & 0x0008;
    status.dDown  = buttons & 0x0004;
    status.dLeft  = buttons & 0x0002;
    status.dRight = buttons & 0x0001;

    //Triggers
    status.analogL = alogL;
    status.analogR = alogR;

    return status;
}
