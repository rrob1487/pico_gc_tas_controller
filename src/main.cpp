#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#include "global.hpp"

#include "dac_algorithms/melee_F1.hpp"

#include "gpio_to_button_sets/F1.hpp"

#include "communication_protocols/joybus.hpp"

uint64_t time;
bool pressA = true;

GpioToButtonSets::F1::ButtonSet getInputs() {
    // Based off clock timing since program start, index into a list of inputs
    GpioToButtonSets::F1::ButtonSet ret;

    if (time == 0) {
        time = time_us_64();
        ret.a = !pressA;
    } else {
        uint64_t diff = time_us_64() - time;

        if (diff >= 5000000) {
            ret.a = pressA;
            pressA = !pressA;
            time = time_us_64();
        }
    }

    return ret;
}

int main() {
    time = 0;

    set_sys_clock_khz(1000*us, true);
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    CommunicationProtocols::Joybus::enterMode(gcDataPin, [](){ return DACAlgorithms::MeleeF1::getGCReport(getInputs()); });
}