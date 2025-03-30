#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/kernel_api.h>
#include <uacpi/namespace.h>

#define UACPI_MAJOR 2
#define UACPI_MINOR 1
#define UACPI_PATCH 1

#ifdef UACPI_REDUCED_HARDWARE
#define UACPI_MAKE_STUB_FOR_REDUCED_HARDWARE(fn, ret) \
    UACPI_NO_UNUSED_PARAMETER_WARNINGS_BEGIN          \
    static inline fn { return ret; }                \
    UACPI_NO_UNUSED_PARAMETER_WARNINGS_END

#define UACPI_STUB_IF_REDUCED_HARDWARE(fn) \
    UACPI_MAKE_STUB_FOR_REDUCED_HARDWARE(fn,)
#define UACPI_ALWAYS_ERROR_FOR_REDUCED_HARDWARE(fn) \
    UACPI_MAKE_STUB_FOR_REDUCED_HARDWARE(fn, UACPI_STATUS_COMPILED_OUT)
#define UACPI_ALWAYS_OK_FOR_REDUCED_HARDWARE(fn) \
    UACPI_MAKE_STUB_FOR_REDUCED_HARDWARE(fn, UACPI_STATUS_OK)
#else

#define UACPI_STUB_IF_REDUCED_HARDWARE(fn) fn;
#define UACPI_ALWAYS_ERROR_FOR_REDUCED_HARDWARE(fn) fn;
#define UACPI_ALWAYS_OK_FOR_REDUCED_HARDWARE(fn) fn;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Set up early access to the table subsystem. What this means is:
 * - uacpi_table_find() and similar API becomes usable before the call to
 *   uacpi_initialize().
 * - No kernel API besides logging and map/unmap will be invoked at this stage,
 *   allowing for heap and scheduling to still be fully offline.
 * - The provided 'temporary_buffer' will be used as a temporary storage for the
 *   internal metadata about the tables (list, reference count, addresses,
 *   sizes, etc).
 * - The 'temporary_buffer' is replaced with a normal heap buffer allocated via
 *   uacpi_kernel_alloc() after the call to uacpi_initialize() and can therefore
 *   be reclaimed by the kernel.
 *
 * The approximate overhead per table is 56 bytes, so a buffer of 4096 bytes
 * yields about 73 tables in terms of capacity. uACPI also has an internal
 * static buffer for tables, "UACPI_STATIC_TABLE_ARRAY_LEN", which is configured
 * as 16 descriptors in length by default.
 *
 * This function is used to initialize the barebones mode, see
 * UACPI_BAREBONES_MODE in config.h for more information.
 */
uacpi_status uacpi_setup_early_table_access(
    void *temporary_buffer, uacpi_size buffer_size
);

/*
 * Bad table checksum should be considered a fatal error
 * (table load is fully aborted in this case)
 */
#define UACPI_FLAG_BAD_CSUM_FATAL (1ull << 0)

/*
 * Unexpected table signature should be considered a fatal error
 * (table load is fully aborted in this case)
 */
#define UACPI_FLAG_BAD_TBL_SIGNATURE_FATAL (1ull << 1)

/*
 * Force uACPI to use RSDT even for later revisions
 */
#define UACPI_FLAG_BAD_XSDT (1ull << 2)

/*
 * If this is set, ACPI mode is not entered during the call to
 * uacpi_initialize. The caller is expected to enter it later at their own
 * discretion by using uacpi_enter_acpi_mode().
 */
#define UACPI_FLAG_NO_ACPI_MODE (1ull << 3)

/*
 * Don't create the \_OSI method when building the namespace.
 * Only enable this if you're certain that having this method breaks your AML
 * blob, a more atomic/granular interface management is available via osi.h
 */
#define UACPI_FLAG_NO_OSI (1ull << 4)

/*
 * Validate table checksums at installation time instead of first use.
 * Note that this makes uACPI map the entire table at once, which not all
 * hosts are able to handle at early init.
 */
#define UACPI_FLAG_PROACTIVE_TBL_CSUM (1ull << 5)

#ifndef UACPI_BAREBONES_MODE

/*
 * Initializes the uACPI subsystem, iterates & records all relevant RSDT/XSDT
 * tables. Enters ACPI mode.
 *
 * 'flags' is any combination of UACPI_FLAG_* above
 */
uacpi_status uacpi_initialize(uacpi_u64 flags);

/*
 * Parses & executes all of the DSDT/SSDT tables.
 * Initializes the event subsystem.
 */
uacpi_status uacpi_namespace_load(void);

/*
 * Initializes all the necessary objects in the namespaces by calling
 * _STA/_INI etc.
 */
