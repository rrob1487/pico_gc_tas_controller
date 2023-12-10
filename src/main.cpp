#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#include "global.hpp"

#include "communication_protocols/joybus.hpp"

int main() {
    set_sys_clock_khz(1000*us, true);
    stdio_init_all();

    CommunicationProtocols::Joybus::enterMode(gcDataPin);
}