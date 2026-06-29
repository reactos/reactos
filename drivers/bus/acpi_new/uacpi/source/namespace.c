#include <uacpi/namespace.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/types.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/interpreter.h>
#include <uacpi/internal/opregion.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/kernel_api.h>

#ifndef UACPI_BAREBONES_MODE

#define UACPI_REV_VALUE 2
#define UACPI_OS_VALUE "Microsoft Windows NT"

#define MAKE_PREDEFINED(c0, c1, c2, c3)          \
    {                                            \
        .name.text = { c0, c1, c2, c3 },         \
        .flags = UACPI_NAMESPACE_NODE_PREDEFINED \
    }

static uacpi_namespace_node
predefined_namespaces[UACPI_PREDEFINED_NAMESPACE_MAX + 1] = {
    [UACPI_PREDEFINED_NAMESPACE_ROOT] = MAKE_PREDEFINED('\\', 0, 0, 0),
    [UACPI_PREDEFINED_NAMESPACE_GPE] = MAKE_PREDEFINED('_', 'G', 'P', 'E'),
    [UACPI_PREDEFINED_NAMESPACE_PR] = MAKE_PREDEFINED('_', 'P', 'R', '_'),
    [UACPI_PREDEFINED_NAMESPACE_SB] = MAKE_PREDEFINED('_', 'S', 'B', '_'),
    [UACPI_PREDEFINED_NAMESPACE_SI] = MAKE_PREDEFINED('_', 'S', 'I', '_'),
    [UACPI_PREDEFINED_NAMESPACE_TZ] = MAKE_PREDEFINED('_', 'T', 'Z', '_'),
    [UACPI_PREDEFINED_NAMESPACE_GL] = MAKE_PREDEFINED('_', 'G', 'L', '_'),
    [UACPI_PREDEFINED_NAMESPACE_OS] = MAKE_PREDEFINED('_', 'O', 'S', '_'),
    [UACPI_PREDEFINED_NAMESPACE_OSI] = MAKE_PREDEFINED('_', 'O', 'S', 'I'),
    [UACPI_PREDEFINED_NAMESPACE_REV] = MAKE_PREDEFINED('_', 'R', 'E', 'V'),
};

static struct uacpi_rw_lock namespace_lock;

uacpi_status uacpi_namespace_read_lock(void)
{
    return uacpi_rw_lock_read(&namespace_lock);
}

uacpi_status uacpi_namespace_read_unlock(void)
{
    return uacpi_rw_unlock_read(&namespace_lock);
}

uacpi_status uacpi_namespace_write_lock(void)
{
    return uacpi_rw_lock_write(&namespace_lock);
}

uacpi_status uacpi_namespace_write_unlock(void)
{
    return uacpi_rw_unlock_write(&namespace_lock);
}

