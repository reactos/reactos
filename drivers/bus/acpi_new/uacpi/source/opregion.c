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
        "%s (%s) operation region %s: %s\n",
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
            "<buffer of %zu bytes>\n", path,
            uacpi_address_space_to_string(space),
            UACPI_FMT64(offset), data.buffer.length
        );
        break;
    case UACPI_ADDRESS_SPACE_SMBUS:
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
        uacpi_trace(
            "%s [%s] %s[0x%016"UACPI_PRIX64"] = "
            "<buffer of %zu bytes>\n", type_str, path,
            uacpi_address_space_to_string(space),
            UACPI_FMT64(offset), data.buffer.length
        );
        break;
    case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
        uacpi_trace(
            "%s [%s] %s pins[%u..%u] = 0x%"UACPI_PRIX64"\n",
            type_str, path, uacpi_address_space_to_string(space),
            field->pin_offset, (field->pin_offset + field->bit_length) - 1,
            UACPI_FMT64(*data.integer)
        );
        break;
    default:
        uacpi_trace(
            "%s [%s] (%d bytes) %s[0x%016"UACPI_PRIX64"] = 0x%"UACPI_PRIX64"\n",
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

static uacpi_status region_run_reg(
    uacpi_namespace_node *node, uacpi_u8 connection_code
)
{
    uacpi_status ret;
    uacpi_namespace_node *reg_node;
    uacpi_object_array method_args;
    uacpi_object *reg_obj, *args[2];

    ret = uacpi_namespace_node_resolve(
        node->parent, "_REG", UACPI_SHOULD_LOCK_NO,
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

    args[0]->integer = uacpi_namespace_node_get_object(node)->op_region->space;
    args[1]->integer = connection_code;
    method_args.objects = args;
    method_args.count = 2;

    ret = uacpi_execute_control_method(
        reg_node, reg_obj->method, &method_args, UACPI_NULL
    );
    if (uacpi_unlikely_error(ret))
        uacpi_trace_region_error(node, "error during _REG execution for", ret);

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

    uacpi_object_ref(obj);
    uacpi_namespace_write_unlock();
    ret = handler->callback(UACPI_REGION_OP_ATTACH, &attach_data);
    uacpi_namespace_write_lock();

    if (uacpi_unlikely_error(ret)) {
        uacpi_object_unref(obj);
        return ret;
    }

    region->state_flags |= UACPI_OP_REGION_STATE_ATTACHED;
    region->user_context = attach_data.out_region_context;
    uacpi_object_unref(obj);
    return ret;
}

static void region_install_handler(
    uacpi_namespace_node *node, uacpi_address_space_handler *handler
)
{
    uacpi_operation_region *region;

    region = uacpi_namespace_node_get_object(node)->op_region;
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

    obj = uacpi_namespace_node_get_object_typed(
        node, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL))
        return;

    region = obj->op_region;

    handler = region->handler;
    if (handler == UACPI_NULL)
        return;

    link = find_previous_region_link(region);
    if (uacpi_unlikely(link == UACPI_NULL)) {
        uacpi_error("operation region @%p not in the handler@%p list(?)\n",
                    region, handler);
        goto out;
    } else if (link == region) {
        link = link->next;
        handler->regions = link;
    } else {
        link->next = region->next;
    }

out:
    if (region->state_flags & UACPI_OP_REGION_STATE_ATTACHED) {
        uacpi_region_detach_data detach_data = {
            .region_node = node,
            .region_context = region->user_context,
            .handler_context = handler->user_context,
        };

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

    if ((region->state_flags & UACPI_OP_REGION_STATE_REG_EXECUTED) &&
        unreg == UNREG_YES) {
        region_run_reg(node, ACPI_REG_DISCONNECT);
        region->state_flags &= ~UACPI_OP_REGION_STATE_REG_EXECUTED;
    }

    uacpi_address_space_handler_unref(region->handler);
    region->handler = UACPI_NULL;
    region->state_flags &= ~UACPI_OP_REGION_STATE_ATTACHED;
}

static uacpi_status upgrade_to_opregion_lock(void)
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

void uacpi_opregion_uninstall_handler(uacpi_namespace_node *node)
{
    if (uacpi_unlikely_error(upgrade_to_opregion_lock()))
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
};

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
            if (region->handler)
                region_uninstall_handler(node, UNREG_NO);

            region_install_handler(node, ctx->handler);
        } else {
            if (uacpi_unlikely(region->handler != ctx->handler)) {
                uacpi_trace_region_error(
                    node, "handler mismatch for",
                    UACPI_STATUS_INTERNAL_ERROR
                );
                return UACPI_ITERATION_DECISION_CONTINUE;
            }

            region_uninstall_handler(node, UNREG_NO);
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
    uacpi_u8 connection_code;
    uacpi_size reg_executed;
    uacpi_size reg_errors;
};

static uacpi_iteration_decision do_run_reg(
    void *opaque, uacpi_namespace_node *node, uacpi_u32 depth
)
{
    struct reg_run_ctx *ctx = opaque;
    uacpi_operation_region *region;
    uacpi_status ret;
    uacpi_bool was_regged;

    UACPI_UNUSED(depth);

    region = uacpi_namespace_node_get_object(node)->op_region;

    if (region->space != ctx->space)
        return UACPI_ITERATION_DECISION_CONTINUE;

    was_regged = region->state_flags & UACPI_OP_REGION_STATE_REG_EXECUTED;
    if (was_regged == (ctx->connection_code == ACPI_REG_CONNECT))
        return UACPI_ITERATION_DECISION_CONTINUE;

    ret = region_run_reg(node, ctx->connection_code);
    if (ctx->connection_code == ACPI_REG_DISCONNECT)
        region->state_flags &= ~UACPI_OP_REGION_STATE_REG_EXECUTED;

    if (ret == UACPI_STATUS_NOT_FOUND)
        return UACPI_ITERATION_DECISION_CONTINUE;

    if (ctx->connection_code == ACPI_REG_CONNECT)
        region->state_flags |= UACPI_OP_REGION_STATE_REG_EXECUTED;

    ctx->reg_executed++;

    if (uacpi_unlikely_error(ret)) {
        ctx->reg_errors++;
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static uacpi_status reg_or_unreg_all_opregions(
    uacpi_namespace_node *device_node, enum uacpi_address_space space,
    uacpi_u8 connection_code
)
{
    uacpi_address_space_handlers *handlers;
    uacpi_bool is_connect;
    enum uacpi_permanent_only perm_only;
    struct reg_run_ctx ctx = {
        .space = space,
        .connection_code = connection_code,
    };

    handlers = uacpi_node_get_address_space_handlers(device_node);
    if (uacpi_unlikely(handlers == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    is_connect = connection_code == ACPI_REG_CONNECT;
    if (uacpi_unlikely(is_connect &&
                       find_handler(handlers, space) == UACPI_NULL))
        return UACPI_STATUS_NO_HANDLER;

    /*
     * We want to unreg non-permanent opregions as well, however,
     * registering them is handled separately and should not be
     * done by us.
     */
    perm_only = is_connect ? UACPI_PERMANENT_ONLY_YES : UACPI_PERMANENT_ONLY_NO;

    uacpi_namespace_do_for_each_child(
        device_node, do_run_reg, UACPI_NULL,
        UACPI_OBJECT_OPERATION_REGION_BIT, UACPI_MAX_DEPTH_ANY,
        UACPI_SHOULD_LOCK_NO, perm_only, &ctx
    );

    uacpi_trace(
        "%sactivated all '%s' opregions controlled by '%.4s', "
        "%zu _REG() calls (%zu errors)\n",
        connection_code == ACPI_REG_CONNECT ? "" : "de",
        uacpi_address_space_to_string(space),
        device_node->name.text, ctx.reg_executed, ctx.reg_errors
    );
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

    ret = reg_or_unreg_all_opregions(device_node, space, ACPI_REG_CONNECT);

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

    iter_ctx.handler = new_handler;
    iter_ctx.action = OPREGION_ITER_ACTION_INSTALL;

    uacpi_namespace_do_for_each_child(
        device_node, do_install_or_uninstall_handler, UACPI_NULL,
        UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY, UACPI_SHOULD_LOCK_NO,
        UACPI_PERMANENT_ONLY_YES, &iter_ctx
    );

    if (!space_needs_reg(space))
        goto out;

     /*
      * Installing an early address space handler, obviously not possible to
      * execute any _REG methods here. Just return and hope that it is either
      * a global address space handler, or a handler installed by a user who
      * will run uacpi_reg_all_opregions manually after loading/initializing
      * the namespace.
      */
    if (g_uacpi_rt_ctx.init_level < UACPI_INIT_LEVEL_NAMESPACE_LOADED)
        goto out;

    // Init level is NAMESPACE_INITIALIZED, so we can safely run _REG now
    ret = reg_or_unreg_all_opregions(
        device_node, space, ACPI_REG_CONNECT
    );

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
    uacpi_namespace_node *device_node,
    enum uacpi_address_space space
)
{
    uacpi_status ret;
    uacpi_address_space_handlers *handlers;
    uacpi_address_space_handler *handler = UACPI_NULL, *prev_handler;
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

    handler = find_handler(handlers, space);
    if (uacpi_unlikely(handler == UACPI_NULL)) {
        ret = UACPI_STATUS_NO_HANDLER;
        goto out;
    }

    iter_ctx.handler = handler;
    iter_ctx.action = OPREGION_ITER_ACTION_UNINSTALL;

    uacpi_namespace_do_for_each_child(
        device_node, do_install_or_uninstall_handler, UACPI_NULL,
        UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY, UACPI_SHOULD_LOCK_NO,
        UACPI_PERMANENT_ONLY_NO, &iter_ctx
    );

    prev_handler = handlers->head;

    // Are we the last linked handler?
    if (prev_handler == handler) {
        handlers->head = handler->next;
        goto out_unreg;
    }

    // Nope, we're somewhere in the middle. Do a search.
    while (prev_handler) {
        if (prev_handler->next == handler) {
            prev_handler->next = handler->next;
            goto out;
        }

        prev_handler = prev_handler->next;
    }

out_unreg:
    if (space_needs_reg(space))
        reg_or_unreg_all_opregions(device_node, space, ACPI_REG_DISCONNECT);

out:
    if (handler != UACPI_NULL)
        uacpi_address_space_handler_unref(handler);

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

    ret = upgrade_to_opregion_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    region = uacpi_namespace_node_get_object(node)->op_region;
    ret = UACPI_STATUS_NOT_FOUND;

    while (parent) {
        handlers = uacpi_node_get_address_space_handlers(parent);
        if (handlers != UACPI_NULL) {
            handler = find_handler(handlers, region->space);

            if (handler != UACPI_NULL) {
                region_install_handler(node, handler);
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

    if (region_run_reg(node, ACPI_REG_CONNECT) != UACPI_STATUS_NOT_FOUND)
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

    ret = upgrade_to_opregion_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_opregion_attach(field->region);
    if (uacpi_unlikely_error(ret)) {
        uacpi_trace_region_error(
            field->region, "unable to attach", ret
        );
        goto out;
    }

    obj = uacpi_namespace_node_get_object_typed(
        field->region, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

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
            "0x%"UACPI_PRIX64"] at 0x%"UACPI_PRIX64" (idx=%u, width=%d)\n",
            path, UACPI_FMT64(region->offset),
            UACPI_FMT64(region->offset + region->length),
            UACPI_FMT64(abs_offset), offset, field->access_width_bytes
        );
        uacpi_free_dynamic_string(path);
        ret = UACPI_STATUS_AML_OUT_OF_BOUNDS_INDEX;
        goto out;
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
         */
        if (offset >= 12 && offset < 16) {
            handler_data.pcc.buffer = (uacpi_data_view){
                .bytes = region->internal_buffer,
                .length = region->length,
            };

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
        goto out;
    }

    if (orig_op == UACPI_REGION_OP_READ) {
        switch (region->space) {
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

out:
    uacpi_recursive_lock_release(&g_opregion_lock);
    return ret;
}

#endif // !UACPI_BAREBONES_MODE
