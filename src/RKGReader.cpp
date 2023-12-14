#include "RKGReader.hpp"

#include <cstring>

// YAZ1 decompression code was translated from AtishaRibeiro
// See https://github.com/AtishaRibeiro/InputDisplay/blob/master/InputDisplay/Core/Yaz1dec.cs

RKGReader::RKGReader(char* pData)
    : m_decodedData(pData + 0x88)
    , m_faceIndex(0)
    , m_dirIndex(0)
    , m_trickIndex(0)
    , m_faceDuration(0)
    , m_dirDuration(0)
    , m_trickDuration(0)
    , m_frameCount(0)
{
    // Maintain a pointer to the start of input data header
    m_decodedData = pData + 0x88;

    m_compressed = pData[0xC] & 0x8;
    if (m_compressed)
        m_decodedData = YAZ1Decompress(m_decodedData);
    
    // Swap endianness
    m_faceCount = m_decodedData[0] << 0x8 | m_decodedData[1];
    m_dirCount = m_decodedData[2] << 0x8 | m_decodedData[3];
    m_trickCount = m_decodedData[4] << 0x8 | m_decodedData[5];

    m_faceStart = INPUT_HEADER_SIZE;
    m_dirStart = m_faceStart + 2 * m_faceCount;
    m_trickStart = m_dirStart + 2 * m_dirCount;
}

RKGReader::~RKGReader() {
    if (m_compressed)
        delete[] m_decodedData;
}

char* RKGReader::YAZ1Decompress(char* pData) {
    char* pRet = nullptr;
    uint32_t retLen = 0;
    
    // get compressed length
    uint32_t compressedLen;
    memcpy(&compressedLen, pData, sizeof(uint32_t));
    compressedLen = __builtin_bswap32(compressedLen); // Swap endianness
    pData += sizeof(uint32_t);
    
    int readBytes = 0;
    while (readBytes < compressedLen) {
        // Search block
        while (readBytes + 3 < compressedLen && memcmp(pData + readBytes, "Yaz1", 4) != 0)
            readBytes++;
        
        if (readBytes + 3 >= compressedLen)
            return pRet;
        
        readBytes += 4;
        
        // Read block size
        uint32_t blockSize;
        memcpy(&blockSize, pData+readBytes, sizeof(uint32_t));
        blockSize = __builtin_bswap32(blockSize); // Swap endianness
        
        char* blockDecompressed = new char[blockSize];
        
        // Seek past 4 byte size + 8 unused bytes
        readBytes += 12;
        
        readBytes += DecompressBlock(pData, readBytes, compressedLen - readBytes, blockDecompressed, blockSize);
        
        // Add to main array
        char* pRetOld = pRet;
        pRet = new char[retLen + blockSize];
        memcpy(pRet, pRetOld, retLen);
        delete[] pRetOld;
        memcpy(pRet + retLen, blockDecompressed, blockSize);
        delete[] blockDecompressed;
        retLen += blockSize;
    }
    
    return pRet;
}

uint16_t RKGReader::DecompressBlock(char* src, int offset, int srcSize, char* dst, uint32_t uncompressedSize) {
    uint16_t srcPos = 0;
    uint16_t destPos = 0;
    
    int validBitCount = 0; // number of valid bits left in "code" byte
    uint8_t currCodeByte = src[offset + srcPos];
    while (destPos < uncompressedSize) {
        // read new "code" byte if the current one is used upper_bound
        if (validBitCount == 0) {
            currCodeByte = src[offset + srcPos++];
            validBitCount = 8;
        }
        
        if ((currCodeByte & 0x80) != 0) {
            // straight copy
            dst[destPos++] = src[offset + srcPos++];
        } else {
            // RLE part
            uint8_t byte1 = src[offset + srcPos++];
            uint8_t byte2 = src[offset + srcPos++];
            
            int dist = ((byte1 & 0xF) << 8) | byte2;
            int copySource = destPos - (dist + 1);
            
            int numBytes = byte1 >> 4;
            if (numBytes == 0)
                numBytes = src[offset + srcPos++] + 0x12;
            else
                numBytes += 2;
            
            // copy run
            for (int i = 0; i < numBytes; i++)
                dst[destPos++] = dst[copySource++];
        }
        
        // use next bit from "code" byte
        currCodeByte <<= 1;
        validBitCount--;
    }
    
    return srcPos;
}

