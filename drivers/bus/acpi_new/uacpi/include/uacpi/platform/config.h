#pragma once

#ifdef UACPI_OVERRIDE_CONFIG
#include "uacpi_config.h"
#else

#include <uacpi/helpers.h>
#include <uacpi/log.h>

/*
 * =======================
 * Context-related options
 * =======================
 */
#ifndef UACPI_DEFAULT_LOG_LEVEL
    #define UACPI_DEFAULT_LOG_LEVEL UACPI_LOG_INFO
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_LOG_LEVEL < UACPI_LOG_ERROR ||
    UACPI_DEFAULT_LOG_LEVEL > UACPI_LOG_DEBUG,
    "configured default log level is invalid"
);

#ifndef UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS
    #define UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS 30
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS < 1,
    "configured default loop timeout is invalid (expecting at least 1 second)"
);

#ifndef UACPI_DEFAULT_MAX_CALL_STACK_DEPTH
    #define UACPI_DEFAULT_MAX_CALL_STACK_DEPTH 256
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_MAX_CALL_STACK_DEPTH < 4,
    "configured default max call stack depth is invalid "
    "(expecting at least 4 frames)"
);

/*
 * ===================
 * Kernel-api options
 * ===================
 */

/*
 * Convenience initialization/deinitialization hooks that will be called by
 * uACPI automatically when appropriate if compiled-in.
 */
// #define UACPI_KERNEL_INITIALIZATION

/*
 * Makes kernel api logging callbacks work with unformatted printf-style
 * strings and va_args instead of a pre-formatted string. Can be useful if
 * your native logging is implemented in terms of this format as well.
 */
// #define UACPI_FORMATTED_LOGGING

/*
 * Makes uacpi_kernel_free take in an additional 'size_hint' parameter, which
 * contains the size of the original allocation. Note that this comes with a
 * performance penalty in some cases.
 */
// #define UACPI_SIZED_FREES


/*
 * Makes uacpi_kernel_alloc_zeroed mandatory to implement by the host, uACPI
 * will not provide a default implementation if this is enabled.
 */
// #define UACPI_NATIVE_ALLOC_ZEROED

/*
 * =========================
 * Platform-specific options
 * =========================
 */

/*
 * Makes uACPI use the internal versions of mem{cpy,move,set,cmp} instead of
 * relying on the host to provide them. Note that compilers like clang and GCC
 * rely on these being available by default, even in freestanding mode, so
 * compiling uACPI may theoretically generate implicit dependencies on them
 * even if this option is defined.
 */
// #define UACPI_USE_BUILTIN_STRING

/*
 * Turns uacpi_phys_addr and uacpi_io_addr into a 32-bit type, and adds extra
 * code for address truncation. Needed for e.g. i686 platforms without PAE
 * support.
 */
// #define UACPI_PHYS_ADDR_IS_32BITS

/*
 * Switches uACPI into reduced-hardware-only mode. Strips all full-hardware
 * ACPI support code at compile-time, including the event subsystem, the global
 * lock, and other full-hardware features.
 */
// #define UACPI_REDUCED_HARDWARE

/*
 * Switches uACPI into tables-subsystem-only mode and strips all other code.
 * This means only the table API will be usable, no other subsystems are
 * compiled in. In this mode, uACPI only depends on the following kernel APIs:
 * - uacpi_kernel_get_rsdp
 * - uacpi_kernel_{map,unmap}
 * - uacpi_kernel_log
 *
 * Use uacpi_setup_early_table_access to initialize, uacpi_state_reset to
 * deinitialize.
 *
 * This mode is primarily designed for these three use-cases:
 * - Bootloader/pre-kernel environments that need to parse ACPI tables, but
 *   don't actually need a fully-featured AML interpreter, and everything else
 *   that a full APCI implementation entails.
 * - A micro-kernel that has the full AML interpreter running in userspace, but
 *   still needs to parse ACPI tables to bootstrap allocators, timers, SMP etc.
 * - A WIP kernel that needs to parse ACPI tables for bootrapping SMP/timers,
 *   ECAM, etc., but doesn't yet have enough subsystems implemented in order
 *   to run a fully-featured AML interpreter.
 */
// #define UACPI_BAREBONES_MODE

/*
 * =============
 * Misc. options
 * =============
 */

/*
 * If UACPI_FORMATTED_LOGGING is not enabled, this is the maximum length of the
 * pre-formatted message that is passed to the logging callback.
 */
#ifndef UACPI_PLAIN_LOG_BUFFER_SIZE
    #define UACPI_PLAIN_LOG_BUFFER_SIZE 128
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_PLAIN_LOG_BUFFER_SIZE < 16,
    "configured log buffer size is too small (expecting at least 16 bytes)"
);

/*
 * The size of the table descriptor inline storage. All table descriptors past
 * this length will be stored in a dynamically allocated heap array. The size
 * of one table descriptor is approximately 56 bytes.
 */
#ifndef UACPI_STATIC_TABLE_ARRAY_LEN
    #define UACPI_STATIC_TABLE_ARRAY_LEN 16
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_STATIC_TABLE_ARRAY_LEN < 1,
    "configured static table array length is too small (expecting at least 1)"
);

#endif