uacpi_status uacpi_namespace_initialize(void);

// Returns the current subsystem initialization level
uacpi_init_level uacpi_get_current_init_level(void);

/*
 * Evaluate an object within the namespace and get back its value.
 * Either root or path must be valid.
 * A value of NULL for 'parent' implies uacpi_namespace_root() relative
 * lookups, unless 'path' is already absolute.
 */
uacpi_status uacpi_eval(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_object **ret
);
uacpi_status uacpi_eval_simple(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_object **ret
);

/*
 * Same as uacpi_eval() but without a return value.
 */
uacpi_status uacpi_execute(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args
);
uacpi_status uacpi_execute_simple(
    uacpi_namespace_node *parent, const uacpi_char *path
);

/*
 * Same as uacpi_eval, but the return value type is validated against
 * the 'ret_mask'. UACPI_STATUS_TYPE_MISMATCH is returned on error.
 */
uacpi_status uacpi_eval_typed(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_object_type_bits ret_mask,
    uacpi_object **ret
);
uacpi_status uacpi_eval_simple_typed(
    uacpi_namespace_node *parent, const uacpi_char *path,
    uacpi_object_type_bits ret_mask, uacpi_object **ret
);

/*
 * A shorthand for uacpi_eval_typed with UACPI_OBJECT_INTEGER_BIT.
 */
uacpi_status uacpi_eval_integer(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_u64 *out_value
);
uacpi_status uacpi_eval_simple_integer(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_u64 *out_value
);

/*
 * A shorthand for uacpi_eval_typed with
 *     UACPI_OBJECT_BUFFER_BIT | UACPI_OBJECT_STRING_BIT
 *
 * Use uacpi_object_get_string_or_buffer to retrieve the resulting buffer data.
 */
uacpi_status uacpi_eval_buffer_or_string(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_object **ret
);
uacpi_status uacpi_eval_simple_buffer_or_string(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_object **ret
);

/*
 * A shorthand for uacpi_eval_typed with UACPI_OBJECT_STRING_BIT.
 *
 * Use uacpi_object_get_string to retrieve the resulting buffer data.
 */
uacpi_status uacpi_eval_string(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_object **ret
);
uacpi_status uacpi_eval_simple_string(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_object **ret
);

/*
 * A shorthand for uacpi_eval_typed with UACPI_OBJECT_BUFFER_BIT.
 *
 * Use uacpi_object_get_buffer to retrieve the resulting buffer data.
 */
uacpi_status uacpi_eval_buffer(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_object **ret
);
uacpi_status uacpi_eval_simple_buffer(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_object **ret
);

/*
 * A shorthand for uacpi_eval_typed with UACPI_OBJECT_PACKAGE_BIT.
 *
 * Use uacpi_object_get_package to retrieve the resulting object array.
 */
uacpi_status uacpi_eval_package(
    uacpi_namespace_node *parent, const uacpi_char *path,
    const uacpi_object_array *args, uacpi_object **ret
);
uacpi_status uacpi_eval_simple_package(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_object **ret
);

/*
 * Get the bitness of the currently loaded AML code according to the DSDT.
 *
 * Returns either 32 or 64.
 */
uacpi_status uacpi_get_aml_bitness(uacpi_u8 *out_bitness);

/*
 * Helpers for entering & leaving ACPI mode. Note that ACPI mode is entered
 * automatically during the call to uacpi_initialize().
 */
UACPI_ALWAYS_OK_FOR_REDUCED_HARDWARE(
    uacpi_status uacpi_enter_acpi_mode(void)
)
UACPI_ALWAYS_ERROR_FOR_REDUCED_HARDWARE(
    uacpi_status uacpi_leave_acpi_mode(void)
)

/*
 * Attempt to acquire the global lock for 'timeout' milliseconds.
 * 0xFFFF implies infinite wait.
 *
 * On success, 'out_seq' is set to a unique sequence number for the current
 * acquire transaction. This number is used for validation during release.
 */
uacpi_status uacpi_acquire_global_lock(uacpi_u16 timeout, uacpi_u32 *out_seq);
uacpi_status uacpi_release_global_lock(uacpi_u32 seq);

#endif // !UACPI_BAREBONES_MODE

/*
 * Reset the global uACPI state by freeing all internally allocated data
 * structures & resetting any global variables. After this call, uACPI must be
 * re-initialized from scratch to be used again.
 *
 * This is called by uACPI automatically if a fatal error occurs during a call
 * to uacpi_initialize/uacpi_namespace_load etc. in order to prevent accidental
 * use of partially uninitialized subsystems.
 */
void uacpi_state_reset(void);

#ifdef __cplusplus
}
#endif
