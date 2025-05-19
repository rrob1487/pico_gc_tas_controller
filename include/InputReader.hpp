#pragma once

#include "GCPadStatus.hpp"
#include <cstdint>

// Abstract base class for all input readers
class InputReader {
public:
    virtual ~InputReader() = default;

    // Pure virtual method to compute the controller state for a given frame
    virtual GCPadStatus CalcFrame(uint16_t frame) = 0;

    //Track when file is over
    virtual bool Done() const = 0;
};
