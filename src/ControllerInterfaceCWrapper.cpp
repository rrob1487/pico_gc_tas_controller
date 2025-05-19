// This thin wrapper just bridges C to C++.
#include "ControllerInterfaceCWrapper.h"
#include "ControllerInterface.hpp"
#include "LUAReader.hpp"

static ControllerInterface* g_instance = nullptr;

void run_ci_loop_c(uint8_t* buffer, size_t size) {
    if (g_instance) {
        delete g_instance;
    }

    // TODO: Replace with dynamic file contents or another reader as needed
    InputReader* reader = new LUAReader(buffer, size);
    g_instance = new ControllerInterface(reader);
    g_instance->runLoop();
    delete g_instance;
    g_instance = nullptr;
}
