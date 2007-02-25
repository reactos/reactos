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
 * $Id: tree.h,v 1.5 1999/12/09 05:38:52 kaz Exp $
 * $Name: austin_0_2 $
 */
/*
 * Modified for use in ReactOS by arty
 */

void udict_tree_init(udict_t *ud);
void udict_tree_insert(udict_t *ud, udict_node_t *node, const void *key);
void udict_tree_delete(udict_t *, udict_node_t *, udict_node_t **, udict_node_t **);
udict_node_t *udict_tree_lookup(udict_t *, const void *);
udict_node_t *udict_tree_lower_bound(udict_t *, const void *);
udict_node_t *udict_tree_upper_bound(udict_t *, const void *);
udict_node_t *udict_tree_first(udict_t *);
udict_node_t *udict_tree_last(udict_t *);
udict_node_t *udict_tree_next(udict_t *, udict_node_t *);
udict_node_t *udict_tree_prev(udict_t *, udict_node_t *);
void udict_tree_convert_to_list(udict_t *);
void udict_tree_convert_from_list(udict_t *);
void udict_tree_rotate_left(udict_node_t *, udict_node_t *);
void udict_tree_rotate_right(udict_node_t *, udict_node_t *);

#define tree_root_priv(T) ((T)->BalancedRoot.left)
#define tree_null_priv(L) ((L)->BalancedRoot.parent)
#define TREE_DEPTH_MAX 64
