#pragma once

#include <uacpi/internal/context.h>
#include <uacpi/internal/interpreter.h>
#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/tables.h>

struct uacpi_installed_table {
    uacpi_phys_addr phys_addr;
    struct acpi_sdt_hdr hdr;
    void *ptr;

    uacpi_u16 reference_count;

    uacpi_u8 flags;
    uacpi_u8 origin;
};

uacpi_status uacpi_initialize_tables(void);
void uacpi_deinitialize_tables(void);

uacpi_bool uacpi_signatures_match(const void *const lhs, const void *const rhs);
uacpi_status uacpi_check_table_signature(void *table, const uacpi_char *expect);
uacpi_status uacpi_verify_table_checksum(
    void *table, uacpi_size size, uacpi_u8 *out_flags
);

uacpi_status uacpi_table_install_physical_with_origin(
    uacpi_phys_addr phys, enum uacpi_table_origin origin, uacpi_table *out_table
);
uacpi_status uacpi_table_install_with_origin(
    void *virt, enum uacpi_table_origin origin, uacpi_table *out_table
);

#ifndef UACPI_BAREBONES_MODE
void uacpi_table_mark_as_loaded(uacpi_size idx);

uacpi_status uacpi_table_load_with_cause(
    uacpi_size idx, enum uacpi_table_load_cause cause
);
#endif // !UACPI_BAREBONES_MODE

typedef uacpi_iteration_decision (*uacpi_installed_table_iteration_callback)
    (void *user, struct uacpi_installed_table *tbl, uacpi_size idx);

uacpi_status uacpi_for_each_installed_table(
    uacpi_size base_idx, uacpi_installed_table_iteration_callback, void *user
);

typedef uacpi_bool (*uacpi_table_match_callback)
    (struct uacpi_installed_table *tbl);

uacpi_status uacpi_table_match(
    uacpi_size base_idx, uacpi_table_match_callback, uacpi_table *out_table
);

#define UACPI_PRI_TBL_HDR "'%.4s' (OEM ID '%.6s' OEM Table ID '%.8s')"
#define UACPI_FMT_TBL_HDR(hdr) (hdr)->signature, (hdr)->oemid, (hdr)->oem_table_id
