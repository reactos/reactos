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
#include <uacpi/platform/atomic.h>
#include <uacpi/acpi.h>

#define UACPI_EVENT_DISABLED 0
#define UACPI_EVENT_ENABLED 1

#if !defined(UACPI_REDUCED_HARDWARE) && !defined(UACPI_BAREBONES_MODE)

/*
 * Protects the state of every GPE, as well as the lists that a GPE is found
 * via. This is the only lock that is taken by the interrupt handler, by the
 * deferred work that re-enables an event, and by the part of the API that is
 * allowed to be called from an interrupt context.
 *
 * An event that was looked up with nothing but this lock held is only
 * guaranteed to be there for as long as the lock is held.
 */
static uacpi_handle g_gpe_state_slock;
static struct uacpi_recursive_lock g_event_lock;

/*
 * Serializes the API that reconfigures an event, which has to wait for the
 * in-flight interrupts & work to complete in order to do that safely. The
 * event lock is dropped while waiting, see wait_for_work_completion_unlocked,
 * so this is what keeps the event from being reconfigured or freed by someone
 * else in the meantime.
 *
 * Unlike the event lock, this one is never taken by the work itself, which
 * makes it safe to wait under. It's always acquired before the event lock.
 */
static struct uacpi_recursive_lock g_event_config_lock;
static uacpi_bool g_gpes_finalized;

static uacpi_status event_config_lock(void)
{
    uacpi_status ret;

    ret = uacpi_recursive_lock_acquire(&g_event_config_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        uacpi_recursive_lock_release(&g_event_config_lock);

    return ret;
}

static void event_config_unlock(void)
{
    uacpi_recursive_lock_release(&g_event_lock);
    uacpi_recursive_lock_release(&g_event_config_lock);
}

/*
 * The work that we're waiting for is allowed to take the event lock: a GPE
 * handler might load a table, in which case we have to match the GPE methods
 * it has brought in, and a notify handler is free to use most of the event
 * API. Waiting for it with the event lock held would hang both sides forever.
 *
 * The caller must hold both the config & the event lock. Since the latter is
 * released while we wait, any event state that was looked at prior to this
 * call must be considered stale.
 */
static void wait_for_work_completion_unlocked(void)
{
    uacpi_recursive_lock_release(&g_event_lock);
    uacpi_kernel_wait_for_work_completion();
    uacpi_recursive_lock_acquire(&g_event_lock);
}

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

/*
 * Protects the handlers above, as well as the enable bit of every fixed event.
 *
 * A handler is installed & removed with the event lock held, but that's not
 * something the interrupt handler is able to take, and neither is the API
 * that is allowed to be called from it. This is what they use instead.
 */
static uacpi_handle g_fixed_event_slock;

static uacpi_status initialize_fixed_events(void)
{
    uacpi_size i;

    for (i = 0; i <= UACPI_FIXED_EVENT_MAX; ++i) {
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
        uacpi_error("failed to %sable fixed event %d",
                    value ? "en" : "dis", event);
        return UACPI_STATUS_HARDWARE_TIMEOUT;
    }

    uacpi_trace("fixed event %d %sabled successfully",
                event, value ? "en" : "dis");
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_enable_fixed_event(uacpi_fixed_event event)
{
    uacpi_status ret;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);

    /*
     * Attempting to enable an event that doesn't have a handler is most likely
     * an error, don't allow it. This includes an event that is in the process
     * of having its handler removed, which is why the check is a part of the
     * same critical section as the enable.
     */
    if (uacpi_unlikely(fixed_event_handlers[event].handler == UACPI_NULL)) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    ret = set_event(event, UACPI_EVENT_ENABLED);

out:
    uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);
    return ret;
}

uacpi_status uacpi_disable_fixed_event(uacpi_fixed_event event)
{
    uacpi_status ret;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);
    ret = set_event(event, UACPI_EVENT_DISABLED);
    uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);

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

void uacpi_events_restore_buttons_post_wake(void)
{
    static const uacpi_u8 buttons[] = {
        UACPI_FIXED_EVENT_POWER_BUTTON,
        UACPI_FIXED_EVENT_SLEEP_BUTTON,
    };
    uacpi_size i;
    uacpi_u8 event;
    uacpi_cpu_flags flags;

    if (uacpi_is_hardware_reduced())
        return;

    for (i = 0; i < UACPI_ARRAY_SIZE(buttons); ++i) {
        event = buttons[i];

        /*
         * The button that has woken us up is still pending at this point. Get
         * rid of it so that it's not delivered as if it was pressed once again,
         * which tends to be handled by going right back to sleep, or by
         * shutting down.
         */
        uacpi_write_register_field(
            fixed_events[event].status_field, ACPI_PM1_STS_CLEAR
        );

        /*
         * Don't rely on the button still being enabled after the sleep, it
         * has to be if there's a handler that is waiting for it.
         *
         * Same as uacpi_enable_fixed_event, this must not enable a button that
         * is in the process of having its handler removed.
         */
        flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);
        if (fixed_event_handlers[event].handler != UACPI_NULL)
            set_event(event, UACPI_EVENT_ENABLED);
        uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);
    }
}

static uacpi_interrupt_ret dispatch_fixed_event(
    const struct fixed_event *ev, uacpi_fixed_event event
)
{
    uacpi_status ret;
    struct fixed_event_handler evh;
    uacpi_u64 is_enabled;
    uacpi_cpu_flags flags;

    /*
     * The event was enabled at the time we were invoked, but that might not
     * be the case anymore: it's disabled by whoever is uninstalling its
     * handler, in which case the handler must not be called, or by someone
     * who doesn't want to hear from it for now. Take another look with the
     * lock held, which is also what makes the handler safe to look at.
     */
    flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);
    ret = uacpi_read_register_field(ev->enable_field, &is_enabled);
    evh = fixed_event_handlers[event];
    uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);

    if (uacpi_unlikely_error(ret) || !is_enabled)
        return UACPI_INTERRUPT_NOT_HANDLED;

    ret = uacpi_write_register_field(ev->status_field, ACPI_PM1_STS_CLEAR);
    if (uacpi_unlikely_error(ret))
        return UACPI_INTERRUPT_NOT_HANDLED;

    if (uacpi_unlikely(evh.handler == UACPI_NULL)) {
        uacpi_warn(
            "fixed event %u fired but no handler installed, disabling...",
            event
        );
        uacpi_write_register_field(ev->enable_field, UACPI_EVENT_DISABLED);
        return UACPI_INTERRUPT_NOT_HANDLED;
    }

    return evh.handler(evh.ctx);
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

    for (i = 0; i <= UACPI_FIXED_EVENT_MAX; ++i)
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

    /*
     * What the deferred work for this event is scheduled with. This is done
     * by the interrupt handler, which is no place to be allocating anything,
     * so an event gets one as soon as it's given a handler that needs it.
     */
    uacpi_handle work_item;

    uacpi_u16 idx;

    uacpi_u8 handler_type : 3;
    uacpi_u8 triggering : 1;
    uacpi_u8 wake : 1;

    /*
     * Set for an event that has a user that it was given automatically when
     * the GPE initialization was finalized, which is taken away from it once
     * it's set up for wake.
     */
    uacpi_u8 has_auto_user : 1;

    /*
     * Everything below is the state of the event, which unlike the above is
     * also modified by the interrupt handler & the deferred work, neither of
     * which is able to take the event lock. It's protected by the GPE state
     * spinlock instead, and is the reason why none of it is a part of the
     * bitfield above.
     */

    // "reference count" of the number of times this event has been enabled
    uacpi_u8 num_users;

    /*
     * Set for an event that is to stay disabled no matter how many users it
     * has: one that is being handled at the moment, or one that was suspended.
     */
    uacpi_bool block_interrupts;

    /*
     * Set for as long as the work item is in use, from the moment the event
     * is dispatched and until it's restored.
     */
    uacpi_bool work_item_in_use;

    /*
     * Set for an event that was dispatched without having its status cleared,
     * which is left until it's restored. This is decided at the time that the
     * event is dispatched: the way it's triggered might get replaced along
     * with its handler while it's still being handled.
     */
    uacpi_bool clear_on_restore;
};

