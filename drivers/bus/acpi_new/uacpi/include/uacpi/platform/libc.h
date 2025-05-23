#pragma once

#ifdef UACPI_OVERRIDE_LIBC
#include "uacpi_libc.h"
#else
/*
 * The following libc functions are used internally by uACPI and have a default
 * (sub-optimal) implementation:
 * - strcmp
 * - strnlen
 * - strlen
 * - snprintf
 * - vsnprintf
 *
 * The following use a builtin implementation only if UACPI_USE_BUILTIN_STRING
 * is defined (more information can be found in the config.h header):
 * - memcpy
 * - memmove
 * - memset
 * - memcmp
 *
 * In case your platform happens to implement optimized verisons of the helpers
 * above, you are able to make uACPI use those instead by overriding them like so:
 *
 * #define uacpi_memcpy my_fast_memcpy
 * #define uacpi_snprintf my_fast_snprintf
 */
#endif
