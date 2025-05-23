#pragma once

#include <uacpi/event.h>

// This fixed event is internal-only, and we don't expose it in the enum
#define UACPI_FIXED_EVENT_GLOBAL_LOCK 0

UACPI_ALWAYS_OK_FOR_REDUCED_HARDWARE(
    uacpi_status uacpi_initialize_events_early(void)
)

UACPI_ALWAYS_OK_FOR_REDUCED_HARDWARE(
    uacpi_status uacpi_initialize_events(void)
)
UACPI_STUB_IF_REDUCED_HARDWARE(
    void uacpi_deinitialize_events(void)
)

UACPI_STUB_IF_REDUCED_HARDWARE(
    void uacpi_events_match_post_dynamic_table_load(void)
)

UACPI_ALWAYS_ERROR_FOR_REDUCED_HARDWARE(
    uacpi_status uacpi_clear_all_events(void)
)