struct gpe_register {
    uacpi_mapped_gas status;
    uacpi_mapped_gas enable;

    /*
     * The masks below, as well as the enable register itself, are protected
     * by the GPE state spinlock, see the note in struct gp_event.
     */
    uacpi_u8 runtime_mask;
    uacpi_u8 wake_mask;
    uacpi_u8 masked_mask;
    uacpi_u8 current_mask;

    /*
     * The events that were enabled at any point since they were last known to
     * be quiescent, and might therefore still be in use by an interrupt
     * handler or by the work that it has scheduled, even if they're disabled
     * by now.
     */
    uacpi_u8 armed_mask;

    uacpi_u16 base_idx;
};

struct gpe_block {
    struct gpe_block *next;

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

    // Set once the events of this block have been enabled by the finalization
    uacpi_bool finalized;
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

// Must be called with the GPE state lock held
static uacpi_status set_gpe_state_unlocked(
    struct gp_event *event, enum gpe_state state
)
{
    uacpi_status ret;
    struct gpe_register *reg = event->reg;
    uacpi_u64 enable_mask;
    uacpi_u8 event_bit;

    event_bit = gpe_get_mask(event);
    if (state != GPE_STATE_DISABLED && (reg->masked_mask & event_bit))
        return UACPI_STATUS_OK;

    if (state == GPE_STATE_ENABLED_CONDITIONALLY) {
        if (!(reg->current_mask & event_bit))
            return UACPI_STATUS_OK;

        state = GPE_STATE_ENABLED;
    }

    ret = uacpi_gas_read_mapped(&reg->enable, &enable_mask);
    if (uacpi_unlikely_error(ret))
        return ret;

    switch (state) {
    case GPE_STATE_ENABLED:
        enable_mask |= event_bit;
        reg->armed_mask |= event_bit;
        break;
    case GPE_STATE_DISABLED:
        enable_mask &= ~event_bit;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return uacpi_gas_write_mapped(&reg->enable, enable_mask);
}

static uacpi_status set_gpe_state(struct gp_event *event, enum gpe_state state)
{
    uacpi_status ret;
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    ret = set_gpe_state_unlocked(event, state);
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    return ret;
}

static uacpi_status clear_gpe(struct gp_event *event)
{
    struct gpe_register *reg = event->reg;

    return uacpi_gas_write_mapped(&reg->status, gpe_get_mask(event));
}

static uacpi_bool gpe_is_armed(struct gp_event *event)
{
    uacpi_bool ret;
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    ret = (event->reg->armed_mask & gpe_get_mask(event)) != 0;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    return ret;
}

static void gpe_disarm(struct gp_event *event)
{
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    event->reg->armed_mask &= ~gpe_get_mask(event);
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
}

// Must be called with the GPE state lock held
static uacpi_status restore_gpe_unlocked(struct gp_event *event)
{
    uacpi_status ret;

    /*
     * A level triggered event is cleared once it's been handled. It might not
     * be level triggered anymore if its handler was replaced in the meantime,
     * so go by what it was when it was dispatched as well.
     */
    if (event->clear_on_restore ||
        event->triggering == UACPI_GPE_TRIGGERING_LEVEL) {
        ret = clear_gpe(event);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    ret = set_gpe_state_unlocked(event, GPE_STATE_ENABLED_CONDITIONALLY);
    event->block_interrupts = UACPI_FALSE;
    event->clear_on_restore = UACPI_FALSE;

    return ret;
}

static uacpi_status restore_gpe(struct gp_event *event)
{
    uacpi_status ret;
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    ret = restore_gpe_unlocked(event);
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    return ret;
}

static void gpe_work_done(struct gp_event *event)
{
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    event->work_item_in_use = UACPI_FALSE;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
}

static void async_restore_gpe(uacpi_handle opaque)
{
    uacpi_status ret;
    struct gp_event *event = opaque;

    /*
     * This is the last thing that we need the work item for, so the event is
     * free to make use of it again as soon as it's enabled below.
     */
    gpe_work_done(event);

    ret = restore_gpe(event);
    if (uacpi_unlikely_error(ret)) {
        uacpi_error("unable to restore GPE(%02X): %s",
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
        uacpi_object_name name;

        method_obj = uacpi_namespace_node_get_object_typed(
            event->aml_handler, UACPI_OBJECT_METHOD_BIT
        );
        if (uacpi_unlikely(method_obj == UACPI_NULL)) {
            uacpi_error("GPE(%02X) AML handler gone", event->idx);
            break;
        }

        name = uacpi_namespace_node_name(event->aml_handler);
        uacpi_trace(
            "executing GPE(%02X) handler %.4s",
            event->idx, name.text
        );

        ret = uacpi_execute_control_method(
            event->aml_handler, method_obj->method, UACPI_NULL, UACPI_NULL
        );
        if (uacpi_unlikely_error(ret)) {
            uacpi_error(
                "error while executing GPE(%02X) handler %.4s: %s",
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

            /*
             * The namespace is unlocked for a moment while the notification
             * is being scheduled, which is all it takes for a table that has
             * a method for this event to get loaded. The list is gone in that
             * case, and so is the need to notify anyone else.
             */
            if (event->handler_type != GPE_HANDLER_TYPE_IMPLICIT_NOTIFY)
                break;

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
     *
     * The work item is the one that we're being executed as: it's ours to
     * reuse now that we were invoked, and it's not needed for anything else
     * until the event is restored.
     */
    uacpi_kernel_schedule_work(
        UACPI_WORK_NOTIFICATION, event->work_item, async_restore_gpe, event
    );
}

enum gpe_dispatcher {
    // The handler of the interrupt that the event has raised
    GPE_DISPATCHER_INTERRUPT,

    // Whoever has enabled the event, and is checking if it's pending already
    GPE_DISPATCHER_POLL,
};

/*
 * Makes sure that an event is only ever dispatched by one of those who notice
 * that it's pending, which besides the interrupt handler might be someone who
 * is polling it, possibly on a different CPU.
 *
 * This is done by disabling the event if it's both pending and enabled, with
 * the lock held from the moment it's looked at and until it's disabled. Anyone
 * else finds it disabled, and leaves it alone. A raw handler is the exception:
 * the event is not ours to manage in that case, so it's only checked for being
 * able to fire to begin with, and is otherwise left the way it was found.
 *
 * That leaves us with nothing to tell that such an event was already taken
 * care of, so it's never claimed by a poll: a raw handler is only ever invoked
 * for an interrupt, and is expected to do any polling that it needs itself.
 */
static uacpi_status gpe_try_claim(
    struct gp_event *event, enum gpe_dispatcher dispatcher,
    uacpi_u8 *out_handler_type, uacpi_u8 *out_triggering,
    uacpi_bool *out_did_claim
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct gpe_register *reg = event->reg;
    uacpi_u64 status, enable;
    uacpi_u8 event_bit;
    uacpi_cpu_flags flags;

    *out_did_claim = UACPI_FALSE;
    event_bit = gpe_get_mask(event);

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    /*
     * The caller goes by this snapshot, as the handler might be replaced as
     * soon as we let go of the lock. It's always replaced with the lock held,
     * which is also what orders these loads against whoever has set it up,
     * possibly on a different CPU just now.
     */
    *out_handler_type = event->handler_type;
    *out_triggering = event->triggering;

    ret = uacpi_gas_read_mapped(&reg->status, &status);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = uacpi_gas_read_mapped(&reg->enable, &enable);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (!(status & enable & event_bit))
        goto out;

    if (*out_handler_type == GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW) {
        *out_did_claim = dispatcher == GPE_DISPATCHER_INTERRUPT;
        goto out;
    }

    ret = uacpi_gas_write_mapped(&reg->enable, enable & ~event_bit);
    if (uacpi_likely_success(ret)) {
        event->block_interrupts = UACPI_TRUE;

        if (*out_triggering == UACPI_GPE_TRIGGERING_LEVEL)
            event->clear_on_restore = UACPI_TRUE;

        *out_did_claim = UACPI_TRUE;
    }

out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
    return ret;
}

static uacpi_interrupt_ret dispatch_gpe(
    uacpi_namespace_node *device_node, struct gp_event *event,
    enum gpe_dispatcher dispatcher
)
{
    uacpi_status ret;
    uacpi_interrupt_ret int_ret = UACPI_INTERRUPT_NOT_HANDLED;
    uacpi_u8 handler_type, triggering;
    uacpi_bool did_claim, in_use;
    uacpi_cpu_flags flags;

    ret = gpe_try_claim(
        event, dispatcher, &handler_type, &triggering, &did_claim
    );
    if (uacpi_unlikely_error(ret)) {
        uacpi_error("failed to claim GPE(%02X): %s",
                    event->idx, uacpi_status_to_string(ret));
        return int_ret;
    }

    // Either someone else got to it first, or there's nothing to handle
    if (!did_claim)
        return int_ret;

    /*
     * For raw handlers we don't do any management whatsoever, we just let the
     * handler know a GPE has triggered and let it handle disable/enable as
     * well as clearing.
     */
    if (handler_type == GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW) {
        return event->native_handler->cb(
            event->native_handler->ctx, device_node, event->idx
        );
    }

    if (handler_type == GPE_HANDLER_TYPE_AML_HANDLER ||
        handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) {
        /*
         * A work item must never be scheduled while it's still pending. The
         * event stays disabled until it's restored, so this only happens if
         * it was enabled behind our back while still being handled, e.g. via
         * uacpi_resume_gpe or uacpi_enable_all_runtime_gpes.
         *
         * Leave it both disabled & pending in that case: it fires again as
         * soon as it's restored, which is when the work item is free.
         */
        flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
        in_use = event->work_item_in_use;
        event->work_item_in_use = UACPI_TRUE;
        uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

        if (uacpi_unlikely(in_use))
            return UACPI_INTERRUPT_HANDLED;
    }

    if (triggering == UACPI_GPE_TRIGGERING_EDGE) {
        ret = clear_gpe(event);
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("unable to clear GPE(%02X): %s",
                        event->idx, uacpi_status_to_string(ret));

            // No work was scheduled, see above
            if (handler_type == GPE_HANDLER_TYPE_AML_HANDLER ||
                handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY)
                gpe_work_done(event);

            /*
             * Nobody is going to handle this event, which means that nobody is
             * going to restore it either, so undo all of the claim.
             */
            flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
            set_gpe_state_unlocked(event, GPE_STATE_ENABLED_CONDITIONALLY);
            event->block_interrupts = UACPI_FALSE;
            uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

            return int_ret;
        }
    }

    switch (handler_type) {
    case GPE_HANDLER_TYPE_NATIVE_HANDLER:
        int_ret = event->native_handler->cb(
            event->native_handler->ctx, device_node, event->idx
        );
        if (!(int_ret & UACPI_GPE_REENABLE))
            break;

        ret = restore_gpe(event);
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("unable to restore GPE(%02X): %s",
                        event->idx, uacpi_status_to_string(ret));
        }
        break;

    case GPE_HANDLER_TYPE_AML_HANDLER:
    case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY:
        uacpi_kernel_schedule_work(
            UACPI_WORK_GPE_EXECUTION, event->work_item, async_run_gpe_handler,
            event
        );
        break;

    default:
        uacpi_warn("GPE(%02X) fired but no handler, keeping disabled",
                   event->idx);
        break;
    }

    return UACPI_INTERRUPT_HANDLED;
}

/*
 * The lists of interrupt contexts & blocks are modified with both the event
 * lock and the GPE state lock held, so having either one is enough to walk
 * them.
 *
 * The interrupt handlers walk the blocks with no locks at all though. A block
 * is fully initialized before it's linked in, and is only freed after every
 * interrupt handler that might've been looking at it at the time it was
 * unlinked has returned.
 */
static struct gpe_block *load_gpe_block(struct gpe_block **block)
{
    return (struct gpe_block*)uacpi_atomic_load_ptr(block);
}

static uacpi_interrupt_ret detect_gpes(
    struct gpe_interrupt_ctx *ctx, enum gpe_dispatcher dispatcher
)
{
    uacpi_status ret;
    uacpi_interrupt_ret int_ret = UACPI_INTERRUPT_NOT_HANDLED;
    struct gpe_block *block;
    struct gpe_register *reg;
    struct gp_event *event;
    uacpi_u64 status, enable;
    uacpi_size i, j;
    uacpi_bool is_unused;
    uacpi_cpu_flags flags;

    block = load_gpe_block(&ctx->gpe_head);

    while (block) {
        for (i = 0; i < block->num_registers; ++i) {
            reg = &block->registers[i];

            flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
            is_unused = !reg->runtime_mask && !reg->wake_mask;
            uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

            if (is_unused)
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
                int_ret |= dispatch_gpe(
                    block->device_node, event, dispatcher
                );
            }
        }

        block = load_gpe_block(&block->next);
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

    dispatch_gpe(gpe_device, event, GPE_DISPATCHER_POLL);
    return ret;
}

static uacpi_interrupt_ret handle_gpes(uacpi_handle opaque)
{
    struct gpe_interrupt_ctx *ctx = opaque;

    if (uacpi_unlikely(ctx == UACPI_NULL))
        return UACPI_INTERRUPT_NOT_HANDLED;

    return detect_gpes(ctx, GPE_DISPATCHER_INTERRUPT);
}

static uacpi_status find_or_create_gpe_interrupt_ctx(
    uacpi_u32 irq, struct gpe_interrupt_ctx **out_ctx
)
{
    uacpi_status ret;
    struct gpe_interrupt_ctx *entry = g_gpe_interrupt_head;
    uacpi_cpu_flags flags;

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

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    entry->next = g_gpe_interrupt_head;
    if (g_gpe_interrupt_head != UACPI_NULL)
        g_gpe_interrupt_head->prev = entry;
    g_gpe_interrupt_head = entry;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    *out_ctx = entry;
    return UACPI_STATUS_OK;
}

/*
 * Makes sure the context is no longer used by anyone, it's up to the caller to
 * free it once that's safe to do. The event lock is dropped while the handler
 * of the context is being uninstalled.
 */
static void unlink_gpe_interrupt_ctx(struct gpe_interrupt_ctx *ctx)
{
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    if (ctx->prev != UACPI_NULL)
        ctx->prev->next = ctx->next;
    else
        g_gpe_interrupt_head = ctx->next;

    if (ctx->next != UACPI_NULL)
        ctx->next->prev = ctx->prev;

    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    /*
     * This might have to wait for the handler to return, and the handler is
     * free to take the lock, so it can't be done with it held.
     *
     * The same goes for the event lock: the handler might be executing the
     * work that it has scheduled right away, and that is free to end up
     * waiting for it, e.g. by loading a table. Everything that the handler
     * is still able to get to stays around until the caller has waited for
     * it, so there's nothing for it to break in the meantime.
     */
    if (ctx->irq != g_uacpi_rt_ctx.fadt.sci_int) {
        uacpi_recursive_lock_release(&g_event_lock);
        uacpi_kernel_uninstall_interrupt_handler(
            handle_gpes, ctx->irq_handle
        );
        uacpi_recursive_lock_acquire(&g_event_lock);
    }
}

static uacpi_status gpe_ensure_work_item(struct gp_event *event)
{
    if (event->work_item != UACPI_NULL)
        return UACPI_STATUS_OK;

    event->work_item = uacpi_kernel_create_work_item();
    if (uacpi_unlikely(event->work_item == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

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
    uacpi_cpu_flags flags;

    for (i = 0; i < block->num_registers; ++i) {
        reg = &block->registers[i];

        if (action == GPE_BLOCK_ACTION_CLEAR_ALL) {
            ret = uacpi_gas_write_mapped(&reg->status, 0xFF);
            if (uacpi_unlikely_error(ret))
                return ret;
            continue;
        }

        /*
         * This is one critical section on purpose: the mask that we pick must
         * still be the one that is in effect by the time it's written to the
         * register.
         */
        flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

        switch (action) {
        case GPE_BLOCK_ACTION_ENABLE_ALL_FOR_RUNTIME:
            value = reg->runtime_mask;
            break;
        case GPE_BLOCK_ACTION_ENABLE_ALL_FOR_WAKE:
            value = reg->wake_mask;
            break;
        default:
            value = 0;
            break;
        }

        reg->current_mask = value;

        /*
         * A masked event is kept disabled for as long as we're running, but
         * it still counts as enabled as far as the current mask goes. This is
         * what allows it to be enabled as soon as it's unmasked.
         */
        if (action == GPE_BLOCK_ACTION_ENABLE_ALL_FOR_RUNTIME)
            value &= ~reg->masked_mask;

        reg->armed_mask |= value;
        ret = uacpi_gas_write_mapped(&reg->enable, value);
        uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

        if (uacpi_unlikely_error(ret))
            return ret;
    }

    return UACPI_STATUS_OK;
}

/*
 * Releases a block that is not reachable by anyone: it's either not installed
 * yet, or was unlinked with everything that might've been using it long gone.
 */
static void free_gpe_block(struct gpe_block *block)
{
    if (block->registers != UACPI_NULL) {
        struct gpe_register *reg;
        uacpi_size i;

        for (i = 0; i < block->num_registers; ++i) {
            reg = &block->registers[i];

            if (reg->enable.total_bit_width)
                uacpi_unmap_gas_nofree(&reg->enable);
            if (reg->status.total_bit_width)
                uacpi_unmap_gas_nofree(&reg->status);
        }
    }

    if (block->events != UACPI_NULL) {
        uacpi_size i;
        struct gp_event *event;
        struct gpe_native_handler *native_handler;

        for (i = 0; i < block->num_events; ++i) {
            event = &block->events[i];

            switch (event->handler_type) {
            case GPE_HANDLER_TYPE_NONE:
            case GPE_HANDLER_TYPE_AML_HANDLER:
                break;

            case GPE_HANDLER_TYPE_NATIVE_HANDLER:
            case GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW:
                native_handler = event->native_handler;

                /*
                 * The handler that this one has replaced is still around in
                 * case this one gets uninstalled, so we have to free it as
                 * well.
                 */
                if (native_handler->previous_handler_type ==
                    GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) {
                    event->implicit_handler = native_handler->previous_handler;
                    gpe_release_implicit_notify_handlers(event);
                }

                uacpi_free(native_handler, sizeof(*native_handler));
                break;

            case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY: {
                gpe_release_implicit_notify_handlers(event);
                break;
            }

            default:
                break;
            }

            if (event->work_item != UACPI_NULL)
                uacpi_kernel_free_work_item(event->work_item);
        }

    }

    uacpi_free(block->registers,
               sizeof(*block->registers) * block->num_registers);
    uacpi_free(block->events,
               sizeof(*block->events) * block->num_events);
    uacpi_free(block, sizeof(*block));
}

/*
 * This must be called with the config & event locks held, the latter is
 * dropped while we wait.
 */
static void uninstall_gpe_block(struct gpe_block *block)
{
    struct gpe_interrupt_ctx *ctx = block->irq_ctx;
    uacpi_bool ctx_is_unused;
    uacpi_cpu_flags flags;
    uacpi_size i;

    /*
     * 1. Mask the GPEs, this makes sure their state is no longer modifyable
     *
     * This is done for every register up front: we drop the event lock while
     * waiting below, and nobody should be able to enable an event in the
     * meantime. The one thing that still is, which is enabling an event for
     * wake, is the reason why the events are only disabled after the wait.
     */
    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    for (i = 0; i < block->num_registers; ++i)
        block->registers[i].masked_mask = 0xFF;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    /*
     * 2. Wait for in-flight work & IRQs to finish, these might already
     *    be past the respective "if (masked)" check and therefore may
     *    try to re-enable a masked GPE.
     */
    wait_for_work_completion_unlocked();

    /*
     * 3. Now that the state of these GPEs is unmodifyable and we know that
     *    currently in-flight IRQs will see the masked state, we can safely
     *    disable all events knowing they won't be re-enabled by a racing IRQ.
     */
    for (i = 0; i < block->num_registers; ++i) {
        /*
         * An interrupt handler that is disabling one of the events right now
         * does so by reading the register and writing it back with one bit
         * cleared. Don't let it undo what we do here.
         */
        flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
        uacpi_gas_write_mapped(&block->registers[i].enable, 0x00);
        uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
    }

    // 4. Make sure the interrupt handler is no longer able to find the block
    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    if (block == ctx->gpe_head) {
        uacpi_atomic_store_ptr(&ctx->gpe_head, block->next);
    } else {
        struct gpe_block *prev_block = ctx->gpe_head;

        while (prev_block) {
            if (prev_block->next == block) {
                uacpi_atomic_store_ptr(&prev_block->next, block->next);
                break;
            }

            prev_block = prev_block->next;
        }
    }

    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    /*
     * This GPE block was the last user of this interrupt context, remove it
     *
     * The context of the SCI is an exception: it's what the SCI handler
     * uses to find the blocks to dispatch, so it stays around for as long
     * as the handler does.
     */
    ctx_is_unused = ctx->gpe_head == UACPI_NULL &&
                    ctx->irq != g_uacpi_rt_ctx.fadt.sci_int;
    if (ctx_is_unused)
        unlink_gpe_interrupt_ctx(ctx);

    /*
     * 5. Wait for the last possible IRQ to finish, as well as any work that
     *    still refers to one of the events.
     *
     * This is done no matter what: an interrupt handler that was invoked for
     * some other block might've been looking at this one right as we were
     * unlinking it, and an event that is disabled by now might still have
     * work in flight from back when it wasn't.
     */
    wait_for_work_completion_unlocked();

    if (ctx_is_unused)
        uacpi_free(ctx, sizeof(*ctx));

    free_gpe_block(block);
}

static struct gp_event *gpe_from_block(struct gpe_block *block, uacpi_u16 idx)
{
    uacpi_u16 offset;

    if (idx < block->base_idx)
        return UACPI_NULL;

    offset = idx - block->base_idx;
    if (offset >= block->num_events)
        return UACPI_NULL;

    return &block->events[offset];
}

struct gpe_match_ctx {
    struct gpe_block *block;
    uacpi_u32 matched_count;
    uacpi_bool post_dynamic_table_load;
};

/*
 * The interrupt handler looks at the handler of an event, as well as at how
 * it's triggered, with the GPE state lock held, so that's what both have to
 * be replaced under for it to see them in their entirety.
 */
static void gpe_set_handler(
    struct gp_event *event, uacpi_handle handler, enum gpe_handler_type type,
    uacpi_u8 triggering
)
{
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    event->any_handler = handler;
    event->handler_type = type;
    event->triggering = triggering;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
}

/*
 * An event that was claimed by the interrupt handler stays blocked until it's
 * restored, which is up to whoever is handling it. If the handler is replaced
 * while that is still the case there's nobody left to do that, and it's not
 * something that the next handler would know to do either, so the claim has
 * to go along with the handler that it was made for.
 *
 * This is only safe to do for an event that is quiescent.
 */
static void gpe_drop_claim(struct gp_event *event)
{
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    event->block_interrupts = UACPI_FALSE;
    event->clear_on_restore = UACPI_FALSE;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
}

/*
 * An event that is configured for implicit notify might be in use at the
 * moment: its work looks at the handler with the namespace locked, and the
 * interrupt handler does so with the GPE state lock held. We can't wait for
 * either one to be done with it, as it might be that very work that got us
 * here by loading a table, so take both locks instead.
 */
static uacpi_status gpe_set_aml_handler(
    struct gp_event *event, uacpi_namespace_node *node, uacpi_u8 triggering
)
{
    uacpi_status ret;

    ret = uacpi_namespace_write_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    /*
     * This had implicit notify configured but this is no longer needed as we
     * now have an actual AML handler. Free the implicit notify list and switch
     * this handler to AML mode.
     */
    if (event->handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY)
        gpe_release_implicit_notify_handlers(event);

    gpe_set_handler(event, node, GPE_HANDLER_TYPE_AML_HANDLER, triggering);

    uacpi_namespace_write_unlock();
    return UACPI_STATUS_OK;
}

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
        uacpi_trace("invalid GPE method name %.4s, ignored", node->name.text);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    event = gpe_from_block(ctx->block, idx);
    if (event == UACPI_NULL)
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (event->handler_type == GPE_HANDLER_TYPE_NONE ||
        event->handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) {
        // The method is executed as deferred work, make sure that's possible
        ret = gpe_ensure_work_item(event);
        if (uacpi_unlikely_error(ret)) {
            uacpi_warn(
                "unable to assign GPE(%02X) to %.4s: %s", (uacpi_u32)idx,
                node->name.text, uacpi_status_to_string(ret)
            );
            return UACPI_ITERATION_DECISION_CONTINUE;
        }
    }

    switch (event->handler_type) {
    case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY:
    case GPE_HANDLER_TYPE_NONE:
        ret = gpe_set_aml_handler(event, node, triggering);
        if (uacpi_unlikely_error(ret)) {
            uacpi_warn(
                "unable to assign GPE(%02X) to %.4s: %s", (uacpi_u32)idx,
                node->name.text, uacpi_status_to_string(ret)
            );
            return UACPI_ITERATION_DECISION_CONTINUE;
        }
        break;

    case GPE_HANDLER_TYPE_AML_HANDLER:
        // This is okay, since we're re-running the detection code
        if (!ctx->post_dynamic_table_load) {
            uacpi_warn(
                "GPE(%02X) already matched %.4s, skipping %.4s",
                (uacpi_u32)idx, event->aml_handler->name.text, node->name.text
            );
        }
        return UACPI_ITERATION_DECISION_CONTINUE;

    case GPE_HANDLER_TYPE_NATIVE_HANDLER:
    case GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW:
        uacpi_trace(
            "not assigning GPE(%02X) to %.4s, override "
            "installed by user", (uacpi_u32)idx, node->name.text
        );
        UACPI_FALLTHROUGH;
    default:
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    uacpi_trace("assigned GPE(%02X) -> %.4s",
                (uacpi_u32)idx, node->name.text);
    ctx->matched_count++;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

void uacpi_events_match_post_dynamic_table_load(void)
{
    struct gpe_match_ctx match_ctx = {
        .post_dynamic_table_load = UACPI_TRUE,
    };
    struct gpe_interrupt_ctx *irq_ctx;

    if (uacpi_is_hardware_reduced())
        return;

    uacpi_namespace_write_unlock();

    if (uacpi_unlikely_error(uacpi_recursive_lock_acquire(&g_event_lock)))
        goto out_no_unlock;

    irq_ctx = g_gpe_interrupt_head;

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
        uacpi_info("matched %u additional GPEs post dynamic table load",
                   match_ctx.matched_count);
    }

    uacpi_recursive_lock_release(&g_event_lock);

out_no_unlock:
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
    struct acpi_gas tmp_gas = { 0 };
    uacpi_size i, j;
    uacpi_u32 num_events;
    uacpi_cpu_flags flags;

    if (uacpi_unlikely(num_registers == 0))
        return UACPI_STATUS_INVALID_ARGUMENT;

    // An event is identified by a 16-bit index
    num_events = num_registers * EVENTS_PER_GPE_REGISTER;
    if (uacpi_unlikely(base_idx + num_events > 0xFFFF))
        return UACPI_STATUS_INVALID_ARGUMENT;

    tmp_gas.address_space_id = address_space_id;
    tmp_gas.register_bit_width = 8;

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

    match_ctx.block = block;

    uacpi_namespace_do_for_each_child(
        device_node, do_match_gpe_methods, UACPI_NULL,
        UACPI_OBJECT_METHOD_BIT, UACPI_MAX_DEPTH_ANY,
        UACPI_SHOULD_LOCK_YES, UACPI_PERMANENT_ONLY_YES, &match_ctx
    );

    // The block is complete, make it visible to the interrupt handler
    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    block->next = block->irq_ctx->gpe_head;
    uacpi_atomic_store_ptr(&block->irq_ctx->gpe_head, block);
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    /*
     * The events of this block are only enabled once the initialization is
     * finalized. Make sure that happens even if it's been done before, which
     * is the case for a block that is installed at runtime.
     */
    g_gpes_finalized = UACPI_FALSE;

    uacpi_trace("initialized GPE block %.4s[%d->%d], %u AML handlers (IRQ %u)",
                device_node->name.text, base_idx, base_idx + block->num_events,
                match_ctx.matched_count, irq);
    return UACPI_STATUS_OK;

error_out:
    // The block is not visible to anyone at this point, so just get rid of it
    free_gpe_block(block);
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
    struct gpe_search_ctx ctx = { 0 };

    ctx.gpe_device = gpe_device;
    ctx.idx = idx;

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

// Must be called with the GPE state lock held
static uacpi_status gpe_remove_user_unlocked(struct gp_event *event)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (uacpi_unlikely(event->num_users == 0))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (--event->num_users == 0) {
        gp_event_toggle_masks(event, UACPI_FALSE);

        ret = set_gpe_state_unlocked(event, GPE_STATE_DISABLED);
        if (uacpi_unlikely_error(ret)) {
            gp_event_toggle_masks(event, UACPI_TRUE);
            event->num_users++;
        }
    }

    return ret;
}

static uacpi_status gpe_remove_user(struct gp_event *event)
{
    uacpi_status ret;
    uacpi_cpu_flags flags;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    ret = gpe_remove_user_unlocked(event);
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    return ret;
}

enum event_clear_if_first {
    EVENT_CLEAR_IF_FIRST_YES,
    EVENT_CLEAR_IF_FIRST_NO,
};

static uacpi_status gpe_add_user(
    struct gp_event *event, enum event_clear_if_first clear_if_first,
    uacpi_bool *out_is_first
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    uacpi_cpu_flags flags;

    *out_is_first = UACPI_FALSE;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    if (uacpi_unlikely(event->num_users == 0xFF)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    if (++event->num_users == 1) {
        if (clear_if_first == EVENT_CLEAR_IF_FIRST_YES)
            clear_gpe(event);

        gp_event_toggle_masks(event, UACPI_TRUE);

        ret = set_gpe_state_unlocked(event, GPE_STATE_ENABLED);
        if (uacpi_unlikely_error(ret)) {
            gp_event_toggle_masks(event, UACPI_FALSE);
            event->num_users--;
            goto out;
        }

        *out_is_first = UACPI_TRUE;
    }

out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
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

/*
 * Some hardware doesn't raise an interrupt for an edge triggered event that
 * is already pending by the time it's enabled, so it has to be checked for by
 * hand. That's only the case for the user that got it enabled: it's up to the
 * interrupt handler from there on.
 */
static uacpi_bool gpe_needs_polling(
    struct gp_event *event, uacpi_bool is_first_user
)
{
    return is_first_user && event->triggering == UACPI_GPE_TRIGGERING_EDGE;
}

/*
 * Mask the event, and get it to the point where it's no longer in use by
 * anyone. The event lock is dropped while we wait.
 *
 * This fails if the event couldn't be disabled, in which case it's still
 * masked, and it's up to the caller to decide what to do about that.
 */
static uacpi_status gpe_mask_and_flush(struct gp_event *event)
{
    uacpi_status ret;
    uacpi_cpu_flags flags;

    // 1. Mask the GPE, this makes sure its state is no longer modifyable
    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    event->reg->masked_mask |= gpe_get_mask(event);
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    /*
     * 2. Wait for in-flight work & IRQs to finish, these might already
     *    be past the respective "if (masked)" check and therefore may
     *    try to re-enable a masked GPE.
     */
    wait_for_work_completion_unlocked();

    do {
        /*
         * 3. Now that this GPE's state is unmodifyable and we know that
         *    currently in-flight IRQs will see the masked state, we can safely
         *    disable this event knowing it won't be re-enabled by a racing
         *    IRQ.
         */
        ret = set_gpe_state(event, GPE_STATE_DISABLED);

        /*
         * An event that we were not able to disable is free to fire for as
         * long as it wants to, so there's no getting it to quiesce.
         */
        if (uacpi_unlikely_error(ret)) {
            uacpi_error("failed to disable GPE(%02X): %s",
                        event->idx, uacpi_status_to_string(ret));
            return ret;
        }

        /*
         * Whatever is still using this event is taken care of by the wait
         * below, so it only has to be flushed again if it gets enabled from
         * here on.
         */
        gpe_disarm(event);

        /*
         * 4. Wait for the last possible IRQ to finish, now that this event is
         *    disabled.
         */
        wait_for_work_completion_unlocked();

        /*
         * The event lock is dropped while we wait, and there's one thing that
         * is able to enable a masked event, which is enabling it for wake. If
         * that happened in the meantime, none of the above is true anymore.
         * It can't happen for as long as we hold the lock though, so do this
         * until it doesn't.
         */
    } while (gpe_is_armed(event));

    return UACPI_STATUS_OK;
}

static uacpi_status gpe_mask_unmask(
    struct gp_event *event, uacpi_bool should_mask
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct gpe_register *reg;
    uacpi_u8 mask;
    uacpi_bool is_masked;
    uacpi_cpu_flags flags;

    reg = event->reg;
    mask = gpe_get_mask(event);

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    is_masked = (reg->masked_mask & mask) != 0;
    if (is_masked == should_mask) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    if (!should_mask) {
        reg->masked_mask &= ~mask;

        if (!event->block_interrupts && event->num_users) {
            ret = set_gpe_state_unlocked(
                event, GPE_STATE_ENABLED_CONDITIONALLY
            );
        }

        // Still masked as far as everyone is concerned, so this can be retried
        if (uacpi_unlikely_error(ret))
            reg->masked_mask |= mask;
    }

out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    if (!should_mask || uacpi_unlikely_error(ret))
        return ret;

    // This has to wait for the event to quiesce, so can't be done above
    ret = gpe_mask_and_flush(event);

    // An event that is still able to fire is not masked in any way that counts
    if (uacpi_unlikely_error(ret))
        gpe_mask_unmask(event, UACPI_FALSE);

    return ret;
}

/*
 * Safely mask the event before we modify its handlers.
 *
 * This makes sure we can't get an IRQ in the middle of modifying this
 * event's structures.
 *
 * NOTE: the event lock is dropped while we wait for the event to quiesce, so
 *       anything other than the native handlers, which are protected by the
 *       config lock, may change by the time this returns.
 *
 * This fails if the event couldn't be made quiescent, in which case it's left
 * the way it was found, and none of its structures are safe to modify.
 */
static uacpi_status gpe_mask_safe(
    struct gp_event *event, uacpi_bool *out_did_mask
)
{
    uacpi_status ret;
    struct gpe_register *reg = event->reg;
    uacpi_u8 mask;
    uacpi_bool was_masked, is_armed;
    uacpi_cpu_flags flags;

    mask = gpe_get_mask(event);

    /*
     * Mask the event before looking at anything else: an event that is not
     * masked can be enabled by anyone at any moment, with nothing but the
     * state lock held.
     *
     * If the event was already masked by the user it's not up to us to unmask
     * it once we're done.
     */
    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    was_masked = (reg->masked_mask & mask) != 0;
    reg->masked_mask |= mask;
    is_armed = (reg->armed_mask & mask) != 0;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

    *out_did_mask = !was_masked;

    /*
     * No need to flush if it was never enabled since the last time we did
     * that. Note that it's not enough for the event to be disabled right now:
     * the work that was scheduled for it earlier might still be pending.
     *
     * This is done even if the event was masked by the user, as there's one
     * way for a masked event to end up enabled, which is being enabled for
     * wake.
     */
    if (!is_armed)
        return UACPI_STATUS_OK;

    ret = gpe_mask_and_flush(event);
    if (uacpi_unlikely_error(ret)) {
        // Leave the event the way we have found it
        if (!was_masked)
            gpe_mask_unmask(event, UACPI_FALSE);

        *out_did_mask = UACPI_FALSE;
    }

    return ret;
}

static uacpi_iteration_decision do_initialize_gpe_block(
    struct gpe_block *block, uacpi_handle opaque
)
{
    uacpi_status ret;
    uacpi_bool *poll_blocks = opaque;
    uacpi_size i, j, count_enabled = 0;
    struct gp_event *event;
    uacpi_bool is_first_user;
    uacpi_cpu_flags flags;

    // Already taken care of by one of the previous calls
    if (block->finalized)
        return UACPI_ITERATION_DECISION_CONTINUE;

    for (i = 0; i < block->num_registers; ++i) {
        for (j = 0; j < EVENTS_PER_GPE_REGISTER; ++j) {
            event = &block->events[j + i * EVENTS_PER_GPE_REGISTER];

            if (event->wake ||
                event->handler_type != GPE_HANDLER_TYPE_AML_HANDLER)
                continue;

            ret = gpe_add_user(
                event, EVENT_CLEAR_IF_FIRST_NO, &is_first_user
            );
            if (uacpi_unlikely_error(ret)) {
                uacpi_warn("failed to enable GPE(%02X): %s",
                           event->idx, uacpi_status_to_string(ret));
                continue;
            }

            // This shares a byte with the handler type, see gpe_set_handler
            flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
            event->has_auto_user = UACPI_TRUE;
            uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

            *poll_blocks |= gpe_needs_polling(event, is_first_user);
            count_enabled++;
        }
    }

    if (count_enabled) {
        uacpi_info(
            "enabled %zu GPEs in block %.4s@[%d->%d]",
            count_enabled, block->device_node->name.text,
            block->base_idx, block->base_idx + block->num_events
        );
    }

    block->finalized = UACPI_TRUE;
    return UACPI_ITERATION_DECISION_CONTINUE;
}

uacpi_status uacpi_finalize_gpe_initialization(void)
{
    uacpi_status ret;
    uacpi_bool poll_blocks = UACPI_FALSE;
    struct gpe_interrupt_ctx *irq_ctx;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = uacpi_recursive_lock_acquire(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (g_gpes_finalized)
        goto out;

    g_gpes_finalized = UACPI_TRUE;

    for_each_gpe_block(do_initialize_gpe_block, &poll_blocks);
    if (poll_blocks) {
        irq_ctx = g_gpe_interrupt_head;

        while (irq_ctx) {
            detect_gpes(irq_ctx, GPE_DISPATCHER_POLL);
            irq_ctx = irq_ctx->next;
        }
    }

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

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    if (uacpi_unlikely(triggering > UACPI_GPE_TRIGGERING_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = event_config_lock();
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

    ret = gpe_mask_safe(event, &did_mask);
    if (uacpi_unlikely_error(ret)) {
        uacpi_free(native_handler, sizeof(*native_handler));
        goto out;
    }

    /*
     * Only look at the current handler after the event is masked, as it might
     * get replaced while we wait, e.g. if a table that was loaded dynamically
     * has brought in a GPE method for it.
     */
    native_handler->previous_handler = event->any_handler;
    native_handler->previous_handler_type = event->handler_type;
    native_handler->previous_triggering = event->triggering;
    native_handler->previously_enabled = UACPI_FALSE;

    if ((event->handler_type == GPE_HANDLER_TYPE_AML_HANDLER ||
        event->handler_type == GPE_HANDLER_TYPE_IMPLICIT_NOTIFY) &&
        uacpi_likely_success(gpe_remove_user(event))) {
        native_handler->previously_enabled = UACPI_TRUE;

        if (uacpi_unlikely(event->triggering != triggering)) {
            uacpi_warn(
                "GPE(%02X) user handler claims %s triggering, originally "
                "configured as %s", idx,
                uacpi_gpe_triggering_to_string(triggering),
                uacpi_gpe_triggering_to_string(event->triggering)
            );
        }
    }

    gpe_set_handler(event, native_handler, type, triggering);
    gpe_drop_claim(event);

    if (did_mask)
        gpe_mask_unmask(event, UACPI_FALSE);
out:
    event_config_unlock();
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
    uacpi_bool did_mask, is_first_user = UACPI_FALSE;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    ret = event_config_lock();
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

    ret = gpe_mask_safe(event, &did_mask);
    if (uacpi_unlikely_error(ret))
        goto out;

    /*
     * The handler that we're about to bring back gets the user that was taken
     * away from it, which is only ever done for an AML or an implicit notify
     * one. This is the one thing here that might not work out, as an event
     * only has room for so many, so do it while there's still nothing to
     * undo. The event is masked, so it's not enabled until we're done.
     */
    if (native_handler->previously_enabled) {
        ret = gpe_add_user(event, EVENT_CLEAR_IF_FIRST_NO, &is_first_user);
        if (uacpi_unlikely_error(ret))
            goto out_unmask;
    }

    gpe_set_handler(
        event, native_handler->previous_handler,
        native_handler->previous_handler_type,
        native_handler->previous_triggering
    );
    gpe_drop_claim(event);

    uacpi_free(native_handler, sizeof(*native_handler));

out_unmask:
    if (did_mask)
        gpe_mask_unmask(event, UACPI_FALSE);

    /*
     * An event that we had to mask was disabled for as long as it took to
     * replace its handler, so it's no different from one that was only now
     * enabled by getting its first user: it might've become pending without
     * anyone noticing.
     */
    if (gpe_needs_polling(event, is_first_user || did_mask))
        maybe_dispatch_gpe(gpe_device, event);
out:
    event_config_unlock();
    return ret;
}

uacpi_status uacpi_enable_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    uacpi_status ret;
    struct gp_event *event;
    uacpi_bool is_first_user;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    /*
     * Unlike the rest of the API that manages a single event, this takes the
     * event lock: an event that is already pending is dispatched by us right
     * here, and the lock is what keeps it from being reconfigured or freed
     * while that is being done.
     */
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

    ret = gpe_add_user(event, EVENT_CLEAR_IF_FIRST_YES, &is_first_user);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (gpe_needs_polling(event, is_first_user))
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
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = gpe_remove_user_unlocked(event);
out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
    return ret;
}

uacpi_status uacpi_clear_gpe(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx
)
{
    uacpi_status ret;
    struct gp_event *event;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = clear_gpe(event);
out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
    return ret;
}

static uacpi_status gpe_suspend_resume(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx, enum gpe_state state
)
{
    uacpi_status ret;
    struct gp_event *event;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    event->block_interrupts = state == GPE_STATE_DISABLED;
    ret = set_gpe_state_unlocked(event, state);
out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
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
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = restore_gpe_unlocked(event);
out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
    return ret;
}

static uacpi_status gpe_get_mask_unmask(
    uacpi_namespace_node *gpe_device, uacpi_u16 idx, uacpi_bool should_mask
)
{
    uacpi_status ret;
    struct gp_event *event;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    ret = event_config_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = gpe_mask_unmask(event, should_mask);

out:
    event_config_unlock();
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
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

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

    ret = event_config_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = sanitize_device_and_find_gpe(&gpe_device, idx, &event);
    if (uacpi_unlikely_error(ret))
        goto out;

    ret = gpe_mask_safe(event, &did_mask);
    if (uacpi_unlikely_error(ret))
        goto out;

    if (wake_device != UACPI_NULL) {
        struct gpe_implicit_notify_handler *implicit_handler;
        uacpi_u8 num_users;

        switch (event->handler_type) {
        case GPE_HANDLER_TYPE_IMPLICIT_NOTIFY:
            implicit_handler = event->implicit_handler;
            while (implicit_handler) {
                if (implicit_handler->device == wake_device) {
                    ret = UACPI_STATUS_ALREADY_EXISTS;
                    goto out_unmask;
                }

                implicit_handler = implicit_handler->next;
            }
            UACPI_FALLTHROUGH;
        case GPE_HANDLER_TYPE_NONE:
            /*
             * This GPE has no known AML handler, so we configure it to receive
             * implicit notifications for wake devices when we get a
             * corresponding GPE triggered. Usually it's the job of a matching
             * AML handler, but we didn't find any.
             *
             * The device is notified via deferred work, so get everything
             * that it takes ready before the event is touched: there's nothing
             * to undo this way if any of it fails.
             */
            implicit_handler = uacpi_kernel_alloc(sizeof(*implicit_handler));
            if (uacpi_unlikely(implicit_handler == UACPI_NULL)) {
                ret = UACPI_STATUS_OUT_OF_MEMORY;
                goto out_unmask;
            }

            ret = gpe_ensure_work_item(event);
            if (uacpi_unlikely_error(ret)) {
                uacpi_free(implicit_handler, sizeof(*implicit_handler));
                goto out_unmask;
            }

            if (event->handler_type == GPE_HANDLER_TYPE_NONE) {
                gpe_set_handler(
                    event, UACPI_NULL, GPE_HANDLER_TYPE_IMPLICIT_NOTIFY,
                    UACPI_GPE_TRIGGERING_LEVEL
                );
            }

            implicit_handler->device = wake_device;
            implicit_handler->next = event->implicit_handler;
            event->implicit_handler = implicit_handler;
            break;

        case GPE_HANDLER_TYPE_AML_HANDLER:
            /*
             * An AML handler already exists, we expect it to call Notify() as
             * it sees fit. For now just make sure this event is disabled if it
             * had been enabled automatically previously during initialization.
             *
             * This is only done for the user that it was given back then: any
             * other one belongs to somebody else, and is not ours to take.
             */
            if (!event->has_auto_user)
                break;

            /*
             * The user might be gone already if the event was disabled by
             * someone who has never enabled it, in which case there's nothing
             * left to take away. Otherwise only forget about it if it was
             * actually removed.
             */
            flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
            num_users = event->num_users;
            if (uacpi_likely(num_users != 0))
                ret = gpe_remove_user_unlocked(event);
            if (uacpi_likely_success(ret))
                event->has_auto_user = UACPI_FALSE;
            uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

            if (uacpi_unlikely(num_users == 0))
                uacpi_warn("inconsistent use count for GPE(%02X)", event->idx);

            if (uacpi_unlikely_error(ret))
                goto out_unmask;
            break;

        case GPE_HANDLER_TYPE_NATIVE_HANDLER_RAW:
        case GPE_HANDLER_TYPE_NATIVE_HANDLER:
            uacpi_warn(
                "not configuring implicit notify for GPE(%02X) -> %.4s: "
                "a user handler already installed", event->idx,
                wake_device->name.text
            );
            break;

        default:
            uacpi_warn("invalid GPE(%02X) handler type: %d",
                       event->idx, event->handler_type);
            ret = UACPI_STATUS_INTERNAL_ERROR;
            goto out_unmask;
        }
    }

    // This shares a byte with the above, so the same goes for it as well
    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);
    event->wake = UACPI_TRUE;
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);

out_unmask:
    if (did_mask)
        gpe_mask_unmask(event, UACPI_FALSE);
out:
    event_config_unlock();
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
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

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
        reg->wake_mask &= ~mask;

out:
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
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

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

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
            uacpi_error("unable to create FADT GPE block 0: %s",
                        uacpi_status_to_string(ret));
        }
    }

    if (fadt->x_gpe1_blk.address && fadt->gpe1_blk_len) {
        gpe1_regs = fadt->gpe1_blk_len / 2;

        if (uacpi_unlikely((gpe0_regs * EVENTS_PER_GPE_REGISTER) >
                           fadt->gpe1_base)) {
            uacpi_error(
                "FADT GPE block 1 [%d->%d] collides with GPE block 0 "
                "[%d->%d], ignoring",
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
            uacpi_error("unable to create FADT GPE block 1: %s",
                        uacpi_status_to_string(ret));
        }
    }

    if (gpe0_regs == 0 && gpe1_regs == 0)
        uacpi_trace("platform has no FADT GPE events");

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

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    ret = uacpi_namespace_node_is(gpe_device, UACPI_OBJECT_DEVICE, &is_dev);
    if (uacpi_unlikely_error(ret))
        return ret;
    if (!is_dev)
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = event_config_lock();
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
    event_config_unlock();
    return ret;
}

uacpi_status uacpi_uninstall_gpe_block(
    uacpi_namespace_node *gpe_device
)
{
    uacpi_status ret;
    uacpi_bool is_dev;
    struct gpe_search_ctx search_ctx = { 0 };

    search_ctx.idx = 0;
    search_ctx.gpe_device = gpe_device;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    ret = uacpi_namespace_node_is(gpe_device, UACPI_OBJECT_DEVICE, &is_dev);
    if (uacpi_unlikely_error(ret))
        return ret;
    if (!is_dev)
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = event_config_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    for_each_gpe_block(do_find_gpe, &search_ctx);
    if (search_ctx.out_block == UACPI_NULL) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out;
    }

    uninstall_gpe_block(search_ctx.out_block);

out:
    event_config_unlock();
    return ret;
}

static uacpi_interrupt_ret handle_global_lock(uacpi_handle ctx)
{
    uacpi_cpu_flags flags;
    UACPI_UNUSED(ctx);

    if (uacpi_unlikely(!g_uacpi_rt_ctx.has_global_lock)) {
        uacpi_warn("platform has no global lock but a release event "
                   "was fired anyway?");
        return UACPI_INTERRUPT_HANDLED;
    }

    flags = uacpi_kernel_lock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock);
    if (!g_uacpi_rt_ctx.global_lock_pending) {
        uacpi_trace("spurious firmware global lock release notification");
        goto out;
    }

    uacpi_trace("received a firmware global lock release notification");

    uacpi_signal_native_event(g_uacpi_rt_ctx.global_lock_event);
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

    g_fixed_event_slock = uacpi_kernel_create_spinlock();
    if (uacpi_unlikely(g_fixed_event_slock == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ret = uacpi_recursive_lock_init(&g_event_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_recursive_lock_init(&g_event_config_lock);
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
    struct gpe_interrupt_ctx *sci_ctx;

    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = initialize_gpes();
    if (uacpi_unlikely_error(ret))
        return ret;

    /*
     * The SCI handler is responsible for every GPE block that shares its
     * interrupt. Make sure it has a context to look for them in even if the
     * FADT doesn't describe any, as one might still be installed later on.
     */
    ret = find_or_create_gpe_interrupt_ctx(
        g_uacpi_rt_ctx.fadt.sci_int, &sci_ctx
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_kernel_install_interrupt_handler(
        g_uacpi_rt_ctx.fadt.sci_int, handle_sci, sci_ctx,
        &g_uacpi_rt_ctx.sci_handle
    );
    if (uacpi_unlikely_error(ret)) {
        uacpi_error(
            "unable to install SCI interrupt handler: %s",
            uacpi_status_to_string(ret)
        );
        return ret;
    }
    g_uacpi_rt_ctx.sci_handle_valid = UACPI_TRUE;

    g_uacpi_rt_ctx.global_lock_event = uacpi_create_native_event();
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
            uacpi_warn("platform has global lock but no FACS was provided");
            return ret;
        }
        g_uacpi_rt_ctx.has_global_lock = UACPI_TRUE;
    } else if (ret == UACPI_STATUS_HARDWARE_TIMEOUT) {
        // has_global_lock remains set to false
        uacpi_trace("platform has no global lock");
        ret = UACPI_STATUS_OK;
    }

    return ret;
}

void uacpi_deinitialize_events(void)
{
    struct gpe_interrupt_ctx *ctx, *next_ctx = g_gpe_interrupt_head;
    uacpi_size i;
    uacpi_bool locked;

    if (uacpi_is_hardware_reduced())
        return;

    g_gpes_finalized = UACPI_FALSE;

    if (g_uacpi_rt_ctx.sci_handle_valid) {
        uacpi_kernel_uninstall_interrupt_handler(
            handle_sci, g_uacpi_rt_ctx.sci_handle
        );
        g_uacpi_rt_ctx.sci_handle_valid = UACPI_FALSE;
    }

    /*
     * The locks don't exist if we never got far enough to initialize them, but
     * that also means there are no GPE blocks that we would need them for.
     */
    locked = uacpi_likely_success(event_config_lock());

    while (next_ctx) {
        struct gpe_block *block, *next_block;
        uacpi_bool is_sci_ctx;

        ctx = next_ctx;
        next_ctx = ctx->next;
        is_sci_ctx = ctx->irq == g_uacpi_rt_ctx.fadt.sci_int;

        next_block = ctx->gpe_head;
        while (next_block) {
            block = next_block;
            next_block = block->next;
            uninstall_gpe_block(block);
        }

        /*
         * Any other context is gone at this point, as it's removed along with
         * the last block that was using it.
         */
        if (is_sci_ctx) {
            unlink_gpe_interrupt_ctx(ctx);
            uacpi_free(ctx, sizeof(*ctx));
        }
    }

    if (locked)
        event_config_unlock();

    for (i = 0; i <= UACPI_FIXED_EVENT_MAX; ++i) {
        if (fixed_event_handlers[i].handler)
            uacpi_uninstall_fixed_event_handler(i);
    }

    if (g_gpe_state_slock != UACPI_NULL) {
        uacpi_kernel_free_spinlock(g_gpe_state_slock);
        g_gpe_state_slock = UACPI_NULL;
    }

    if (g_fixed_event_slock != UACPI_NULL) {
        uacpi_kernel_free_spinlock(g_fixed_event_slock);
        g_fixed_event_slock = UACPI_NULL;
    }

    uacpi_recursive_lock_deinit(&g_event_lock);
    uacpi_recursive_lock_deinit(&g_event_config_lock);

    g_gpe_interrupt_head = UACPI_NULL;
}

uacpi_status uacpi_install_fixed_event_handler(
    uacpi_fixed_event event, uacpi_interrupt_handler handler,
    uacpi_handle user
)
{
    uacpi_status ret;
    struct fixed_event_handler *ev;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = event_config_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ev = &fixed_event_handlers[event];

    flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);

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
    uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);
    event_config_unlock();
    return ret;
}

uacpi_status uacpi_uninstall_fixed_event_handler(
    uacpi_fixed_event event
)
{
    uacpi_status ret;
    struct fixed_event_handler *ev;
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_OK;

    ret = event_config_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ev = &fixed_event_handlers[event];

    /*
     * The handler is gone as far as everyone else is concerned as soon as the
     * lock is dropped: the event can't be enabled without one, and an
     * interrupt handler that is yet to look at it finds the event disabled.
     */
    flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);

    ret = set_event(event, UACPI_EVENT_DISABLED);
    if (uacpi_likely_success(ret)) {
        ev->handler = UACPI_NULL;
        ev->ctx = UACPI_NULL;
    }

    uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);

    if (uacpi_unlikely_error(ret))
        goto out;

    // The handler might still be in use by the interrupts that are in flight
    wait_for_work_completion_unlocked();

out:
    event_config_unlock();
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
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(event < 0 || event > UACPI_FIXED_EVENT_MAX))
        return UACPI_STATUS_INVALID_ARGUMENT;
    if (uacpi_is_hardware_reduced())
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_fixed_event_slock);

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
    uacpi_kernel_unlock_spinlock(g_fixed_event_slock, flags);
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
    uacpi_cpu_flags flags;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_NOT_FOUND;

    flags = uacpi_kernel_lock_spinlock(g_gpe_state_slock);

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
    uacpi_kernel_unlock_spinlock(g_gpe_state_slock, flags);
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

    if (uacpi_unlikely(uacpi_is_hardware_reduced()))
        return UACPI_STATUS_OK;

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
