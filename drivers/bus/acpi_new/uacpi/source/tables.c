#include <uacpi/internal/tables.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/interpreter.h>
#include <uacpi/internal/mutex.h>

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(
    table_array, struct uacpi_installed_table, UACPI_STATIC_TABLE_ARRAY_LEN
)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    table_array, struct uacpi_installed_table, static
)

static struct table_array tables;
static uacpi_bool early_table_access;
static uacpi_bool fadt_available;
static uacpi_table_installation_handler installation_handler;

#ifndef UACPI_BAREBONES_MODE

static uacpi_handle table_mutex;

uacpi_bool uacpi_table_subsystem_available(void)
{
    return early_table_access ||
        g_uacpi_rt_ctx.init_level >= UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED;
}

#define ENSURE_TABLES_ONLINE()                         \
    do {                                               \
        if (!early_table_access)                       \
            UACPI_ENSURE_INIT_LEVEL_AT_LEAST(          \
                UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED \
            );                                         \
    } while (0)

#else

uacpi_bool uacpi_table_subsystem_available(void)
{
    return early_table_access;
}

/*
 * Use a dummy function instead of a macro to prevent the following error:
 *     error: statement with no effect [-Werror=unused-value]
 */
static inline uacpi_status dummy_mutex_acquire_release(uacpi_handle mtx)
{
    UACPI_UNUSED(mtx);
    return UACPI_STATUS_OK;
}

#define table_mutex UACPI_NULL
#define uacpi_acquire_native_mutex_may_be_null dummy_mutex_acquire_release
#define uacpi_release_native_mutex_may_be_null dummy_mutex_acquire_release

#define ENSURE_TABLES_ONLINE()                       \
    do {                                             \
        if (!early_table_access)                     \
            return UACPI_STATUS_INIT_LEVEL_MISMATCH; \
    } while (0)

#endif // !UACPI_BAREBONES_MODE

static uacpi_status table_install_physical_with_origin_unlocked(
    uacpi_phys_addr phys, enum uacpi_table_origin origin,
    const uacpi_char *expected_signature, uacpi_table *out_table
);
static uacpi_status table_install_with_origin_unlocked(
    void *virt, enum uacpi_table_origin origin, uacpi_table *out_table
);

UACPI_PACKED(struct uacpi_rxsdt {
    struct acpi_sdt_hdr hdr;
    uacpi_u8 ptr_bytes[];
})

static void dump_table_header(
    uacpi_phys_addr phys_addr, void *hdr
)
{
    struct acpi_sdt_hdr *sdt = hdr;

    if (uacpi_signatures_match(hdr, ACPI_FACS_SIGNATURE)) {
        uacpi_info(
            "FACS 0x%016"UACPI_PRIX64" %08X", UACPI_FMT64(phys_addr),
            sdt->length
        );
        return;
    }

    if (!uacpi_memcmp(hdr, ACPI_RSDP_SIGNATURE, sizeof(ACPI_RSDP_SIGNATURE) - 1)) {
        struct acpi_rsdp *rsdp = hdr;

        uacpi_info(
            "RSDP 0x%016"UACPI_PRIX64" %08X v%02X (%6.6s)",
            UACPI_FMT64(phys_addr), rsdp->revision >= 2 ? rsdp->length : 20,
            rsdp->revision, rsdp->oemid
        );
        return;
    }

    uacpi_info(
        "%.4s 0x%016"UACPI_PRIX64" %08X v%02X (%6.6s %8.8s)",
        sdt->signature, UACPI_FMT64(phys_addr), sdt->length, sdt->revision,
        sdt->oemid, sdt->oem_table_id
    );
}

static uacpi_status initialize_from_rxsdt(uacpi_phys_addr rxsdt_addr,
                                          uacpi_size entry_size)
{
    struct uacpi_rxsdt *rxsdt;
    uacpi_size i, entry_bytes, map_len = sizeof(*rxsdt);
    uacpi_phys_addr entry_addr;
    uacpi_status ret;

    rxsdt = uacpi_kernel_map(rxsdt_addr, map_len);
    if (rxsdt == UACPI_MAP_FAILED)
        return UACPI_STATUS_MAPPING_FAILED;

    dump_table_header(rxsdt_addr, rxsdt);

    ret = uacpi_check_table_signature(rxsdt,
        entry_size == 8 ? ACPI_XSDT_SIGNATURE : ACPI_RSDT_SIGNATURE);
    if (uacpi_unlikely_error(ret))
        goto error_out;

    map_len = rxsdt->hdr.length;
    uacpi_kernel_unmap(rxsdt, sizeof(*rxsdt));

    if (uacpi_unlikely(map_len < (sizeof(*rxsdt) + entry_size)))
        return UACPI_STATUS_INVALID_TABLE_LENGTH;

    // Make sure length is aligned to entry size so we don't OOB
    entry_bytes = map_len - sizeof(*rxsdt);
    entry_bytes &= ~(entry_size - 1);

    rxsdt = uacpi_kernel_map(rxsdt_addr, map_len);
    if (uacpi_unlikely(rxsdt == UACPI_MAP_FAILED))
        return UACPI_STATUS_MAPPING_FAILED;

    ret = uacpi_verify_table_checksum(rxsdt, map_len, UACPI_NULL);
    if (uacpi_unlikely_error(ret))
        goto error_out;

    for (i = 0; i < entry_bytes; i += entry_size) {
        uacpi_u64 entry_phys_addr_large = 0;
        uacpi_memcpy(&entry_phys_addr_large, &rxsdt->ptr_bytes[i], entry_size);

        if (!entry_phys_addr_large)
            continue;

        entry_addr = uacpi_truncate_phys_addr_with_warn(entry_phys_addr_large);
        ret = uacpi_table_install_physical_with_origin(
            entry_addr, UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL, UACPI_NULL
        );
        if (uacpi_unlikely(ret != UACPI_STATUS_OK &&
                           ret != UACPI_STATUS_OVERRIDDEN))
            goto error_out;
    }

    ret = UACPI_STATUS_OK;

error_out:
    uacpi_kernel_unmap(rxsdt, map_len);
    return ret;
}

