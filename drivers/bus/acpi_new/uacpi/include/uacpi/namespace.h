#pragma once

#include <uacpi/types.h>
#include <uacpi/status.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

typedef struct uacpi_namespace_node uacpi_namespace_node;

uacpi_namespace_node *uacpi_namespace_root(void);

typedef enum uacpi_predefined_namespace {
    UACPI_PREDEFINED_NAMESPACE_ROOT = 0,
    UACPI_PREDEFINED_NAMESPACE_GPE,
    UACPI_PREDEFINED_NAMESPACE_PR,
    UACPI_PREDEFINED_NAMESPACE_SB,
    UACPI_PREDEFINED_NAMESPACE_SI,
    UACPI_PREDEFINED_NAMESPACE_TZ,
    UACPI_PREDEFINED_NAMESPACE_GL,
    UACPI_PREDEFINED_NAMESPACE_OS,
    UACPI_PREDEFINED_NAMESPACE_OSI,
    UACPI_PREDEFINED_NAMESPACE_REV,
    UACPI_PREDEFINED_NAMESPACE_MAX = UACPI_PREDEFINED_NAMESPACE_REV,
} uacpi_predefined_namespace;
uacpi_namespace_node *uacpi_namespace_get_predefined(
    uacpi_predefined_namespace
);

/*
 * Returns UACPI_TRUE if the provided 'node' is an alias.
 */
uacpi_bool uacpi_namespace_node_is_alias(uacpi_namespace_node *node);

uacpi_object_name uacpi_namespace_node_name(const uacpi_namespace_node *node);

/*
 * Returns the type of object stored at the namespace node.
 *
 * NOTE: due to the existance of the CopyObject operator in AML, the
 *       return value of this function is subject to TOCTOU bugs.
 */
uacpi_status uacpi_namespace_node_type(
    const uacpi_namespace_node *node, uacpi_object_type *out_type
);

/*
 * Returns UACPI_TRUE via 'out' if the type of the object stored at the
 * namespace node matches the provided value, UACPI_FALSE otherwise.
 *
 * NOTE: due to the existance of the CopyObject operator in AML, the
 *       return value of this function is subject to TOCTOU bugs.
 */
uacpi_status uacpi_namespace_node_is(
    const uacpi_namespace_node *node, uacpi_object_type type, uacpi_bool *out
);

/*
 * Returns UACPI_TRUE via 'out' if the type of the object stored at the
 * namespace node matches any of the type bits in the provided value,
 * UACPI_FALSE otherwise.
 *
 * NOTE: due to the existance of the CopyObject operator in AML, the
 *       return value of this function is subject to TOCTOU bugs.
 */
uacpi_status uacpi_namespace_node_is_one_of(
    const uacpi_namespace_node *node, uacpi_object_type_bits type_mask,
    uacpi_bool *out
);

uacpi_size uacpi_namespace_node_depth(const uacpi_namespace_node *node);

uacpi_namespace_node *uacpi_namespace_node_parent(
    uacpi_namespace_node *node
);

uacpi_status uacpi_namespace_node_find(
    uacpi_namespace_node *parent,
    const uacpi_char *path,
    uacpi_namespace_node **out_node
);

/*
 * Same as uacpi_namespace_node_find, except the search recurses upwards when
 * the namepath consists of only a single nameseg. Usually, this behavior is
 * only desired if resolving a namepath specified in an aml-provided object,
 * such as a package element.
 */
uacpi_status uacpi_namespace_node_resolve_from_aml_namepath(
    uacpi_namespace_node *scope,
    const uacpi_char *path,
    uacpi_namespace_node **out_node
);

typedef uacpi_iteration_decision (*uacpi_iteration_callback) (
    void *user, uacpi_namespace_node *node, uacpi_u32 node_depth
);

#define UACPI_MAX_DEPTH_ANY 0xFFFFFFFF

/*
 * Depth-first iterate the namespace starting at the first child of 'parent'.
 */
uacpi_status uacpi_namespace_for_each_child_simple(
    uacpi_namespace_node *parent, uacpi_iteration_callback callback, void *user
);

/*
 * Depth-first iterate the namespace starting at the first child of 'parent'.
 *
 * 'descending_callback' is invoked the first time a node is visited when
 * walking down. 'ascending_callback' is invoked the second time a node is
 * visited after we reach the leaf node without children and start walking up.
 * Either of the callbacks may be NULL, but not both at the same time.
 *
 * Only nodes matching 'type_mask' are passed to the callbacks.
 *
 * 'max_depth' is used to limit the maximum reachable depth from 'parent',
 * where 1 is only direct children of 'parent', 2 is children of first-level
 * children etc. Use UACPI_MAX_DEPTH_ANY or -1 to specify infinite depth.
 */
uacpi_status uacpi_namespace_for_each_child(
    uacpi_namespace_node *parent, uacpi_iteration_callback descending_callback,
    uacpi_iteration_callback ascending_callback,
    uacpi_object_type_bits type_mask, uacpi_u32 max_depth, void *user
);

/*
 * Retrieve the next peer namespace node of '*iter', or, if '*iter' is
 * UACPI_NULL, retrieve the first child of 'parent' instead. The resulting
 * namespace node is stored at '*iter'.
 *
 * This API can be used to implement an "iterator" version of the
 * for_each_child helpers.
 *
 * Example usage:
 *     void recurse(uacpi_namespace_node *parent) {
 *        uacpi_namespace_node *iter = UACPI_NULL;
 *
 *        while (uacpi_namespace_node_next(parent, &iter) == UACPI_STATUS_OK) {
 *             // Do something with iter...
 *             descending_callback(iter);
 *
 *             // Recurse down to walk over the children of iter
 *             recurse(iter);
 *        }
 *     }
 *
 * Prefer the for_each_child family of helpers if possible instead of this API
 * as they avoid recursion and/or the need to use dynamic data structures
 * entirely.
 */
uacpi_status uacpi_namespace_node_next(
    uacpi_namespace_node *parent, uacpi_namespace_node **iter
);

/*
 * Retrieve the next peer namespace node of '*iter', or, if '*iter' is
 * UACPI_NULL, retrieve the first child of 'parent' instead. The resulting
 * namespace node is stored at '*iter'. Only nodes which type matches one
 * of the types set in 'type_mask' are returned.
 *
 * See comment above 'uacpi_namespace_node_next' for usage examples.
 *
 * Prefer the for_each_child family of helpers if possible instead of this API
 * as they avoid recursion and/or the need to use dynamic data structures
 * entirely.
 */
uacpi_status uacpi_namespace_node_next_typed(
    uacpi_namespace_node *parent, uacpi_namespace_node **iter,
    uacpi_object_type_bits type_mask
);

const uacpi_char *uacpi_namespace_node_generate_absolute_path(
    const uacpi_namespace_node *node
);
void uacpi_free_absolute_path(const uacpi_char *path);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