static uacpi_object *make_object_for_predefined(
    enum uacpi_predefined_namespace ns
)
{
    uacpi_object *obj;

    switch (ns) {
    case UACPI_PREDEFINED_NAMESPACE_ROOT:
        /*
         * The real root object is stored in the global context, whereas the \
         * node gets a placeholder uninitialized object instead. This is to
         * protect against CopyObject(JUNK, \), so that all of the opregion and
         * notify handlers are preserved if AML decides to do that.
         */
        g_uacpi_rt_ctx.root_object = uacpi_create_object(UACPI_OBJECT_DEVICE);
        if (uacpi_unlikely(g_uacpi_rt_ctx.root_object == UACPI_NULL))
            return UACPI_NULL;

        obj = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
        break;

    case UACPI_PREDEFINED_NAMESPACE_OS:
        obj = uacpi_create_object(UACPI_OBJECT_STRING);
        if (uacpi_unlikely(obj == UACPI_NULL))
            return obj;

        obj->buffer->text = uacpi_kernel_alloc(sizeof(UACPI_OS_VALUE));
        if (uacpi_unlikely(obj->buffer->text == UACPI_NULL)) {
            uacpi_object_unref(obj);
            return UACPI_NULL;
        }

        obj->buffer->size = sizeof(UACPI_OS_VALUE);
        uacpi_memcpy(obj->buffer->text, UACPI_OS_VALUE, obj->buffer->size);
        break;

    case UACPI_PREDEFINED_NAMESPACE_REV:
        obj = uacpi_create_object(UACPI_OBJECT_INTEGER);
        if (uacpi_unlikely(obj == UACPI_NULL))
            return obj;

        obj->integer = UACPI_REV_VALUE;
        break;

    case UACPI_PREDEFINED_NAMESPACE_GL:
        obj = uacpi_create_object(UACPI_OBJECT_MUTEX);
        if (uacpi_likely(obj != UACPI_NULL)) {
            uacpi_shareable_ref(obj->mutex);
            g_uacpi_rt_ctx.global_lock_mutex = obj->mutex;
        }
        break;

    case UACPI_PREDEFINED_NAMESPACE_OSI:
        obj = uacpi_create_object(UACPI_OBJECT_METHOD);
        if (uacpi_unlikely(obj == UACPI_NULL))
            return obj;

        obj->method->native_call = UACPI_TRUE;
        obj->method->handler = uacpi_osi;
        obj->method->args = 1;
        break;

    default:
        obj = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
        break;
    }

    return obj;
}

static void namespace_node_detach_object(uacpi_namespace_node *node)
{
    uacpi_object *object;

    object = uacpi_namespace_node_get_object(node);
    if (object != UACPI_NULL) {
        if (object->type == UACPI_OBJECT_OPERATION_REGION)
            uacpi_opregion_uninstall_handler(node);

        uacpi_object_unref(node->object);
        node->object = UACPI_NULL;
    }
}

static void free_namespace_node(uacpi_handle handle)
{
    uacpi_namespace_node *node = handle;

    if (uacpi_likely(!uacpi_namespace_node_is_predefined(node))) {
        uacpi_free(node, sizeof(*node));
        return;
    }

    node->flags = UACPI_NAMESPACE_NODE_PREDEFINED;
    node->object = UACPI_NULL;
    node->parent = UACPI_NULL;
    node->child = UACPI_NULL;
    node->next = UACPI_NULL;
}

