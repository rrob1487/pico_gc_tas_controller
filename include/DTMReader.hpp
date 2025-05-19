#pragma once

#include "GCPadStatus.hpp"
#include "InputReader.hpp"

#include <cstddef>
#include <cstdint>

class DTMReader : public InputReader {
public:
    DTMReader(uint8_t* pData, size_t size);
    ~DTMReader();

    GCPadStatus CalcFrame(uint16_t frame);

    bool Done() const override { return done; }

private:
    uint8_t* m_inputData;
    size_t m_inputSize;
    size_t m_inputOffset;  // where input data starts
    size_t m_totalFrames;
    size_t m_bytesPerFrame = 8;
    size_t m_numControllers = 1;
    uint8_t* m_frameData = nullptr; // New buffer that holds all frame inputs

    // Set when file completes
    bool done = false;

    size_t GetInputOffset(uint16_t frame) const;
};
