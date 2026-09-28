#include <uacpi/kernel_api.h>

#include <uacpi/internal/opregion.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/internal/interpreter.h>

#ifndef UACPI_BAREBONES_MODE

struct uacpi_recursive_lock g_opregion_lock;

uacpi_status uacpi_initialize_opregion(void)
{
    return uacpi_recursive_lock_init(&g_opregion_lock);
}

void uacpi_deinitialize_opregion(void)
{
    uacpi_recursive_lock_deinit(&g_opregion_lock);
}

void uacpi_trace_region_error(
    uacpi_namespace_node *node, uacpi_char *message, uacpi_status ret
)
{
    const uacpi_char *path, *space_string = "<unknown>";
    uacpi_object *obj;

    path = uacpi_namespace_node_generate_absolute_path(node);

    obj = uacpi_namespace_node_get_object_typed(
        node, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_likely(obj != UACPI_NULL))
        space_string = uacpi_address_space_to_string(obj->op_region->space);

    uacpi_error(
        "%s (%s) operation region %s: %s",
        message, space_string, path, uacpi_status_to_string(ret)
    );
    uacpi_free_dynamic_string(path);
}

static void trace_region_io(
    uacpi_field_unit *field, uacpi_address_space space, uacpi_u64 offset,
    uacpi_region_op op, union uacpi_opregion_io_data data
)
{
    const uacpi_char *path;
    const uacpi_char *type_str;

    if (!uacpi_should_log(UACPI_LOG_TRACE))
        return;

    switch (op) {
    case UACPI_REGION_OP_READ:
        type_str = "read from";
        break;
    case UACPI_REGION_OP_WRITE:
        type_str = "write to";
        break;
    default:
        type_str = "<INVALID-OP>";
    }

    path = uacpi_namespace_node_generate_absolute_path(field->region);

    switch (space) {
    case UACPI_ADDRESS_SPACE_IPMI:
    case UACPI_ADDRESS_SPACE_PRM:
    case UACPI_ADDRESS_SPACE_FFIXEDHW:
        uacpi_trace(
            "write-then-read from [%s] %s[0x%016"UACPI_PRIX64"] = "
            "<buffer of %zu bytes>", path,
            uacpi_address_space_to_string(space),
            UACPI_FMT64(offset), data.buffer.length
        );
        break;
    case UACPI_ADDRESS_SPACE_SMBUS:
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
        uacpi_trace(
            "%s [%s] %s[0x%016"UACPI_PRIX64"] = "
            "<buffer of %zu bytes>", type_str, path,
            uacpi_address_space_to_string(space),
            UACPI_FMT64(offset), data.buffer.length
        );
        break;
    case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
        uacpi_trace(
            "%s [%s] %s pins[%u..%u] = 0x%"UACPI_PRIX64,
            type_str, path, uacpi_address_space_to_string(space),
            field->pin_offset, (field->pin_offset + field->bit_length) - 1,
            UACPI_FMT64(*data.integer)
        );
        break;
    default:
        uacpi_trace(
            "%s [%s] (%d bytes) %s[0x%016"UACPI_PRIX64"] = 0x%"UACPI_PRIX64,
            type_str, path, field->access_width_bytes,
            uacpi_address_space_to_string(space),
            UACPI_FMT64(offset), UACPI_FMT64(*data.integer)
        );
        break;
    }

    uacpi_free_dynamic_string(path);
}

static uacpi_bool space_needs_reg(enum uacpi_address_space space)
{
    if (space == UACPI_ADDRESS_SPACE_SYSTEM_MEMORY ||
        space == UACPI_ADDRESS_SPACE_SYSTEM_IO ||
        space == UACPI_ADDRESS_SPACE_TABLE_DATA)
        return UACPI_FALSE;

    return UACPI_TRUE;
}

