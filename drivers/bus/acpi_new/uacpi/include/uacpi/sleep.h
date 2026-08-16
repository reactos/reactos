#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/uacpi.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

/**
 * Set the firmware waking vector in FACS.
 *
 * 'addr32' is the real mode entry-point address
 * 'addr64' is the protected mode entry-point address
 */
UACPI_ALWAYS_ERROR_FOR_REDUCED_HARDWARE(
uacpi_status uacpi_set_waking_vector(
    uacpi_phys_addr addr32, uacpi_phys_addr addr64
))

typedef enum uacpi_sleep_state {
    UACPI_SLEEP_STATE_S0 = 0,
    UACPI_SLEEP_STATE_S1,
    UACPI_SLEEP_STATE_S2,
    UACPI_SLEEP_STATE_S3,
    UACPI_SLEEP_STATE_S4,
    UACPI_SLEEP_STATE_S5,
    UACPI_SLEEP_STATE_MAX = UACPI_SLEEP_STATE_S5,
} uacpi_sleep_state;

/**
 * Prepare for a given sleep state.
 */
uacpi_status uacpi_prepare_for_sleep_state(uacpi_sleep_state);

/**
 * Enter the given sleep state after preparation.
 */
uacpi_status uacpi_enter_sleep_state(uacpi_sleep_state);

/**
 * Prepare & enter a given sleep state
 * May be used as a shortcut if the kernel doesn't have any other work to do
 * inbetween calling the two separate helpers
 */
uacpi_status uacpi_enter_sleep_state_simple(uacpi_sleep_state);

/**
 * Prepare to leave the given sleep state.
 */
uacpi_status uacpi_prepare_for_wake_from_sleep_state(uacpi_sleep_state);

/**
 * Wake from the given sleep state.
 */
uacpi_status uacpi_wake_from_sleep_state(uacpi_sleep_state);

/**
 * Attempt reset via the FADT reset register.
 */
uacpi_status uacpi_reboot(void);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
