#include <uacpi/internal/notify.h>
#include <uacpi/internal/shareable.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/kernel_api.h>

#ifndef UACPI_BAREBONES_MODE

static uacpi_handle notify_mutex;

uacpi_status uacpi_initialize_notify(void)
{
    notify_mutex = uacpi_kernel_create_mutex();
    if (uacpi_unlikely(notify_mutex == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

void uacpi_deinitialize_notify(void)
{
    if (notify_mutex != UACPI_NULL)
        uacpi_kernel_free_mutex(notify_mutex);

    notify_mutex = UACPI_NULL;
}

struct notification_ctx {
    uacpi_namespace_node *node;
    uacpi_u64 value;
    uacpi_object *node_object;
};

static void free_notification_ctx(struct notification_ctx *ctx)
{
    uacpi_namespace_node_release_object(ctx->node_object);
    uacpi_namespace_node_unref(ctx->node);
    uacpi_free(ctx, sizeof(*ctx));
}

static void do_notify(uacpi_handle opaque)
{
    struct notification_ctx *ctx = opaque;
    uacpi_device_notify_handler *handler;
    uacpi_bool did_notify_root = UACPI_FALSE;

    handler = ctx->node_object->handlers->notify_head;

    for (;;) {
        if (handler == UACPI_NULL) {
            if (did_notify_root) {
                free_notification_ctx(ctx);
                return;
            }

            handler = g_uacpi_rt_ctx.root_object->handlers->notify_head;
            did_notify_root = UACPI_TRUE;
            continue;
        }

        handler->callback(handler->user_context, ctx->node, ctx->value);
        handler = handler->next;
    }
}

uacpi_status uacpi_notify_all(uacpi_namespace_node *node, uacpi_u64 value)
{
    uacpi_status ret;
    struct notification_ctx *ctx;
    uacpi_object *node_object;

    node_object = uacpi_namespace_node_get_object_typed(
        node, UACPI_OBJECT_DEVICE_BIT | UACPI_OBJECT_THERMAL_ZONE_BIT |
              UACPI_OBJECT_PROCESSOR_BIT
    );
    if (uacpi_unlikely(node_object == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_acquire_native_mutex(notify_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (node_object->handlers->notify_head == UACPI_NULL &&
        g_uacpi_rt_ctx.root_object->handlers->notify_head == UACPI_NULL) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    ctx = uacpi_kernel_alloc(sizeof(*ctx));
    if (uacpi_unlikely(ctx == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }

    ctx->node = node;
    // In case this node goes out of scope
    uacpi_shareable_ref(node);

    ctx->value = value;
    ctx->node_object = uacpi_namespace_node_get_object(node);
    uacpi_object_ref(ctx->node_object);

    ret = uacpi_kernel_schedule_work(UACPI_WORK_NOTIFICATION, do_notify, ctx);
    if (uacpi_unlikely_error(ret)) {
        uacpi_warn("unable to schedule notification work: %s\n",
                   uacpi_status_to_string(ret));
        free_notification_ctx(ctx);
    }

out:
    uacpi_release_native_mutex(notify_mutex);
    return ret;
}

static uacpi_device_notify_handler *handler_container(
    uacpi_handlers *handlers, uacpi_notify_handler target_handler
)
{
    uacpi_device_notify_handler *handler = handlers->notify_head;

    while (handler) {
        if (handler->callback == target_handler)
            return handler;

        handler = handler->next;
    }

    return UACPI_NULL;
}

uacpi_status uacpi_install_notify_handler(
    uacpi_namespace_node *node, uacpi_notify_handler handler,
    uacpi_handle handler_context
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_handlers *handlers;
    uacpi_device_notify_handler *new_handler;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (node == uacpi_namespace_root()) {
        obj = g_uacpi_rt_ctx.root_object;
    } else {
        ret = uacpi_namespace_node_acquire_object_typed(
            node, UACPI_OBJECT_DEVICE_BIT | UACPI_OBJECT_THERMAL_ZONE_BIT |
                  UACPI_OBJECT_PROCESSOR_BIT, &obj
        );
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    ret = uacpi_acquire_native_mutex(notify_mutex);
    if (uacpi_unlikely_error(ret))
        goto out_no_mutex;

    uacpi_kernel_wait_for_work_completion();

    handlers = obj->handlers;

    if (handler_container(handlers, handler) != UACPI_NULL) {
        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    new_handler = uacpi_kernel_alloc_zeroed(sizeof(*new_handler));
    if (uacpi_unlikely(new_handler == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    new_handler->callback = handler;
    new_handler->user_context = handler_context;
    new_handler->next = handlers->notify_head;

    handlers->notify_head = new_handler;

out:
    uacpi_release_native_mutex(notify_mutex);
out_no_mutex:
    if (node != uacpi_namespace_root())
        uacpi_object_unref(obj);

    return ret;
}

uacpi_status uacpi_uninstall_notify_handler(
    uacpi_namespace_node *node, uacpi_notify_handler handler
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_handlers *handlers;
    uacpi_device_notify_handler *prev_handler, *containing = UACPI_NULL;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (node == uacpi_namespace_root()) {
        obj = g_uacpi_rt_ctx.root_object;
    } else {
        ret = uacpi_namespace_node_acquire_object_typed(
            node, UACPI_OBJECT_DEVICE_BIT | UACPI_OBJECT_THERMAL_ZONE_BIT |
                  UACPI_OBJECT_PROCESSOR_BIT, &obj
        );
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    ret = uacpi_acquire_native_mutex(notify_mutex);
    if (uacpi_unlikely_error(ret))
        goto out_no_mutex;

    uacpi_kernel_wait_for_work_completion();

    handlers = obj->handlers;

    containing = handler_container(handlers, handler);
    if (containing == UACPI_NULL) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out;
    }

    prev_handler = handlers->notify_head;

    // Are we the last linked handler?
    if (prev_handler == containing) {
        handlers->notify_head = containing->next;
        goto out;
    }

    // Nope, we're somewhere in the middle. Do a search.
    while (prev_handler) {
        if (prev_handler->next == containing) {
            prev_handler->next = containing->next;
            goto out;
        }

        prev_handler = prev_handler->next;
    }

out:
    uacpi_release_native_mutex(notify_mutex);
out_no_mutex:
    if (node != uacpi_namespace_root())
        uacpi_object_unref(obj);

    if (uacpi_likely_success(ret))
        uacpi_free(containing, sizeof(*containing));

    return ret;
}

#endif // !UACPI_BAREBONES_MODE