// Returns UACPI_STATUS_NOT_FOUND if 'scope' has no _REG
static uacpi_status run_reg(
    uacpi_namespace_node *scope, uacpi_u16 space, uacpi_u8 connection_code
)
{
    uacpi_status ret;
    uacpi_namespace_node *reg_node;
    uacpi_object_array method_args;
    uacpi_object *reg_obj, *args[2];

    ret = uacpi_namespace_node_resolve(
        scope, "_REG", UACPI_SHOULD_LOCK_NO,
        UACPI_MAY_SEARCH_ABOVE_PARENT_NO, UACPI_PERMANENT_ONLY_NO, &reg_node
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    reg_obj = uacpi_namespace_node_get_object_typed(
        reg_node, UACPI_OBJECT_METHOD_BIT
    );
    if (uacpi_unlikely(reg_obj == UACPI_NULL))
        return UACPI_STATUS_OK;

    args[0] = uacpi_create_object(UACPI_OBJECT_INTEGER);
    if (uacpi_unlikely(args[0] == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    args[1] = uacpi_create_object(UACPI_OBJECT_INTEGER);
    if (uacpi_unlikely(args[1] == UACPI_NULL)) {
        uacpi_object_unref(args[0]);
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    args[0]->integer = space;
    args[1]->integer = connection_code;
    method_args.objects = args;
    method_args.count = 2;

    ret = uacpi_execute_control_method(
        reg_node, reg_obj->method, &method_args, UACPI_NULL
    );
    if (uacpi_unlikely_error(ret)) {
        const uacpi_char *path;

        path = uacpi_namespace_node_generate_absolute_path(scope);
        uacpi_error(
            "error during _REG execution for %s (%s): %s", path,
            uacpi_address_space_to_string(space), uacpi_status_to_string(ret)
        );
        uacpi_free_dynamic_string(path);
    }

    uacpi_object_unref(args[0]);
    uacpi_object_unref(args[1]);
    return ret;
}

uacpi_address_space_handlers *uacpi_node_get_address_space_handlers(
    uacpi_namespace_node *node
)
{
    uacpi_object *object;

    if (node == uacpi_namespace_root())
        return g_uacpi_rt_ctx.root_object->address_space_handlers;

    object = uacpi_namespace_node_get_object(node);
    if (uacpi_unlikely(object == UACPI_NULL))
        return UACPI_NULL;

    switch (object->type) {
    case UACPI_OBJECT_DEVICE:
    case UACPI_OBJECT_PROCESSOR:
    case UACPI_OBJECT_THERMAL_ZONE:
        return object->address_space_handlers;
    default:
        return UACPI_NULL;
    }
}

static uacpi_address_space_handler *find_handler(
    uacpi_address_space_handlers *handlers,
    enum uacpi_address_space space
)
{
    uacpi_address_space_handler *handler = handlers->head;

    while (handler) {
        if (handler->space == space)
            return handler;

        handler = handler->next;
    }

    return UACPI_NULL;
}

static uacpi_operation_region *find_previous_region_link(
    uacpi_operation_region *region
)
{
    uacpi_address_space_handler *handler = region->handler;
    uacpi_operation_region *parent = handler->regions;

    if (parent == region)
        // This is the last attached region, it has no previous link
        return region;

    while (parent->next != region) {
        parent = parent->next;

        if (uacpi_unlikely(parent == UACPI_NULL))
            return UACPI_NULL;
    }

    return parent;
}

uacpi_status uacpi_opregion_attach(uacpi_namespace_node *node)
{
    uacpi_object *obj;
    uacpi_operation_region *region;
    uacpi_address_space_handler *handler;
    uacpi_status ret;
    uacpi_region_attach_data attach_data = { 0 };

    if (uacpi_namespace_node_is_dangling(node))
        return UACPI_STATUS_NAMESPACE_NODE_DANGLING;

    obj = uacpi_namespace_node_get_object_typed(
        node, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    region = obj->op_region;

    if (region->handler == UACPI_NULL)
        return UACPI_STATUS_NO_HANDLER;
    if (region->state_flags & UACPI_OP_REGION_STATE_ATTACHED)
        return UACPI_STATUS_OK;

    handler = region->handler;
    attach_data.region_node = node;

    switch (region->space) {
    case UACPI_ADDRESS_SPACE_PCC:
        if (region->length) {
            region->internal_buffer = uacpi_kernel_alloc_zeroed(region->length);
            if (uacpi_unlikely(region->internal_buffer == UACPI_NULL))
                return UACPI_STATUS_OUT_OF_MEMORY;
        }

        attach_data.pcc_info.buffer.bytes = region->internal_buffer;
        attach_data.pcc_info.buffer.length = region->length;
        attach_data.pcc_info.subspace_id = region->offset;
        break;
    case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
        attach_data.gpio_info.num_pins = region->length;
        break;
    default:
        attach_data.generic_info.base = region->offset;
        attach_data.generic_info.length = region->length;
        break;
    }

    attach_data.handler_context = handler->user_context;

    /*
     * The handler is invoked with the namespace unlocked, and is free to do
     * whatever it wants in the meantime, which includes evaluating a method
     * that gets rid of the region, as well as uninstalling itself. Keep both
     * of them alive until we're done.
     */
    uacpi_object_ref(obj);
    uacpi_shareable_ref(handler);

    uacpi_namespace_write_unlock();
    ret = handler->callback(UACPI_REGION_OP_ATTACH, &attach_data);
    uacpi_namespace_write_lock();

    if (uacpi_unlikely_error(ret))
        goto out;

    if (uacpi_unlikely(region->handler != handler)) {
        uacpi_region_detach_data detach_data = { 0 };

        /*
         * The region was taken away from the handler before it was done
         * attaching it. This is not something that the handler was told
         * about, as the region was not attached at the time, so do it now:
         * a region that was attached is always detached as well.
         */
        detach_data.region_node = node;
        detach_data.region_context = attach_data.out_region_context;
        detach_data.handler_context = handler->user_context;

        uacpi_namespace_write_unlock();
        handler->callback(UACPI_REGION_OP_DETACH, &detach_data);
        uacpi_namespace_write_lock();

        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    if (uacpi_unlikely(region->state_flags & UACPI_OP_REGION_STATE_ATTACHED)) {
        uacpi_region_detach_data detach_data = { 0 };

        /*
         * The region was attached once more in the meantime, which is what
         * happens if the handler ends up accessing the region that it's still
         * in the middle of attaching. It only gets to be attached once, and
         * that one was there first, so this is the one to be undone.
         */
        detach_data.region_node = node;
        detach_data.region_context = attach_data.out_region_context;
        detach_data.handler_context = handler->user_context;

        uacpi_namespace_write_unlock();
        handler->callback(UACPI_REGION_OP_DETACH, &detach_data);
        uacpi_namespace_write_lock();

        goto out;
    }

    region->state_flags |= UACPI_OP_REGION_STATE_ATTACHED;
    region->user_context = attach_data.out_region_context;

out:
    uacpi_address_space_handler_unref(handler);
    uacpi_object_unref(obj);
    return ret;
}

static void region_install_handler(
    uacpi_operation_region *region, uacpi_address_space_handler *handler
)
{
    region->handler = handler;
    uacpi_shareable_ref(handler);

    region->next = handler->regions;
    handler->regions = region;
}

enum unreg {
    UNREG_NO = 0,
    UNREG_YES,
};

static void region_uninstall_handler(
    uacpi_namespace_node *node, enum unreg unreg
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_address_space_handler *handler;
    uacpi_operation_region *region, *link;
    uacpi_bool was_attached;

    obj = uacpi_namespace_node_get_object_typed(
        node, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL))
        return;

    region = obj->op_region;

    handler = region->handler;
    if (handler == UACPI_NULL)
        return;

    // We're in the middle of doing this already, see below
    if (region->state_flags & UACPI_OP_REGION_STATE_DISCONNECTING)
        return;

    link = find_previous_region_link(region);
    if (uacpi_unlikely(link == UACPI_NULL)) {
        uacpi_error("operation region @%p not in the handler@%p list(?)",
                    region, handler);
        goto out;
    } else if (link == region) {
        link = link->next;
        handler->regions = link;
    } else {
        link->next = region->next;
    }

out:
    /*
     * Both the _REG method and the handler are free to do whatever they want
     * to the region once we invoke them, which includes getting rid of it,
     * e.g. by overwriting it via CopyObject. That leads right back here, so
     * make sure there's nothing left to do if it happens. It's also up to us
     * to keep the region alive until we're done.
     */
    uacpi_object_ref(obj);

    /*
     * This is done while the region still has its handler, as _REG is what
     * lets AML know that it's about to lose access to the region, and is
     * thus its last chance to make use of it.
     */
    if ((region->state_flags & UACPI_OP_REGION_STATE_REG_EXECUTED) &&
        unreg == UNREG_YES) {
        region->state_flags &= ~UACPI_OP_REGION_STATE_REG_EXECUTED;
        region->state_flags |= UACPI_OP_REGION_STATE_DISCONNECTING;

        run_reg(node->parent, region->space, ACPI_REG_DISCONNECT);

        region->state_flags &= ~UACPI_OP_REGION_STATE_DISCONNECTING;
    }

    /*
     * The region has no handler as far as anyone else is concerned from this
     * point on, which includes whatever the handler decides to do while it's
     * detaching it. Note that the region might've only been attached by the
     * _REG method above, so this is not something to check any earlier.
     */
    was_attached = region->state_flags & UACPI_OP_REGION_STATE_ATTACHED;

    region->handler = UACPI_NULL;
    region->state_flags &= ~UACPI_OP_REGION_STATE_ATTACHED;

    if (was_attached) {
        uacpi_region_detach_data detach_data = { 0 };

        detach_data.region_node = node;
        detach_data.region_context = region->user_context;
        detach_data.handler_context = handler->user_context;

        uacpi_shareable_ref(node);
        uacpi_namespace_write_unlock();

        ret = handler->callback(UACPI_REGION_OP_DETACH, &detach_data);

        uacpi_namespace_write_lock();
        uacpi_namespace_node_unref(node);

        if (uacpi_unlikely_error(ret)) {
            uacpi_trace_region_error(
                node, "error during handler detach for", ret
            );
        }
    }

    uacpi_address_space_handler_unref(handler);
    uacpi_object_unref(obj);
}

uacpi_status uacpi_upgrade_to_opregion_lock(void)
{
    uacpi_status ret;

    /*
     * Drop the namespace lock, and reacquire it after the opregion lock
     * so we keep the ordering with user API.
     */
    uacpi_namespace_write_unlock();

    ret = uacpi_recursive_lock_acquire(&g_opregion_lock);
    uacpi_namespace_write_lock();
    return ret;
}

void uacpi_release_opregion_lock(void)
{
    uacpi_recursive_lock_release(&g_opregion_lock);
}

void uacpi_opregion_uninstall_handler(uacpi_namespace_node *node)
{
    if (uacpi_unlikely_error(uacpi_upgrade_to_opregion_lock()))
        return;

    region_uninstall_handler(node, UNREG_YES);

    uacpi_recursive_lock_release(&g_opregion_lock);
}

uacpi_bool uacpi_address_space_handler_is_default(
    uacpi_address_space_handler *handler
)
{
    return handler->flags & UACPI_ADDRESS_SPACE_HANDLER_DEFAULT;
}

enum opregion_iter_action {
    OPREGION_ITER_ACTION_UNINSTALL,
    OPREGION_ITER_ACTION_INSTALL,
};

struct opregion_iter_ctx {
    enum opregion_iter_action action;
    uacpi_address_space_handler *handler;

    // The handlers of the device that the above is installed for
    uacpi_address_space_handlers *handlers;
};

static uacpi_bool handler_is_installed(struct opregion_iter_ctx *ctx)
{
    return find_handler(ctx->handlers, ctx->handler->space) == ctx->handler;
}

static uacpi_iteration_decision do_install_or_uninstall_handler(
    uacpi_handle opaque, uacpi_namespace_node *node, uacpi_u32 depth
)
{
    struct opregion_iter_ctx *ctx = opaque;
    uacpi_address_space_handlers *handlers;
    uacpi_object *object;

    UACPI_UNUSED(depth);

    object = uacpi_namespace_node_get_object(node);
    if (object->type == UACPI_OBJECT_OPERATION_REGION) {
        uacpi_operation_region *region = object->op_region;

        if (region->space != ctx->handler->space)
            return UACPI_ITERATION_DECISION_CONTINUE;

        if (ctx->action == OPREGION_ITER_ACTION_INSTALL) {
            if (region->handler) {
                region_uninstall_handler(node, UNREG_NO);

                /*
                 * The previous handler was invoked by the above, and might've
                 * uninstalled the one that we're in the middle of installing.
                 * There's nothing left for us to do in that case.
                 */
                if (uacpi_unlikely(!handler_is_installed(ctx)))
                    return UACPI_ITERATION_DECISION_BREAK;

                /*
                 * The previous handler was just told that the region is being
                 * detached, and is free to have done whatever it wanted to it
                 * in response, which includes getting rid of it. Make sure
                 * that there's still something to install the handler for.
                 */
                object = uacpi_namespace_node_get_object_typed(
                    node, UACPI_OBJECT_OPERATION_REGION_BIT
                );
                if (uacpi_unlikely(object == UACPI_NULL))
                    return UACPI_ITERATION_DECISION_CONTINUE;

                region = object->op_region;
                if (uacpi_unlikely(region->space != ctx->handler->space ||
                                   region->handler != UACPI_NULL))
                    return UACPI_ITERATION_DECISION_CONTINUE;
            }

            region_install_handler(region, ctx->handler);
        } else {
            /*
             * A region that has no handler is not ours to uninstall, and is
             * nothing to complain about either: it's one that was left this
             * way by a closer handler that is gone by now, or one that is in
             * the middle of being detached by whoever has called us.
             */
            if (region->handler == UACPI_NULL)
                return UACPI_ITERATION_DECISION_CONTINUE;

            if (uacpi_unlikely(region->handler != ctx->handler)) {
                uacpi_trace_region_error(
                    node, "handler mismatch for",
                    UACPI_STATUS_INTERNAL_ERROR
                );
                return UACPI_ITERATION_DECISION_CONTINUE;
            }

            region_uninstall_handler(node, UNREG_YES);
        }

        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    handlers = uacpi_node_get_address_space_handlers(node);
    if (handlers == UACPI_NULL)
        return UACPI_ITERATION_DECISION_CONTINUE;

    // Device already has a handler for this space installed
    if (find_handler(handlers, ctx->handler->space) != UACPI_NULL)
        return UACPI_ITERATION_DECISION_NEXT_PEER;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

struct reg_run_ctx {
    uacpi_u8 space;
    uacpi_size reg_executed;
    uacpi_size reg_errors;
};

static uacpi_iteration_decision do_run_reg(
    void *opaque, uacpi_namespace_node *node, uacpi_u32 depth
)
{
    struct reg_run_ctx *ctx = opaque;
    uacpi_object *obj;
    uacpi_operation_region *region;
    uacpi_status ret;

    UACPI_UNUSED(depth);

    obj = uacpi_namespace_node_get_object(node);
    region = obj->op_region;

    if (region->space != ctx->space)
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (region->state_flags & UACPI_OP_REGION_STATE_REG_EXECUTED)
        return UACPI_ITERATION_DECISION_CONTINUE;

    /*
     * A region that has no handler is not accessible, so there's nothing for
     * it to be connected to. The same goes for one that is in the middle of
     * being disconnected, as its handler is about to be gone.
     */
    if (region->handler == UACPI_NULL ||
        (region->state_flags & UACPI_OP_REGION_STATE_DISCONNECTING))
        return UACPI_ITERATION_DECISION_CONTINUE;

    /*
     * The _REG method is free to do whatever it wants to the region, which
     * includes getting rid of it, e.g. by overwriting it via CopyObject. Keep
     * it alive until we're done with it.
     */
    uacpi_object_ref(obj);

    ret = run_reg(node->parent, region->space, ACPI_REG_CONNECT);
    if (ret == UACPI_STATUS_NOT_FOUND)
        goto out;

    region->state_flags |= UACPI_OP_REGION_STATE_REG_EXECUTED;
    ctx->reg_executed++;

    if (uacpi_unlikely_error(ret))
        ctx->reg_errors++;

out:
    uacpi_object_unref(obj);
    return UACPI_ITERATION_DECISION_CONTINUE;
}

/*
 * Firmware might expect _REG to be executed for an address space that it has
 * no regions of, e.g. for an EC that it only ever accesses via SystemMemory.
 */
static void run_orphan_reg(
    uacpi_namespace_node *node, enum uacpi_address_space space
)
{
    uacpi_namespace_node *child = UACPI_NULL;

    for (;;) {
        uacpi_status ret;

        ret = uacpi_namespace_node_next_typed_unlocked(
            node, &child, UACPI_OBJECT_OPERATION_REGION_BIT
        );
        if (ret != UACPI_STATUS_OK)
            /*
             * No opregion child of this device is of this address space, so
             * this _REG is indeed orphan as far as we're concerned.
             */
            break;

        if (uacpi_namespace_node_get_object(child)->op_region->space == space)
            return;
    }

    run_reg(node, space, ACPI_REG_CONNECT);
}

static uacpi_bool node_has_handler(
    uacpi_namespace_node *node, enum uacpi_address_space space
)
{
    uacpi_address_space_handlers *handlers;

    handlers = uacpi_node_get_address_space_handlers(node);
    if (handlers == UACPI_NULL)
        return UACPI_FALSE;

    return find_handler(handlers, space) != UACPI_NULL;
}

static uacpi_iteration_decision do_run_orphan_ec_reg(
    void *opaque, uacpi_namespace_node *node, uacpi_u32 depth
)
{
    static const uacpi_char *const ec_ids[] = { "PNP0C09", UACPI_NULL };
    uacpi_namespace_node *device_node = opaque;
    uacpi_bool has_own_handler = UACPI_FALSE;

    UACPI_UNUSED(depth);

    if (uacpi_unlikely_error(uacpi_namespace_write_lock()))
        return UACPI_ITERATION_DECISION_BREAK;

    if (node != device_node) {
        has_own_handler = node_has_handler(
            node, UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER
        );
    }
    uacpi_namespace_write_unlock();

    // That handler takes care of everything underneath it
    if (has_own_handler)
        return UACPI_ITERATION_DECISION_NEXT_PEER;

    // This might execute AML, so it's done with the namespace unlocked
    if (!uacpi_device_matches_pnp_id(node, ec_ids))
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (uacpi_unlikely_error(uacpi_namespace_write_lock()))
        return UACPI_ITERATION_DECISION_BREAK;

    // The handler might've been uninstalled while the namespace was unlocked
    has_own_handler = node_has_handler(
        device_node, UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER
    );

    if (has_own_handler)
        run_orphan_reg(node, UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER);

    uacpi_namespace_write_unlock();
    return UACPI_ITERATION_DECISION_CONTINUE;
}

/*
 * An embedded controller is looked for by its PNP id, as the handler that it
 * uses is not necessarily installed to the EC device itself, e.g. it might be
 * installed to the root instead, so that the regions outside of the EC device
 * are covered as well.
 *
 * Must be called with the namespace write-locked. The lock is dropped while
 * the devices are looked at, and is held again by the time this returns.
 */
static void run_orphan_ec_regs(uacpi_namespace_node *device_node)
{
    uacpi_iteration_decision decision;

    uacpi_namespace_write_unlock();

    decision = do_run_orphan_ec_reg(device_node, device_node, 0);
    if (decision == UACPI_ITERATION_DECISION_CONTINUE) {
        uacpi_namespace_do_for_each_child(
            device_node, do_run_orphan_ec_reg, UACPI_NULL,
            UACPI_OBJECT_DEVICE_BIT, UACPI_MAX_DEPTH_ANY,
            UACPI_SHOULD_LOCK_YES, UACPI_PERMANENT_ONLY_YES, device_node
        );
    }

    uacpi_namespace_write_lock();
}

static uacpi_status reg_all_opregions(
    uacpi_namespace_node *device_node, enum uacpi_address_space space
)
{
    uacpi_address_space_handlers *handlers;
    struct reg_run_ctx ctx = { 0 };

    ctx.space = space;

    handlers = uacpi_node_get_address_space_handlers(device_node);
    if (uacpi_unlikely(handlers == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (uacpi_unlikely(find_handler(handlers, space) == UACPI_NULL))
        return UACPI_STATUS_NO_HANDLER;

    /*
     * Registering non-permanent opregions is handled separately and should
     * not be done by us.
     */
    uacpi_namespace_do_for_each_child(
        device_node, do_run_reg, UACPI_NULL,
        UACPI_OBJECT_OPERATION_REGION_BIT, UACPI_MAX_DEPTH_ANY,
        UACPI_SHOULD_LOCK_NO, UACPI_PERMANENT_ONLY_YES, &ctx
    );

    uacpi_trace(
        "activated all '%s' opregions controlled by '%.4s', "
        "%zu _REG() calls (%zu errors)",
        uacpi_address_space_to_string(space),
        device_node->name.text, ctx.reg_executed, ctx.reg_errors
    );

    if (space == UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER)
        run_orphan_ec_regs(device_node);

    /*
     * Unlike an EC, a GPIO controller has no common PNP id to be looked for
     * by. Its handler is normally installed to the controller itself though,
     * so that is the only one that is checked.
     */
    if (space == UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO)
        run_orphan_reg(device_node, space);

    return UACPI_STATUS_OK;
}

static uacpi_address_space_handlers *extract_handlers(
    uacpi_namespace_node *node
)
{
    uacpi_object *handlers_obj;

    if (node == uacpi_namespace_root())
        return g_uacpi_rt_ctx.root_object->address_space_handlers;

    handlers_obj = uacpi_namespace_node_get_object_typed(
        node,
        UACPI_OBJECT_DEVICE_BIT | UACPI_OBJECT_THERMAL_ZONE_BIT |
        UACPI_OBJECT_PROCESSOR_BIT
    );
    if (uacpi_unlikely(handlers_obj == UACPI_NULL))
        return UACPI_NULL;

    return handlers_obj->address_space_handlers;
}

uacpi_status uacpi_reg_all_opregions(
    uacpi_namespace_node *device_node,
    enum uacpi_address_space space
)
{
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    if (!space_needs_reg(space))
        return UACPI_STATUS_OK;

    ret = uacpi_recursive_lock_acquire(&g_opregion_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_namespace_write_lock();
    if (uacpi_unlikely_error(ret)) {
        uacpi_recursive_lock_release(&g_opregion_lock);
        return ret;
    }

    if (uacpi_unlikely(extract_handlers(device_node) == UACPI_NULL)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    ret = reg_all_opregions(device_node, space);

out:
    uacpi_namespace_write_unlock();
    uacpi_recursive_lock_release(&g_opregion_lock);
    return ret;
}

uacpi_status uacpi_install_address_space_handler_with_flags(
    uacpi_namespace_node *device_node, enum uacpi_address_space space,
    uacpi_region_handler handler, uacpi_handle handler_context,
    uacpi_u16 flags
)
{
    uacpi_status ret;
    uacpi_address_space_handlers *handlers;
    uacpi_address_space_handler *this_handler, *new_handler;
    struct opregion_iter_ctx iter_ctx;

    ret = uacpi_recursive_lock_acquire(&g_opregion_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_namespace_write_lock();
    if (uacpi_unlikely_error(ret)) {
        uacpi_recursive_lock_release(&g_opregion_lock);
        return ret;
    }

    handlers = extract_handlers(device_node);
    if (uacpi_unlikely(handlers == UACPI_NULL)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    this_handler = find_handler(handlers, space);
    if (this_handler != UACPI_NULL) {
        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    new_handler = uacpi_kernel_alloc(sizeof(*new_handler));
    if (new_handler == UACPI_NULL) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }
    uacpi_shareable_init(new_handler);

    new_handler->next = handlers->head;
    new_handler->space = space;
    new_handler->user_context = handler_context;
    new_handler->callback = handler;
    new_handler->regions = UACPI_NULL;
    new_handler->flags = flags;
    handlers->head = new_handler;

    /*
     * The handlers that some of the regions already have are invoked as the
     * regions are taken away from them below, and are free to do whatever they
     * want in response, which includes uninstalling the handler that we have
     * just put on the list. Keep it alive until we're done.
     */
    uacpi_shareable_ref(new_handler);

    iter_ctx.handler = new_handler;
    iter_ctx.handlers = handlers;
    iter_ctx.action = OPREGION_ITER_ACTION_INSTALL;

    uacpi_namespace_do_for_each_child(
        device_node, do_install_or_uninstall_handler, UACPI_NULL,
        UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY, UACPI_SHOULD_LOCK_NO,
        UACPI_PERMANENT_ONLY_YES, &iter_ctx
    );

    if (uacpi_unlikely(!handler_is_installed(&iter_ctx))) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out_unref;
    }

    if (!space_needs_reg(space))
        goto out_unref;

     /*
      * Installing an early address space handler, obviously not possible to
      * execute any _REG methods here. Just return and hope that it is either
      * a global address space handler, or a handler installed by a user who
      * will run uacpi_reg_all_opregions manually after loading/initializing
      * the namespace.
      */
    if (g_uacpi_rt_ctx.init_level < UACPI_INIT_LEVEL_NAMESPACE_LOADED)
        goto out_unref;

    // Init level is NAMESPACE_INITIALIZED, so we can safely run _REG now
    ret = reg_all_opregions(device_node, space);

out_unref:
    uacpi_address_space_handler_unref(new_handler);
out:
    uacpi_namespace_write_unlock();
    uacpi_recursive_lock_release(&g_opregion_lock);
    return ret;
}

uacpi_status uacpi_install_address_space_handler(
    uacpi_namespace_node *device_node, enum uacpi_address_space space,
    uacpi_region_handler handler, uacpi_handle handler_context
)
{
    return uacpi_install_address_space_handler_with_flags(
        device_node, space, handler, handler_context, 0
    );
}

uacpi_status uacpi_uninstall_address_space_handler(
    uacpi_namespace_node *device_node, enum uacpi_address_space space,
    uacpi_region_handler handler
)
{
    uacpi_status ret;
    uacpi_address_space_handlers *handlers;
    uacpi_address_space_handler *this_handler, *prev_handler;
    struct opregion_iter_ctx iter_ctx;

    ret = uacpi_recursive_lock_acquire(&g_opregion_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_namespace_write_lock();
    if (uacpi_unlikely_error(ret)) {
        uacpi_recursive_lock_release(&g_opregion_lock);
        return ret;
    }

    handlers = extract_handlers(device_node);
    if (uacpi_unlikely(handlers == UACPI_NULL)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    this_handler = find_handler(handlers, space);
    if (uacpi_unlikely(this_handler == UACPI_NULL)) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    // This is somebody else's handler, and is thus not ours to uninstall
    if (uacpi_unlikely(this_handler->callback != handler)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    /*
     * Both the handler and the _REG methods are invoked as the regions are
     * taken away below, and are free to do whatever they want in response.
     * Make sure that the handler is no longer there to be found by then, be
     * it by a region that is created in the meantime, or by the handler
     * itself in case it decides to uninstall itself as it's told to detach
     * a region. The reference that the device had is now ours, which is what
     * keeps the handler alive until we're done.
     */
    prev_handler = handlers->head;

    // Are we the last linked handler?
    if (prev_handler == this_handler) {
        handlers->head = this_handler->next;
    } else {
        // Nope, we're somewhere in the middle. Do a search.
        while (prev_handler->next != this_handler)
            prev_handler = prev_handler->next;

        prev_handler->next = this_handler->next;
    }

    iter_ctx.handler = this_handler;
    iter_ctx.handlers = handlers;
    iter_ctx.action = OPREGION_ITER_ACTION_UNINSTALL;

    uacpi_namespace_do_for_each_child(
        device_node, do_install_or_uninstall_handler, UACPI_NULL,
        UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY, UACPI_SHOULD_LOCK_NO,
        UACPI_PERMANENT_ONLY_NO, &iter_ctx
    );

    uacpi_address_space_handler_unref(this_handler);

out:
    uacpi_namespace_write_unlock();
    uacpi_recursive_lock_release(&g_opregion_lock);
    return ret;
}

uacpi_status uacpi_initialize_opregion_node(uacpi_namespace_node *node)
{
    uacpi_status ret;
    uacpi_namespace_node *parent = node->parent;
    uacpi_operation_region *region;
    uacpi_address_space_handlers *handlers;
    uacpi_address_space_handler *handler;

    ret = uacpi_upgrade_to_opregion_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    region = uacpi_namespace_node_get_object(node)->op_region;
    ret = UACPI_STATUS_NOT_FOUND;

    while (parent) {
        handlers = uacpi_node_get_address_space_handlers(parent);
        if (handlers != UACPI_NULL) {
            handler = find_handler(handlers, region->space);

            if (handler != UACPI_NULL) {
                region_install_handler(region, handler);
                ret = UACPI_STATUS_OK;
                break;
            }
        }

        parent = parent->parent;
    }

    if (ret != UACPI_STATUS_OK)
        goto out;
    if (!space_needs_reg(region->space))
        goto out;
    if (uacpi_get_current_init_level() < UACPI_INIT_LEVEL_NAMESPACE_LOADED)
        goto out;

    if (run_reg(node->parent, region->space, ACPI_REG_CONNECT) !=
        UACPI_STATUS_NOT_FOUND)
        region->state_flags |= UACPI_OP_REGION_STATE_REG_EXECUTED;

out:
    uacpi_recursive_lock_release(&g_opregion_lock);
    return ret;
}

uacpi_bool uacpi_is_buffer_access_address_space(uacpi_address_space space)
{
    switch (space) {
    case UACPI_ADDRESS_SPACE_SMBUS:
    case UACPI_ADDRESS_SPACE_IPMI:
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
    case UACPI_ADDRESS_SPACE_PRM:
    case UACPI_ADDRESS_SPACE_FFIXEDHW:
        return UACPI_TRUE;
    default:
        return UACPI_FALSE;
    }
}

static uacpi_bool space_needs_bounds_checking(uacpi_address_space space)
{
    return !uacpi_is_buffer_access_address_space(space) &&
        space != UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO;
}

/*
 * A command is sent over a PCC subspace as soon as its command field is
 * written, which is any field that starts where the command does: it doesn't
 * have to be as wide, nor is it limited to it.
 *
 * A field is written one access at a time though, and a PCC one is only
 * supposed to be accessed byte by byte, so what was written is not all there
 * until the last access of the field. That's the only one to send it for.
 */
#define PCC_COMMAND_OFFSET 8
#define PCC_COMMAND_SIZE 4

static uacpi_bool pcc_write_completes_command(
    uacpi_field_unit *field, uacpi_u32 offset
)
{
    uacpi_u32 access_bits, last_bit, last_offset;

    if (field->byte_offset < PCC_COMMAND_OFFSET ||
        field->byte_offset >= PCC_COMMAND_OFFSET + PCC_COMMAND_SIZE)
        return UACPI_FALSE;

    access_bits = field->access_width_bytes * 8;
    last_bit = field->bit_offset_within_first_byte + field->bit_length - 1;

    last_offset = field->byte_offset;
    last_offset += (last_bit / access_bits) * field->access_width_bytes;

    return offset == last_offset;
}

uacpi_status uacpi_dispatch_opregion_io(
    uacpi_field_unit *field, uacpi_u32 offset, uacpi_region_op op,
    union uacpi_opregion_io_data data
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_operation_region *region;
    uacpi_address_space_handler *handler;
    uacpi_address_space space;
    uacpi_u64 abs_offset, offset_end = offset;
    uacpi_bool is_oob = UACPI_FALSE;
    uacpi_region_op orig_op = op;

    union {
        uacpi_region_rw_data rw;
        uacpi_region_pcc_send_data pcc;
        uacpi_region_gpio_rw_data gpio;
        uacpi_region_ipmi_rw_data ipmi;
        uacpi_region_ffixedhw_rw_data ffixedhw;
        uacpi_region_prm_rw_data prm;
        uacpi_region_serial_rw_data serial;
    } handler_data;

    ret = uacpi_opregion_attach(field->region);
    if (uacpi_unlikely_error(ret)) {
        uacpi_trace_region_error(
            field->region, "unable to attach", ret
        );
        return ret;
    }

    obj = uacpi_namespace_node_get_object_typed(
        field->region, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    region = obj->op_region;
    space = region->space;
    handler = region->handler;

    abs_offset = region->offset + offset;
    offset_end += field->access_width_bytes;

    if (uacpi_likely(space_needs_bounds_checking(region->space)))
        is_oob = region->length < offset_end || abs_offset < offset;
    if (uacpi_unlikely(is_oob)) {
        const uacpi_char *path;

        path = uacpi_namespace_node_generate_absolute_path(field->region);
        uacpi_error(
            "out-of-bounds access to opregion %s[0x%"UACPI_PRIX64"->"
            "0x%"UACPI_PRIX64"] at 0x%"UACPI_PRIX64" (idx=%u, width=%d)",
            path, UACPI_FMT64(region->offset),
            UACPI_FMT64(region->offset + region->length),
            UACPI_FMT64(abs_offset), offset, field->access_width_bytes
        );
        uacpi_free_dynamic_string(path);
        return UACPI_STATUS_AML_OUT_OF_BOUNDS_INDEX;
    }

    handler_data.rw.region_context = region->user_context;
    handler_data.rw.handler_context = handler->user_context;

    switch (region->space) {
    case UACPI_ADDRESS_SPACE_PCC: {
        uacpi_u8 *cursor;

        cursor = region->internal_buffer + offset;

        /*
         * Reads from PCC just return the current contents of the internal
         * buffer.
         */
        if (op == UACPI_REGION_OP_READ) {
            uacpi_memcpy_zerout(
                data.integer, cursor, sizeof(*data.integer),
                field->access_width_bytes
            );
            goto io_done;
        }

        uacpi_memcpy(cursor, data.integer, field->access_width_bytes);

        /*
         * Dispatch a PCC send command if this was a write to the command field
         *
         * ACPI 6.5: 14.3. Extended PCC Subspace Shared Memory Region
         *
         * Note that a PCC region covers whatever follows the signature that
         * the shared memory region starts with, so the offset of every field
         * is 4 bytes less than what it is there.
         */
        if (pcc_write_completes_command(field, offset)) {
            uacpi_memzero(&handler_data.pcc.buffer, sizeof(handler_data.pcc.buffer));
            handler_data.pcc.buffer.bytes = region->internal_buffer;
            handler_data.pcc.buffer.length = region->length;

            op = UACPI_REGION_OP_PCC_SEND;
            break;
        }

        // No dispatch needed, IO is done
        goto io_done;
    }
    case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
        handler_data.gpio.pin_offset = field->pin_offset;
        handler_data.gpio.num_pins = field->bit_length;
        handler_data.gpio.value = *data.integer;

        ret = uacpi_object_get_string_or_buffer(
            field->connection, &handler_data.gpio.connection
        );
        if (uacpi_unlikely_error(ret))
            goto io_done;

        op = op == UACPI_REGION_OP_READ ?
            UACPI_REGION_OP_GPIO_READ : UACPI_REGION_OP_GPIO_WRITE;
        break;
    case UACPI_ADDRESS_SPACE_IPMI:
        handler_data.ipmi.in_out_message = data.buffer;
        handler_data.ipmi.command = abs_offset;
        op = UACPI_REGION_OP_IPMI_COMMAND;
        break;
    case UACPI_ADDRESS_SPACE_FFIXEDHW:
        handler_data.ffixedhw.in_out_message = data.buffer;
        handler_data.ffixedhw.command = abs_offset;
        op = UACPI_REGION_OP_FFIXEDHW_COMMAND;
        break;
    case UACPI_ADDRESS_SPACE_PRM:
        handler_data.prm.in_out_message = data.buffer;
        op = UACPI_REGION_OP_PRM_COMMAND;
        break;
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
    case UACPI_ADDRESS_SPACE_SMBUS:
        ret = uacpi_object_get_string_or_buffer(
            field->connection, &handler_data.serial.connection
        );
        if (uacpi_unlikely_error(ret))
            goto io_done;

        handler_data.serial.command = abs_offset;
        handler_data.serial.in_out_buffer = data.buffer;
        handler_data.serial.access_attribute = field->attributes;

        switch (field->attributes) {
        case UACPI_ACCESS_ATTRIBUTE_BYTES:
        case UACPI_ACCESS_ATTRIBUTE_RAW_BYTES:
        case UACPI_ACCESS_ATTRIBUTE_RAW_PROCESS_BYTES:
            handler_data.serial.access_length = field->access_length;
            break;
        default:
            handler_data.serial.access_length = 0;
        }

        op = op == UACPI_REGION_OP_READ ?
            UACPI_REGION_OP_SERIAL_READ : UACPI_REGION_OP_SERIAL_WRITE;
        break;
    default:
        handler_data.rw.byte_width = field->access_width_bytes;
        handler_data.rw.offset = abs_offset;
        handler_data.rw.value = *data.integer;
        break;
    }

    uacpi_object_ref(obj);
    uacpi_namespace_write_unlock();

    ret = handler->callback(op, &handler_data);

    uacpi_namespace_write_lock();
    uacpi_object_unref(obj);

io_done:
    if (uacpi_unlikely_error(ret)) {
        uacpi_trace_region_error(field->region, "unable to perform IO", ret);
        return ret;
    }

    if (orig_op == UACPI_REGION_OP_READ) {
        switch (space) {
        case UACPI_ADDRESS_SPACE_PCC:
        case UACPI_ADDRESS_SPACE_IPMI:
        case UACPI_ADDRESS_SPACE_FFIXEDHW:
        case UACPI_ADDRESS_SPACE_PRM:
        case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
        case UACPI_ADDRESS_SPACE_SMBUS:
            break;
        case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
            *data.integer = handler_data.gpio.value;
            break;
        default:
            *data.integer = handler_data.rw.value;
            break;
        }
    }

    trace_region_io(field, space, abs_offset, orig_op, data);
    return ret;
}

#endif // !UACPI_BAREBONES_MODE
