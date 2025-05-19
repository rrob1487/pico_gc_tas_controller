#pragma once

#include "GCPadStatus.hpp"
#include "InputReader.hpp"
#include <string>        // For std::string
#include <vector>        // For std::vector
#include <cstdint>       // For uint8_t and uint32_t

class LUAReader : public InputReader {
public:
    LUAReader(const uint8_t* data, size_t size);
    ~LUAReader();

    GCPadStatus CalcFrame(uint16_t frame);

    bool Done() const override { return done; }

private:
    struct LUAInput {
        std::string buttons;
        uint8_t xStick;
        uint8_t yStick;
        uint8_t cXStick;
        uint8_t cYStick;
        uint8_t analogL;
        uint8_t analogR;
    };

    void HandleCommand(char* tokens[], size_t count);
    void ParseData(const uint8_t* data, size_t size);
    void AddIdle(uint32_t repeatCount);
    void AddDirectionalInputs(const std::string& direction, uint32_t repeatCount, uint8_t value);
    void AddStickInputs(uint32_t repeatCount, uint8_t xStick, uint8_t yStick, uint8_t cXStick, uint8_t cYStick);
    void AddGeneralInputs(uint32_t repeatCount, const std::string& buttons, uint8_t xStick, uint8_t yStick, uint8_t cXStick, uint8_t cYStick);

    // Set when file completes
    bool done = false;

    std::vector<LUAInput> m_inputs;
};