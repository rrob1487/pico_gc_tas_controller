#pragma once

#include <cstdint>
#include <cstdio>

struct __attribute__((packed)) GCPadStatus {
    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;
    uint8_t start : 1;
    uint8_t pad0 : 3;
    uint8_t dLeft : 1;
    uint8_t dRight : 1;
    uint8_t dDown : 1;
    uint8_t dUp : 1;
    uint8_t z : 1;
    uint8_t r : 1;
    uint8_t l : 1;
    uint8_t pad1 : 1;
    uint8_t xStick;
    uint8_t yStick;
    uint8_t cxStick;
    uint8_t cyStick;
    uint8_t analogL;
    uint8_t analogR;
};
static_assert(sizeof(GCPadStatus) == 0x8);

static constexpr GCPadStatus s_defaultGCPadStatus = {
        .a = 0,
        .b = 0,
        .x = 0,
        .y = 0,
        .start = 0,
        .pad0 = 0,
        .dLeft = 0,
        .dRight = 0,
        .dDown = 0,
        .dUp = 0,
        .z = 0,
        .r = 0,
        .l = 0,
        .pad1 = 1,
        .xStick = 128,
        .yStick = 128,
        .cxStick = 128,
        .cyStick = 128,
        .analogL = 0,
        .analogR = 0,
};

// Function definition within the header (small utility function)
inline void printGCPadStatus(const GCPadStatus& status) {
    printf("GCPadStatus:\n");
    printf("  Buttons:\n");
    printf("    A: %d | ", status.a);
    printf("B: %d | ", status.b);
    printf("X: %d | ", status.x);
    printf("Y: %d | ", status.y);
    printf("Z: %d | ", status.z);
    printf("L: %d | ", status.l);
    printf("R: %d | ", status.r);
    printf("Start: %d\n", status.start);
    printf("    DPad:\n");
    printf("      Up: %d | ", status.dUp);
    printf("      Down: %d | ", status.dDown);
    printf("      Left: %d | ", status.dLeft);
    printf("      Right: %d\n", status.dRight);
    printf("  Sticks and Triggers:\n");
    printf("    xStick: %u\n", status.xStick);
    printf("    yStick: %u\n", status.yStick);
    printf("    cXStick: %u\n", status.cxStick);
    printf("    cYStick: %u\n", status.cyStick);
    printf("    Analog L: %u\n", status.analogL);
    printf("    Analog R: %u\n", status.analogR);
}