uint8_t RKGReader::GetFace(uint16_t frame) {
    // End of input?
    if (m_faceIndex >= m_faceCount)
        return 0;

    uint8_t inputs = m_decodedData[m_faceStart + (2 * m_faceIndex)];
    uint8_t tupleDuration = m_decodedData[m_faceStart + (2 * m_faceIndex) + 1];

    // If new frame, then update our trackers
    if (frame > m_frameCount && ++m_faceDuration == tupleDuration) {
        m_faceIndex++;
        m_faceDuration = 0;

        inputs = m_decodedData[m_faceStart + (2 * m_faceIndex)];
    }

    return inputs;
}

uint8_t RKGReader::GetDir(uint16_t frame) {
    // End of input?
    if (m_dirIndex >= m_dirCount)
        return 0;

    uint8_t inputs = m_decodedData[m_dirStart + (2 * m_dirIndex)];
    uint8_t tupleDuration = m_decodedData[m_dirStart + (2 * m_dirIndex) + 1];

    // If new frame, then update our trackers
    if (frame > m_frameCount && ++m_dirDuration == tupleDuration) {
        m_dirIndex++;
        m_dirDuration = 0;

        inputs = m_decodedData[m_dirStart + (2 * m_dirIndex)];
    }

    return inputs;
}

uint8_t RKGReader::GetTrick(uint16_t frame) {
    // End of input?
    if (m_trickIndex >= m_trickCount)
        return 0;
    
    uint8_t inputs = m_decodedData[m_trickStart + (2 * m_trickIndex)];
    uint8_t tupleDuration = m_decodedData[m_trickStart + (2 * m_trickIndex) + 1];
    // Trick tuple duration is computed differently
    // The lower 4 bits of inputs represent how many repetitions of 256 frames there are for the tuple duration
    uint16_t idleDuration = (inputs & 0x0F) * 256;

    // If new frame, then update our trackers
    if (frame > m_frameCount && ++m_trickDuration == idleDuration + tupleDuration) {
        m_trickIndex++;
        m_trickDuration = 0;

        inputs = m_decodedData[m_trickStart + (2 * m_trickIndex)];
        idleDuration = (inputs & 0x0F) * 256;
    }

    // Check if we are in the idle period
    if (m_trickDuration < idleDuration)
        inputs = 0;

    return (inputs >> 4) & 0x07;
}

uint8_t RKGReader::RawToStick(uint8_t raw) {
    switch (raw) {
        case 0: return 59;
        case 1: return 68;
        case 2: return 77;
        case 3: return 86;
        case 4: return 95;
        case 5: return 104;
        case 6: return 112;
        case 7: return 128;
        case 8: return 152;
        case 9: return 161;
        case 10: return 170;
        case 11: return 179;
        case 12: return 188;
        case 13: return 197;
        case 14:
        default: return 205;
    }
}

GCPadStatus RKGReader::GetGCPadStatus(uint16_t frame) {
    GCPadStatus ret = defaultGCPadStatus;

    // Factor in controller disconnection screen A press + fade out
    if (frame == 0) {
        ret.a = 1;
        return ret;
    } else if (frame < 283) {
        return ret;
    }

    frame -= 283;

    uint8_t faceData = GetFace(frame);
    uint8_t dirData = GetDir(frame);
    uint8_t trickData = GetTrick(frame);

    ret.a = (bool)(faceData & 0x01);
    ret.b = (bool)(faceData & 0x02);
    ret.l = (bool)(faceData & 0x04);

    ret.xStick = RawToStick(dirData >> 4 & 0x0F);
    ret.yStick = RawToStick(dirData & 0x0F);

    ret.dLeft = (bool)(trickData & 0x3 == 0x3);
    ret.dRight = (bool)(trickData & 0x4);
    ret.dUp = (bool)(trickData & 0x1);
    ret.dDown = (bool)(trickData & 0x2);

    // If new frame, update frame count
    if (frame > m_frameCount)
        m_frameCount++;

    return ret;
}