#include <uacpi/internal/event.h>
#include <uacpi/internal/registers.h>
#include <uacpi/internal/context.h>
#include <uacpi/internal/io.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/interpreter.h>
#include <uacpi/internal/notify.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/acpi.h>

#define UACPI_EVENT_DISABLED 0
#define UACPI_EVENT_ENABLED 1

#if !defined(UACPI_REDUCED_HARDWARE) && !defined(UACPI_BAREBONES_MODE)

static uacpi_handle g_gpe_state_slock;
static struct uacpi_recursive_lock g_event_lock;
static uacpi_bool g_gpes_finalized;

struct fixed_event {
    uacpi_u8 enable_field;
    uacpi_u8 status_field;
    uacpi_u16 enable_mask;
    uacpi_u16 status_mask;
};

struct fixed_event_handler {
    uacpi_interrupt_handler handler;
    uacpi_handle ctx;
};

static const struct fixed_event fixed_events[UACPI_FIXED_EVENT_MAX + 1] = {
    [UACPI_FIXED_EVENT_GLOBAL_LOCK] = {
        .status_field = UACPI_REGISTER_FIELD_GBL_STS,
        .enable_field = UACPI_REGISTER_FIELD_GBL_EN,
        .enable_mask = ACPI_PM1_EN_GBL_EN_MASK,
        .status_mask = ACPI_PM1_STS_GBL_STS_MASK,
    },
    [UACPI_FIXED_EVENT_TIMER_STATUS] = {
        .status_field = UACPI_REGISTER_FIELD_TMR_STS,
        .enable_field = UACPI_REGISTER_FIELD_TMR_EN,
        .enable_mask = ACPI_PM1_EN_TMR_EN_MASK,
        .status_mask = ACPI_PM1_STS_TMR_STS_MASK,
    },
    [UACPI_FIXED_EVENT_POWER_BUTTON] = {
        .status_field = UACPI_REGISTER_FIELD_PWRBTN_STS,
        .enable_field = UACPI_REGISTER_FIELD_PWRBTN_EN,
        .enable_mask = ACPI_PM1_EN_PWRBTN_EN_MASK,
        .status_mask = ACPI_PM1_STS_PWRBTN_STS_MASK,
    },
    [UACPI_FIXED_EVENT_SLEEP_BUTTON] = {
        .status_field = UACPI_REGISTER_FIELD_SLPBTN_STS,
        .enable_field = UACPI_REGISTER_FIELD_SLPBTN_EN,
        .enable_mask = ACPI_PM1_EN_SLPBTN_EN_MASK,
        .status_mask = ACPI_PM1_STS_SLPBTN_STS_MASK,
    },
    [UACPI_FIXED_EVENT_RTC] = {
        .status_field = UACPI_REGISTER_FIELD_RTC_STS,
        .enable_field = UACPI_REGISTER_FIELD_RTC_EN,
        .enable_mask = ACPI_PM1_EN_RTC_EN_MASK,
        .status_mask = ACPI_PM1_STS_RTC_STS_MASK,
    },
};

static struct fixed_event_handler
fixed_event_handlers[UACPI_FIXED_EVENT_MAX + 1];

static uacpi_status initialize_fixed_events(void)
{
    uacpi_size i;

    for (i = 0; i < UACPI_FIXED_EVENT_MAX; ++i) {
        uacpi_write_register_field(
            fixed_events[i].enable_field, UACPI_EVENT_DISABLED
        );
    }

    return UACPI_STATUS_OK;
}

