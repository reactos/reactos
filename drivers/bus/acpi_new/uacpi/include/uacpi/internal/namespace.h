#pragma once

#include <uacpi/types.h>
#include <uacpi/internal/shareable.h>
#include <uacpi/status.h>
#include <uacpi/namespace.h>

#ifndef UACPI_BAREBONES_MODE

#define UACPI_NAMESPACE_NODE_FLAG_ALIAS (1 << 0)

/*
 * This node has been uninstalled and has no object associated with it.
 *
 * This is used to handle edge cases where an object needs to reference
 * a namespace node, where the node might end up going out of scope before
 * the object lifetime ends.
 */
#define UACPI_NAMESPACE_NODE_FLAG_DANGLING (1u << 1)

/*
 * This node is method-local and must not be exposed via public API as its
 * lifetime is limited.
 */
#define UACPI_NAMESPACE_NODE_FLAG_TEMPORARY (1u << 2)

#define UACPI_NAMESPACE_NODE_PREDEFINED (1u << 31)

typedef struct uacpi_namespace_node {
    struct uacpi_shareable shareable;
    uacpi_object_name name;
    uacpi_u32 flags;
    uacpi_object *object;
    struct uacpi_namespace_node *parent;
    struct uacpi_namespace_node *child;
    struct uacpi_namespace_node *next;
} uacpi_namespace_node;

uacpi_status uacpi_initialize_namespace(void);
void uacpi_deinitialize_namespace(void);

uacpi_namespace_node *uacpi_namespace_node_alloc(uacpi_object_name name);
void uacpi_namespace_node_unref(uacpi_namespace_node *node);


uacpi_status uacpi_namespace_node_type_unlocked(
    const uacpi_namespace_node *node, uacpi_object_type *out_type
);
uacpi_status uacpi_namespace_node_is_one_of_unlocked(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask,
    uacpi_bool *out
);

uacpi_object *uacpi_namespace_node_get_object(const uacpi_namespace_node *node);

uacpi_object *uacpi_namespace_node_get_object_typed(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask
);

uacpi_status uacpi_namespace_node_acquire_object(
    const uacpi_namespace_node *node, uacpi_object **out_obj
);
uacpi_status uacpi_namespace_node_acquire_object_typed(
    const uacpi_namespace_node *node, uacpi_object_type_bits,
    uacpi_object **out_obj
);

uacpi_status uacpi_namespace_node_reacquire_object(
    uacpi_object *obj
);
uacpi_status uacpi_namespace_node_release_object(
    uacpi_object *obj
);

uacpi_status uacpi_namespace_node_install(
    uacpi_namespace_node *parent, uacpi_namespace_node *node
);
uacpi_status uacpi_namespace_node_uninstall(uacpi_namespace_node *node);

uacpi_namespace_node *uacpi_namespace_node_find_sub_node(
    uacpi_namespace_node *parent,
    uacpi_object_name name
);

enum uacpi_may_search_above_parent {
    UACPI_MAY_SEARCH_ABOVE_PARENT_NO,
    UACPI_MAY_SEARCH_ABOVE_PARENT_YES,
};

enum uacpi_permanent_only {
    UACPI_PERMANENT_ONLY_NO,
    UACPI_PERMANENT_ONLY_YES,
};

enum uacpi_should_lock {
    UACPI_SHOULD_LOCK_NO,
    UACPI_SHOULD_LOCK_YES,
};

uacpi_status uacpi_namespace_node_resolve(
    uacpi_namespace_node *scope, const uacpi_char *path, enum uacpi_should_lock,
    enum uacpi_may_search_above_parent, enum uacpi_permanent_only,
    uacpi_namespace_node **out_node
);

uacpi_status uacpi_namespace_do_for_each_child(
    uacpi_namespace_node *parent, uacpi_iteration_callback descending_callback,
    uacpi_iteration_callback ascending_callback,
    uacpi_object_type_bits, uacpi_u32 max_depth, enum uacpi_should_lock,
    enum uacpi_permanent_only, void *user
);

uacpi_bool uacpi_namespace_node_is_dangling(uacpi_namespace_node *node);
uacpi_bool uacpi_namespace_node_is_temporary(uacpi_namespace_node *node);
uacpi_bool uacpi_namespace_node_is_predefined(uacpi_namespace_node *node);

uacpi_status uacpi_namespace_read_lock(void);
uacpi_status uacpi_namespace_read_unlock(void);

uacpi_status uacpi_namespace_write_lock(void);
uacpi_status uacpi_namespace_write_unlock(void);

#endif // !UACPI_BAREBONES_MODE