uacpi_status uacpi_initialize_namespace(void)
{
    enum uacpi_predefined_namespace ns;
    uacpi_object *obj;
    uacpi_namespace_node *node;
    uacpi_status ret;

    ret = uacpi_rw_lock_init(&namespace_lock);
    if (uacpi_unlikely_error(ret))
        return ret;

    for (ns = 0; ns <= UACPI_PREDEFINED_NAMESPACE_MAX; ns++) {
        node = &predefined_namespaces[ns];
        uacpi_shareable_init(node);

        obj = make_object_for_predefined(ns);
        if (uacpi_unlikely(obj == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        node->object = uacpi_create_internal_reference(
            UACPI_REFERENCE_KIND_NAMED, obj
        );
        if (uacpi_unlikely(node->object == UACPI_NULL)) {
            uacpi_object_unref(obj);
            return UACPI_STATUS_OUT_OF_MEMORY;
        }

        uacpi_object_unref(obj);
    }

    for (ns = UACPI_PREDEFINED_NAMESPACE_GPE;
         ns <= UACPI_PREDEFINED_NAMESPACE_MAX; ns++) {

        /*
         * Skip the installation of \_OSI if it was disabled by user.
         * We still create the object, but it's not attached to the namespace.
         */
        if (ns == UACPI_PREDEFINED_NAMESPACE_OSI &&
            uacpi_check_flag(UACPI_FLAG_NO_OSI))
            continue;

        uacpi_namespace_node_install(
            uacpi_namespace_root(), &predefined_namespaces[ns]
        );
    }

    return UACPI_STATUS_OK;
}

void uacpi_deinitialize_namespace(void)
{
    uacpi_status ret;
    uacpi_namespace_node *current, *next = UACPI_NULL;
    uacpi_u32 depth = 1;

    current = uacpi_namespace_root();

    ret = uacpi_namespace_write_lock();

    while (depth) {
        next = next == UACPI_NULL ? current->child : next->next;

        /*
         * The previous value of 'next' was the last child of this subtree,
         * we can now remove the entire scope of 'current->child'
         */
        if (next == UACPI_NULL) {
            depth--;

            // Wipe the subtree
            while (current->child != UACPI_NULL)
                uacpi_namespace_node_uninstall(current->child);

            // Reset the pointers back as if this iteration never happened
            next = current;
            current = current->parent;

            continue;
        }

        /*
         * We have more nodes to process, proceed to the next one, either the
         * child of the 'next' node, if one exists, or its peer
         */
        if (next->child) {
            depth++;
            current = next;
            next = UACPI_NULL;
        }

        // This node has no children, move on to its peer
    }

    namespace_node_detach_object(uacpi_namespace_root());
    free_namespace_node(uacpi_namespace_root());

    if (ret == UACPI_STATUS_OK)
        uacpi_namespace_write_unlock();

    uacpi_object_unref(g_uacpi_rt_ctx.root_object);
    g_uacpi_rt_ctx.root_object = UACPI_NULL;

    uacpi_mutex_unref(g_uacpi_rt_ctx.global_lock_mutex);
    g_uacpi_rt_ctx.global_lock_mutex = UACPI_NULL;

    uacpi_rw_lock_deinit(&namespace_lock);
}

uacpi_namespace_node *uacpi_namespace_root(void)
{
    return &predefined_namespaces[UACPI_PREDEFINED_NAMESPACE_ROOT];
}

uacpi_namespace_node *uacpi_namespace_get_predefined(
    enum uacpi_predefined_namespace ns
)
{
    if (uacpi_unlikely(ns > UACPI_PREDEFINED_NAMESPACE_MAX)) {
        uacpi_warn("requested invalid predefined namespace %d\n", ns);
        return UACPI_NULL;
    }

    return &predefined_namespaces[ns];
}

uacpi_namespace_node *uacpi_namespace_node_alloc(uacpi_object_name name)
{
    uacpi_namespace_node *ret;

    ret = uacpi_kernel_alloc_zeroed(sizeof(*ret));
    if (uacpi_unlikely(ret == UACPI_NULL))
        return ret;

    uacpi_shareable_init(ret);
    ret->name = name;
    return ret;
}

void uacpi_namespace_node_unref(uacpi_namespace_node *node)
{
    uacpi_shareable_unref_and_delete_if_last(node, free_namespace_node);
}

uacpi_status uacpi_namespace_node_install(
    uacpi_namespace_node *parent,
    uacpi_namespace_node *node
)
{
    if (parent == UACPI_NULL)
        parent = uacpi_namespace_root();

    if (uacpi_unlikely(uacpi_namespace_node_is_dangling(node))) {
        uacpi_warn("attempting to install a dangling namespace node %.4s\n",
                   node->name.text);
        return UACPI_STATUS_NAMESPACE_NODE_DANGLING;
    }

    if (parent->child == UACPI_NULL) {
        parent->child = node;
    } else {
        uacpi_namespace_node *prev = parent->child;

        while (prev->next != UACPI_NULL)
            prev = prev->next;

        prev->next = node;
    }

    node->parent = parent;
    return UACPI_STATUS_OK;
}

uacpi_bool uacpi_namespace_node_is_alias(uacpi_namespace_node *node)
{
    return node->flags & UACPI_NAMESPACE_NODE_FLAG_ALIAS;
}

uacpi_bool uacpi_namespace_node_is_dangling(uacpi_namespace_node *node)
{
    return node->flags & UACPI_NAMESPACE_NODE_FLAG_DANGLING;
}

uacpi_bool uacpi_namespace_node_is_temporary(uacpi_namespace_node *node)
{
    return node->flags & UACPI_NAMESPACE_NODE_FLAG_TEMPORARY;
}

uacpi_bool uacpi_namespace_node_is_predefined(uacpi_namespace_node *node)
{
    return node->flags & UACPI_NAMESPACE_NODE_PREDEFINED;
}

uacpi_status uacpi_namespace_node_uninstall(uacpi_namespace_node *node)
{
    uacpi_namespace_node *prev;

    if (uacpi_unlikely(uacpi_namespace_node_is_dangling(node))) {
        uacpi_warn("attempting to uninstall a dangling namespace node %.4s\n",
                   node->name.text);
        return UACPI_STATUS_INTERNAL_ERROR;
    }

    /*
     * The way to trigger this is as follows:
     *
     * Method (FOO) {
     *     // Temporary device, will be deleted upon returning from FOO
     *     Device (\BAR) {
     *     }
     *
     *     //
     *     // Load TBL where TBL is:
     *     //     Scope (\BAR) {
     *     //         Name (TEST, 123)
     *     //     }
     *     //
     *     Load(TBL)
     * }
     *
     * In the above example, TEST is a permanent node attached by bad AML to a
     * temporary node created inside the FOO method at \BAR. The cleanup code
     * will attempt to remove the \BAR device upon exit from FOO, but that is
     * no longer possible as there's now a permanent child attached to it.
     */
    if (uacpi_unlikely(node->child != UACPI_NULL)) {
        uacpi_warn(
            "refusing to uninstall node %.4s with a child (%.4s)\n",
            node->name.text, node->child->name.text
        );
        return UACPI_STATUS_DENIED;
    }

    /*
     * Even though namespace_node is reference-counted it still has an 'invalid'
     * state that is entered after it is uninstalled from the global namespace.
     *
     * Reference counting is only needed to combat dangling pointer issues
     * whereas bad AML might try to prolong a local object lifetime by
     * returning it from a method, or CopyObject it somewhere. In that case the
     * namespace node object itself is still alive, but no longer has a valid
     * object associated with it.
     *
     * Example:
     *     Method (BAD) {
     *         OperationRegion(REG, SystemMemory, 0xDEADBEEF, 4)
     *         Field (REG, AnyAcc, NoLock) {
     *             FILD, 8,
     *         }
     *
     *         Return (RefOf(FILD))
     *     }
     *
     *     // Local0 is now the sole owner of the 'FILD' object that under the
     *     // hood is still referencing the 'REG' operation region object from
     *     // the 'BAD' method.
     *     Local0 = DerefOf(BAD())
     *
     * This is done to prevent potential very deep recursion where an object
     * frees a namespace node that frees an attached object that frees a
     * namespace node as well as potential infinite cycles between a namespace
     * node and an object.
     */
    namespace_node_detach_object(node);

    prev = node->parent ? node->parent->child : UACPI_NULL;

    if (prev == node) {
        node->parent->child = node->next;
    } else {
        while (uacpi_likely(prev != UACPI_NULL) && prev->next != node)
            prev = prev->next;

        if (uacpi_unlikely(prev == UACPI_NULL)) {
            uacpi_warn(
                "trying to uninstall a node %.4s (%p) not linked to any peer\n",
                node->name.text, node
            );
            return UACPI_STATUS_INTERNAL_ERROR;
        }

        prev->next = node->next;
    }

    node->flags |= UACPI_NAMESPACE_NODE_FLAG_DANGLING;
    uacpi_namespace_node_unref(node);

    return UACPI_STATUS_OK;
}

uacpi_namespace_node *uacpi_namespace_node_find_sub_node(
    uacpi_namespace_node *parent,
    uacpi_object_name name
)
{
    if (parent == UACPI_NULL)
        parent = uacpi_namespace_root();

    uacpi_namespace_node *node = parent->child;

    while (node) {
        if (node->name.id == name.id)
            return node;

        node = node->next;
    }

    return UACPI_NULL;
}

static uacpi_object_name segment_to_name(
    const uacpi_char **string, uacpi_size *in_out_size
)
{
    uacpi_object_name out_name;
    const uacpi_char *cursor = *string;
    uacpi_size offset, bytes_left = *in_out_size;

    for (offset = 0; offset < 4; offset++) {
        if (bytes_left < 1 || *cursor == '.') {
            out_name.text[offset] = '_';
            continue;
        }

        out_name.text[offset] = *cursor++;
        bytes_left--;
    }

    *string = cursor;
    *in_out_size = bytes_left;
    return out_name;
}

uacpi_status uacpi_namespace_node_resolve(
    uacpi_namespace_node *parent, const uacpi_char *path,
    enum uacpi_should_lock should_lock,
    enum uacpi_may_search_above_parent may_search_above_parent,
    enum uacpi_permanent_only permanent_only,
    uacpi_namespace_node **out_node
)
{
    uacpi_namespace_node *cur_node = parent;
    uacpi_status ret = UACPI_STATUS_OK;
    const uacpi_char *cursor = path;
    uacpi_size bytes_left;
    uacpi_char prev_char = 0;
    uacpi_bool single_nameseg = UACPI_TRUE;

    if (cur_node == UACPI_NULL)
        cur_node = uacpi_namespace_root();

    bytes_left = uacpi_strlen(path);

    if (should_lock == UACPI_SHOULD_LOCK_YES) {
        ret = uacpi_namespace_read_lock();
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    for (;;) {
        if (bytes_left == 0)
            goto out;

        switch (*cursor) {
        case '\\':
            single_nameseg = UACPI_FALSE;

            if (prev_char == '^') {
                ret = UACPI_STATUS_INVALID_ARGUMENT;
                goto out;
            }

            cur_node = uacpi_namespace_root();
            break;
        case '^':
            single_nameseg = UACPI_FALSE;

            // Tried to go behind root
            if (uacpi_unlikely(cur_node == uacpi_namespace_root())) {
                ret = UACPI_STATUS_INVALID_ARGUMENT;
                goto out;
            }

            cur_node = cur_node->parent;
            break;
        default:
            break;
        }

        prev_char = *cursor;

        switch (prev_char) {
        case '^':
        case '\\':
            cursor++;
            bytes_left--;
            break;
        default:
            break;
        }

        if (prev_char != '^')
            break;
    }

    while (bytes_left != 0) {
        uacpi_object_name nameseg;

        if (*cursor == '.') {
            cursor++;
            bytes_left--;
        }

        nameseg = segment_to_name(&cursor, &bytes_left);
        if (bytes_left != 0 && single_nameseg)
            single_nameseg = UACPI_FALSE;

        cur_node = uacpi_namespace_node_find_sub_node(cur_node, nameseg);
        if (cur_node == UACPI_NULL) {
            if (may_search_above_parent == UACPI_MAY_SEARCH_ABOVE_PARENT_NO ||
                !single_nameseg)
                goto out;

            parent = parent->parent;

            while (parent) {
                cur_node = uacpi_namespace_node_find_sub_node(parent, nameseg);
                if (cur_node != UACPI_NULL)
                    goto out;

                parent = parent->parent;
            }

            goto out;
        }
    }

out:
    if (uacpi_unlikely(ret == UACPI_STATUS_INVALID_ARGUMENT)) {
        uacpi_warn("invalid path '%s'\n", path);
        goto out_read_unlock;
    }

    if (cur_node == UACPI_NULL) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out_read_unlock;
    }

    if (uacpi_namespace_node_is_temporary(cur_node) &&
        permanent_only == UACPI_PERMANENT_ONLY_YES) {
        uacpi_warn("denying access to temporary namespace node '%.4s'\n",
                   cur_node->name.text);
        ret = UACPI_STATUS_DENIED;
        goto out_read_unlock;
    }

    if (out_node != UACPI_NULL)
        *out_node = cur_node;

out_read_unlock:
    if (should_lock == UACPI_SHOULD_LOCK_YES)
        uacpi_namespace_read_unlock();
    return ret;
}

uacpi_status uacpi_namespace_node_find(
    uacpi_namespace_node *parent, const uacpi_char *path,
    uacpi_namespace_node **out_node
)
{
    return uacpi_namespace_node_resolve(
        parent, path, UACPI_SHOULD_LOCK_YES, UACPI_MAY_SEARCH_ABOVE_PARENT_NO,
        UACPI_PERMANENT_ONLY_YES, out_node
    );
}

uacpi_status uacpi_namespace_node_resolve_from_aml_namepath(
    uacpi_namespace_node *scope,
    const uacpi_char *path,
    uacpi_namespace_node **out_node
)
{
    return uacpi_namespace_node_resolve(
        scope, path, UACPI_SHOULD_LOCK_YES, UACPI_MAY_SEARCH_ABOVE_PARENT_YES,
        UACPI_PERMANENT_ONLY_YES, out_node
    );
}

uacpi_object *uacpi_namespace_node_get_object(const uacpi_namespace_node *node)
{
    if (node == UACPI_NULL || node->object == UACPI_NULL)
        return UACPI_NULL;

    return uacpi_unwrap_internal_reference(node->object);
}

uacpi_object *uacpi_namespace_node_get_object_typed(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask
)
{
    uacpi_object *obj;

    obj = uacpi_namespace_node_get_object(node);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return obj;

    if (!uacpi_object_is_one_of(obj, type_mask))
        return UACPI_NULL;

    return obj;
}

uacpi_status uacpi_namespace_node_acquire_object_typed(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask,
    uacpi_object **out_obj
)
{
    uacpi_status ret;
    uacpi_object *obj;

    ret = uacpi_namespace_read_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    obj = uacpi_namespace_node_get_object(node);

    if (uacpi_unlikely(obj == UACPI_NULL) ||
        !uacpi_object_is_one_of(obj, type_mask)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        goto out;
    }

    uacpi_object_ref(obj);
    *out_obj = obj;

out:
    uacpi_namespace_read_unlock();
    return ret;
}

uacpi_status uacpi_namespace_node_acquire_object(
    const uacpi_namespace_node *node, uacpi_object **out_obj
)
{
    return uacpi_namespace_node_acquire_object_typed(
        node, UACPI_OBJECT_ANY_BIT, out_obj
    );
}

enum action {
    ACTION_REACQUIRE,
    ACTION_PUT,
};

static uacpi_status object_mutate_refcount(
    uacpi_object *obj, void (*cb)(uacpi_object*)
)
{
   uacpi_status ret = UACPI_STATUS_OK;

    if (uacpi_likely(!uacpi_object_is(obj, UACPI_OBJECT_REFERENCE))) {
        cb(obj);
        return ret;
    }

    /*
     * Reference objects must be (un)referenced under at least a read lock, as
     * this requires walking down the entire reference chain and dropping each
     * object ref-count by 1. This might race with the interpreter and
     * object_replace_child in case an object in the chain is CopyObject'ed
     * into.
     */
    ret = uacpi_namespace_read_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    cb(obj);

    uacpi_namespace_read_unlock();
    return ret;
}

uacpi_status uacpi_namespace_node_reacquire_object(
    uacpi_object *obj
)
{
    return object_mutate_refcount(obj, uacpi_object_ref);
}

uacpi_status uacpi_namespace_node_release_object(uacpi_object *obj)
{
    return object_mutate_refcount(obj, uacpi_object_unref);
}

uacpi_object_name uacpi_namespace_node_name(const uacpi_namespace_node *node)
{
    return node->name;
}

uacpi_status uacpi_namespace_node_type_unlocked(
    const uacpi_namespace_node *node, uacpi_object_type *out_type
)
{
    uacpi_object *obj;

    if (uacpi_unlikely(node == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    obj = uacpi_namespace_node_get_object(node);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_NOT_FOUND;

    *out_type = obj->type;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_namespace_node_type(
    const uacpi_namespace_node *node, uacpi_object_type *out_type
)
{
    uacpi_status ret;

    ret = uacpi_namespace_read_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_namespace_node_type_unlocked(node, out_type);

    uacpi_namespace_read_unlock();
    return ret;
}

uacpi_status uacpi_namespace_node_is_one_of_unlocked(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask, uacpi_bool *out
)
{
    uacpi_object *obj;

    if (uacpi_unlikely(node == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    obj = uacpi_namespace_node_get_object(node);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_NOT_FOUND;

    *out = uacpi_object_is_one_of(obj, type_mask);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_namespace_node_is_one_of(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask,
    uacpi_bool *out
)
{
    uacpi_status ret;

    ret = uacpi_namespace_read_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_namespace_node_is_one_of_unlocked(node,type_mask, out);

    uacpi_namespace_read_unlock();
    return ret;
}

uacpi_status uacpi_namespace_node_is(
    const uacpi_namespace_node *node, uacpi_object_type type, uacpi_bool *out
)
{
    return uacpi_namespace_node_is_one_of(
        node, 1u << type, out
    );
}

uacpi_status uacpi_namespace_do_for_each_child(
    uacpi_namespace_node *node, uacpi_iteration_callback descending_callback,
    uacpi_iteration_callback ascending_callback,
    uacpi_object_type_bits type_mask, uacpi_u32 max_depth,
    enum uacpi_should_lock should_lock,
    enum uacpi_permanent_only permanent_only, void *user
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    uacpi_iteration_decision decision;
    uacpi_iteration_callback cb;
    uacpi_bool walking_up = UACPI_FALSE, matches = UACPI_FALSE;
    uacpi_u32 depth = 1;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(descending_callback == UACPI_NULL &&
                       ascending_callback == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (uacpi_unlikely(node == UACPI_NULL || max_depth == 0))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (should_lock == UACPI_SHOULD_LOCK_YES) {
        ret = uacpi_namespace_read_lock();
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    if (node->child == UACPI_NULL)
        goto out;

    node = node->child;

    while (depth) {
        uacpi_namespace_node_is_one_of_unlocked(node, type_mask, &matches);
        if (!matches) {
            decision = UACPI_ITERATION_DECISION_CONTINUE;
            goto do_next;
        }

        if (permanent_only == UACPI_PERMANENT_ONLY_YES &&
            uacpi_namespace_node_is_temporary(node)) {
            decision = UACPI_ITERATION_DECISION_NEXT_PEER;
            goto do_next;
        }

        cb = walking_up ? ascending_callback : descending_callback;
        if (cb != UACPI_NULL) {
            if (should_lock == UACPI_SHOULD_LOCK_YES) {
                ret = uacpi_namespace_read_unlock();
                if (uacpi_unlikely_error(ret))
                    return ret;
            }

            decision = cb(user, node, depth);
            if (decision == UACPI_ITERATION_DECISION_BREAK)
                return ret;

            if (should_lock == UACPI_SHOULD_LOCK_YES) {
                ret = uacpi_namespace_read_lock();
                if (uacpi_unlikely_error(ret))
                    return ret;
            }
        } else {
            decision = UACPI_ITERATION_DECISION_CONTINUE;
        }

    do_next:
        if (walking_up) {
            if (node->next) {
                node = node->next;
                walking_up = UACPI_FALSE;
                continue;
            }

            depth--;
            node = node->parent;
            continue;
        }

        switch (decision) {
        case UACPI_ITERATION_DECISION_CONTINUE:
            if ((depth != max_depth) && (node->child != UACPI_NULL)) {
                node = node->child;
                depth++;
                continue;
            }
            UACPI_FALLTHROUGH;
        case UACPI_ITERATION_DECISION_NEXT_PEER:
            walking_up = UACPI_TRUE;
            continue;
        default:
            ret = UACPI_STATUS_INVALID_ARGUMENT;
            goto out;
        }
    }

out:
    if (should_lock == UACPI_SHOULD_LOCK_YES)
        uacpi_namespace_read_unlock();
    return ret;
}

uacpi_status uacpi_namespace_for_each_child_simple(
    uacpi_namespace_node *parent, uacpi_iteration_callback callback, void *user
)
{
    return uacpi_namespace_do_for_each_child(
        parent, callback, UACPI_NULL, UACPI_OBJECT_ANY_BIT, UACPI_MAX_DEPTH_ANY,
        UACPI_SHOULD_LOCK_YES, UACPI_PERMANENT_ONLY_YES, user
    );
}

uacpi_status uacpi_namespace_for_each_child(
    uacpi_namespace_node *parent, uacpi_iteration_callback descending_callback,
    uacpi_iteration_callback ascending_callback,
    uacpi_object_type_bits type_mask, uacpi_u32 max_depth, void *user
)
{
    return uacpi_namespace_do_for_each_child(
        parent, descending_callback, ascending_callback, type_mask, max_depth,
        UACPI_SHOULD_LOCK_YES, UACPI_PERMANENT_ONLY_YES, user
    );
}

uacpi_status uacpi_namespace_node_next_typed(
    uacpi_namespace_node *parent, uacpi_namespace_node **iter,
    uacpi_object_type_bits type_mask
)
{
    uacpi_status ret;
    uacpi_bool is_one_of;
    uacpi_namespace_node *node;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(parent == UACPI_NULL && *iter == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_namespace_read_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    node = *iter;
    if (node == UACPI_NULL)
        node = parent->child;
    else
        node = node->next;

    for (; node != UACPI_NULL; node = node->next) {
        if (uacpi_namespace_node_is_temporary(node))
            continue;

        ret = uacpi_namespace_node_is_one_of_unlocked(
            node, type_mask, &is_one_of
        );
        if (uacpi_unlikely_error(ret))
            break;
        if (is_one_of)
            break;
    }

    uacpi_namespace_read_unlock();
    if (node == UACPI_NULL)
        return UACPI_STATUS_NOT_FOUND;

    if (uacpi_likely_success(ret))
        *iter = node;
    return ret;
}

uacpi_status uacpi_namespace_node_next(
    uacpi_namespace_node *parent, uacpi_namespace_node **iter
)
{
    return uacpi_namespace_node_next_typed(
        parent, iter, UACPI_OBJECT_ANY_BIT
    );
}

uacpi_size uacpi_namespace_node_depth(const uacpi_namespace_node *node)
{
    uacpi_size depth = 0;

    while (node->parent) {
        depth++;
        node = node->parent;
    }

    return depth;
}

uacpi_namespace_node *uacpi_namespace_node_parent(
    uacpi_namespace_node *node
)
{
    return node->parent;
}

const uacpi_char *uacpi_namespace_node_generate_absolute_path(
    const uacpi_namespace_node *node
)
{
    uacpi_size depth, offset;
    uacpi_size bytes_needed;
    uacpi_char *path;

    depth = uacpi_namespace_node_depth(node) + 1;

    // \ only needs 1 byte, the rest is 4 bytes
    bytes_needed = 1 + (depth - 1) * sizeof(uacpi_object_name);

    // \ and the first NAME don't need a '.', every other segment does
    bytes_needed += depth > 2 ? depth - 2 : 0;

    // Null terminator
    bytes_needed += 1;

    path = uacpi_kernel_alloc(bytes_needed);
    if (uacpi_unlikely(path == UACPI_NULL))
        return path;

    path[0] = '\\';

    offset = bytes_needed - 1;
    path[offset] = '\0';

    while (node != uacpi_namespace_root()) {
        offset -= sizeof(uacpi_object_name);
        uacpi_memcpy(&path[offset], node->name.text, sizeof(uacpi_object_name));

        node = node->parent;
        if (node != uacpi_namespace_root())
            path[--offset] = '.';
    }

    return path;
}

void uacpi_free_absolute_path(const uacpi_char *path)
{
    uacpi_free_dynamic_string(path);
}

#endif // !UACPI_BAREBONES_MODE
