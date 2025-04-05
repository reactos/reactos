#include <uacpi/internal/registers.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/context.h>
#include <uacpi/internal/io.h>
#include <uacpi/internal/log.h>
#include <uacpi/platform/atomic.h>
#include <uacpi/acpi.h>

#ifndef UACPI_BAREBONES_MODE

static uacpi_handle g_reg_lock;

enum register_kind {
    REGISTER_KIND_GAS,
    REGISTER_KIND_IO,
};

enum register_access_kind {
    REGISTER_ACCESS_KIND_PRESERVE,
    REGISTER_ACCESS_KIND_WRITE_TO_CLEAR,
    REGISTER_ACCESS_KIND_NORMAL,
};

struct register_spec {
    uacpi_u8 kind;
    uacpi_u8 access_kind;
    uacpi_u8 access_width; // only REGISTER_KIND_IO
    void *accessors[2];
    uacpi_u64 write_only_mask;
    uacpi_u64 preserve_mask;
};

static const struct register_spec g_registers[UACPI_REGISTER_MAX + 1] = {
    [UACPI_REGISTER_PM1_STS] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_WRITE_TO_CLEAR,
        .accessors = {
            &g_uacpi_rt_ctx.pm1a_status_blk,
            &g_uacpi_rt_ctx.pm1b_status_blk,
        },
        .preserve_mask = ACPI_PM1_STS_IGN0_MASK,
    },
    [UACPI_REGISTER_PM1_EN] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessors = {
            &g_uacpi_rt_ctx.pm1a_enable_blk,
            &g_uacpi_rt_ctx.pm1b_enable_blk,
        },
    },
    [UACPI_REGISTER_PM1_CNT] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessors = {
            &g_uacpi_rt_ctx.fadt.x_pm1a_cnt_blk,
            &g_uacpi_rt_ctx.fadt.x_pm1b_cnt_blk,
        },
        .write_only_mask = ACPI_PM1_CNT_SLP_EN_MASK |
                           ACPI_PM1_CNT_GBL_RLS_MASK,
        .preserve_mask = ACPI_PM1_CNT_PRESERVE_MASK,
    },
    [UACPI_REGISTER_PM_TMR] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessors = { &g_uacpi_rt_ctx.fadt.x_pm_tmr_blk, },
    },
    [UACPI_REGISTER_PM2_CNT] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessors = { &g_uacpi_rt_ctx.fadt.x_pm2_cnt_blk, },
        .preserve_mask = ACPI_PM2_CNT_PRESERVE_MASK,
    },
    [UACPI_REGISTER_SLP_CNT] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_PRESERVE,
        .accessors = { &g_uacpi_rt_ctx.fadt.sleep_control_reg, },
        .write_only_mask = ACPI_SLP_CNT_SLP_EN_MASK,
        .preserve_mask = ACPI_SLP_CNT_PRESERVE_MASK,
    },
    [UACPI_REGISTER_SLP_STS] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_WRITE_TO_CLEAR,
        .accessors = { &g_uacpi_rt_ctx.fadt.sleep_status_reg, },
        .preserve_mask = ACPI_SLP_STS_PRESERVE_MASK,
    },
    [UACPI_REGISTER_RESET] = {
        .kind = REGISTER_KIND_GAS,
        .access_kind = REGISTER_ACCESS_KIND_NORMAL,
        .accessors = { &g_uacpi_rt_ctx.fadt.reset_reg, },
    },
    [UACPI_REGISTER_SMI_CMD] = {
        .kind = REGISTER_KIND_IO,
        .access_kind = REGISTER_ACCESS_KIND_NORMAL,
        .access_width = 1,
        .accessors = { &g_uacpi_rt_ctx.fadt.smi_cmd, },
    },
};

enum register_mapping_state {
   REGISTER_MAPPING_STATE_NONE = 0,
   REGISTER_MAPPING_STATE_NOT_NEEDED,
   REGISTER_MAPPING_STATE_MAPPED,
};

struct register_mapping {
    uacpi_mapped_gas mappings[2];
    uacpi_u8 states[2];
};
static struct register_mapping g_register_mappings[UACPI_REGISTER_MAX + 1];

