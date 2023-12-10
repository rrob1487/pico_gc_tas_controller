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
    
bool InputInst::getButton(int frame, int index) { return inputs[frame][index] != 0; }
bool InputInst::getDPadUp(int frame) { return inputs[frame][5] == 1; }
bool InputInst::getDPadDown(int frame) { return inputs[frame][5] == 2; }
bool InputInst::getDPadLeft(int frame) { return inputs[frame][5] == 3; }
bool InputInst::getDPadRight(int frame) { return inputs[frame][5] == 4; }
bool InputInst::frameValid(int frame) { return frame < inputs.size(); }