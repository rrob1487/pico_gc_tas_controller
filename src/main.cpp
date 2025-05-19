// Standard Stuff
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <cstdio>

// Needed BLE Stuff
#include "pico/btstack_cyw43.h"
#include "btstack.h"

// Bird From Finding Nemo(Mine)
#include "BLEServer.h"
#include "ControllerInterfaceCWrapper.h"

static btstack_packet_callback_registration_t hci_event_callback_registration;

constexpr bool USE_BLE = true;

// Used for no ble mode
// External binary data (from assembly file)
extern uint8_t g_rkg[];
extern uint8_t g_rkg_end[];

int main() {
    stdio_init_all();

    // Initialize cyw43 chip (needed to use onboard LED)
    if (cyw43_arch_init()) {
        // Init failed
        printf("oops, cant initialize cyw43");
        return -1;
    }

    // Turn on the onboard LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    sleep_ms(5000); // This is only here to give me time to start the serial listener

    if( USE_BLE){
        l2cap_init();
        sm_init();

        att_server_init(profile_data, att_read_callback, att_write_callback);

        // inform about BTstack state
        hci_event_callback_registration.callback = &packet_handler;
        hci_add_event_handler(&hci_event_callback_registration);

        // register for ATT event
        att_server_register_packet_handler(packet_handler);

        // turn on bluetooth!
        hci_power_control(HCI_POWER_ON);
        btstack_run_loop_execute();  // Required for BLE to run
    }
    else {
        size_t rkgSize = g_rkg_end - g_rkg;
        printf("Running File\n");
        run_ci_loop_c(g_rkg, rkgSize);
        printf("Done Running File\n");
    }
    return 0;
}