static uacpi_status map_one(
    const struct register_spec *spec, struct register_mapping *mapping,
    uacpi_u8 idx
)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (mapping->states[idx] != REGISTER_MAPPING_STATE_NONE)
        return ret;

    if (spec->kind == REGISTER_KIND_GAS) {
        struct acpi_gas *gas = spec->accessors[idx];

        if (gas == UACPI_NULL || gas->address == 0) {
            mapping->states[idx] = REGISTER_MAPPING_STATE_NOT_NEEDED;
            return ret;
        }

        ret = uacpi_map_gas_noalloc(gas, &mapping->mappings[idx]);
    } else {
        if (idx != 0) {
            mapping->states[idx] = REGISTER_MAPPING_STATE_NOT_NEEDED;
            return ret;
        }

        struct acpi_gas temp_gas = {
            .address_space_id = UACPI_ADDRESS_SPACE_SYSTEM_IO,
            .address = *(uacpi_u32*)spec->accessors[0],
            .register_bit_width = spec->access_width * 8,
        };

        ret = uacpi_map_gas_noalloc(&temp_gas, &mapping->mappings[idx]);
    }

    if (uacpi_likely_success(ret))
        mapping->states[idx] = REGISTER_MAPPING_STATE_MAPPED;

    return ret;
}

static uacpi_status ensure_register_mapped(
    const struct register_spec *spec, struct register_mapping *mapping
)
{
    uacpi_status ret;
    uacpi_bool needs_mapping = UACPI_FALSE;
    uacpi_u8 state;
    uacpi_cpu_flags flags;

    state = uacpi_atomic_load8(&mapping->states[0]);
    needs_mapping |= state == REGISTER_MAPPING_STATE_NONE;

    state = uacpi_atomic_load8(&mapping->states[1]);
    needs_mapping |= state == REGISTER_MAPPING_STATE_NONE;

    if (!needs_mapping)
        return UACPI_STATUS_OK;

    flags = uacpi_kernel_lock_spinlock(g_reg_lock);

    ret = map_one(spec, mapping, 0);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = map_one(spec, mapping, 1);
out:
    uacpi_kernel_unlock_spinlock(g_reg_lock, flags);
    return ret;
}

static uacpi_status get_reg(
    uacpi_u8 idx, const struct register_spec **out_spec,
    struct register_mapping **out_mapping
)
{
    if (idx > UACPI_REGISTER_MAX)
        return UACPI_STATUS_INVALID_ARGUMENT;

    *out_spec = &g_registers[idx];
    *out_mapping = &g_register_mappings[idx];
    return UACPI_STATUS_OK;
}

static uacpi_status do_read_one(
    struct register_mapping *mapping, uacpi_u8 idx, uacpi_u64 *out_value
)
{
    if (mapping->states[idx] != REGISTER_MAPPING_STATE_MAPPED)
        return UACPI_STATUS_OK;

    return uacpi_gas_read_mapped(&mapping->mappings[idx], out_value);
}

