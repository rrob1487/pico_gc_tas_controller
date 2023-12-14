#ifndef __RKGREADER_HPP
#define __RKGREADER_HPP

#include <stdint.h>

#include "GCPadStatus.hpp"

#define INPUT_HEADER_SIZE 0x8

class RKGReader {

public:
    RKGReader(char* pData);
    ~RKGReader();

    GCPadStatus GetGCPadStatus(uint16_t frame);

private:
    char* YAZ1Decompress(char* pData);
    uint16_t DecompressBlock(char* src, int offset, int srcSize, char* dst, uint32_t uncompressedSize);

    uint8_t GetFace(uint16_t frame);
    uint8_t GetDir(uint16_t frame);
    uint8_t RawToStick(uint8_t raw);
    uint8_t GetTrick(uint16_t frame);

    // Current computed frame (since sometimes we will poll the same frame multiple times)
    uint16_t m_frameCount;

    // The number of tuples in each data section
    uint16_t m_faceCount;
    uint16_t m_dirCount;
    uint16_t m_trickCount;

    // Track the start of each data section so that we don't recompute every input poll
    uint16_t m_faceStart;
    uint16_t m_dirStart;
    uint16_t m_trickStart;

    // What is the current tuple we're looking at?
    uint16_t m_faceIndex;
    uint16_t m_dirIndex;
    uint16_t m_trickIndex;

    // If a given tuple is x frames long, how many frames have we elapsed in this tuple so far?
    uint8_t m_faceDuration;
    uint8_t m_dirDuration;
    uint16_t m_trickDuration; // may be greater than 256

    char* m_decodedData;
    bool m_compressed;
};

#endif