#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declared to avoid including the entire acpi.h here
struct acpi_fadt;

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

/*
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
/*
 * Load a previously installed table by feeding it to the interpreter.
 */
uacpi_status uacpi_table_load(uacpi_size index);
#endif // !UACPI_BAREBONES_MODE

/*
 * Helpers for finding tables.
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
uacpi_status uacpi_table_find_next_with_same_signature(
    uacpi_table *in_out_table
);
uacpi_status uacpi_table_find(
    const uacpi_table_identifiers *id, uacpi_table *out_table
);

/*
 * Increment/decrement a table's reference count.
 * The table is unmapped when the reference count drops to 0.
 */
uacpi_status uacpi_table_ref(uacpi_table*);
uacpi_status uacpi_table_unref(uacpi_table*);

/*
 * Returns the pointer to a sanitized internal version of FADT.
 *
 * The revision is guaranteed to be correct. All of the registers are converted
 * to GAS format. Fields that might contain garbage are cleared.
 */
uacpi_status uacpi_table_fadt(struct acpi_fadt**);

typedef enum uacpi_table_installation_disposition {
    // Allow the table to be installed as-is
    UACPI_TABLE_INSTALLATION_DISPOSITON_ALLOW = 0,

    /*
     * Deny the table from being installed completely. This is useful for
     * debugging various problems, e.g. AML loading bad SSDTs that cause the
     * system to hang or enter an undesired state.
     */
    UACPI_TABLE_INSTALLATION_DISPOSITON_DENY,

    /*
     * Override the table being installed with the table at the virtual address
     * returned in 'out_override_address'.
     */
    UACPI_TABLE_INSTALLATION_DISPOSITON_VIRTUAL_OVERRIDE,

    /*
     * Override the table being installed with the table at the physical address
     * returned in 'out_override_address'.
     */
    UACPI_TABLE_INSTALLATION_DISPOSITON_PHYSICAL_OVERRIDE,
} uacpi_table_installation_disposition;

typedef uacpi_table_installation_disposition (*uacpi_table_installation_handler)
    (struct acpi_sdt_hdr *hdr, uacpi_u64 *out_override_address);

/*
 * Set a handler that is invoked for each table before it gets installed.
 *
 * Depending on the return value, the table is either allowed to be installed
 * as-is, denied, or overriden with a new one.
 */
uacpi_status uacpi_set_table_installation_handler(
    uacpi_table_installation_handler handler
);

#ifdef __cplusplus
}
#endif