static uacpi_status do_read_register(
    const struct register_spec *reg, struct register_mapping *mapping,
    uacpi_u64 *out_value
)
{
    uacpi_status ret;
    uacpi_u64 value0 = 0, value1 = 0;

    ret = do_read_one(mapping, 0, &value0);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = do_read_one(mapping, 1, &value1);
    if (uacpi_unlikely_error(ret))
        return ret;

    *out_value = value0 | value1;
    if (reg->write_only_mask)
        *out_value &= ~reg->write_only_mask;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_read_register(
    enum uacpi_register reg_enum, uacpi_u64 *out_value
)
{
    uacpi_status ret;
    const struct register_spec *reg;
    struct register_mapping *mapping;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = get_reg(reg_enum, &reg, &mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = ensure_register_mapped(reg, mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    return do_read_register(reg, mapping, out_value);
}

static uacpi_status do_write_one(
    struct register_mapping *mapping, uacpi_u8 idx, uacpi_u64 in_value
)
{
    if (mapping->states[idx] != REGISTER_MAPPING_STATE_MAPPED)
        return UACPI_STATUS_OK;

    return uacpi_gas_write_mapped(&mapping->mappings[idx], in_value);
}

static uacpi_status do_write_register(
    const struct register_spec *reg, struct register_mapping *mapping,
    uacpi_u64 in_value
)
{
    uacpi_status ret;

    if (reg->preserve_mask) {
        in_value &= ~reg->preserve_mask;

        if (reg->access_kind == REGISTER_ACCESS_KIND_PRESERVE) {
            uacpi_u64 data;

            ret = do_read_register(reg, mapping, &data);
            if (uacpi_unlikely_error(ret))
                return ret;

            in_value |= data & reg->preserve_mask;
        }
    }

    ret = do_write_one(mapping, 0, in_value);
    if (uacpi_unlikely_error(ret))
        return ret;

    return do_write_one(mapping, 1, in_value);
}

uacpi_status uacpi_write_register(
    enum uacpi_register reg_enum, uacpi_u64 in_value
)
{
    uacpi_status ret;
    const struct register_spec *reg;
    struct register_mapping *mapping;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = get_reg(reg_enum, &reg, &mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = ensure_register_mapped(reg, mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    return do_write_register(reg, mapping, in_value);
}

uacpi_status uacpi_write_registers(
    enum uacpi_register reg_enum, uacpi_u64 in_value0, uacpi_u64 in_value1
)
{
    uacpi_status ret;
    const struct register_spec *reg;
    struct register_mapping *mapping;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = get_reg(reg_enum, &reg, &mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = ensure_register_mapped(reg, mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = do_write_one(mapping, 0, in_value0);
    if (uacpi_unlikely_error(ret))
        return ret;

    return do_write_one(mapping, 1, in_value1);
}

struct register_field {
    uacpi_u8 reg;
    uacpi_u8 offset;
    uacpi_u16 mask;
};

static const struct register_field g_fields[UACPI_REGISTER_FIELD_MAX + 1] = {
    [UACPI_REGISTER_FIELD_TMR_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_TMR_STS_IDX,
        .mask = ACPI_PM1_STS_TMR_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_BM_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_BM_STS_IDX,
        .mask = ACPI_PM1_STS_BM_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_GBL_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_GBL_STS_IDX,
        .mask = ACPI_PM1_STS_GBL_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_PWRBTN_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_PWRBTN_STS_IDX,
        .mask = ACPI_PM1_STS_PWRBTN_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_SLPBTN_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_SLPBTN_STS_IDX,
        .mask = ACPI_PM1_STS_SLPBTN_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_RTC_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_RTC_STS_IDX,
        .mask = ACPI_PM1_STS_RTC_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_HWR_WAK_STS] = {
        .reg = UACPI_REGISTER_SLP_STS,
        .offset = ACPI_SLP_STS_WAK_STS_IDX,
        .mask = ACPI_SLP_STS_WAK_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_WAK_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_WAKE_STS_IDX,
        .mask = ACPI_PM1_STS_WAKE_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_PCIEX_WAKE_STS] = {
        .reg = UACPI_REGISTER_PM1_STS,
        .offset = ACPI_PM1_STS_PCIEXP_WAKE_STS_IDX,
        .mask = ACPI_PM1_STS_PCIEXP_WAKE_STS_MASK,
    },
    [UACPI_REGISTER_FIELD_TMR_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_TMR_EN_IDX,
        .mask = ACPI_PM1_EN_TMR_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_GBL_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_GBL_EN_IDX,
        .mask = ACPI_PM1_EN_GBL_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_PWRBTN_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_PWRBTN_EN_IDX,
        .mask = ACPI_PM1_EN_PWRBTN_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_SLPBTN_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_SLPBTN_EN_IDX,
        .mask = ACPI_PM1_EN_SLPBTN_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_RTC_EN] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_RTC_EN_IDX,
        .mask = ACPI_PM1_EN_RTC_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_PCIEXP_WAKE_DIS] = {
        .reg = UACPI_REGISTER_PM1_EN,
        .offset = ACPI_PM1_EN_PCIEXP_WAKE_DIS_IDX,
        .mask = ACPI_PM1_EN_PCIEXP_WAKE_DIS_MASK,
    },
    [UACPI_REGISTER_FIELD_SCI_EN] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_SCI_EN_IDX,
        .mask = ACPI_PM1_CNT_SCI_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_BM_RLD] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_BM_RLD_IDX,
        .mask = ACPI_PM1_CNT_BM_RLD_MASK,
    },
    [UACPI_REGISTER_FIELD_GBL_RLS] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_GBL_RLS_IDX,
        .mask = ACPI_PM1_CNT_GBL_RLS_MASK,
    },
    [UACPI_REGISTER_FIELD_SLP_TYP] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_SLP_TYP_IDX,
        .mask = ACPI_PM1_CNT_SLP_TYP_MASK,
    },
    [UACPI_REGISTER_FIELD_SLP_EN] = {
        .reg = UACPI_REGISTER_PM1_CNT,
        .offset = ACPI_PM1_CNT_SLP_EN_IDX,
        .mask = ACPI_PM1_CNT_SLP_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_HWR_SLP_TYP] = {
        .reg = UACPI_REGISTER_SLP_CNT,
        .offset = ACPI_SLP_CNT_SLP_TYP_IDX,
        .mask = ACPI_SLP_CNT_SLP_TYP_MASK,
    },
    [UACPI_REGISTER_FIELD_HWR_SLP_EN] = {
        .reg = UACPI_REGISTER_SLP_CNT,
        .offset = ACPI_SLP_CNT_SLP_EN_IDX,
        .mask = ACPI_SLP_CNT_SLP_EN_MASK,
    },
    [UACPI_REGISTER_FIELD_ARB_DIS] = {
        .reg = UACPI_REGISTER_PM2_CNT,
        .offset = ACPI_PM2_CNT_ARB_DIS_IDX,
        .mask = ACPI_PM2_CNT_ARB_DIS_MASK,
    },
};

uacpi_status uacpi_initialize_registers(void)
{
    g_reg_lock = uacpi_kernel_create_spinlock();
    if (uacpi_unlikely(g_reg_lock == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

void uacpi_deinitialize_registers(void)
{
    uacpi_u8 i;
    struct register_mapping *mapping;

    if (g_reg_lock != UACPI_NULL) {
        uacpi_kernel_free_spinlock(g_reg_lock);
        g_reg_lock = UACPI_NULL;
    }

    for (i = 0; i <= UACPI_REGISTER_MAX; ++i) {
        mapping = &g_register_mappings[i];

        if (mapping->states[0] == REGISTER_MAPPING_STATE_MAPPED)
            uacpi_unmap_gas_nofree(&mapping->mappings[0]);
        if (mapping->states[1] == REGISTER_MAPPING_STATE_MAPPED)
            uacpi_unmap_gas_nofree(&mapping->mappings[1]);
    }

    uacpi_memzero(&g_register_mappings, sizeof(g_register_mappings));
}

uacpi_status uacpi_read_register_field(
    enum uacpi_register_field field_enum, uacpi_u64 *out_value
)
{
    uacpi_status ret;
    uacpi_u8 field_idx = field_enum;
    const struct register_field *field;
    const struct register_spec *reg;
    struct register_mapping *mapping;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(field_idx > UACPI_REGISTER_FIELD_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    field = &g_fields[field_idx];
    reg = &g_registers[field->reg];
    mapping = &g_register_mappings[field->reg];

    ret = ensure_register_mapped(reg, mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = do_read_register(reg, mapping, out_value);
    if (uacpi_unlikely_error(ret))
        return ret;

    *out_value = (*out_value & field->mask) >> field->offset;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_write_register_field(
    enum uacpi_register_field field_enum, uacpi_u64 in_value
)
{
    uacpi_status ret;
    uacpi_u8 field_idx = field_enum;
    const struct register_field *field;
    const struct register_spec *reg;
    struct register_mapping *mapping;

    uacpi_u64 data;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(field_idx > UACPI_REGISTER_FIELD_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    field = &g_fields[field_idx];
    reg = &g_registers[field->reg];
    mapping = &g_register_mappings[field->reg];

    ret = ensure_register_mapped(reg, mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    in_value = (in_value << field->offset) & field->mask;

    flags = uacpi_kernel_lock_spinlock(g_reg_lock);

    if (reg->kind == REGISTER_ACCESS_KIND_WRITE_TO_CLEAR) {
        if (in_value == 0) {
            ret = UACPI_STATUS_OK;
            goto out;
        }

        ret = do_write_register(reg, mapping, in_value);
        goto out;
    }

    ret = do_read_register(reg, mapping, &data);
    if (uacpi_unlikely_error(ret))
        goto out;

    data &= ~field->mask;
    data |= in_value;

    ret = do_write_register(reg, mapping, data);

out:
    uacpi_kernel_unlock_spinlock(g_reg_lock, flags);
    return ret;
}

#endif // !UACPI_BAREBONES_MODE
