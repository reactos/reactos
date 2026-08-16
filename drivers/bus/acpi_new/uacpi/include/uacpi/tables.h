#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declared to avoid including the entire acpi.h here
struct acpi_fadt;
struct acpi_entry_hdr;
struct acpi_sdt_hdr;

typedef struct uacpi_table_identifiers {
    uacpi_object_name signature;

    // if oemid[0] == 0 this field is ignored
    char oemid[6];

    // if oem_table_id[0] == 0 this field is ignored
    char oem_table_id[8];
} uacpi_table_identifiers;

typedef struct uacpi_table {
    union {
        uacpi_virt_addr virt_addr;
        void *ptr;
        struct acpi_sdt_hdr *hdr;
    };

     // Index number used to identify this table internally
    uacpi_size index;
} uacpi_table;

/**
 * Returns the number of tables stored in the internal array via 'out_count'.
 * This includes all firmware-provided, dynamically loaded, as well as
 * client-installed tables via 'uacpi_table_install' etc.
 */
uacpi_size uacpi_table_count(void);

/**
 * Install a table from either a virtual or a physical address.
 * The table is simply stored in the internal table array, and not loaded by
 * the interpreter (see uacpi_table_load).
 *
 * The table is optionally returned via 'out_table'.
 *
 * Manual calls to uacpi_table_install are not subject to filtering via the
 * table installation callback (if any).
 */
uacpi_status uacpi_table_install(
    void*, uacpi_table *out_table
);
uacpi_status uacpi_table_install_physical(
    uacpi_phys_addr, uacpi_table *out_table
);

#ifndef UACPI_BAREBONES_MODE
/**
 * Load a previously installed table by feeding it to the interpreter.
 */
uacpi_status uacpi_table_load(uacpi_size index);
#endif // !UACPI_BAREBONES_MODE

/**
 * Helpers for finding tables.
 *
 * for find_by_signature:
 *     'signature' is an array of 4 characters, a null terminator is not
 *     necessary and can be omitted (especially useful for non-C language
 *     bindings)
 *
 * for find_by_signature_at:
 *    'offset' specifies the base index in the internal array where to start
 *    searching for the table
 *
 * for find_nth_by_signature:
 *    This works exactly like find_by_signature, but finds the nth copy of the
 *    specified table in the firmware enumeration order
 *
 * 'out_table' is a pointer to a caller allocated uacpi_table structure that
 * receives the table pointer & its internal index in case the call was
 * successful.
 *
 * NOTE:
 * The returned table's reference count is incremented by 1, which keeps its
 * mapping alive forever unless uacpi_table_unref() is called for this table
 * later on. Calling uacpi_table_find_next_with_same_signature() on a table also
 * drops its reference count by 1, so if you want to keep it mapped you must
 * manually call uacpi_table_ref() beforehand.
 */
uacpi_status uacpi_table_find_by_signature(
    const uacpi_char *signature, uacpi_table *out_table
);
uacpi_status uacpi_table_find_by_signature_at(
    const uacpi_char *signature, uacpi_size offset, uacpi_table *out_table
);
uacpi_status uacpi_table_find_nth_by_signature(
    const uacpi_char *signature, uacpi_size nth, uacpi_table *out_table
);

uacpi_status uacpi_table_find_next_with_same_signature(
    uacpi_table *in_out_table
);
uacpi_status uacpi_table_find(
    const uacpi_table_identifiers *id, uacpi_table *out_table
);

/**
 * Returns a table by its index in the internal table array.
 *
 * The number of available tables can be queried via 'uacpi_table_count()'.
 *
 * NOTE:
 * Even if a table is present, this may still fail with
 * UACPI_STATUS_BAD_CHECKSUM in case UACPI_FLAG_BAD_CSUM_FATAL is enabled.
 */
uacpi_status uacpi_table_get_by_index(uacpi_size, uacpi_table *out_table);

/**
 * Increment/decrement a table's reference count.
 * The table is unmapped when the reference count drops to 0.
 */
uacpi_status uacpi_table_ref(uacpi_table*);
uacpi_status uacpi_table_ref_by_index(uacpi_size);
uacpi_status uacpi_table_unref(uacpi_table*);
uacpi_status uacpi_table_unref_by_index(uacpi_size);

/**
 * Returns the pointer to a sanitized internal version of FADT.
 *
 * The revision is guaranteed to be correct. All of the registers are converted
 * to GAS format. Fields that might contain garbage are cleared.
 */
uacpi_status uacpi_table_fadt(struct acpi_fadt**);

typedef enum uacpi_table_installation_disposition {
    // Allow the table to be installed as-is
    UACPI_TABLE_INSTALLATION_DISPOSITON_ALLOW = 0,

    /**
     * Deny the table from being installed completely. This is useful for
     * debugging various problems, e.g. AML loading bad SSDTs that cause the
     * system to hang or enter an undesired state.
     */
    UACPI_TABLE_INSTALLATION_DISPOSITON_DENY,

    /**
     * Override the table being installed with the table at the virtual address
     * returned in 'out_override_address'.
     */
    UACPI_TABLE_INSTALLATION_DISPOSITON_VIRTUAL_OVERRIDE,

    /**
     * Override the table being installed with the table at the physical address
     * returned in 'out_override_address'.
     */
    UACPI_TABLE_INSTALLATION_DISPOSITON_PHYSICAL_OVERRIDE,
} uacpi_table_installation_disposition;

