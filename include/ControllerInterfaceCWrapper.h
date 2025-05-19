#pragma once

#include <stdint.h>  // for uint8_t
#include <stddef.h>  // for size_t

#ifdef __cplusplus
extern "C" {
#endif

// C-accessible wrapper for the C++ function
void run_ci_loop_c(uint8_t* buffer, size_t size);

#ifdef __cplusplus
}
#endif
