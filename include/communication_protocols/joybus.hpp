#ifndef COMMUNICATION_PROTOCOLS__JOYBUS_HPP
#define COMMUNICATION_PROTOCOLS__JOYBUS_HPP

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "joybus/gcReport.hpp"

#include <functional>

namespace CommunicationProtocols
{
namespace Joybus
{

/**
 * @short Enters the Joybus communication mode
 * 
 * @param dataPin GPIO number of the console data line pin
 */
void enterMode(int dataPin);

void probe(const uint& offset, const pio_sm_config& config);
void origin(const uint& offset, const pio_sm_config& config);
void poll(const uint& offset, const pio_sm_config& config);
void fail(const uint& offset, const pio_sm_config& config);
void resetState(const uint& offset, const pio_sm_config& config, bool bWrite, uint32_t* result = nullptr, uint32_t resultLen = 0);

}
}

#endif