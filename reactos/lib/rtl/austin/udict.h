/*
 * Austin---Astonishing Universal Search Tree Interface Novelty
 * Copyright (C) 2000 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id: udict.h,v 1.6 1999/12/09 07:32:48 kaz Exp $
 * $Name: austin_0_2 $
 */
/*
 * Modified for use in ReactOS by arty
 */

#ifndef UDICT_H
#define UDICT_H

#include <limits.h>

#define WIN32_NO_STATUS
#define _INC_SWPRINTF_INL_

#include <windows.h>
#include <ndk/ntndk.h>
#include "avl.h"

#define UDICT_COUNT_T_MAX ULONG_MAX
typedef unsigned long udict_count_t;

typedef unsigned int udict_alg_id_t;

#define UDICT_LIST	0
#define UDICT_BST	1
#define UDICT_REDBLACK	2
#define UDICT_SPLAY	3
#define UDICT_AVL	4

typedef enum {
    udict_bst,
    udict_list,
    udict_other
} udict_algkind_t;

typedef enum {
    udict_red,
    udict_black
} udict_rb_color_t;

typedef enum {
    udict_balanced = 0,
    udict_leftheavy = 1,
    udict_rightheavy = 2
} udict_avl_balance_t;

typedef union {
    int udict_dummy;
    udict_rb_color_t udict_rb_color;
    udict_avl_balance_t udict_avl_balance;
} udict_algdata_t;

typedef struct _RTL_BALANCED_LINKS udict_node_t;

typedef int (*udict_compare_t)(const void *, const void *);
typedef udict_node_t *(*udict_nodealloc_t)(void *);
typedef void (*udict_nodefree_t)(void *, udict_node_t *);

typedef struct _RTL_AVL_TABLE udict_t;

typedef struct udict_operations {
    void (*udict_init)(udict_t *);
    void (*udict_insert)(udict_t *, udict_node_t *, const void *);
    void (*udict_delete)(udict_t *, udict_node_t *);
    udict_node_t *(*udict_lookup)(udict_t *, const void *);
    udict_node_t *(*udict_lower_bound)(udict_t *, const void *);
    udict_node_t *(*udict_upper_bound)(udict_t *, const void *);
    udict_node_t *(*udict_first)(udict_t *);
    udict_node_t *(*udict_last)(udict_t *);
    udict_node_t *(*udict_next)(udict_t *, udict_node_t *);
    udict_node_t *(*udict_prev)(udict_t *, udict_node_t *);
    void (*udict_convert_to_list)(udict_t *);
    void (*udict_convert_from_list)(udict_t *);
    udict_algkind_t udict_kind;
} udict_operations_t;

/* non-virtual dict methods */
void udict_init(udict_t *, int, udict_count_t, udict_compare_t);
udict_t *udict_create(int, udict_count_t, udict_compare_t);
void udict_destroy(udict_t *);
void udict_convert_to(udict_t *, int);
udict_count_t udict_count(udict_t *);
int udict_isempty(udict_t *);
int udict_isfull(udict_t *);
int udict_alloc_insert(udict_t *, const void *, void *);
void udict_delete_free(udict_t *, udict_node_t *);
void udict_set_allocator(udict_t *, udict_nodealloc_t, udict_nodefree_t, void *);
void udict_allow_dupes(udict_t *);

/* non-virtual node methods */
void udict_node_init(udict_node_t *, void *);
udict_node_t *udict_node_create(void *);
void udict_node_destroy(udict_node_t *);
void *udict_node_getdata(udict_node_t *);
void udict_node_setdata(udict_node_t *, void *);
const void *udict_node_getkey(udict_node_t *);

/* virtual dict method wrappers */
void udict_insert(udict_t *, udict_node_t *, const void *);
void udict_delete(udict_t *, udict_node_t *);
udict_node_t *udict_lookup(udict_t *, const void *);
udict_node_t *udict_lower_bound(udict_t *, const void *);
udict_node_t *udict_upper_bound(udict_t *, const void *);
udict_node_t *udict_first(udict_t *);
udict_node_t *udict_last(udict_t *);
udict_node_t *udict_next(udict_t *, udict_node_t *);
udict_node_t *udict_prev(udict_t *, udict_node_t *);

#endif