typedef uacpi_table_installation_disposition (*uacpi_table_installation_handler)
    (struct acpi_sdt_hdr *hdr, uacpi_u64 *out_override_address);

/**
 * Set a handler that is invoked for each table before it gets installed.
 *
 * Depending on the return value, the table is either allowed to be installed
 * as-is, denied, or overriden with a new one.
 */
uacpi_status uacpi_set_table_installation_handler(
    uacpi_table_installation_handler handler
);

typedef enum uacpi_table_origin {
    /**
     * A table that originated from a physical address provided by the firmware.
     * All tables discovered via RSDP have this origin.
     */
    UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL = 1 << 0,

    /**
     * A table that was dynamically loaded by the AML firmware.
     * This includes both Load/LoadTable opcodes. Such tables live in a
     * heap-allocated kernel buffer.
     */
    UACPI_TABLE_ORIGIN_FIRMWARE_VIRTUAL = 1 << 1,

    /**
     * A table installed by the client code via uacpi_table_install_physical().
     */
    UACPI_TABLE_ORIGIN_HOST_PHYSICAL = 1 << 2,

    /**
     * A table installed by the client code via uacpi_table_install().
     */
    UACPI_TABLE_ORIGIN_HOST_VIRTUAL = 1 << 3,
} uacpi_table_origin;

typedef struct uacpi_table_info {
    /**
     * The index of this table in the internal array
     */
    uacpi_size idx;

    /**
     * Size of this table in bytes
     */
    uacpi_size size;

    union {
        /**
         * The physical address of this table, only applicable for
         * UACPI_TABLE_ORIGIN_*_PHYSICAL.
         */
        uacpi_phys_addr phys_addr;

        /**
         * The virtual address of this table, only applicable for
         * UACPI_TABLE_ORIGIN_*_VIRTUAL.
         *
         * NOTE: use uacpi_table_get_by_index() if you need a virtual address
         *       for a UACPI_TABLE_ORIGIN_*_PHYSICAL table in order to map it.
         */
        void *virt_addr;
    };

    /**
     * Signature of this table
     */
    uacpi_char signature[4];

    /**
     * One of 'uacpi_table_origin' values
     */
    uacpi_u8 origin;

/**
 * This table has been processed & loaded by the AML interpreter.
 * Only applicable for AML bytecode tables.
 */
#define UACPI_TABLE_LOADED (1 << 0)

/**
 * This table's checksum has been checked. The checksum is valid if
 * UACPI_TABLE_CSUM_BAD is not set.
 */
#define UACPI_TABLE_CSUM_CHECKED (1 << 1)

/**
 * This table's checksum was found to be incorrect.
 * Note that if UACPI_FLAG_PROACTIVE_TBL_CSUM is enabled alongside
 * UACPI_FLAG_BAD_CSUM_FATAL, such tables are never installed in the first
 * place.
 */
#define UACPI_TABLE_CSUM_BAD (1 << 2)

    /**
     * A mask of UACPI_TABLE_* flags
     */
    uacpi_u8 flags;

    /**
     * Reference count of this table
     */
    uacpi_u16 reference_count;
} uacpi_table_info;

typedef uacpi_iteration_decision (*uacpi_table_iteration_callback)
    (uacpi_handle, uacpi_table_info *info);

/**
 * Iterate every installed table on the system.
 *
 * The provided callback receives the information about each table in the
 * 'info' pointer. A table may be mapped by client code if needed via
 * uacpi_table_get_by_index().
 *
 * Note that this is a low level helper that iterates _every_ installed table,
 * even tables that are unreachable via uacpi_table_find() etc. due to bad
 * checksum.
 */
uacpi_status uacpi_for_each_table(
    uacpi_table_iteration_callback, void *user
);

/*
 * Retrieve information about the table installed internally at 'idx'.
 *
 * The number of available tables can be retrieved via uacpi_table_count().
 */
uacpi_status uacpi_table_info_get_by_index(
    uacpi_size idx, uacpi_table_info *out_info
);

typedef uacpi_iteration_decision (*uacpi_subtable_iteration_callback)
    (uacpi_handle, struct acpi_entry_hdr*);

/**
 * Iterate every subtable of a table such as MADT or SRAT.
 *
 * 'hdr' is the pointer to the main table, 'hdr_size' is the number of bytes in
 * the table before the beginning of the subtable records. 'cb' is the callback
 * invoked for each subtable with the 'user' context pointer passed for every
 * invocation.
 *
 * Example usage:
 *    uacpi_table tbl;
 *
 *    uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &tbl);
 *    uacpi_for_each_subtable(
 *        tbl.hdr, sizeof(struct acpi_madt), parse_madt, NULL
 *    );
 */
uacpi_status uacpi_for_each_subtable(
    struct acpi_sdt_hdr *hdr, size_t hdr_size,
    uacpi_subtable_iteration_callback cb, void *user
);

#ifdef __cplusplus
}
#endif
