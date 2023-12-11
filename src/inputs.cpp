#include "inputs.hpp"

int InputInst::getStick(int frame, int index) {
    int inputRaw = inputs[frame][index];

    switch (inputRaw) {
        case -7:
            return 59;
        case -6:
            return 68;
        case -5:
            return 77;
        case -4:
            return 86;
        case -3:
            return 95;
        case -2:
            return 104;
        case -1:
            return 112;
        case 0:
            return 128;
        case 1:
            return 152;
        case 2:
            return 161;
        case 3:
            return 170;
        case 4:
            return 179;
        case 5:
            return 188;
        case 6:
            return 197;
        case 7:
        default:
            return 205;
    }
}

GCReport InputInst::getReport(int frame) {
    GCReport ret = defaultGcReport;
    ret.a = (uint8_t) getButton(frame, BUTTON_A);
    ret.b = (uint8_t) getButton(frame, BUTTON_B);
    ret.l = (uint8_t) getButton(frame, BUTTON_L);

    ret.xStick = (uint8_t) getStick(frame, STICK_X);
    ret.yStick = (uint8_t) getStick(frame, STICK_Y);

    ret.dLeft = (uint8_t) getDPadLeft(frame);
    ret.dRight = (uint8_t) getDPadRight(frame);
    ret.dUp = (uint8_t) getDPadUp(frame);
    ret.dDown = (uint8_t) getDPadDown(frame);

    return ret;
}