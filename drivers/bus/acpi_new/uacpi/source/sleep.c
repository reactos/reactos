#include <uacpi/sleep.h>
#include <uacpi/internal/context.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/io.h>
#include <uacpi/internal/registers.h>
#include <uacpi/internal/event.h>
#include <uacpi/platform/arch_helpers.h>

#ifndef UACPI_BAREBONES_MODE

#ifndef UACPI_REDUCED_HARDWARE
#define CALL_SLEEP_FN(name, state)                       \
    (uacpi_is_hardware_reduced() ?                       \
        name##_hw_reduced(state) : name##_hw_full(state))
#else
#define CALL_SLEEP_FN(name, state) name##_hw_reduced(state);
#endif

static uacpi_status eval_wak(uacpi_u8 state);
static uacpi_status eval_sst(uacpi_u8 value);

#ifndef UACPI_REDUCED_HARDWARE
uacpi_status uacpi_set_waking_vector(
    uacpi_phys_addr addr32, uacpi_phys_addr addr64
)
{
    struct acpi_facs *facs = g_uacpi_rt_ctx.facs;

    if (facs == UACPI_NULL)
        return UACPI_STATUS_OK;

    facs->firmware_waking_vector = addr32;

    // The 64-bit wake vector doesn't exist, we're done
    if (facs->length < 32)
        return UACPI_STATUS_OK;

    // Only allow 64-bit wake vector on 1.0 and above FACS
    if (facs->version >= 1)
        facs->x_firmware_waking_vector = addr64;
    else
        facs->x_firmware_waking_vector = 0;

    return UACPI_STATUS_OK;
}

static uacpi_status enter_sleep_state_hw_full(uacpi_u8 state)
{
    uacpi_status ret;
    uacpi_u64 wake_status, pm1a, pm1b;

    ret = uacpi_write_register_field(
        UACPI_REGISTER_FIELD_WAK_STS, ACPI_PM1_STS_CLEAR
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_disable_all_gpes();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_clear_all_events();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_enable_all_wake_gpes();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_read_register(UACPI_REGISTER_PM1_CNT, &pm1a);
    if (uacpi_unlikely_error(ret))
        return ret;

    pm1a &= ~((uacpi_u64)(ACPI_PM1_CNT_SLP_TYP_MASK | ACPI_PM1_CNT_SLP_EN_MASK));
    pm1b = pm1a;

    pm1a |= g_uacpi_rt_ctx.last_sleep_typ_a << ACPI_PM1_CNT_SLP_TYP_IDX;
    pm1b |= g_uacpi_rt_ctx.last_sleep_typ_b << ACPI_PM1_CNT_SLP_TYP_IDX;

    /*
     * Just like ACPICA, split writing SLP_TYP and SLP_EN to work around
     * buggy firmware that can't handle both written at the same time.
     */
    ret = uacpi_write_registers(UACPI_REGISTER_PM1_CNT, pm1a, pm1b);
    if (uacpi_unlikely_error(ret))
        return ret;

    pm1a |= ACPI_PM1_CNT_SLP_EN_MASK;
    pm1b |= ACPI_PM1_CNT_SLP_EN_MASK;

    if (state < UACPI_SLEEP_STATE_S4)
        UACPI_ARCH_FLUSH_CPU_CACHE();

    ret = uacpi_write_registers(UACPI_REGISTER_PM1_CNT, pm1a, pm1b);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (state > UACPI_SLEEP_STATE_S3) {
        /*
         * We're still here, this is a bug or very slow firmware.
         * Just try spinning for a bit.
         */
        uacpi_u64 stalled_time = 0;

        // 10 seconds max
        while (stalled_time < (10 * 1000 * 1000)) {
            uacpi_kernel_stall(100);
            stalled_time += 100;
        }

        // Try one more time
        ret = uacpi_write_registers(UACPI_REGISTER_PM1_CNT, pm1a, pm1b);
        if (uacpi_unlikely_error(ret))
            return ret;

        // Nothing we can do here, give up
        return UACPI_STATUS_HARDWARE_TIMEOUT;
    }

    do {
        ret = uacpi_read_register_field(
            UACPI_REGISTER_FIELD_WAK_STS, &wake_status
        );
        if (uacpi_unlikely_error(ret))
            return ret;
    } while (wake_status != 1);

    return UACPI_STATUS_OK;
}

static uacpi_status prepare_for_wake_from_sleep_state_hw_full(uacpi_u8 state)
{
    uacpi_status ret;
    uacpi_u64 pm1a, pm1b;
    UACPI_UNUSED(state);

    /*
     * Some hardware apparently relies on S0 values being written to the PM1
     * control register on wake, so do this here.
     */

    if (g_uacpi_rt_ctx.s0_sleep_typ_a == UACPI_SLEEP_TYP_INVALID)
        goto out;

    ret = uacpi_read_register(UACPI_REGISTER_PM1_CNT, &pm1a);
    if (uacpi_unlikely_error(ret))
        goto out;

    pm1a &= ~((uacpi_u64)(ACPI_PM1_CNT_SLP_TYP_MASK | ACPI_PM1_CNT_SLP_EN_MASK));
    pm1b = pm1a;

    pm1a |= g_uacpi_rt_ctx.s0_sleep_typ_a << ACPI_PM1_CNT_SLP_TYP_IDX;
    pm1b |= g_uacpi_rt_ctx.s0_sleep_typ_b << ACPI_PM1_CNT_SLP_TYP_IDX;

    uacpi_write_registers(UACPI_REGISTER_PM1_CNT, pm1a, pm1b);
out:
    // Errors ignored intentionally, we don't want to abort because of this
    return UACPI_STATUS_OK;
}

static uacpi_status wake_from_sleep_state_hw_full(uacpi_u8 state)
{
    uacpi_status ret;
    g_uacpi_rt_ctx.last_sleep_typ_a = UACPI_SLEEP_TYP_INVALID;
    g_uacpi_rt_ctx.last_sleep_typ_b = UACPI_SLEEP_TYP_INVALID;

    // Set the status to 2 (waking) while we execute the wake method.
    eval_sst(2);

    ret = uacpi_disable_all_gpes();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_enable_all_runtime_gpes();
    if (uacpi_unlikely_error(ret))
        return ret;

    eval_wak(state);

    // Apparently some BIOSes expect us to clear this, so do it
    uacpi_write_register_field(
        UACPI_REGISTER_FIELD_WAK_STS, ACPI_PM1_STS_CLEAR
    );

    // Now that we're awake set the status to 1 (running)
    eval_sst(1);

    return UACPI_STATUS_OK;
}
#endif

static uacpi_status get_slp_type_for_state(
    uacpi_u8 state, uacpi_u8 *a, uacpi_u8 *b
)
{
    uacpi_char path[] = "_S0";
    uacpi_status ret;
    uacpi_object *obj0, *obj1, *ret_obj = UACPI_NULL;

    path[2] += state;

    ret = uacpi_eval_typed(
        uacpi_namespace_root(), path, UACPI_NULL,
        UACPI_OBJECT_PACKAGE_BIT, &ret_obj
    );
    if (ret != UACPI_STATUS_OK) {
        if (uacpi_unlikely(ret != UACPI_STATUS_NOT_FOUND)) {
            uacpi_warn("error while evaluating %s: %s\n", path,
                       uacpi_status_to_string(ret));
        } else {
            uacpi_trace("sleep state %d is not supported as %s was not found\n",
                        state, path);
        }
        goto out;
    }

    switch (ret_obj->package->count) {
    case 0:
        uacpi_error("empty package while evaluating %s!\n", path);
        ret = UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
        goto out;

    case 1:
        obj0 = ret_obj->package->objects[0];
        if (uacpi_unlikely(obj0->type != UACPI_OBJECT_INTEGER)) {
            uacpi_error(
                "invalid object type at pkg[0] => %s when evaluating %s\n",
                uacpi_object_type_to_string(obj0->type), path
            );
            goto out;
        }

        *a = obj0->integer;
        *b = obj0->integer >> 8;
        break;

    default:
        obj0 = ret_obj->package->objects[0];
        obj1 = ret_obj->package->objects[1];

        if (uacpi_unlikely(obj0->type != UACPI_OBJECT_INTEGER ||
                           obj1->type != UACPI_OBJECT_INTEGER)) {
            uacpi_error(
                "invalid object type when evaluating %s: "
                "pkg[0] => %s, pkg[1] => %s\n", path,
                uacpi_object_type_to_string(obj0->type),
                uacpi_object_type_to_string(obj1->type)
            );
            ret = UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
            goto out;
        }

        *a = obj0->integer;
        *b = obj1->integer;
        break;
    }

out:
    if (ret != UACPI_STATUS_OK) {
        *a = UACPI_SLEEP_TYP_INVALID;
        *b = UACPI_SLEEP_TYP_INVALID;
    }

    uacpi_object_unref(ret_obj);
    return ret;
}

static uacpi_status eval_sleep_helper(
    uacpi_namespace_node *parent, const uacpi_char *path, uacpi_u8 value
)
{
    uacpi_object *arg;
    uacpi_object_array args;
    uacpi_status ret;

    arg = uacpi_create_object(UACPI_OBJECT_INTEGER);
    if (uacpi_unlikely(arg == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    arg->integer = value;
    args.objects = &arg;
    args.count = 1;

    ret = uacpi_eval(parent, path, &args, UACPI_NULL);
    switch (ret) {
    case UACPI_STATUS_OK:
        break;
    case UACPI_STATUS_NOT_FOUND:
        ret = UACPI_STATUS_OK;
        break;
    default:
        uacpi_error("error while evaluating %s: %s\n",
                    path, uacpi_status_to_string(ret));
        break;
    }

    uacpi_object_unref(arg);
    return ret;
}

static uacpi_status eval_pts(uacpi_u8 state)
{
    return eval_sleep_helper(uacpi_namespace_root(), "_PTS", state);
}

static uacpi_status eval_wak(uacpi_u8 state)
{
    return eval_sleep_helper(uacpi_namespace_root(), "_WAK", state);
}

static uacpi_status eval_sst(uacpi_u8 value)
{
    return eval_sleep_helper(
        uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SI),
        "_SST", value
    );
}

static uacpi_status eval_sst_for_state(enum uacpi_sleep_state state)
{
    uacpi_u8 arg;

    /*
     * This optional object is a control method that OSPM invokes to set the
     * system status indicator as desired.
     * Arguments:(1)
     * Arg0 - An Integer containing the system status indicator identifier:
     *     0 - No system state indication. Indicator off
     *     1 - Working
     *     2 - Waking
     *     3 - Sleeping. Used to indicate system state S1, S2, or S3
     *     4 - Sleeping with context saved to non-volatile storage
     */
    switch (state) {
    case UACPI_SLEEP_STATE_S0:
        arg = 1;
        break;
    case UACPI_SLEEP_STATE_S1:
    case UACPI_SLEEP_STATE_S2:
    case UACPI_SLEEP_STATE_S3:
        arg = 3;
        break;
    case UACPI_SLEEP_STATE_S4:
        arg = 4;
        break;
    case UACPI_SLEEP_STATE_S5:
        arg = 0;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return eval_sst(arg);
}

uacpi_status uacpi_prepare_for_sleep_state(enum uacpi_sleep_state state_enum)
{
    uacpi_u8 state = state_enum;
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED);

    if (uacpi_unlikely(state > UACPI_SLEEP_STATE_S5))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = get_slp_type_for_state(
        state,
        &g_uacpi_rt_ctx.last_sleep_typ_a,
        &g_uacpi_rt_ctx.last_sleep_typ_b
    );
    if (ret != UACPI_STATUS_OK)
        return ret;

    ret = get_slp_type_for_state(
        0,
        &g_uacpi_rt_ctx.s0_sleep_typ_a,
        &g_uacpi_rt_ctx.s0_sleep_typ_b
    );

    ret = eval_pts(state);
    if (uacpi_unlikely_error(ret))
        return ret;

    eval_sst_for_state(state);
    return UACPI_STATUS_OK;
}

static uacpi_u8 make_hw_reduced_sleep_control(uacpi_u8 slp_typ)
{
    uacpi_u8 value;

    value = (slp_typ << ACPI_SLP_CNT_SLP_TYP_IDX);
    value &= ACPI_SLP_CNT_SLP_TYP_MASK;
    value |= ACPI_SLP_CNT_SLP_EN_MASK;

    return value;
}

static uacpi_status enter_sleep_state_hw_reduced(uacpi_u8 state)
{
    uacpi_status ret;
    uacpi_u8 sleep_control;
    uacpi_u64 wake_status;
    struct acpi_fadt *fadt = &g_uacpi_rt_ctx.fadt;

    if (!fadt->sleep_control_reg.address || !fadt->sleep_status_reg.address)
        return UACPI_STATUS_NOT_FOUND;

    ret = uacpi_write_register_field(
        UACPI_REGISTER_FIELD_HWR_WAK_STS,
        ACPI_SLP_STS_CLEAR
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    sleep_control = make_hw_reduced_sleep_control(
        g_uacpi_rt_ctx.last_sleep_typ_a
    );

    if (state < UACPI_SLEEP_STATE_S4)
        UACPI_ARCH_FLUSH_CPU_CACHE();

    /*
     * To put the system into a sleep state, software will write the HW-reduced
     * Sleep Type value (obtained from the \_Sx object in the DSDT) and the
     * SLP_EN bit to the sleep control register.
     */
    ret = uacpi_write_register(UACPI_REGISTER_SLP_CNT, sleep_control);
    if (uacpi_unlikely_error(ret))
        return ret;

    /*
     * The OSPM then polls the WAK_STS bit of the SLEEP_STATUS_REG waiting for
     * it to be one (1), indicating that the system has been transitioned
     * back to the Working state.
     */
    do {
        ret = uacpi_read_register_field(
            UACPI_REGISTER_FIELD_HWR_WAK_STS, &wake_status
        );
        if (uacpi_unlikely_error(ret))
            return ret;
    } while (wake_status != 1);

    return UACPI_STATUS_OK;
}

static uacpi_status prepare_for_wake_from_sleep_state_hw_reduced(uacpi_u8 state)
{
    uacpi_u8 sleep_control;
    UACPI_UNUSED(state);

    if (g_uacpi_rt_ctx.s0_sleep_typ_a == UACPI_SLEEP_TYP_INVALID)
        goto out;

    sleep_control = make_hw_reduced_sleep_control(
        g_uacpi_rt_ctx.s0_sleep_typ_a
    );
    uacpi_write_register(UACPI_REGISTER_SLP_CNT, sleep_control);

out:
    return UACPI_STATUS_OK;
}

static uacpi_status wake_from_sleep_state_hw_reduced(uacpi_u8 state)
{
    g_uacpi_rt_ctx.last_sleep_typ_a = UACPI_SLEEP_TYP_INVALID;
    g_uacpi_rt_ctx.last_sleep_typ_b = UACPI_SLEEP_TYP_INVALID;

    // Set the status to 2 (waking) while we execute the wake method.
    eval_sst(2);

    eval_wak(state);

    // Apparently some BIOSes expect us to clear this, so do it
    uacpi_write_register_field(
        UACPI_REGISTER_FIELD_HWR_WAK_STS, ACPI_SLP_STS_CLEAR
    );

    // Now that we're awake set the status to 1 (running)
    eval_sst(1);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_enter_sleep_state(enum uacpi_sleep_state state_enum)
{
    uacpi_u8 state = state_enum;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED);

    if (uacpi_unlikely(state > UACPI_SLEEP_STATE_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (uacpi_unlikely(g_uacpi_rt_ctx.last_sleep_typ_a > ACPI_SLP_TYP_MAX ||
                       g_uacpi_rt_ctx.last_sleep_typ_b > ACPI_SLP_TYP_MAX)) {
        uacpi_error("invalid SLP_TYP values: 0x%02X:0x%02X\n",
                    g_uacpi_rt_ctx.last_sleep_typ_a,
                    g_uacpi_rt_ctx.last_sleep_typ_b);
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    return CALL_SLEEP_FN(enter_sleep_state, state);
}

uacpi_status uacpi_prepare_for_wake_from_sleep_state(
    uacpi_sleep_state state_enum
)
{
    uacpi_u8 state = state_enum;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED);

    if (uacpi_unlikely(state > UACPI_SLEEP_STATE_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return CALL_SLEEP_FN(prepare_for_wake_from_sleep_state, state);
}

uacpi_status uacpi_wake_from_sleep_state(
    uacpi_sleep_state state_enum
)
{
    uacpi_u8 state = state_enum;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED);

    if (uacpi_unlikely(state > UACPI_SLEEP_STATE_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return CALL_SLEEP_FN(wake_from_sleep_state, state);
}

uacpi_status uacpi_reboot(void)
{
    uacpi_status ret;
    uacpi_handle pci_dev = UACPI_NULL, io_handle = UACPI_NULL;
    struct acpi_fadt *fadt = &g_uacpi_rt_ctx.fadt;
    struct acpi_gas *reset_reg = &fadt->reset_reg;

    /*
     * Allow restarting earlier than namespace load so that the kernel can
     * use this in case of some initialization error.
     */
    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (!(fadt->flags & ACPI_RESET_REG_SUP) || !reset_reg->address)
        return UACPI_STATUS_NOT_FOUND;

    switch (reset_reg->address_space_id) {
    case UACPI_ADDRESS_SPACE_SYSTEM_IO:
        /*
         * For SystemIO we don't do any checking, and we ignore bit width
         * because that's what NT does.
         */
        ret = uacpi_kernel_io_map(reset_reg->address, 1, &io_handle);
        if (uacpi_unlikely_error(ret))
            return ret;

        ret = uacpi_kernel_io_write8(io_handle, 0, fadt->reset_value);
        break;
    case UACPI_ADDRESS_SPACE_SYSTEM_MEMORY:
        ret = uacpi_write_register(UACPI_REGISTER_RESET, fadt->reset_value);
        break;
    case UACPI_ADDRESS_SPACE_PCI_CONFIG: {
        // Bus is assumed to be 0 here
        uacpi_pci_address address = {
            .segment = 0,
            .bus = 0,
            .device = (reset_reg->address >> 32) & 0xFF,
            .function = (reset_reg->address >> 16) & 0xFF,
        };

        ret = uacpi_kernel_pci_device_open(address, &pci_dev);
        if (uacpi_unlikely_error(ret))
            break;

        ret = uacpi_kernel_pci_write8(
            pci_dev, reset_reg->address & 0xFFFF, fadt->reset_value
        );
        break;
    }
    default:
        uacpi_warn(
            "unable to perform a reset: unsupported address space '%s' (%d)\n",
            uacpi_address_space_to_string(reset_reg->address_space_id),
            reset_reg->address_space_id
        );
        ret = UACPI_STATUS_UNIMPLEMENTED;
    }

    if (ret == UACPI_STATUS_OK) {
        /*
         * This should've worked but we're still here.
         * Spin for a bit then give up.
         */
        uacpi_u64 stalled_time = 0;

        while (stalled_time < (1000 * 1000)) {
            uacpi_kernel_stall(100);
            stalled_time += 100;
        }

        uacpi_error("reset timeout\n");
        ret = UACPI_STATUS_HARDWARE_TIMEOUT;
    }

    if (pci_dev != UACPI_NULL)
        uacpi_kernel_pci_device_close(pci_dev);
    if (io_handle != UACPI_NULL)
        uacpi_kernel_io_unmap(io_handle);

    return ret;
}

#endif // !UACPI_BAREBONES_MODE
