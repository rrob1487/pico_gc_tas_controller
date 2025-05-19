#include "LUAReader.hpp"
#include <cstring>    // For strtok and C-string related functionality
#include <cstdio>     // For printf
#include <cstdlib>    // For strtoul
#include <string>     // For std::string
#include <vector>     // For std::vector


LUAReader::LUAReader(const uint8_t* data, size_t size) {
    m_inputs.reserve(10000);
    ParseData(data, size);
}

LUAReader::~LUAReader() = default;

void LUAReader::HandleCommand(char* tokens[], size_t count) {
    const char* type = tokens[0];

    if (strcmp(type, "idle") == 0 && count == 2) {
        uint32_t repeat = strtoul(tokens[1], nullptr, 10);
        AddIdle(repeat);

    } else if ((strcmp(type, "left") == 0 || strcmp(type, "right") == 0 ||
                strcmp(type, "up") == 0 || strcmp(type, "down") == 0) && count == 3) {
        uint32_t repeat = strtoul(tokens[1], nullptr, 10);
        uint8_t value = static_cast<uint8_t>(strtoul(tokens[2], nullptr, 10));
        AddDirectionalInputs(type, repeat, value);

    } else if (strcmp(type, "all") == 0 && count == 7) {
        uint32_t repeat = strtoul(tokens[1], nullptr, 10);
        const char* buttons = tokens[2];
        uint8_t x = strtoul(tokens[3], nullptr, 10);
        uint8_t y = strtoul(tokens[4], nullptr, 10);
        uint8_t cx = strtoul(tokens[5], nullptr, 10);
        uint8_t cy = strtoul(tokens[6], nullptr, 10);
        AddGeneralInputs(repeat, buttons, x, y, cx, cy);

    } else if (strcmp(type, "sticks") == 0 && count == 6) {
        uint32_t repeat = strtoul(tokens[1], nullptr, 10);
        uint8_t x = strtoul(tokens[2], nullptr, 10);
        uint8_t y = strtoul(tokens[3], nullptr, 10);
        uint8_t cx = strtoul(tokens[4], nullptr, 10);
        uint8_t cy = strtoul(tokens[5], nullptr, 10);
        AddStickInputs(repeat, x, y, cx, cy);

    } else {
        printf("Unknown or invalid command: '%s'\n", type);
    }
}

void LUAReader::ParseData(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        printf("Error: No data to parse\n");
        return;
    }

    constexpr size_t LINE_BUFFER_SIZE = 256;
    char lineBuffer[LINE_BUFFER_SIZE] = {0};

    constexpr size_t MAX_TOKENS = 10;
    char* tokens[MAX_TOKENS] = {nullptr};

    // Simulate reading line-by-line from data (avoid full copy)
    size_t i = 0;
    size_t lineStart = 0;

    while (i < size) {
        if (data[i] == '\n' || i == size - 1) {
            size_t lineLen = i - lineStart;
            if (lineLen >= LINE_BUFFER_SIZE) {
                lineStart = i + 1;
                ++i;
                continue; // Skip overly long lines
            }

            memcpy(lineBuffer, &data[lineStart], lineLen);
            lineBuffer[lineLen] = '\0'; // Null-terminate
            lineStart = i + 1;

            // Skip irrelevant lines
            if (strstr(lineBuffer, "return inputs") || strstr(lineBuffer, "local inputs =")) {
                ++i;
                continue;
            }

            // Inline clean
            for (char* p = lineBuffer; *p; ++p) {
                if (*p == '{' || *p == '}' || *p == ',' || *p == '"') {
                    *p = ' ';
                }
            }

            // Tokenize
            size_t tokenCount = 0;
            tokens[tokenCount] = strtok(lineBuffer, " ");
            while (tokens[tokenCount] && tokenCount < MAX_TOKENS - 1) {
                ++tokenCount;
                tokens[tokenCount] = strtok(nullptr, " ");
            }

            if (tokenCount == 0)
                continue;

            // Handle the command without std::vector<std::string>
            HandleCommand(tokens, tokenCount);
        }
        ++i;
    }

    printf("ParseData completed. Commands loaded: %zu\n", m_inputs.size());
}

void LUAReader::AddIdle(uint32_t repeatCount) {
    for (uint32_t i = 0; i < repeatCount; ++i) {
        m_inputs.push_back({"", 128, 128, 128, 128, 0, 0});
    }
}

void LUAReader::AddDirectionalInputs(const std::string& direction, uint32_t repeatCount, uint8_t value) {
    for (uint32_t i = 0; i < repeatCount; ++i) {
        if (direction == "up" || direction == "down") {
            m_inputs.push_back({"", 128, value, 128, 128, 0, 0});
        } else if (direction == "left" || direction == "right") {
            m_inputs.push_back({"", value, 128, 128, 128, 0, 0});
        }
    }
}

void LUAReader::AddStickInputs(uint32_t repeatCount, uint8_t xStick, uint8_t yStick, uint8_t cXStick, uint8_t cYStick) {
    for (uint32_t i = 0; i < repeatCount; ++i) {
        m_inputs.push_back({"", xStick, yStick, cXStick, cYStick, 0, 0});
    }
}

void LUAReader::AddGeneralInputs(uint32_t repeatCount, const std::string& buttons, uint8_t xStick, uint8_t yStick, uint8_t cXStick, uint8_t cYStick) {
    for (uint32_t i = 0; i < repeatCount; ++i) {
        m_inputs.push_back({buttons, xStick, yStick, cXStick, cYStick, 0, 0});
    }
}

GCPadStatus LUAReader::CalcFrame(uint16_t frame) {
   if (m_inputs.empty()) {
       printf("Warning: No inputs to process. Returning default GCPadStatus.\n");
       this->done = true;
       return s_defaultGCPadStatus; // No input, return default
   }

   if (frame >= m_inputs.size()) {
       printf("Warning: Frame out of range. Returning default GCPadStatus.\n");
       this->done = true;
       return s_defaultGCPadStatus; // Frame too high, return default
   }

   // Process the input for the given frame
   const LUAInput& input = m_inputs[frame];
   GCPadStatus status = s_defaultGCPadStatus;
   status.xStick = input.xStick;
   status.yStick = input.yStick;
   status.cxStick = input.cXStick;
   status.cyStick = input.cYStick;
   status.analogL = input.analogL;
   status.analogR = input.analogR;

   status.a = input.buttons.find('A') != std::string::npos;
   status.b = input.buttons.find('B') != std::string::npos;
   status.x = input.buttons.find('X') != std::string::npos;
   status.y = input.buttons.find('Y') != std::string::npos;
   status.l = input.buttons.find('L') != std::string::npos;
   status.r = input.buttons.find('R') != std::string::npos;
   status.z = input.buttons.find('Z') != std::string::npos;
   status.start = input.buttons.find('S') != std::string::npos;

   return status;
}