static uacpi_status initialize_from_rsdp(void)
{
    uacpi_status ret;
    uacpi_phys_addr rsdp_phys;
    struct acpi_rsdp *rsdp;
    uacpi_phys_addr rxsdt;
    uacpi_size rxsdt_entry_size;

    g_uacpi_rt_ctx.is_rev1 = UACPI_TRUE;

    ret = uacpi_kernel_get_rsdp(&rsdp_phys);
    if (uacpi_unlikely_error(ret))
        return ret;

    rsdp = uacpi_kernel_map(rsdp_phys, sizeof(struct acpi_rsdp));
    if (rsdp == UACPI_MAP_FAILED)
        return UACPI_STATUS_MAPPING_FAILED;

    dump_table_header(rsdp_phys, rsdp);

    if (rsdp->revision > 1 && rsdp->xsdt_addr &&
        !uacpi_check_flag(UACPI_FLAG_BAD_XSDT))
    {
        rxsdt = uacpi_truncate_phys_addr_with_warn(rsdp->xsdt_addr);
        rxsdt_entry_size = 8;
    } else {
        rxsdt = (uacpi_phys_addr)rsdp->rsdt_addr;
        rxsdt_entry_size = 4;
    }

    uacpi_kernel_unmap(rsdp, sizeof(struct acpi_rsdp));

    if (!rxsdt) {
        uacpi_error("both RSDT & XSDT tables are NULL!");
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return initialize_from_rxsdt(rxsdt, rxsdt_entry_size);
}

uacpi_size uacpi_table_count(void)
{
    return table_array_size(&tables);
}

uacpi_status uacpi_setup_early_table_access(
    void *temporary_buffer, uacpi_size buffer_size
)
{
    uacpi_status ret;
    uacpi_virt_addr buffer_addr, aligned_buffer_addr;

#ifndef UACPI_BAREBONES_MODE
    UACPI_ENSURE_INIT_LEVEL_IS(UACPI_INIT_LEVEL_EARLY);
#endif
    if (uacpi_unlikely(early_table_access))
        return UACPI_STATUS_INIT_LEVEL_MISMATCH;

    uacpi_logger_initialize();

    buffer_addr = UACPI_PTR_TO_VIRT_ADDR(temporary_buffer);
    aligned_buffer_addr = UACPI_ALIGN_UP(
        buffer_addr, UACPI_POINTER_SIZE, uacpi_virt_addr
    );
    if (buffer_addr != aligned_buffer_addr) {
        uacpi_size stripped_bytes;

        stripped_bytes = aligned_buffer_addr - buffer_addr;
        uacpi_warn(
            "fixed up misaligned early tables buffer (%zu bytes stripped)",
            stripped_bytes
        );

        buffer_size -= UACPI_MIN(stripped_bytes, buffer_size);
        temporary_buffer = UACPI_VIRT_ADDR_TO_PTR(aligned_buffer_addr);
    }

    if (uacpi_unlikely(buffer_size < sizeof(struct uacpi_installed_table)))
        return UACPI_STATUS_INVALID_ARGUMENT;

    tables.dynamic_storage = temporary_buffer;
    tables.dynamic_capacity = buffer_size / sizeof(struct uacpi_installed_table);
    early_table_access = UACPI_TRUE;

    ret = initialize_from_rsdp();
    if (uacpi_unlikely_error(ret))
        uacpi_deinitialize_tables();

    return ret;
}

#ifndef UACPI_BAREBONES_MODE
static uacpi_iteration_decision warn_if_early_referenced(
    void *user, struct uacpi_installed_table *tbl, uacpi_size idx
)
{
    UACPI_UNUSED(user);

    if (uacpi_unlikely(tbl->reference_count != 0)) {
        uacpi_warn(
            "table "UACPI_PRI_TBL_HDR" (%zu) still has %d early reference(s)!",
            UACPI_FMT_TBL_HDR(&tbl->hdr), idx, tbl->reference_count
        );
    }

    return UACPI_ITERATION_DECISION_CONTINUE;
}

uacpi_status uacpi_initialize_tables(void)
{
    if (early_table_access) {
        uacpi_size num_tables;

        uacpi_for_each_installed_table(0, warn_if_early_referenced, UACPI_NULL);

        // Reallocate the user buffer into a normal heap array
        num_tables = table_array_size(&tables);
        if (num_tables > table_array_inline_capacity(&tables)) {
            void *new_buf;

            /*
             * Allocate a new buffer with size equal to exactly the number of
             * dynamic tables (that live in the user provided temporary buffer).
             */
            num_tables -= table_array_inline_capacity(&tables);
            new_buf = uacpi_kernel_alloc(
                sizeof(struct uacpi_installed_table) * num_tables
            );
            if (uacpi_unlikely(new_buf == UACPI_NULL))
                return UACPI_STATUS_OUT_OF_MEMORY;

            uacpi_memcpy(new_buf, tables.dynamic_storage,
                         sizeof(struct uacpi_installed_table) * num_tables);
            tables.dynamic_storage = new_buf;
            tables.dynamic_capacity = num_tables;
        } else {
            /*
             * User-provided temporary buffer was not used at all, just remove
             * any references to it.
             */
            tables.dynamic_storage = UACPI_NULL;
            tables.dynamic_capacity = 0;
        }

        early_table_access = UACPI_FALSE;
    } else {
        uacpi_status ret;

        ret = initialize_from_rsdp();
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    if (!uacpi_is_hardware_reduced()) {
        struct acpi_fadt *fadt = &g_uacpi_rt_ctx.fadt;
        uacpi_table tbl;

        if (fadt->x_firmware_ctrl) {
            uacpi_status ret;

            ret = table_install_physical_with_origin_unlocked(
                fadt->x_firmware_ctrl, UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL,
                ACPI_FACS_SIGNATURE, &tbl
            );
            if (uacpi_unlikely(ret != UACPI_STATUS_OK &&
                               ret != UACPI_STATUS_OVERRIDDEN))
                return ret;

            g_uacpi_rt_ctx.facs = tbl.ptr;
        }
    }

    table_mutex = uacpi_kernel_create_mutex();
    if (uacpi_unlikely(table_mutex == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}
#endif // !UACPI_BAREBONES_MODE

void uacpi_deinitialize_tables(void)
{
    uacpi_size i;

    for (i = 0; i < table_array_size(&tables); ++i) {
        struct uacpi_installed_table *tbl = table_array_at(&tables, i);

        switch (tbl->origin) {
#ifndef UACPI_BAREBONES_MODE
        case UACPI_TABLE_ORIGIN_FIRMWARE_VIRTUAL:
            uacpi_free(tbl->ptr, tbl->hdr.length);
            break;
#endif
        case UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL:
        case UACPI_TABLE_ORIGIN_HOST_PHYSICAL:
            if (tbl->reference_count != 0)
                uacpi_kernel_unmap(tbl->ptr, tbl->hdr.length);
            break;
        default:
            break;
        }
    }

    if (early_table_access) {
        uacpi_memzero(&tables, sizeof(tables));
        early_table_access = UACPI_FALSE;
    } else {
        table_array_clear(&tables);
    }

    installation_handler = UACPI_NULL;
    fadt_available = UACPI_FALSE;

#ifndef UACPI_BAREBONES_MODE
    if (table_mutex)
        uacpi_kernel_free_mutex(table_mutex);

    table_mutex = UACPI_NULL;
#endif
}

uacpi_status uacpi_set_table_installation_handler(
    uacpi_table_installation_handler handler
)
{
    uacpi_status ret;

    ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (installation_handler != UACPI_NULL && handler != UACPI_NULL)
        goto out;

    installation_handler = handler;

out:
    uacpi_release_native_mutex_may_be_null(table_mutex);
    return ret;
}

static uacpi_status initialize_fadt(const void*);

static uacpi_u8 table_checksum(void *table, uacpi_size size)
{
    uacpi_u8 *bytes = table;
    uacpi_u8 csum = 0;
    uacpi_size i;

    for (i = 0; i < size; ++i)
        csum += bytes[i];

    return csum;
}

uacpi_status uacpi_verify_table_checksum(
    void *table, uacpi_size size, uacpi_u8 *out_flags
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    uacpi_u8 csum;

    csum = table_checksum(table, size);
    if (out_flags != UACPI_NULL)
        *out_flags |= UACPI_TABLE_CSUM_CHECKED;

    if (uacpi_unlikely(csum != 0)) {
        enum uacpi_log_level lvl = UACPI_LOG_WARN;
        struct acpi_sdt_hdr *hdr = table;

        if (out_flags != UACPI_NULL)
            *out_flags |= UACPI_TABLE_CSUM_BAD;

        if (uacpi_check_flag(UACPI_FLAG_BAD_CSUM_FATAL)) {
            ret = UACPI_STATUS_BAD_CHECKSUM;
            lvl = UACPI_LOG_ERROR;
        }

        uacpi_log_lvl(
            lvl, "invalid table "UACPI_PRI_TBL_HDR" checksum %d!",
            UACPI_FMT_TBL_HDR(hdr), csum
        );
    }

    return ret;
}

static bool skip_due_to_bad_csum(struct uacpi_installed_table *tbl)
{
    return uacpi_check_flag(UACPI_FLAG_BAD_CSUM_FATAL) &&
           (tbl->flags & UACPI_TABLE_CSUM_BAD);
}

uacpi_bool uacpi_signatures_match(const void *const lhs, const void *const rhs)
{
    return uacpi_memcmp(lhs, rhs, sizeof(uacpi_object_name)) == 0;
}

uacpi_status uacpi_check_table_signature(void *table, const uacpi_char *expect)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (!uacpi_signatures_match(table, expect)) {
        enum uacpi_log_level lvl = UACPI_LOG_WARN;
        struct acpi_sdt_hdr *hdr = table;

        if (uacpi_check_flag(UACPI_FLAG_BAD_TBL_SIGNATURE_FATAL)) {
            ret = UACPI_STATUS_INVALID_SIGNATURE;
            lvl = UACPI_LOG_ERROR;
        }

        uacpi_log_lvl(
            lvl,
            "invalid table "UACPI_PRI_TBL_HDR" signature (expected '%.4s')",
            UACPI_FMT_TBL_HDR(hdr), expect
        );
    }

    return ret;
}

static uacpi_status table_alloc(
    struct uacpi_installed_table **out_tbl, uacpi_size *out_idx
)
{
    struct uacpi_installed_table *tbl;

    if (early_table_access &&
        table_array_size(&tables) == table_array_capacity(&tables)) {
        uacpi_warn("early table access buffer capacity exhausted!");
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    tbl = table_array_alloc(&tables);
    if (uacpi_unlikely(tbl == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    *out_tbl = tbl;
    *out_idx = table_array_size(&tables) - 1;
    return UACPI_STATUS_OK;
}

static uacpi_status get_external_table_header(
    uacpi_phys_addr phys_addr, struct acpi_sdt_hdr *out_hdr
)
{
    void *virt;

    virt = uacpi_kernel_map(phys_addr, sizeof(*out_hdr));
    if (uacpi_unlikely(virt == UACPI_MAP_FAILED))
        return UACPI_STATUS_MAPPING_FAILED;

    uacpi_memcpy(out_hdr, virt, sizeof(*out_hdr));

    uacpi_kernel_unmap(virt,  sizeof(*out_hdr));
    return UACPI_STATUS_OK;
}

static uacpi_status table_ref_unlocked(struct uacpi_installed_table *tbl)
{
    switch (tbl->reference_count) {
    case 0: {
        uacpi_status ret;

        if (uacpi_unlikely(skip_due_to_bad_csum(tbl)))
            return UACPI_STATUS_BAD_CHECKSUM;

        if (tbl->origin != UACPI_TABLE_ORIGIN_HOST_PHYSICAL &&
            tbl->origin != UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL)
            break;

        tbl->ptr = uacpi_kernel_map(tbl->phys_addr, tbl->hdr.length);
        if (uacpi_unlikely(tbl->ptr == UACPI_MAP_FAILED))
            return UACPI_STATUS_MAPPING_FAILED;

        if (!(tbl->flags & UACPI_TABLE_CSUM_CHECKED)) {
            ret = uacpi_verify_table_checksum(
                tbl->ptr, tbl->hdr.length, &tbl->flags
            );
            if (uacpi_unlikely_error(ret)) {
                uacpi_kernel_unmap(tbl->ptr, tbl->hdr.length);
                tbl->ptr = UACPI_NULL;
                return ret;
            }
        }
        break;
    }
    case 0xFFFF - 1:
        uacpi_warn(
            "too many references for "UACPI_PRI_TBL_HDR
            ", mapping permanently", UACPI_FMT_TBL_HDR(&tbl->hdr)
        );
        break;
    default:
        break;
    }

    if (uacpi_likely(tbl->reference_count != 0xFFFF))
        tbl->reference_count++;
    return UACPI_STATUS_OK;
}

static uacpi_status table_unref_unlocked(struct uacpi_installed_table *tbl)
{
    switch (tbl->reference_count) {
    case 0:
        uacpi_warn(
            "tried to unref table "UACPI_PRI_TBL_HDR" with no references",
            UACPI_FMT_TBL_HDR(&tbl->hdr)
        );
        return UACPI_STATUS_INVALID_ARGUMENT;
    case 1:
        if (tbl->origin != UACPI_TABLE_ORIGIN_HOST_PHYSICAL &&
            tbl->origin != UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL)
            break;

        uacpi_kernel_unmap(tbl->ptr, tbl->hdr.length);
        tbl->ptr = UACPI_NULL;
        break;
    case 0xFFFF:
        /*
         * Consider the reference count (overflow) of 0xFFFF to be a permanently
         * mapped table as we don't know the actual number of references.
         */
        return UACPI_STATUS_OK;
    default:
        break;
    }

    tbl->reference_count--;
    return UACPI_STATUS_OK;
}

static void table_info_by_index_unchecked(
    uacpi_size idx, uacpi_table_info *out_info
)
{
    struct uacpi_installed_table *tbl;

    tbl = table_array_at(&tables, idx);

    out_info->idx = idx;
    out_info->size = tbl->hdr.length;
    uacpi_memcpy(
        out_info->signature, tbl->hdr.signature,
        sizeof(out_info->signature)
    );
    out_info->origin = tbl->origin;
    out_info->flags = tbl->flags;
    out_info->reference_count = tbl->reference_count;

    if (out_info->origin & (UACPI_TABLE_ORIGIN_HOST_VIRTUAL |
                            UACPI_TABLE_ORIGIN_FIRMWARE_VIRTUAL)) {
        out_info->virt_addr = tbl->ptr;
    } else {
        out_info->phys_addr = tbl->phys_addr;
    }
}

uacpi_status uacpi_table_info_get_by_index(
    uacpi_size idx, uacpi_table_info *out_info
)
{
    uacpi_status ret;

    ENSURE_TABLES_ONLINE();

    ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (uacpi_unlikely(table_array_size(&tables) <= idx)) {
        uacpi_error(
            "requested invalid table index %zu (%zu tables installed)",
            idx, table_array_size(&tables)
        );
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    table_info_by_index_unchecked(idx, out_info);

out:
    uacpi_release_native_mutex_may_be_null(table_mutex);
    return ret;
}

uacpi_status uacpi_for_each_table(
    uacpi_table_iteration_callback cb, void *user
)
{
    uacpi_status ret;
    uacpi_size num_tables, i = 0;
    uacpi_table_info info;

    ENSURE_TABLES_ONLINE();

    for (;;) {
        ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
        if (uacpi_unlikely_error(ret))
            return ret;

        // Number of tables may have increased while we were in the callback
        num_tables = table_array_size(&tables);
        if (i >= num_tables) {
            ret = uacpi_release_native_mutex_may_be_null(table_mutex);
            return ret;
        }

        table_info_by_index_unchecked(i, &info);

        ret = uacpi_release_native_mutex_may_be_null(table_mutex);
        if (uacpi_unlikely_error(ret))
            return ret;

        if (cb(user, &info) == UACPI_ITERATION_DECISION_BREAK)
            break;

        i++;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status verify_and_install_table(
    struct acpi_sdt_hdr *hdr, uacpi_phys_addr phys_addr, void *virt_addr,
    enum uacpi_table_origin origin, uacpi_table *out_table
)
{
    uacpi_status ret;
    struct uacpi_installed_table *table;
    uacpi_bool is_fadt;
    uacpi_size idx;
    uacpi_u8 flags = 0;

    is_fadt = uacpi_signatures_match(hdr->signature, ACPI_FADT_SIGNATURE);

    /*
     * FACS is the only(?) table without a checksum because it has OSPM
     * writable fields. Don't try to validate it here.
     */
    if (uacpi_signatures_match(hdr->signature, ACPI_FACS_SIGNATURE)) {
        flags |= UACPI_TABLE_CSUM_CHECKED;
    } else if (uacpi_check_flag(UACPI_FLAG_PROACTIVE_TBL_CSUM) || is_fadt ||
               out_table != UACPI_NULL) {
        void *mapping = virt_addr;

        // We may already have a valid mapping, reuse it if we do
        if (mapping == UACPI_NULL)
            mapping = uacpi_kernel_map(phys_addr, hdr->length);
        if (uacpi_unlikely(mapping == UACPI_MAP_FAILED))
            return UACPI_STATUS_MAPPING_FAILED;

        ret = uacpi_verify_table_checksum(mapping, hdr->length, &flags);
        if (uacpi_likely_success(ret)) {
            if (is_fadt)
                ret = initialize_fadt(mapping);
        }

        if (virt_addr == UACPI_NULL)
            uacpi_kernel_unmap(mapping, hdr->length);

        if (uacpi_unlikely_error(ret))
            return ret;
    }

    if (uacpi_signatures_match(hdr->signature, ACPI_DSDT_SIGNATURE))
        g_uacpi_rt_ctx.is_rev1 = hdr->revision < 2;

    ret = table_alloc(&table, &idx);
    if (uacpi_unlikely_error(ret))
        return ret;

    dump_table_header(phys_addr, hdr);

    uacpi_memcpy(&table->hdr, hdr, sizeof(*hdr));
    table->reference_count = 0;
    table->phys_addr = phys_addr;
    table->ptr = virt_addr;
    table->flags = flags;
    table->origin = origin;

    if (out_table == UACPI_NULL)
        return UACPI_STATUS_OK;

    table->reference_count++;
    out_table->ptr = virt_addr;
    out_table->index = idx;
    return UACPI_STATUS_OK;
}

static uacpi_status handle_table_override(
    uacpi_table_installation_disposition disposition, uacpi_u64 address,
    uacpi_table *out_table
)
{
    uacpi_status ret;

    switch (disposition) {
    case UACPI_TABLE_INSTALLATION_DISPOSITON_VIRTUAL_OVERRIDE:
        ret = table_install_with_origin_unlocked(
            UACPI_VIRT_ADDR_TO_PTR((uacpi_virt_addr)address),
            UACPI_TABLE_ORIGIN_HOST_VIRTUAL,
            out_table
        );
        return ret;
    case UACPI_TABLE_INSTALLATION_DISPOSITON_PHYSICAL_OVERRIDE:
        return table_install_physical_with_origin_unlocked(
            (uacpi_phys_addr)address,
            UACPI_TABLE_ORIGIN_HOST_PHYSICAL,
            UACPI_NULL,
            out_table
        );
    default:
        uacpi_error("invalid table installation disposition %d", disposition);
        return UACPI_STATUS_INTERNAL_ERROR;
    }
}

static uacpi_status table_install_physical_with_origin_unlocked(
    uacpi_phys_addr phys, enum uacpi_table_origin origin,
    const uacpi_char *expected_signature, uacpi_table *out_table
)
{
    struct acpi_sdt_hdr hdr;
    void *virt = UACPI_NULL;
    uacpi_status ret;

    ret = get_external_table_header(phys, &hdr);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (uacpi_unlikely(hdr.length < sizeof(struct acpi_sdt_hdr))) {
        uacpi_warn(
            "bogus table '%.4s' (0x%016"UACPI_PRIX64") size: %u bytes",
            hdr.signature, UACPI_FMT64(phys), hdr.length
        );
    }

    if (expected_signature != UACPI_NULL) {
        ret = uacpi_check_table_signature(&hdr, expected_signature);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    if (installation_handler != UACPI_NULL || out_table != UACPI_NULL) {
        virt = uacpi_kernel_map(phys, hdr.length);
        if (uacpi_unlikely(virt == UACPI_MAP_FAILED))
            return UACPI_STATUS_MAPPING_FAILED;
    }

    if (origin == UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL &&
        installation_handler != UACPI_NULL) {
        uacpi_u64 override;
        uacpi_table_installation_disposition disposition;

        disposition = installation_handler(virt, &override);

        switch (disposition) {
        case UACPI_TABLE_INSTALLATION_DISPOSITON_ALLOW:
            break;
        case UACPI_TABLE_INSTALLATION_DISPOSITON_DENY:
            uacpi_info(
                "table '%.4s' (0x%016"UACPI_PRIX64") installation denied "
                "by host", hdr.signature, UACPI_FMT64(phys)
            );
            ret = UACPI_STATUS_DENIED;
            goto out;

        default:
            uacpi_info(
                "table '%.4s' (0x%016"UACPI_PRIX64") installation "
                "overridden by host", hdr.signature, UACPI_FMT64(phys)
            );

            ret = handle_table_override(disposition, override, out_table);
            if (uacpi_likely_success(ret))
                ret = UACPI_STATUS_OVERRIDDEN;

            goto out;
        }
    }

    ret = verify_and_install_table(&hdr, phys, virt, origin, out_table);
out:
    // We don't unmap only in this case
    if (ret == UACPI_STATUS_OK && out_table != UACPI_NULL)
        return ret;

    if (virt != UACPI_NULL)
        uacpi_kernel_unmap(virt, hdr.length);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_table_install_physical_with_origin(
    uacpi_phys_addr phys, enum uacpi_table_origin origin, uacpi_table *out_table
)
{
    uacpi_status ret;

    ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = table_install_physical_with_origin_unlocked(
        phys, origin, UACPI_NULL, out_table
    );
    uacpi_release_native_mutex_may_be_null(table_mutex);

    return ret;
}

static uacpi_status table_install_with_origin_unlocked(
    void *virt, enum uacpi_table_origin origin, uacpi_table *out_table
)
{
    struct acpi_sdt_hdr *hdr = virt;

    if (uacpi_unlikely(hdr->length < sizeof(struct acpi_sdt_hdr))) {
        uacpi_error("invalid table '%.4s' (%p) size: %u",
                    hdr->signature, virt, hdr->length);
        return UACPI_STATUS_INVALID_TABLE_LENGTH;
    }

#ifndef UACPI_BAREBONES_MODE
    if (origin == UACPI_TABLE_ORIGIN_FIRMWARE_VIRTUAL &&
        installation_handler != UACPI_NULL) {
        uacpi_u64 override;
        uacpi_table_installation_disposition disposition;

        disposition = installation_handler(virt, &override);

        switch (disposition) {
        case UACPI_TABLE_INSTALLATION_DISPOSITON_ALLOW:
            break;
        case UACPI_TABLE_INSTALLATION_DISPOSITON_DENY:
            uacpi_info(
                "table "UACPI_PRI_TBL_HDR" installation denied by host",
                UACPI_FMT_TBL_HDR(hdr)
            );
            return UACPI_STATUS_DENIED;

        default: {
            uacpi_status ret;
            uacpi_info(
                "table "UACPI_PRI_TBL_HDR" installation overridden by host",
                UACPI_FMT_TBL_HDR(hdr)
            );

            ret = handle_table_override(disposition, override, out_table);
            if (uacpi_likely_success(ret))
                ret = UACPI_STATUS_OVERRIDDEN;

            return ret;
        }
        }
    }
#endif

    return verify_and_install_table(
        hdr, 0, virt, origin, out_table
    );
}

uacpi_status uacpi_table_install_with_origin(
    void *virt, enum uacpi_table_origin origin, uacpi_table *out_table
)
{
    uacpi_status ret;

    ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = table_install_with_origin_unlocked(virt, origin, out_table);

    uacpi_release_native_mutex_may_be_null(table_mutex);
    return ret;
}

uacpi_status uacpi_table_install(void *virt, uacpi_table *out_table)
{
    ENSURE_TABLES_ONLINE();

    return uacpi_table_install_with_origin(
        virt, UACPI_TABLE_ORIGIN_HOST_VIRTUAL, out_table
    );
}

uacpi_status uacpi_table_install_physical(
    uacpi_phys_addr addr, uacpi_table *out_table
)
{
    ENSURE_TABLES_ONLINE();

    return uacpi_table_install_physical_with_origin(
        addr, UACPI_TABLE_ORIGIN_HOST_PHYSICAL, out_table
    );
}

uacpi_status uacpi_for_each_installed_table(
    uacpi_size base_idx, uacpi_installed_table_iteration_callback cb, void *user
)
{
    uacpi_status ret;
    uacpi_size idx;
    struct uacpi_installed_table *tbl;
    uacpi_iteration_decision dec;

    ENSURE_TABLES_ONLINE();

    ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    for (idx = base_idx; idx < table_array_size(&tables); ++idx) {
        tbl = table_array_at(&tables, idx);

        if (uacpi_unlikely(skip_due_to_bad_csum(tbl)))
            continue;

        dec = cb(user, tbl, idx);
        if (dec == UACPI_ITERATION_DECISION_BREAK)
            break;
    }

    uacpi_release_native_mutex_may_be_null(table_mutex);
    return ret;
}

enum search_type {
    SEARCH_TYPE_BY_ID,
    SEARCH_TYPE_MATCH,
};

struct table_search_ctx {
    union {
        const uacpi_table_identifiers *id;
        uacpi_table_match_callback match_cb;
    };

    uacpi_table *out_table;
    uacpi_u8 search_type;
    uacpi_size to_skip;
    uacpi_status status;
};

static uacpi_iteration_decision do_search_tables(
    void *user, struct uacpi_installed_table *tbl, uacpi_size idx
)
{
    struct table_search_ctx *ctx = user;
    uacpi_table *out_table;
    uacpi_status ret;

    switch (ctx->search_type) {
    case SEARCH_TYPE_BY_ID: {
        const uacpi_table_identifiers *id = ctx->id;

        if (!uacpi_signatures_match(&id->signature, tbl->hdr.signature))
            return UACPI_ITERATION_DECISION_CONTINUE;
        if (id->oemid[0] != '\0' &&
            uacpi_memcmp(id->oemid, tbl->hdr.oemid, sizeof(id->oemid)) != 0)
            return UACPI_ITERATION_DECISION_CONTINUE;

        if (id->oem_table_id[0] != '\0' &&
            uacpi_memcmp(id->oem_table_id, tbl->hdr.oem_table_id,
                         sizeof(id->oem_table_id)) != 0)
            return UACPI_ITERATION_DECISION_CONTINUE;

        break;
    }

    case SEARCH_TYPE_MATCH:
        if (!ctx->match_cb(tbl))
            return UACPI_ITERATION_DECISION_CONTINUE;
        break;

    default:
        ctx->status = UACPI_STATUS_INVALID_ARGUMENT;
        return UACPI_ITERATION_DECISION_BREAK;
    }

    // Match, but we were asked for an nth table
    if (ctx->to_skip) {
        ctx->to_skip--;
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    ret = table_ref_unlocked(tbl);
    if (uacpi_likely_success(ret)) {
        out_table = ctx->out_table;
        out_table->ptr = tbl->ptr;
        out_table->index = idx;
        ctx->status = ret;
        return UACPI_ITERATION_DECISION_BREAK;
    }

    /*
     * Don't abort nor propagate bad checksums, just pretend this table never
     * existed and go on with the search.
     */
    if (ret == UACPI_STATUS_BAD_CHECKSUM)
        return UACPI_ITERATION_DECISION_CONTINUE;

    ctx->status = ret;
    return UACPI_ITERATION_DECISION_BREAK;
}

#ifndef UACPI_BAREBONES_MODE
uacpi_status uacpi_table_match(
    uacpi_size base_idx, uacpi_table_match_callback cb, uacpi_table *out_table
)
{
    uacpi_status ret;
    struct table_search_ctx ctx = { 0 };

    ctx.match_cb = cb;
    ctx.search_type = SEARCH_TYPE_MATCH;
    ctx.out_table = out_table;
    ctx.status = UACPI_STATUS_NOT_FOUND;

    ret = uacpi_for_each_installed_table(base_idx, do_search_tables, &ctx);
    if (uacpi_unlikely_error(ret))
        return ret;

    return ctx.status;
}
#endif

static uacpi_status find_table(
    uacpi_size idx, uacpi_size nth, const uacpi_table_identifiers *id,
    uacpi_table *out_table
)
{
    uacpi_status ret;
    struct table_search_ctx ctx = { 0 };

    ctx.id = id;
    ctx.out_table = out_table;
    ctx.search_type = SEARCH_TYPE_BY_ID;
    ctx.to_skip = nth;
    ctx.status = UACPI_STATUS_NOT_FOUND;

    ret = uacpi_for_each_installed_table(idx, do_search_tables, &ctx);
    if (uacpi_unlikely_error(ret))
        return ret;

    return ctx.status;
}

static uacpi_status find_table_by_signature(
    const uacpi_char *signature_string, uacpi_size idx, uacpi_size nth,
    uacpi_table *out_table
)
{
    struct uacpi_table_identifiers id = { 0 };

    ENSURE_TABLES_ONLINE();

    id.signature.text[0] = signature_string[0];
    id.signature.text[1] = signature_string[1];
    id.signature.text[2] = signature_string[2];
    id.signature.text[3] = signature_string[3];

    return find_table(idx, nth, &id, out_table);
}

uacpi_status uacpi_table_find_by_signature_at(
    const uacpi_char *signature, uacpi_size idx, uacpi_table *out_table
)
{
    return find_table_by_signature(signature, idx, 0, out_table);
}

uacpi_status uacpi_table_find_nth_by_signature(
    const uacpi_char *signature, uacpi_size nth, uacpi_table *out_table
)
{
    return find_table_by_signature(signature, 0, nth, out_table);
}

uacpi_status uacpi_table_find_by_signature(
    const uacpi_char *signature_string, struct uacpi_table *out_table
)
{
    return find_table_by_signature(signature_string, 0, 0, out_table);
}

uacpi_status uacpi_table_find_next_with_same_signature(
    uacpi_table *in_out_table
)
{
    struct uacpi_table_identifiers id = { 0 };

    ENSURE_TABLES_ONLINE();

    if (uacpi_unlikely(in_out_table->ptr == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    uacpi_memcpy(&id.signature, in_out_table->hdr->signature,
                 sizeof(id.signature));
    uacpi_table_unref(in_out_table);

    return find_table(in_out_table->index + 1, 0, &id, in_out_table);
}

uacpi_status uacpi_table_find(
    const uacpi_table_identifiers *id, uacpi_table *out_table
)
{
    ENSURE_TABLES_ONLINE();

    return find_table(0, 0, id, out_table);
}

#define TABLE_CTL_SET_FLAGS (1 << 0)
#define TABLE_CTL_CLEAR_FLAGS (1 << 1)
#define TABLE_CTL_VALIDATE_SET_FLAGS (1 << 2)
#define TABLE_CTL_VALIDATE_CLEAR_FLAGS (1 << 3)
#define TABLE_CTL_GET (1 << 4)
#define TABLE_CTL_PUT (1 << 5)

struct table_ctl_request {
    uacpi_u8 type;

    uacpi_u8 expect_set;
    uacpi_u8 expect_clear;
    uacpi_u8 set;
    uacpi_u8 clear;

    void *out_tbl;
};

static uacpi_status table_ctl(uacpi_size idx, struct table_ctl_request *req)
{
    uacpi_status ret;
    struct uacpi_installed_table *tbl;

    ENSURE_TABLES_ONLINE();

    ret = uacpi_acquire_native_mutex_may_be_null(table_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (uacpi_unlikely(table_array_size(&tables) <= idx)) {
        uacpi_error(
            "requested invalid table index %zu (%zu tables installed)",
            idx, table_array_size(&tables)
        );
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    tbl = table_array_at(&tables, idx);
    if (uacpi_unlikely(uacpi_unlikely(skip_due_to_bad_csum(tbl))))
        return UACPI_STATUS_BAD_CHECKSUM;

    if (req->type & TABLE_CTL_VALIDATE_SET_FLAGS) {
        uacpi_u8 mask = req->expect_set;

        if (uacpi_unlikely((tbl->flags & mask) != mask)) {
            uacpi_error(
                "unexpected table '%.4s' flags %02X, expected %02X to be set",
                tbl->hdr.signature, tbl->flags, mask
            );
            ret = UACPI_STATUS_INVALID_ARGUMENT;
            goto out;
        }
    }

    if (req->type & TABLE_CTL_VALIDATE_CLEAR_FLAGS) {
        uacpi_u8 mask = req->expect_clear;

        if (uacpi_unlikely((tbl->flags & mask) != 0)) {
            uacpi_error(
                "unexpected table '%.4s' flags %02X, expected %02X "
                "to be clear", tbl->hdr.signature, tbl->flags, mask
            );
            ret = UACPI_STATUS_ALREADY_EXISTS;
            goto out;
        }
    }

    if (req->type & TABLE_CTL_GET) {
        ret = table_ref_unlocked(tbl);
        if (uacpi_unlikely_error(ret))
            goto out;

        req->out_tbl = tbl->ptr;
    }

    if (req->type & TABLE_CTL_PUT) {
        ret = table_unref_unlocked(tbl);
        if (uacpi_unlikely_error(ret))
            goto out;
    }

    if (req->type & TABLE_CTL_SET_FLAGS)
        tbl->flags |= req->set;
    if (req->type & TABLE_CTL_CLEAR_FLAGS)
        tbl->flags &= ~req->clear;

out:
    uacpi_release_native_mutex_may_be_null(table_mutex);
    return ret;
}

#ifndef UACPI_BAREBONES_MODE
uacpi_status uacpi_table_load_with_cause(
    uacpi_size idx, enum uacpi_table_load_cause cause
)
{
    uacpi_status ret;
    struct table_ctl_request req = {
        .type = TABLE_CTL_SET_FLAGS | TABLE_CTL_VALIDATE_CLEAR_FLAGS |
                TABLE_CTL_GET,
        .set = UACPI_TABLE_LOADED,
        .expect_clear = UACPI_TABLE_LOADED,
    };

    ret = table_ctl(idx, &req);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_execute_table(req.out_tbl, cause);

    req.type = TABLE_CTL_PUT;
    table_ctl(idx, &req);
    return ret;
}

uacpi_status uacpi_table_load(uacpi_size idx)
{
    return uacpi_table_load_with_cause(idx, UACPI_TABLE_LOAD_CAUSE_HOST);
}

void uacpi_table_mark_as_loaded(uacpi_size idx)
{
    struct table_ctl_request req = {
        .type = TABLE_CTL_SET_FLAGS, .set = UACPI_TABLE_LOADED
    };

    table_ctl(idx, &req);
}
#endif // !UACPI_BAREBONES_MODE

uacpi_status uacpi_table_get_by_index(uacpi_size idx, uacpi_table *out_table)
{
    uacpi_status ret;
    struct table_ctl_request req = {
        .type = TABLE_CTL_GET,
    };

    ret = table_ctl(idx, &req);
    if (uacpi_unlikely_error(ret))
        return ret;

    out_table->ptr = req.out_tbl;
    out_table->index = idx;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_table_ref_by_index(uacpi_size index)
{
    struct table_ctl_request req = {
        .type = TABLE_CTL_GET
    };

    return table_ctl(index, &req);
}

uacpi_status uacpi_table_ref(uacpi_table *tbl)
{
    return uacpi_table_ref_by_index(tbl->index);
}

uacpi_status uacpi_table_unref_by_index(uacpi_size index)
{
    struct table_ctl_request req = {
        .type = TABLE_CTL_PUT
    };

    return table_ctl(index, &req);
}

uacpi_status uacpi_table_unref(uacpi_table *tbl)
{
    return uacpi_table_unref_by_index(tbl->index);
}

uacpi_u16 fadt_version_sizes[] = {
    116, 132, 244, 244, 268, 276
};

static void fadt_ensure_correct_revision(struct acpi_fadt *fadt)
{
    uacpi_size current_rev, rev;

    current_rev = fadt->hdr.revision;

    for (rev = 0; rev < UACPI_ARRAY_SIZE(fadt_version_sizes); ++rev) {
        if (fadt->hdr.length <= fadt_version_sizes[rev])
            break;
    }

    if (rev == UACPI_ARRAY_SIZE(fadt_version_sizes)) {
        uacpi_trace(
            "FADT revision (%zu) is likely greater than the last "
            "supported, reducing to %zu", current_rev, rev
        );
        fadt->hdr.revision = rev;
        return;
    }

    rev++;

    if (current_rev != rev && !(rev == 3 && current_rev == 4)) {
        uacpi_warn(
            "FADT length %u doesn't match expected for revision %zu, "
            "assuming version %zu", fadt->hdr.length, current_rev,
            rev
        );
        fadt->hdr.revision = rev;
    }
}

static void gas_init_system_io(
    struct acpi_gas *gas, uacpi_u64 address, uacpi_u8 byte_size
)
{
    gas->address = address;
    gas->address_space_id = UACPI_ADDRESS_SPACE_SYSTEM_IO;
    gas->register_bit_width = UACPI_MIN(255, byte_size * 8);
    gas->register_bit_offset = 0;
    gas->access_size = 0;
}


struct register_description {
    uacpi_size offset, xoffset;
    uacpi_size length_offset;
};

#define fadt_offset(field) uacpi_offsetof(struct acpi_fadt, field)

/*
 * We convert all the legacy registers into GAS format and write them into
 * the x_* fields for convenience and faster access at runtime.
 */
static struct register_description fadt_registers[] = {
    {
        .offset = fadt_offset(pm1a_evt_blk),
        .xoffset = fadt_offset(x_pm1a_evt_blk),
        .length_offset = fadt_offset(pm1_evt_len),
    },
    {
        .offset = fadt_offset(pm1b_evt_blk),
        .xoffset = fadt_offset(x_pm1b_evt_blk),
        .length_offset = fadt_offset(pm1_evt_len),
    },
    {
        .offset = fadt_offset(pm1a_cnt_blk),
        .xoffset = fadt_offset(x_pm1a_cnt_blk),
        .length_offset = fadt_offset(pm1_cnt_len),
    },
    {
        .offset = fadt_offset(pm1b_cnt_blk),
        .xoffset = fadt_offset(x_pm1b_cnt_blk),
        .length_offset = fadt_offset(pm1_cnt_len),
    },
    {
        .offset = fadt_offset(pm2_cnt_blk),
        .xoffset = fadt_offset(x_pm2_cnt_blk),
        .length_offset = fadt_offset(pm2_cnt_len),
    },
    {
        .offset = fadt_offset(pm_tmr_blk),
        .xoffset = fadt_offset(x_pm_tmr_blk),
        .length_offset = fadt_offset(pm_tmr_len),
    },
    {
        .offset = fadt_offset(gpe0_blk),
        .xoffset = fadt_offset(x_gpe0_blk),
        .length_offset = fadt_offset(gpe0_blk_len),
    },
    {
        .offset = fadt_offset(gpe1_blk),
        .xoffset = fadt_offset(x_gpe1_blk),
        .length_offset = fadt_offset(gpe1_blk_len),
    },
};

static void *fadt_relative(uacpi_size offset)
{
    return ((uacpi_u8*)&g_uacpi_rt_ctx.fadt) + offset;
}

static void convert_registers_to_gas(void)
{
    uacpi_size i;
    struct register_description *desc;
    struct acpi_gas *gas;
    uacpi_u32 legacy_addr;
    uacpi_u8 length;

    for (i = 0; i < UACPI_ARRAY_SIZE(fadt_registers); ++i) {
        desc = &fadt_registers[i];

        legacy_addr = *(uacpi_u32*)fadt_relative(desc->offset);
        length = *(uacpi_u8*)fadt_relative(desc->length_offset);
        gas = fadt_relative(desc->xoffset);

        if (gas->address)
            continue;

        gas_init_system_io(gas, legacy_addr, length);
    }
}

#ifndef UACPI_BAREBONES_MODE
static void split_one_block(
    struct acpi_gas *src, struct acpi_gas *dst0, struct acpi_gas *dst1
)
{
    uacpi_size byte_length;

    if (src->address == 0)
        return;

    byte_length = src->register_bit_width / 8;
    byte_length /= 2;

    gas_init_system_io(dst0, src->address, byte_length);
    gas_init_system_io(dst1, src->address + byte_length, byte_length);
}

static void split_event_blocks(void)
{
    split_one_block(
        &g_uacpi_rt_ctx.fadt.x_pm1a_evt_blk,
        &g_uacpi_rt_ctx.pm1a_status_blk,
        &g_uacpi_rt_ctx.pm1a_enable_blk
    );
    split_one_block(
        &g_uacpi_rt_ctx.fadt.x_pm1b_evt_blk,
        &g_uacpi_rt_ctx.pm1b_status_blk,
        &g_uacpi_rt_ctx.pm1b_enable_blk
    );
}
#endif // !UACPI_BAREBONES_MODE

static uacpi_status initialize_fadt(const void *virt)
{
    uacpi_status ret;
    struct acpi_fadt *fadt = &g_uacpi_rt_ctx.fadt;
    const struct acpi_sdt_hdr *hdr = virt;

    /*
     * Here we (roughly) follow ACPICA initialization sequence to make sure we
     * handle potential BIOS quirks with garbage inside FADT correctly.
     */

    uacpi_memcpy(fadt, hdr, UACPI_MIN(sizeof(*fadt), hdr->length));

#ifndef UACPI_REDUCED_HARDWARE
    g_uacpi_rt_ctx.is_hardware_reduced = fadt->flags & ACPI_HW_REDUCED_ACPI;
#endif

    fadt_ensure_correct_revision(fadt);

    /*
     * These are reserved prior to version 3, so zero them out to work around
     * BIOS implementations that might dirty these.
     */
    if (fadt->hdr.revision <= 2) {
        fadt->preferred_pm_profile = 0;
        fadt->pstate_cnt = 0;
        fadt->cst_cnt = 0;
        fadt->iapc_boot_arch = 0;
    }

    if (!fadt->x_dsdt)
        fadt->x_dsdt = fadt->dsdt;

    if (fadt->x_dsdt) {
        ret = table_install_physical_with_origin_unlocked(
            fadt->x_dsdt, UACPI_TABLE_ORIGIN_FIRMWARE_PHYSICAL,
            ACPI_DSDT_SIGNATURE, UACPI_NULL
        );
        if (uacpi_unlikely(ret != UACPI_STATUS_OK &&
                           ret != UACPI_STATUS_OVERRIDDEN))
            return ret;
    }

    /*
     * Unconditionally use 32 bit FACS if it exists, as 64 bit FACS is known
     * to cause issues on some firmware:
     * https://bugzilla.kernel.org/show_bug.cgi?id=74021
     *
     * Note that we don't install it here as FACS needs permanent mapping, which
     * we might not be able to obtain at this point in case of early table
     * access.
     */
    if (fadt->firmware_ctrl)
        fadt->x_firmware_ctrl = fadt->firmware_ctrl;

    if (!uacpi_is_hardware_reduced()) {
        convert_registers_to_gas();
#ifndef UACPI_BAREBONES_MODE
        split_event_blocks();
#endif
    }

    fadt_available = true;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_table_fadt(struct acpi_fadt **out_fadt)
{
    ENSURE_TABLES_ONLINE();

    if (!fadt_available)
        return UACPI_STATUS_NOT_FOUND;

    *out_fadt = &g_uacpi_rt_ctx.fadt;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_for_each_subtable(
    struct acpi_sdt_hdr *hdr, size_t hdr_size,
    uacpi_subtable_iteration_callback cb, void *user
)
{
    void *cursor;
    size_t bytes_left;

    cursor = UACPI_PTR_ADD(hdr, hdr_size);
    bytes_left = hdr->length - hdr_size;

    if (uacpi_unlikely(bytes_left > hdr->length))
        return UACPI_STATUS_INVALID_TABLE_LENGTH;

    while (bytes_left > sizeof(struct acpi_entry_hdr)) {
        struct acpi_entry_hdr *subtable_hdr = cursor;

        if (uacpi_unlikely(subtable_hdr->length > bytes_left ||
                           subtable_hdr->length < sizeof(*subtable_hdr))) {
            uacpi_error(
                "corrupted '%.4s' subtable length: %u (%zu bytes left)",
                hdr->signature, subtable_hdr->length, bytes_left
            );
            return UACPI_STATUS_INVALID_TABLE_LENGTH;
        }

        switch (cb(user, subtable_hdr)) {
        case UACPI_ITERATION_DECISION_CONTINUE:
            break;
        case UACPI_ITERATION_DECISION_BREAK:
            return UACPI_STATUS_OK;
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        cursor = UACPI_PTR_ADD(cursor, subtable_hdr->length);
        bytes_left -= subtable_hdr->length;
    }

    if (uacpi_unlikely(bytes_left != 0)) {
        uacpi_warn(
            "found %zu stray bytes in table '%.4s'",
            bytes_left, hdr->signature
        );
    }

    return UACPI_STATUS_OK;
}