static uacpi_status set_event(uacpi_u8 event, uacpi_u8 value)
{
    uacpi_status ret;
    uacpi_u64 raw_value;
    const struct fixed_event *ev = &fixed_events[event];

    ret = uacpi_write_register_field(ev->enable_field, value);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_read_register_field(ev->enable_field, &raw_value);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (raw_value != value) {
        uacpi_error("failed to %sable fixed event %d\n",
                    value ? "en" : "dis", event);
        return UACPI_STATUS_HARDWARE_TIMEOUT;
    }

    uacpi_trace("fixed event %d %sabled successfully\n",
                event, value ? "en" : "dis");
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_enable_fixed_event(uacpi_fixed_event event)
{
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    /*
     * Attempting to enable an event that doesn't have a handler is most likely
     * an error, don't allow it.
     */
    if (uacpi_unlikely(fixed_event_handlers[event].handler == UACPI_NULL)) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    ret = set_event(event, UACPI_EVENT_ENABLED);

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_disable_fixed_event(uacpi_fixed_event event)
{
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = set_event(event, UACPI_EVENT_DISABLED);

    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_clear_fixed_event(uacpi_fixed_event event)
{
    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    return uacpi_write_register_field(
        fixed_events[event].status_field, ACPI_PM1_STS_CLEAR
    );
}

static uacpi_interrupt_ret dispatch_fixed_event(
    const struct fixed_event *ev, uacpi_fixed_event event
)
{
    uacpi_status ret;
    struct fixed_event_handler *evh = &fixed_event_handlers[event];

    ret = uacpi_write_register_field(ev->status_field, ACPI_PM1_STS_CLEAR);
    if (uacpi_unlikely_error(ret))
        return UACPI_INTERRUPT_NOT_HANDLED;

    if (uacpi_unlikely(evh->handler == UACPI_NULL)) {
        uacpi_warn(
            "fixed event %d fired but no handler installed, disabling...\n",
            event
        );
        uacpi_write_register_field(ev->enable_field, UACPI_EVENT_DISABLED);
        return UACPI_INTERRUPT_NOT_HANDLED;
    }

    return evh->handler(evh->ctx);
}

static uacpi_interrupt_ret handle_fixed_events(void)
{
    uacpi_interrupt_ret int_ret = UACPI_INTERRUPT_NOT_HANDLED;
    uacpi_status ret;
    uacpi_u64 enable_mask, status_mask;
    uacpi_size i;

    ret = uacpi_read_register(UACPI_REGISTER_PM1_STS, &status_mask);
    if (uacpi_unlikely_error(ret))
        return int_ret;

    ret = uacpi_read_register(UACPI_REGISTER_PM1_EN, &enable_mask);
    if (uacpi_unlikely_error(ret))
        return int_ret;

    for (i = 0; i < UACPI_FIXED_EVENT_MAX; ++i)
    {
        const struct fixed_event *ev = &fixed_events[i];

        if (!(status_mask & ev->status_mask) ||
            !(enable_mask & ev->enable_mask))
            continue;

        int_ret |= dispatch_fixed_event(ev, i);
    }

    return int_ret;
}

struct gpe_native_handler {
    uacpi_gpe_handler cb;
    uacpi_handle ctx;

    /*
     * Preserved values to be used for state restoration if this handler is
     * removed at any point.
     */
    uacpi_handle previous_handler;
    uacpi_u8 previous_triggering : 1;
    uacpi_u8 previous_handler_type : 3;
    uacpi_u8 previously_enabled : 1;
};

struct gpe_implicit_notify_handler {
    struct gpe_implicit_notify_handler *next;
    uacpi_namespace_node *device;
};

#define EVENTS_PER_GPE_REGISTER 8

/*
 * NOTE:
 * This API and handler types are inspired by ACPICA, let's not reinvent the
 * wheel and follow a similar path that people ended up finding useful after
 * years of dealing with ACPI. Obviously credit goes to them for inventing
 * "implicit notify" and other neat API.
 */
enum gpe_handler_type {
    GPE_HANDLER_TYPE_NONE = 0,
    GPE_HANDLER_TYPE_AML_HANDLER = 1,
    GPE_HANDLER_TYPE_NATIVE_HANDLER = 2,
    GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW = 3,
    GPE_HANDLER_TYPE_IMPLICIT_NOTIFY = 4,
};

struct gp_event {
    union {
        struct gpe_native_handler *native_handler;
        struct gpe_implicit_notify_handler *implicit_handler;
        uacpi_namespace_node *aml_handler;
        uacpi_handle *any_handler;
    };

    struct gpe_register *reg;
    uacpi_u16 idx;

    // "reference count" of the number of times this event has been enabled
    uacpi_u8 num_users;

    uacpi_u8 handler_type : 3;
    uacpi_u8 triggering : 1;
    uacpi_u8 wake : 1;
    uacpi_u8 block_interrupts : 1;
};

struct gpe_register {
    uacpi_mapped_gas status;
    uacpi_mapped_gas enable;

    uacpi_u8 runtime_mask;
    uacpi_u8 wake_mask;
    uacpi_u8 masked_mask;
    uacpi_u8 current_mask;

    uacpi_u16 base_idx;
};

struct gpe_block {
    struct gpe_block *prev, *next;

    /*
     * Technically this can only refer to \_GPE, but there's also apparently a
     * "GPE Block Device" with id "ACPI0006", which is not used by anyone. We
     * still keep it as a possibility that someone might eventually use it, so
     * it is supported here.
     */
    uacpi_namespace_node *device_node;

    struct gpe_register *registers;
    struct gp_event *events;
    struct gpe_interrupt_ctx *irq_ctx;

    uacpi_u16 num_registers;
    uacpi_u16 num_events;
    uacpi_u16 base_idx;
};

struct gpe_interrupt_ctx {
    struct gpe_interrupt_ctx *prev, *next;

    struct gpe_block *gpe_head;
    uacpi_handle irq_handle;
    uacpi_u32 irq;
};
static struct gpe_interrupt_ctx *g_gpe_interrupt_head;

static uacpi_u8 gpe_get_mask(struct gp_event *event)
{
    return 1 << (event->idx - event->reg->base_idx);
}

enum gpe_state {
    GPE_STATE_ENABLED,
    GPE_STATE_ENABLED_CONDITIONALLY,
    GPE_STATE_DISABLED,
};

static uacpi_status set_gpe_state(struct gp_event *event, enum gpe_state state)
{
    uacpi_status ret;
    struct gpe_register *reg = event->reg;
    uacpi_u64 enable_mask;
    uacpi_u8 event_bit;
    uacpi_cpu_flags flags;

    event_bit = gpe_get_mask(event);
    if (state != GPE_STATE_DISABLED && (reg->masked_mask & event_bit))
        return UACPI_STATUS_OK;

    if (state == GPE_STATE_ENABLED_CONDITIONALLY) {
        if (!(reg->current_mask & event_bit))
            return UACPI_STATUS_OK;

        state = GPE_STATE_ENABLED;
    }

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    ret = uacpi_gas_read_mapped(&reg->enable, &enable_mask);
    if (uacpi_unlikely_error(ret))
        goto out;

    switch (state) {
    case GPE_STATE_ENABLED:
        enable_mask |= event_bit;
        break;
    case GPE_STATE_DISABLED:
        enable_mask &= ~event_bit;
        break;
    default:
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    ret = uacpi_gas_write_mapped(&reg->enable, enable_mask);
out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
    return ret;
}

static uacpi_status clear_gpe(struct gp_event *event)
{
    struct gpe_register *reg = event->reg;

    return uacpi_gas_write_mapped(&reg->status, gpe_get_mask(event));
}

static uacpi_status restore_gpe(struct gp_event *event)
{
    uacpi_status ret;

    if (event->triggering == UACPI_GPE_TRIGGERING_LEVEL) {
        ret = clear_gpe(event);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    ret = set_gpe_state(event, GPE_STATE_ENABLED_CONDITIONALLY);
    event->block_interrupts = UACPI_FALSE;

    return ret;
}

static void async_restore_gpe(uacpi_handle opaque)
{
    uacpi_status ret;
    struct gp_event *event = opaque;

    ret = restore_gpe(event);
    if (uacpi_unlikely_error(ret)) {
        uacpi_error("unable to restore GPE(%02X): %s\n",
                    event->idx, uacpi_status_to_string(ret));
    }
}

static void async_run_gpe_handler(uacpi_handle opaque)
{
    uacpi_status ret;
    struct gp_event *event = opaque;

    ret = uacpi_namespace_write_lock();
    if (uacpi_unlikely_error(ret))
        goto out_no_unlock;

    switch (event->handler_type) {
    case GPE_HANDLER_TYPE_AML_HANDLER: {
        uacpi_object *method_obj;

        method_obj = uacpi_namespace_node_get_object_typed(
            event->aml_handler, UACPI_OBJECT_METHOD_BIT
        );
        if (uacpi_unlikely(method_obj == UACPI_NULL)) {
            uacpi_error("GPE(%02X) AML handler gone\n", event->idx);
            break;
        }

        uacpi_trace(
            "executing GPE(%02X) handler %.4s\n",
            event->idx, uacpi_namespace_node_name(event->aml_handler).text
        );

        ret = uacpi_execute_control_method(
            event->aml_handler, method_obj->method, UACPI_NULL, UACPI_NULL
        );
        if (uacpi_unlikely_error(ret)) {
            uacpi_error(
                "error while executing GPE(%02X) handler %.4s: %s\n",
                event->idx, event->aml_handler->name.text,
                uacpi_status_to_string(ret)
            );
        }
        break;
    }

    case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY: {
        struct gpe_implicit_notify_handler *handler;

        handler = event->implicit_handler;
        while (handler) {
            /*
             * 2 - Device Wake. Used to notify OSPM that the device has signaled
             * its wake event, and that OSPM needs to notify OSPM native device
             * driver for the device.
             */
            uacpi_notify_all(handler->device, 2);
            handler = handler->next;
        }
        break;
    }

    default:
        break;
    }

    uacpi_namespace_write_unlock();

out_no_unlock:
    /*
     * We schedule the work as NOTIFICATION to make sure all other notifications
     * finish before this GPE is re-enabled.
     */
    ret = uacpi_kernel_schedule_work(
        UACPI_WORK_NOTIFICATION, async_restore_gpe, event
    );
    if (uacpi_unlikely_error(ret)) {
        uacpi_error("unable to schedule GPE(%02X) restore: %s\n",
                    event->idx, uacpi_status_to_string(ret));
        async_restore_gpe(event);
    }
}

static uacpi_interrupt_ret dispatch_gpe(
    uacpi_namespace_node *device_node, struct gp_event *event
)
{
    uacpi_status ret;
    uacpi_interrupt_ret int_ret = UACPI_INTERRUPT_NOT_HANDLED;

    /*
     * For raw handlers we don't do any management whatsoever, we just let the
     * handler know a GPE has triggered and let it handle disable/enable as
     * well as clearing.
     */
    if (event->handler_type == GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW) {
        return event->native_handler->cb(
            event->native_handler->ctx, device_node, event->idx
        );
    }

    ret = set_gpe_state(event, GPE_STATE_DISABLED);
    if (uacpi_unlikely_error(ret)) {
        uacpi_error("failed to disable GPE(%02X): %s\n",
                    event->idx, uacpi_status_to_string(ret));
        return int_ret;
    }

    event->block_interrupts = UACPI_TRUE;

    if (event->triggering == UACPI_GPE_TRIGGERING_EDGE) {
        ret = clear_gpe(event);
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("unable to clear GPE(%02X): %s\n",
                        event->idx, uacpi_status_to_string(ret));
            set_gpe_state(event, GPE_STATE_ENABLED_CONDITIONALLY);
            return int_ret;
        }
    }

    switch (event->handler_type) {
    case GPE_HANDLER_TYPE_NATIVE_HANDLER:
        int_ret = event->native_handler->cb(
            event->native_handler->ctx, device_node, event->idx
        );
        if (!(int_ret & UACPI_GPE_REENABLE))
            break;

        ret = restore_gpe(event);
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("unable to restore GPE(%02X): %s\n",
                        event->idx, uacpi_status_to_string(ret));
        }
        break;

    case GPE_HANDLER_TYPE_AML_HANDLER:
    case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY:
        ret = uacpi_kernel_schedule_work(
            UACPI_WORK_GPE_EXECUTION, async_run_gpe_handler, event
        );
        if (uacpi_unlikely_error(ret)) {
            uacpi_warn(
                "unable to schedule GPE(%02X) for execution: %s\n",
                event->idx, uacpi_status_to_string(ret)
            );
        }
        break;

    default:
        uacpi_warn("GPE(%02X) fired but no handler, keeping disabled\n",
                   event->idx);
        break;
    }

    return UACPI_INTERRUPT_HANDLED;
}

static uacpi_interrupt_ret detect_gpes(struct gpe_block *block)
{
    uacpi_status ret;
    uacpi_interrupt_ret int_ret = UACPI_INTERRUPT_NOT_HANDLED;
    struct gpe_register *reg;
    struct gp_event *event;
    uacpi_u64 status, enable;
    uacpi_size i, j;

    while (block) {
        for (i = 0; i < block->num_registers; ++i) {
            reg = &block->registers[i];

            if (!reg->runtime_mask && !reg->wake_mask)
                continue;

            ret = uacpi_gas_read_mapped(&reg->status, &status);
            if (uacpi_unlikely_error(ret))
                return int_ret;

            ret = uacpi_gas_read_mapped(&reg->enable, &enable);
            if (uacpi_unlikely_error(ret))
                return int_ret;

            if (status == 0)
                continue;

            for (j = 0; j < EVENTS_PER_GPE_REGISTER; ++j) {
                if (!((status & enable) & (1ull << j)))
                    continue;

                event = &block->events[j + i * EVENTS_PER_GPE_REGISTER];
                int_ret |= dispatch_gpe(block->device_node, event);
            }
        }

        block = block->next;
    }

    return int_ret;
}

static uacpi_status maybe_dispatch_gpe(
    uacpi_namespace_node *gpe_device, struct gp_event *event
)
{
    uacpi_status ret;
    struct gpe_register *reg = event->reg;
    uacpi_u64 status;

    ret = uacpi_gas_read_mapped(&reg->status, &status);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (!(status & gpe_get_mask(event)))
        return ret;

    dispatch_gpe(gpe_device, event);
    return ret;
}

static uacpi_interrupt_ret handle_gpes(uacpi_handle opaque)
{
    struct gpe_interrupt_ctx *ctx = opaque;

    if (uacpi_unlikely(ctx == UACPI_NULL))
        return UACPI_INTERRUPT_NOT_HANDLED;

    return detect_gpes(ctx->gpe_head);
}

static uacpi_status find_or_create_gpe_interrupt_ctx(
    uacpi_u32 irq, struct gpe_interrupt_ctx **out_ctx
)
{
    uacpi_status ret;
    struct gpe_interrupt_ctx *entry = g_gpe_interrupt_head;

    while (entry) {
        if (entry->irq == irq) {
            *out_ctx = entry;
            return UACPI_STATUS_OK;
        }

        entry = entry->next;
    }

    entry = uacpi_kernel_alloc_zeroed(sizeof(*entry));
    if (uacpi_unlikely(entry == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    /*
     * SCI interrupt is installed by other code and is responsible for more
     * things than just the GPE handling. Don't install it here.
     */
    if (irq != g_uacpi_rt_ctx.fadt.sci_int) {
        ret = uacpi_kernel_install_interrupt_handler(
            irq, handle_gpes, entry, &entry->irq_handle
        );
        if (uacpi_unlikely_error(ret)) {
            uacpi_free(entry, sizeof(*entry));
            return ret;
        }
    }

    entry->irq = irq;
    entry->next = g_gpe_interrupt_head;
    g_gpe_interrupt_head = entry;

    *out_ctx = entry;
    return UACPI_STATUS_OK;
}

static void gpe_release_implicit_notify_handlers(struct gp_event *event)
{
    struct gpe_implicit_notify_handler *handler, *next_handler;

    handler = event->implicit_handler;
    while (handler) {
        next_handler = handler->next;
        uacpi_free(handler, sizeof(*handler));
        handler = next_handler;
    }

    event->implicit_handler = UACPI_NULL;
}

enum gpe_block_action
{
    GPE_BLOCK_ACTION_DISABLE_ALL,
    GPE_BLOCK_ACTION_ENABLE_ALL_FOR_RUNTIME,
    GPE_BLOCK_ACTION_ENABLE_ALL_FOR_WAKE,
    GPE_BLOCK_ACTION_CLEAR_ALL,
};

static uacpi_status gpe_block_apply_action(
    struct gpe_block *block, enum gpe_block_action action
)
{
    uacpi_status ret;
    uacpi_size i;
    uacpi_u8 value;
    struct gpe_register *reg;

    for (i = 0; i < block->num_registers; ++i) {
        reg = &block->registers[i];

        switch (action) {
        case GPE_BLOCK_ACTION_DISABLE_ALL:
            value = 0;
            break;
        case GPE_BLOCK_ACTION_ENABLE_ALL_FOR_RUNTIME:
            value = reg->runtime_mask & ~reg->masked_mask;
            break;
        case GPE_BLOCK_ACTION_ENABLE_ALL_FOR_WAKE:
            value = reg->wake_mask;
            break;
        case GPE_BLOCK_ACTION_CLEAR_ALL:
            ret = uacpi_gas_write_mapped(&reg->status, 0xFF);
            if (uacpi_unlikely_error(ret))
                return ret;
            continue;
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        reg->current_mask = value;
        ret = uacpi_gas_write_mapped(&reg->enable, value);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    return UACPI_STATUS_OK;
}

static void gpe_block_mask_safe(struct gpe_block *block)
{
    uacpi_size i;
    struct gpe_register *reg;

    for (i = 0; i < block->num_registers; ++i) {
        reg = &block->registers[i];

        // No need to flush or do anything if it's not currently enabled
        if (!reg->current_mask)
            continue;

        // 1. Mask the GPEs, this makes sure their state is no longer modifyable
        reg->masked_mask = 0xFF;

        /*
         * 2. Wait for in-flight work & IRQs to finish, these might already
         *    be past the respective "if (masked)" check and therefore may
         *    try to re-enable a masked GPE.
         */
        uacpi_kernel_wait_for_work_completion();

        /*
         * 3. Now that this GPE's state is unmodifyable and we know that
         *    currently in-flight IRQs will see the masked state, we can
         *    safely disable all events knowing they won't be re-enabled by
         *    a racing IRQ.
         */
        uacpi_gas_write_mapped(&reg->enable, 0x00);

        /*
         * 4. Wait for the last possible IRQ to finish, now that this event is
         *    disabled.
         */
        uacpi_kernel_wait_for_work_completion();
    }
}

static void uninstall_gpe_block(struct gpe_block *block)
{
    if (block->registers != UACPI_NULL) {
        struct gpe_register *reg;
        uacpi_size i;

        gpe_block_mask_safe(block);

        for (i = 0; i < block->num_registers; ++i) {
            reg = &block->registers[i];

            if (reg->enable.total_bit_width)
                uacpi_unmap_gas_nofree(&reg->enable);
            if (reg->status.total_bit_width)
                uacpi_unmap_gas_nofree(&reg->status);
        }
    }

    if (block->prev)
        block->prev->next = block->next;

    if (block->irq_ctx) {
        struct gpe_interrupt_ctx *ctx = block->irq_ctx;

        // Are we the first GPE block?
        if (block == ctx->gpe_head) {
            ctx->gpe_head = ctx->gpe_head->next;
        } else {
            struct gpe_block *prev_block = ctx->gpe_head;

            // We're not, do a search
            while (prev_block) {
                if (prev_block->next == block) {
                    prev_block->next = block->next;
                    break;
                }

                prev_block = prev_block->next;
            }
        }

        // This GPE block was the last user of this interrupt context, remove it
        if (ctx->gpe_head == UACPI_NULL) {
            if (ctx->prev)
                ctx->prev->next = ctx->next;

            if (ctx->irq != g_uacpi_rt_ctx.fadt.sci_int) {
                uacpi_kernel_uninstall_interrupt_handler(
                    handle_gpes, ctx->irq_handle
                );
            }

            uacpi_free(block->irq_ctx, sizeof(*block->irq_ctx));
        }
    }

    if (block->events != UACPI_NULL) {
        uacpi_size i;
        struct gp_event *event;

        for (i = 0; i < block->num_events; ++i) {
            event = &block->events[i];

            switch (event->handler_type) {
            case GPE_HANDLER_TYPE_NONE:
            case GPE_HANDLER_TYPE_AML_HANDLER:
                break;

            case GPE_HANDLER_TYPE_NATIVE_HANDLER:
            case GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW:
                uacpi_free(event->native_handler,
                           sizeof(*event->native_handler));
                break;

            case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY: {
                gpe_release_implicit_notify_handlers(event);
                break;
            }

            default:
                break;
            }
        }

    }

    uacpi_free(block->registers,
               sizeof(*block->registers) * block->num_registers);
    uacpi_free(block->events,
               sizeof(*block->events) * block->num_events);
    uacpi_free(block, sizeof(*block));
}

static struct gp_event *gpe_from_block(struct gpe_block *block, uacpi_u16 idx)
{
    uacpi_u16 offset;

    if (idx < block->base_idx)
        return UACPI_NULL;

    offset = idx - block->base_idx;
    if (offset > block->num_events)
        return UACPI_NULL;

    return &block->events[offset];
}

struct gpe_match_ctx {
    struct gpe_block *block;
    uacpi_u32 matched_count;
    uacpi_bool post_dynamic_table_load;
};

static uacpi_iteration_decision do_match_gpe_methods(
    uacpi_handle opaque, uacpi_namespace_node *node, uacpi_u32 depth
)
{
    uacpi_status ret;
    struct gpe_match_ctx *ctx = opaque;
    struct gp_event *event;
    uacpi_u8 triggering;
    uacpi_u64 idx;

    UACPI_UNUSED(depth);

    if (node->name.text[0] != '_')
        return UACPI_ITERATION_DECISION_CONTINUE;

    switch (node->name.text[1]) {
    case 'L':
        triggering = UACPI_GPE_TRIGGERING_LEVEL;
        break;
    case 'E':
        triggering = UACPI_GPE_TRIGGERING_EDGE;
        break;
    default:
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    ret = uacpi_string_to_integer(&node->name.text[2], 2, UACPI_BASE_HEX, &idx);
    if (uacpi_unlikely_error(ret)) {
        uacpi_trace("invalid GPE method name %.4s, ignored\n", node->name.text);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    event = gpe_from_block(ctx->block, idx);
    if (event == UACPI_NULL)
        return UACPI_ITERATION_DECISION_CONTINUE;

    switch (event->handler_type) {
    /*
     * This had implicit notify configured but this is no longer needed as we
     * now have an actual AML handler. Free the implicit notify list and switch
     * this handler to AML mode.
     */
    case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY:
       gpe_release_implicit_notify_handlers(event);
       UACPI_FALLTHROUGH;
    case GPE_HANDLER_TYPE_NONE:
        event->aml_handler = node;
        event->handler_type = GPE_HANDLER_TYPE_AML_HANDLER;
        break;

    case GPE_HANDLER_TYPE_AML_HANDLER:
        // This is okay, since we're re-running the detection code
        if (!ctx->post_dynamic_table_load) {
            uacpi_warn(
                "GPE(%02X) already matched %.4s, skipping %.4s\n",
                (uacpi_u32)idx, event->aml_handler->name.text, node->name.text
            );
        }
        return UACPI_ITERATION_DECISION_CONTINUE;

    case GPE_HANDLER_TYPE_NATIVE_HANDLER:
    case GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW:
        uacpi_trace(
            "not assigning GPE(%02X) to %.4s, override "
            "installed by user\n", (uacpi_u32)idx, node->name.text
        );
        UACPI_FALLTHROUGH;
    default:
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    uacpi_trace("assigned GPE(%02X) -> %.4s\n",
                (uacpi_u32)idx, node->name.text);
    event->triggering = triggering;
    ctx->matched_count++;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

void uacpi_events_match_post_dynamic_table_load(void)
{
    struct gpe_match_ctx match_ctx = {
        .post_dynamic_table_load = UACPI_TRUE,
    };

    uacpi_namespace_write_unlock();

    if (uacpi_unlikely_error(uacpi_recursive_lock_acquire(&g_event_lock)))
        goto out;

    struct gpe_interrupt_ctx *irq_ctx = g_gpe_interrupt_head;

    while (irq_ctx) {
        match_ctx.block = irq_ctx->gpe_head;

        while (match_ctx.block) {
            uacpi_namespace_do_for_each_child(
                match_ctx.block->device_node, do_match_gpe_methods, UACPI_NULL,
                UACPI_OBJECT_METHOD_BIT, UACPI_MAX_DEPTH_ANY,
                UACPI_SHOULD_LOCK_YES, UACPI_PERMANENT_ONLY_YES, &match_ctx
            );
            match_ctx.block = match_ctx.block->next;
        }

        irq_ctx = irq_ctx->next;
    }

    if (match_ctx.matched_count) {
        uacpi_info("matched %u additional GPEs post dynamic table load\n",
                   match_ctx.matched_count);
    }

out:
    uacpi_recursive_lock_release(&g_event_lock);
    uacpi_namespace_write_lock();
}

static uacpi_status create_gpe_block(
    uacpi_namespace_node *device_node, uacpi_u32 irq, uacpi_u16 base_idx,
    uacpi_u64 address, uacpi_u8 address_space_id, uacpi_u16 num_registers
)
{
    uacpi_status ret = UACPI_STATUS_OUT_OF_MEMORY;
    struct gpe_match_ctx match_ctx = { 0 };
    struct gpe_block *block;
    struct gpe_register *reg;
    struct gp_event *event;
    struct acpi_gas tmp_gas = {
        .address_space_id = address_space_id,
        .register_bit_width = 8,
    };
    uacpi_size i, j;

    block = uacpi_kernel_alloc_zeroed(sizeof(*block));
    if (uacpi_unlikely(block == UACPI_NULL))
        return ret;

    block->device_node = device_node;
    block->base_idx = base_idx;

    block->num_registers = num_registers;
    block->registers = uacpi_kernel_alloc_zeroed(
        num_registers * sizeof(*block->registers)
    );
    if (uacpi_unlikely(block->registers == UACPI_NULL))
        goto error_out;

    block->num_events = num_registers * EVENTS_PER_GPE_REGISTER;
    block->events = uacpi_kernel_alloc_zeroed(
        block->num_events * sizeof(*block->events)
    );
    if (uacpi_unlikely(block->events == UACPI_NULL))
        goto error_out;

    for (reg = block->registers, event = block->events, i = 0;
         i < num_registers; ++i, ++reg) {

        /*
         * Initialize this register pair as well as all the events within it.
         *
         * Each register has two sub registers: status & enable, 8 bits each.
         * Each bit corresponds to one event that we initialize below.
         */
        reg->base_idx = base_idx + (i * EVENTS_PER_GPE_REGISTER);


        tmp_gas.address = address + i;
        ret = uacpi_map_gas_noalloc(&tmp_gas, &reg->status);
        if (uacpi_unlikely_error(ret))
            goto error_out;

        tmp_gas.address += num_registers;
        ret = uacpi_map_gas_noalloc(&tmp_gas, &reg->enable);
        if (uacpi_unlikely_error(ret))
            goto error_out;

        for (j = 0; j < EVENTS_PER_GPE_REGISTER; ++j, ++event) {
            event->idx = reg->base_idx + j;
            event->reg = reg;
        }

        /*
         * Disable all GPEs in this register & clear anything that might be
         * pending from earlier.
         */
        ret = uacpi_gas_write_mapped(&reg->enable, 0x00);
        if (uacpi_unlikely_error(ret))
            goto error_out;

        ret = uacpi_gas_write_mapped(&reg->status, 0xFF);
        if (uacpi_unlikely_error(ret))
            goto error_out;
    }

    ret = find_or_create_gpe_interrupt_ctx(irq, &block->irq_ctx);
    if (uacpi_unlikely_error(ret))
        goto error_out;

    block->next = block->irq_ctx->gpe_head;
    block->irq_ctx->gpe_head = block;
    match_ctx.block = block;

    uacpi_namespace_do_for_each_child(
        device_node, do_match_gpe_methods, UACPI_NULL,
        UACPI_OBJECT_METHOD_BIT, UACPI_MAX_DEPTH_ANY,
        UACPI_SHOULD_LOCK_YES, UACPI_PERMANENT_ONLY_YES, &match_ctx
    );

    uacpi_trace("initialized GPE block %.4s[%d->%d], %d AML handlers (IRQ %d)\n",
                device_node->name.text, base_idx, base_idx + block->num_events,
                match_ctx.matched_count, irq);
    return UACPI_STATUS_OK;

error_out:
    uninstall_gpe_block(block);
    return ret;
}

typedef uacpi_iteration_decision (*gpe_block_iteration_callback)
    (struct gpe_block*, uacpi_handle);

static void for_each_gpe_block(
    gpe_block_iteration_callback cb, uacpi_handle handle
)
{
    uacpi_iteration_decision decision;
    struct gpe_interrupt_ctx *irq_ctx = g_gpe_interrupt_head;
    struct gpe_block *block;

    while (irq_ctx) {
        block = irq_ctx->gpe_head;

        while (block) {
            decision = cb(block, handle);
            if (decision == UACPI_ITERATION_DECISION_BREAK)
                return;

            block = block->next;
        }

        irq_ctx = irq_ctx->next;
    }
}

struct gpe_search_ctx {
    uacpi_namespace_node *gpe_device;
    uacpi_u16 idx;
    struct gpe_block *out_block;
    struct gp_event *out_event;
};

static uacpi_iteration_decision do_find_gpe(
    struct gpe_block *block, uacpi_handle opaque
)
{
    struct gpe_search_ctx *ctx = opaque;

    if (block->device_node != ctx->gpe_device)
        return UACPI_ITERATION_DECISION_CONTINUE;

    ctx->out_block = block;
    ctx->out_event = gpe_from_block(block, ctx->idx);
    if (ctx->out_event == UACPI_NULL)
        return UACPI_ITERATION_DECISION_CONTINUE;

    return UACPI_ITERATION_DECISION_BREAK;
}

static struct gp_event *get_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    struct gpe_search_ctx ctx = {
        .gpe_device = gpe_device,
        .idx = idx,
    };

    for_each_gpe_block(do_find_gpe, &ctx);
    return ctx.out_event;
}

static void gp_event_toggle_masks(struct gp_event *event, uacpi_bool set_on)
{
    uacpi_u8 this_mask;
    struct gpe_register *reg = event->reg;

    this_mask = gpe_get_mask(event);

    if (set_on) {
        reg->runtime_mask |= this_mask;
        reg->current_mask = reg->runtime_mask;
        return;
    }

    reg->runtime_mask &= ~this_mask;
    reg->current_mask = reg->runtime_mask;
}

static uacpi_status gpe_remove_user(struct gp_event *event)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (uacpi_unlikely(event->num_users == 0))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (--event->num_users == 0) {
        gp_event_toggle_masks(event, UACPI_FALSE);

        ret = set_gpe_state(event, GPE_STATE_DISABLED);
        if (uacpi_unlikely_error(ret)) {
            gp_event_toggle_masks(event, UACPI_TRUE);
            event->num_users++;
        }
    }

    return ret;
}

enum event_clear_if_first {
    EVENT_CLEAR_IF_FIRST_YES,
    EVENT_CLEAR_IF_FIRST_NO,
};

static uacpi_status gpe_add_user(
    struct gp_event *event, enum event_clear_if_first clear_if_first
)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (uacpi_unlikely(event->num_users == 0xFF))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (++event->num_users == 1) {
        if (clear_if_first == EVENT_CLEAR_IF_FIRST_YES)
            clear_gpe(event);

        gp_event_toggle_masks(event, UACPI_TRUE);

        ret = set_gpe_state(event, GPE_STATE_ENABLED);
        if (uacpi_unlikely_error(ret)) {
            gp_event_toggle_masks(event, UACPI_FALSE);
            event->num_users--;
        }
    }

    return ret;
}

const uacpi_char *uacpi_gpe_triggering_to_string(
    uacpi_gpe_triggering triggering
)
{
    switch (triggering) {
    case UACPI_GPE_TRIGGERING_EDGE:
        return "edge";
    case UACPI_GPE_TRIGGERING_LEVEL:
        return "level";
    default:
        return "invalid";
    }
}

static uacpi_bool gpe_needs_polling(struct gp_event *event)
{
    return event->num_users && event->triggering == UACPI_GPE_TRIGGERING_EDGE;
}

static uacpi_status gpe_mask_unmask(
    struct gp_event *event, uacpi_bool should_mask
)
{
    struct gpe_register *reg;
    uacpi_u8 mask;

    reg = event->reg;
    mask = gpe_get_mask(event);

    if (should_mask) {
        if (reg->masked_mask & mask)
            return UACPI_STATUS_INVALID_ARGUMENT;

        // 1. Mask the GPE, this makes sure its state is no longer modifyable
        reg->masked_mask |= mask;

        /*
         * 2. Wait for in-flight work & IRQs to finish, these might already
         *    be past the respective "if (masked)" check and therefore may
         *    try to re-enable a masked GPE.
         */
        uacpi_kernel_wait_for_work_completion();

        /*
         * 3. Now that this GPE's state is unmodifyable and we know that currently
         *    in-flight IRQs will see the masked state, we can safely disable this
         *    event knowing it won't be re-enabled by a racing IRQ.
         */
        set_gpe_state(event, GPE_STATE_DISABLED);

        /*
         * 4. Wait for the last possible IRQ to finish, now that this event is
         *    disabled.
         */
        uacpi_kernel_wait_for_work_completion();

        return UACPI_STATUS_OK;
    }

    if (!(reg->masked_mask & mask))
        return UACPI_STATUS_INVALID_ARGUMENT;

    reg->masked_mask &= ~mask;
    if (!event->block_interrupts && event->num_users)
        set_gpe_state(event, GPE_STATE_ENABLED_CONDITIONALLY);

    return UACPI_STATUS_OK;
}

/*
 * Safely mask the event before we modify its handlers.
 *
 * This makes sure we can't get an IRQ in the middle of modifying this
 * event's structures.
 */
static uacpi_bool gpe_mask_safe(struct gp_event *event)
{
    // No need to flush or do anything if it's not currently enabled
    if (!(event->reg->current_mask & gpe_get_mask(event)))
        return UACPI_FALSE;

    gpe_mask_unmask(event, UACPI_TRUE);
    return UACPI_TRUE;
}

static uacpi_iteration_decision do_initialize_gpe_block(
    struct gpe_block *block, uacpi_handle opaque
)
{
    uacpi_status ret;
    uacpi_bool *poll_blocks = opaque;
    uacpi_size i, j, count_enabled = 0;
    struct gp_event *event;

    for (i = 0; i < block->num_registers; ++i) {
        for (j = 0; j < EVENTS_PER_GPE_REGISTER; ++j) {
            event = &block->events[j + i * EVENTS_PER_GPE_REGISTER];

            if (event->wake ||
                event->handler_type != GPE_HANDLER_TYPE_AML_HANDLER)
                continue;

            ret = gpe_add_user(event, EVENT_CLEAR_IF_FIRST_NO);
            if (uacpi_unlikely_error(ret)) {
                uacpi_warn("failed to enable GPE(%02X): %s\n",
                           event->idx, uacpi_status_to_string(ret));
                continue;
            }

            *poll_blocks |= gpe_needs_polling(event);
            count_enabled++;
        }
    }

    if (count_enabled) {
        uacpi_info(
            "enabled %zu GPEs in block %.4s@[%d->%d]\n",
            count_enabled, block->device_node->name.text,
            block->base_idx, block->base_idx + block->num_events
        );
    }
    return UACPI_ITERATION_DECISION_CONTINUE;
}

uacpi_status uacpi_finalize_gpe_initialization(void)
{
    uacpi_status ret;
    uacpi_bool poll_blocks = UACPI_FALSE;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (g_gpes_finalized)
        goto out;

    g_gpes_finalized = UACPI_TRUE;

    for_each_gpe_block(do_initialize_gpe_block, &poll_blocks);
    if (poll_blocks)
        detect_gpes(g_gpe_interrupt_head->gpe_head);

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

static uacpi_status sanitize_device_and_find_gpe(
    uacpi_namespace_node **gpe_device, uacpi_u16 idx,
    struct gp_event **out_event
)
{
    if (*gpe_device == UACPI_NULL) {
        *gpe_device = uacpi_namespace_get_predefined(
            UACPI_PREDEFINED_NAMESPACE_GPE
        );
    }

    *out_event = get_gpe(*gpe_device, idx);
    if (*out_event == UACPI_NULL)
        return UACPI_STATUS_NOT_FOUND;

    return UACPI_STATUS_OK;
}

static uacpi_status do_install_gpe_handler(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx,
    uacpi_gpe_triggering triggering, enum gpe_handler_type type,
    uacpi_gpe_handler handler, uacpi_handle ctx
)
{
    uacpi_status ret;
    struct gp_event *event;
    struct gpe_native_handler *native_handler;
    uacpi_bool did_mask;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(triggering > UACPI_GPE_TRIGGERING_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (event->handler_type == GPE_HANDLER_TYPE_NATIVE_HANDLER ||
        event->handler_type == GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW) {
        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    native_handler = uacpi_kernel_alloc(sizeof(*native_handler));
    if (uacpi_unlikely(native_handler == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }

    native_handler->cb = handler;
    native_handler->ctx = ctx;
    native_handler->previous_handler = event->any_handler;
    native_handler->previous_handler_type = event->handler_type;
    native_handler->previous_triggering = event->triggering;
    native_handler->previously_enabled = UACPI_FALSE;

    did_mask = gpe_mask_safe(event);

    if ((event->handler_type == GPE_HANDLER_TYPE_AML_HANDLER ||
        event->handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) &&
        event->num_users != 0) {
        native_handler->previously_enabled = UACPI_TRUE;
        gpe_remove_user(event);

        if (uacpi_unlikely(event->triggering != triggering)) {
            uacpi_warn(
                "GPE(%02X) user handler claims %s triggering, originally "
                "configured as %s\n", idx,
                uacpi_gpe_triggering_to_string(triggering),
                uacpi_gpe_triggering_to_string(event->triggering)
            );
        }
    }

    event->native_handler = native_handler;
    event->handler_type = type;
    event->triggering = triggering;

    if (did_mask)
        gpe_mask_unmask(event, UACPI_FALSE);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_install_gpe_handler(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx,
    uacpi_gpe_triggering triggering, uacpi_gpe_handler handler,
    uacpi_handle ctx
)
{
    return do_install_gpe_handler(
        gpe_device, idx, triggering, GPE_HANDLER_TYPE_NATIVE_HANDLER,
        handler, ctx
    );
}

uacpi_status uacpi_install_gpe_handler_raw(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx,
    uacpi_gpe_triggering triggering, uacpi_gpe_handler handler,
    uacpi_handle ctx
)
{
    return do_install_gpe_handler(
        gpe_device, idx, triggering, GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW,
        handler, ctx
    );
}

uacpi_status uacpi_uninstall_gpe_handler(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx,
    uacpi_gpe_handler handler
)
{
    uacpi_status ret;
    struct gp_event *event;
    struct gpe_native_handler *native_handler;
    uacpi_bool did_mask;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (event->handler_type != GPE_HANDLER_TYPE_NATIVE_HANDLER &&
        event->handler_type != GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out;
    }

    native_handler = event->native_handler;
    if (uacpi_unlikely(native_handler->cb != handler)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    did_mask = gpe_mask_safe(event);

    event->aml_handler = native_handler->previous_handler;
    event->triggering = native_handler->previous_triggering;
    event->handler_type = native_handler->previous_handler_type;

    if ((event->handler_type == GPE_HANDLER_TYPE_AML_HANDLER ||
         event->handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) &&
         native_handler->previously_enabled) {
        gpe_add_user(event, EVENT_CLEAR_IF_FIRST_NO);
    }

    uacpi_free(native_handler, sizeof(*native_handler));

    if (did_mask)
        gpe_mask_unmask(event, UACPI_FALSE);

    if (gpe_needs_polling(event))
        maybe_dispatch_gpe(gpe_device, event);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_enable_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (uacpi_unlikely(event->handler_type == GPE_HANDLER_TYPE_NONE)) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    ret = gpe_add_user(event, EVENT_CLEAR_IF_FIRST_YES);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (gpe_needs_polling(event))
        maybe_dispatch_gpe(gpe_device, event);

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_disable_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = gpe_remove_user(event);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_clear_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = clear_gpe(event);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

static uacpi_status gpe_suspend_resume(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx, enum gpe_state state
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    event->block_interrupts = state == GPE_STATE_DISABLED;
    ret = set_gpe_state(event, state);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_suspend_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    return gpe_suspend_resume(gpe_device, idx, GPE_STATE_DISABLED);
}

uacpi_status uacpi_resume_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    return gpe_suspend_resume(gpe_device, idx, GPE_STATE_ENABLED);
}

uacpi_status uacpi_finish_handling_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    event = get_gpe(gpe_device, idx);
    if (uacpi_unlikely(event == UACPI_NULL)) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out;
    }

    ret = restore_gpe(event);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;

}

static uacpi_status gpe_get_mask_unmask(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx, uacpi_bool should_mask
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = gpe_mask_unmask(event, should_mask);

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_mask_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    return gpe_get_mask_unmask(gpe_device, idx, UACPI_TRUE);
}

uacpi_status uacpi_unmask_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    return gpe_get_mask_unmask(gpe_device, idx, UACPI_FALSE);
}

uacpi_status uacpi_setup_gpe_for_wake(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx,
    uacpi_namespace_node *wake_device
)
{
    uacpi_status ret;
    struct gp_event *event;
    uacpi_bool did_mask;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (wake_device != UACPI_NULL) {
        uacpi_bool is_dev = wake_device == uacpi_namespace_root();

        if (!is_dev) {
            ret = uacpi_namespace_node_is(wake_device, UACPI_OBJECT_DEVICE, &is_dev);
            if (uacpi_unlikely_error(ret))
                return ret;
        }

        if (!is_dev)
            return UACPI_STATUS_INVALID_ARGUMENT;
    }

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    did_mask = gpe_mask_safe(event);

    if (wake_device != UACPI_NULL) {
        switch (event->handler_type) {
        case GPE_HANDLER_TYPE_NONE:
            event->handler_type = GPE_HANDLER_TYPE_IMPLICIT_NOTIFY;
            event->triggering = UACPI_GPE_TRIGGERING_LEVEL;
            break;

        case GPE_HANDLER_TYPE_AML_HANDLER:
            /*
             * An AML handler already exists, we expect it to call Notify() as
             * it sees fit. For now just make sure this event is disabled if it
             * had been enabled automatically previously during initialization.
             */
            gpe_remove_user(event);
            break;

        case GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW:
        case GPE_HANDLER_TYPE_NATIVE_HANDLER:
            uacpi_warn(
                "not configuring implicit notify for GPE(%02X) -> %.4s: "
                " a user handler already installed\n", event->idx,
                wake_device->name.text
            );
            break;

        // We will re-check this below
        case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY:
            break;

        default:
            uacpi_warn("invalid GPE(%02X) handler type: %d\n",
                       event->idx, event->handler_type);
            ret = UACPI_STATUS_INTERNAL_ERROR;
            goto out_unmask;
        }

        /*
         * This GPE has no known AML handler, so we configure it to receive
         * implicit notifications for wake devices when we get a corresponding
         * GPE triggered. Usually it's the job of a matching AML handler, but
         * we didn't find any.
         */
        if (event->handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) {
            struct gpe_implicit_notify_handler *implicit_handler;

            implicit_handler = event->implicit_handler;
            while (implicit_handler) {
                if (implicit_handler->device == wake_device) {
                    ret = UACPI_STATUS_ALREADY_EXISTS;
                    goto out_unmask;
                }

                implicit_handler = implicit_handler->next;
            }

            implicit_handler = uacpi_kernel_alloc(sizeof(*implicit_handler));
            if (uacpi_likely(implicit_handler != UACPI_NULL)) {
                implicit_handler->device = wake_device;
                implicit_handler->next = event->implicit_handler;
                event->implicit_handler = implicit_handler;
            } else {
                uacpi_warn(
                    "unable to configure implicit wake for GPE(%02X) -> %.4s: "
                    "out of memory\n", event->idx, wake_device->name.text
                );
            }
        }
    }

    event->wake = UACPI_TRUE;

out_unmask:
    if (did_mask)
        gpe_mask_unmask(event, UACPI_FALSE);
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

static uacpi_status gpe_enable_disable_for_wake(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx, uacpi_bool enabled
)
{
    uacpi_status ret;
    struct gp_event *event;
    struct gpe_register *reg;
    uacpi_u8 mask;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (!event->wake) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    reg = event->reg;
    mask = gpe_get_mask(event);

    if (enabled)
        reg->wake_mask |= mask;
    else
        reg->wake_mask &= mask;

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_enable_gpe_for_wake(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    return gpe_enable_disable_for_wake(gpe_device, idx, UACPI_TRUE);
}

uacpi_status uacpi_disable_gpe_for_wake(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    return gpe_enable_disable_for_wake(gpe_device, idx, UACPI_FALSE);
}

struct do_for_all_gpes_ctx {
    enum gpe_block_action action;
    uacpi_status ret;
};

static uacpi_iteration_decision do_for_all_gpes(
    struct gpe_block *block, uacpi_handle opaque
)
{
    struct do_for_all_gpes_ctx *ctx = opaque;

    ctx->ret = gpe_block_apply_action(block, ctx->action);
    if (uacpi_unlikely_error(ctx->ret))
        return UACPI_ITERATION_DECISION_BREAK;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static uacpi_status for_all_gpes_locked(struct do_for_all_gpes_ctx *ctx)
{
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    for_each_gpe_block(do_for_all_gpes, ctx);

    uacpi_recursive_lock_release(&g_event_lock);
    return ctx->ret;
}

uacpi_status uacpi_disable_all_gpes(void)
{
    struct do_for_all_gpes_ctx ctx = {
        .action = GPE_BLOCK_ACTION_DISABLE_ALL,
    };
    return for_all_gpes_locked(&ctx);
}

uacpi_status uacpi_enable_all_runtime_gpes(void)
{
    struct do_for_all_gpes_ctx ctx = {
        .action = GPE_BLOCK_ACTION_ENABLE_ALL_FOR_RUNTIME,
    };
    return for_all_gpes_locked(&ctx);
}

uacpi_status uacpi_enable_all_wake_gpes(void)
{
    struct do_for_all_gpes_ctx ctx = {
        .action = GPE_BLOCK_ACTION_ENABLE_ALL_FOR_WAKE,
    };
    return for_all_gpes_locked(&ctx);
}

static uacpi_status initialize_gpes(void)
{
    uacpi_status ret;
    uacpi_namespace_node *gpe_node;
    struct acpi_fadt *fadt = &g_uacpi_rt_ctx.fadt;
    uacpi_u8 gpe0_regs = 0, gpe1_regs = 0;

    gpe_node = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_GPE);

    if (fadt->x_gpe0_blk.address && fadt->gpe0_blk_len) {
        gpe0_regs = fadt->gpe0_blk_len / 2;

        ret = create_gpe_block(
            gpe_node, fadt->sci_int, 0, fadt->x_gpe0_blk.address,
            fadt->x_gpe0_blk.address_space_id, gpe0_regs
        );
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("unable to create FADT GPE block 0: %s\n",
                        uacpi_status_to_string(ret));
        }
    }

    if (fadt->x_gpe1_blk.address && fadt->gpe1_blk_len) {
        gpe1_regs = fadt->gpe1_blk_len / 2;

        if (uacpi_unlikely((gpe0_regs * EVENTS_PER_GPE_REGISTER) >
                           fadt->gpe1_base)) {
            uacpi_error(
                "FADT GPE block 1 [%d->%d] collides with GPE block 0 "
                "[%d->%d], ignoring\n",
                0, gpe0_regs * EVENTS_PER_GPE_REGISTER, fadt->gpe1_base,
                gpe1_regs * EVENTS_PER_GPE_REGISTER
            );
            gpe1_regs = 0;
            goto out;
        }

        ret = create_gpe_block(
            gpe_node, fadt->sci_int, fadt->gpe1_base, fadt->x_gpe1_blk.address,
            fadt->x_gpe1_blk.address_space_id, gpe1_regs
        );
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("unable to create FADT GPE block 1: %s\n",
                        uacpi_status_to_string(ret));
        }
    }

    if (gpe0_regs == 0 && gpe1_regs == 0)
        uacpi_trace("platform has no FADT GPE events\n");

out:
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_install_gpe_block(
    uacpi_namespace_node *gpe_device, uacpi_u64 address,
    uacpi_address_space address_space, uacpi_u16 num_registers, uacpi_u32 irq
)
{
    uacpi_status ret;
    uacpi_bool is_dev;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_namespace_node_is(gpe_device, UACPI_OBJECT_DEVICE, &is_dev);
    if (uacpi_unlikely_error(ret))
        return ret;
    if (!is_dev)
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (uacpi_unlikely(get_gpe(gpe_device, 0) != UACPI_NULL)) {
        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    ret = create_gpe_block(
        gpe_device, irq, 0, address, address_space, num_registers
    );

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_uninstall_gpe_block(
    uacpi_namespace_node *gpe_device
)
{
    uacpi_status ret;
    uacpi_bool is_dev;
    struct gpe_search_ctx search_ctx = {
        .idx = 0,
        .gpe_device = gpe_device,
    };

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_namespace_node_is(gpe_device, UACPI_OBJECT_DEVICE, &is_dev);
    if (uacpi_unlikely_error(ret))
        return ret;
    if (!is_dev)
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    for_each_gpe_block(do_find_gpe, &search_ctx);
    if (search_ctx.out_block == UACPI_NULL) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out;
    }

    uninstall_gpe_block(search_ctx.out_block);

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

static uacpi_interrupt_ret handle_global_lock(uacpi_handle ctx)
{
    uacpi_cpu_flags flags;
    UACPI_UNUSED(ctx);

    if (uacpi_unlikely(!g_uacpi_rt_ctx.has_global_lock)) {
        uacpi_warn("platform has no global lock but a release event "
                   "was fired anyway?\n");
        return UACPI_INTERRUPT_HANDLED;
    }

    flags = uacpi_kernel_lock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock);
    if (!g_uacpi_rt_ctx.global_lock_pending) {
        uacpi_trace("spurious firmware global lock release notification\n");
        goto out;
    }

    uacpi_trace("received a firmware global lock release notification\n");

    uacpi_kernel_signal_event(g_uacpi_rt_ctx.global_lock_event);
    g_uacpi_rt_ctx.global_lock_pending = UACPI_FALSE;

out:
    uacpi_kernel_unlock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock, flags);
    return UACPI_INTERRUPT_HANDLED;
}

static uacpi_interrupt_ret handle_sci(uacpi_handle ctx)
{
    uacpi_interrupt_ret int_ret = UACPI_INTERRUPT_NOT_HANDLED;

    int_ret |= handle_fixed_events();
    int_ret |= handle_gpes(ctx);

    return int_ret;
}

uacpi_status uacpi_initialize_events_early(void)
{
    uacpi_status ret;

    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    g_gpe_state_slock = uacpi_kernel_create_spinlock();
    if (uacpi_unlikely(g_gpe_state_slock == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ret = uacpi_recursive_lock_init(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = initialize_fixed_events();
    if (uacpi_unlikely_error(ret))
        return ret;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_initialize_events(void)
{
    uacpi_status ret;

    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = initialize_gpes();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_kernel_install_interrupt_handler(
        g_uacpi_rt_ctx.fadt.sci_int, handle_sci, g_gpe_interrupt_head,
        &g_uacpi_rt_ctx.sci_handle
    );
    if (uacpi_unlikely_error(ret)) {
        uacpi_error(
            "unable to install SCI interrupt handler: %s\n",
            uacpi_status_to_string(ret)
        );
        return ret;
    }
    g_uacpi_rt_ctx.sci_handle_valid = UACPI_TRUE;

    g_uacpi_rt_ctx.global_lock_event = uacpi_kernel_create_event();
    if (uacpi_unlikely(g_uacpi_rt_ctx.global_lock_event == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    g_uacpi_rt_ctx.global_lock_spinlock = uacpi_kernel_create_spinlock();
    if (uacpi_unlikely(g_uacpi_rt_ctx.global_lock_spinlock == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ret = uacpi_install_fixed_event_handler(
        UACPI_FIXED_EVENT_GLOBAL_LOCK, handle_global_lock, UACPI_NULL
    );
    if (uacpi_likely_success(ret)) {
        if (uacpi_unlikely(g_uacpi_rt_ctx.facs == UACPI_NULL)) {
            uacpi_uninstall_fixed_event_handler(UACPI_FIXED_EVENT_GLOBAL_LOCK);
            uacpi_warn("platform has global lock but no FACS was provided\n");
            return ret;
        }
        g_uacpi_rt_ctx.has_global_lock = UACPI_TRUE;
    } else if (ret == UACPI_STATUS_HARDWARE_TIMEOUT) {
        // has_global_lock remains set to false
        uacpi_trace("platform has no global lock\n");
        ret = UACPI_STATUS_OK;
    }

    return ret;
}

void uacpi_deinitialize_events(void)
{
    struct gpe_interrupt_ctx *ctx, *next_ctx = g_gpe_interrupt_head;
    uacpi_size i;

    g_gpes_finalized = UACPI_FALSE;

    if (g_uacpi_rt_ctx.sci_handle_valid) {
        uacpi_kernel_uninstall_interrupt_handler(
            handle_sci, g_uacpi_rt_ctx.sci_handle
        );
        g_uacpi_rt_ctx.sci_handle_valid = UACPI_FALSE;
    }

    while (next_ctx) {
        ctx = next_ctx;
        next_ctx = ctx->next;

        struct gpe_block *block, *next_block = ctx->gpe_head;
        while (next_block) {
            block = next_block;
            next_block = block->next;
            uninstall_gpe_block(block);
        }
    }

    for (i = 0; i < UACPI_FIXED_EVENT_MAX; ++i) {
        if (fixed_event_handlers[i].handler)
            uacpi_uninstall_fixed_event_handler(i);
    }

    if (g_gpe_state_slock != UACPI_NULL) {
        uacpi_kernel_free_spinlock(g_gpe_state_slock);
        g_gpe_state_slock = UACPI_NULL;
    }

    uacpi_recursive_lock_deinit(&g_event_lock);

    g_gpe_interrupt_head = UACPI_NULL;
}

uacpi_status uacpi_install_fixed_event_handler(
    uacpi_fixed_event event, uacpi_interrupt_handler handler,
    uacpi_handle user
)
{
    uacpi_status ret;
    struct fixed_event_handler *ev;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ev = &fixed_event_handlers[event];

    if (ev->handler != UACPI_NULL) {
        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    ev->handler = handler;
    ev->ctx = user;

    ret = set_event(event, UACPI_EVENT_ENABLED);
    if (uacpi_unlikely_error(ret)) {
        ev->handler = UACPI_NULL;
        ev->ctx = UACPI_NULL;
    }

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_uninstall_fixed_event_handler(
    uacpi_fixed_event event
)
{
    uacpi_status ret;
    struct fixed_event_handler *ev;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ev = &fixed_event_handlers[event];

    ret = set_event(event, UACPI_EVENT_DISABLED);
    if (uacpi_unlikely_error(ret))
        goto out;

    uacpi_kernel_wait_for_work_completion();

    ev->handler = UACPI_NULL;
    ev->ctx = UACPI_NULL;

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_fixed_event_info(
    uacpi_fixed_event event, uacpi_event_info *out_info
)
{
    uacpi_status ret;
    const struct fixed_event *ev;
    uacpi_u64 raw_value;
    uacpi_event_info info = 0;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_NOT_FOUND;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (fixed_event_handlers[event].handler != UACPI_NULL)
        info |= UACPI_EVENT_INFO_HAS_HANDLER;

    ev = &fixed_events[event];

    ret = uacpi_read_register_field(ev->enable_field, &raw_value);
    if (uacpi_unlikely_error(ret))
        goto out;
    if (raw_value)
        info |= UACPI_EVENT_INFO_ENABLED | UACPI_EVENT_INFO_HW_ENABLED;

    ret = uacpi_read_register_field(ev->status_field, &raw_value);
    if (uacpi_unlikely_error(ret))
        goto out;
    if (raw_value)
        info |= UACPI_EVENT_INFO_HW_STATUS;

    *out_info = info;
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

uacpi_status uacpi_gpe_info(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx, uacpi_event_info *out_info
)
{
    uacpi_status ret;
    struct gp_event *event;
    struct gpe_register *reg;
    uacpi_u8 mask;
    uacpi_u64 raw_value;
    uacpi_event_info info = 0;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (event->handler_type != GPE_HANDLER_TYPE_NONE)
        info |= UACPI_EVENT_INFO_HAS_HANDLER;

    mask = gpe_get_mask(event);
    reg = event->reg;

    if (reg->runtime_mask & mask)
        info |= UACPI_EVENT_INFO_ENABLED;
    if (reg->masked_mask & mask)
        info |= UACPI_EVENT_INFO_MASKED;
    if (reg->wake_mask & mask)
        info |= UACPI_EVENT_INFO_ENABLED_FOR_WAKE;

    ret = uacpi_gas_read_mapped(&reg->enable, &raw_value);
    if (uacpi_unlikely_error(ret))
        goto out;
    if (raw_value & mask)
        info |= UACPI_EVENT_INFO_HW_ENABLED;

    ret = uacpi_gas_read_mapped(&reg->status, &raw_value);
    if (uacpi_unlikely_error(ret))
        goto out;
    if (raw_value & mask)
        info |= UACPI_EVENT_INFO_HW_STATUS;

    *out_info = info;
out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

#define PM1_STATUS_BITS (               \
    ACPI_PM1_STS_TMR_STS_MASK |         \
    ACPI_PM1_STS_BM_STS_MASK |          \
    ACPI_PM1_STS_GBL_STS_MASK |         \
    ACPI_PM1_STS_PWRBTN_STS_MASK |      \
    ACPI_PM1_STS_SLPBTN_STS_MASK |      \
    ACPI_PM1_STS_RTC_STS_MASK |         \
    ACPI_PM1_STS_PCIEXP_WAKE_STS_MASK | \
    ACPI_PM1_STS_WAKE_STS_MASK          \
)

uacpi_status uacpi_clear_all_events(void)
{
    uacpi_status ret;
    struct do_for_all_gpes_ctx ctx = {
        .action = GPE_BLOCK_ACTION_CLEAR_ALL,
    };

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_write_register(UACPI_REGISTER_PM1_STS, PM1_STATUS_BITS);
    if (uacpi_unlikely_error(ret))
        goto out;

    for_each_gpe_block(do_for_all_gpes, &ctx);
    ret = ctx.ret;

out:
    uacpi_recursive_lock_release(&g_event_lock);
    return ret;
}

#endif // !UACPI_REDUCED_HARDWARE && !UACPI_BAREBONES_MODE
