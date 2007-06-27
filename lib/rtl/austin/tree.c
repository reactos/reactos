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
 * $Id: tree.c,v 1.8 1999/12/09 05:38:52 kaz Exp $
 * $Name: austin_0_2 $
 */
/*
 * Modified for use in ReactOS by arty
 */
#include "rtl.h"
#include "udict.h"
#include "tree.h"
#include "macros.h"

void udict_tree_delete(udict_t *ud, udict_node_t *node, udict_node_t **pswap, udict_node_t **pchild)
{
    udict_node_t *nil = tree_null_priv(ud), *child, *delparent = node->parent;
    udict_node_t *next = node, *nextparent;

    if( tree_root_priv(ud) == node )
	delparent = &ud->BalancedRoot;

    if (node->left != nil && node->right != nil) {
	next = udict_tree_next(ud, node);
	nextparent = next->parent;

	if( tree_root_priv(ud) == next )
	    nextparent = &ud->BalancedRoot;

	assert (next != nil);
	assert (next->parent != nil);
	assert (next->left == nil);

	/*
	 * First, splice out the successor from the tree completely, by
	 * moving up its right child into its place.
	 */

	child = next->right;
	child->parent = nextparent;

	if (nextparent->left == next) {
	    nextparent->left = child;
	} else {
	    assert (nextparent->right == next);
	    nextparent->right = child;
	}

	/*
	 * Now that the successor has been extricated from the tree, install it
	 * in place of the node that we want deleted.
	 */

	next->parent = delparent;
	next->left = node->left;
	next->right = node->right;
	next->left->parent = next;
	next->right->parent = next;

	if (delparent->left == node) {
	    delparent->left = next;
	} else {
	    assert (delparent->right == node);
	    delparent->right = next;
	}

    } else {
	assert (node != nil);
	assert (node->left == nil || node->right == nil);

	child = (node->left != nil) ? node->left : node->right;
	child->parent = delparent = node->parent;	    

	if (node == delparent->left) {
	    delparent->left = child;    
	} else {
	    assert (node == delparent->right);
	    delparent->right = child;
	}
    }

    node->parent = nil;
    node->right = nil;
    node->left = nil;

    ud->nodecount--;

    *pswap = next;
    *pchild = child;
}

udict_node_t *udict_tree_lookup(udict_t *ud, const void *_key)
{
    udict_node_t *root = tree_root_priv(ud);
    udict_node_t *nil = tree_null_priv(ud);
    int result;

    /* simple binary search adapted for trees that contain duplicate keys */

    while (root != nil) {
	result = ud->compare(ud, (void *)_key, key(root));
	if (result < 0)
	    root = root->left;
	else if (result > 0)
	    root = root->right;
	else {
	    return root;
	}
    }

    return 0;
}

udict_node_t *udict_tree_lower_bound(udict_t *ud, const void *_key)
{
    udict_node_t *root = tree_root_priv(ud);
    udict_node_t *nil = tree_null_priv(ud);
    udict_node_t *tentative = 0;

    while (root != nil) {
	int result = ud->compare(ud, (void *)_key, key(root));

	if (result > 0) {
	    root = root->right;
	} else if (result < 0) {
	    tentative = root;
	    root = root->left;
	} else {
	    return root;
	} 
    }
    
    return tentative;
}

udict_node_t *udict_tree_upper_bound(udict_t *ud, const void *_key)
{
    udict_node_t *root = tree_root_priv(ud);
    udict_node_t *nil = tree_null_priv(ud);
    udict_node_t *tentative = 0;

    while (root != nil) {
	int result = ud->compare(ud, (void *)_key, key(root));

	if (result < 0) {
	    root = root->left;
	} else if (result > 0) {
	    tentative = root;
	    root = root->right;
	} else {
	    return root;
	} 
    }
    
    return tentative;
}

udict_node_t *udict_tree_first(udict_t *ud)
{
    udict_node_t *nil = tree_null_priv(ud), *root = tree_root_priv(ud), *left;

    if (root != nil)
	while ((left = root->left) != nil)
	    root = left;

    return (root == nil) ? 0 : root;
}

udict_node_t *udict_tree_last(udict_t *ud)
{
    udict_node_t *nil = tree_null_priv(ud), *root = tree_root_priv(ud), *right;

    if (root != nil)
	while ((right = root->right) != nil)
	    root = right;

    return (root == nil) ? 0 : root;
}

udict_node_t *udict_tree_next(udict_t *ud, udict_node_t *curr)
{
    udict_node_t *nil = tree_null_priv(ud), *parent, *left;

    if (curr->right != nil) {
	curr = curr->right;
	while ((left = curr->left) != nil)
	    curr = left;
	return curr;
    }

    parent = curr->parent;

    while (parent != nil && curr == parent->right) {
	curr = parent;
	parent = curr->parent;
    }

    return (parent == nil) ? 0 : parent;
}

udict_node_t *udict_tree_prev(udict_t *ud, udict_node_t *curr)
{
    udict_node_t *nil = tree_null_priv(ud), *parent, *right;

    if (curr->left != nil) {
	curr = curr->left;
	while ((right = curr->right) != nil)
	    curr = right;
	return curr;
    }

    parent = curr->parent;

    while (parent != nil && curr == parent->left) {
	curr = parent;
	parent = curr->parent;
    }

    return (parent == nil) ? 0 : parent;
}

/*
 * Perform a ``left rotation'' adjustment on the tree.  The given parent node P
 * and its right child C are rearranged so that the P instead becomes the left
 * child of C.   The left subtree of C is inherited as the new right subtree
 * for P.  The ordering of the keys within the tree is thus preserved.
 */

void udict_tree_rotate_left(udict_node_t *child, udict_node_t *parent)
{
    udict_node_t *leftgrandchild, *grandpa;

    assert (parent->right == child);

    child = parent->right;
    parent->right = leftgrandchild = child->left;
    leftgrandchild->parent = parent;

    child->parent = grandpa = parent->parent;

    if (parent == grandpa->left) {
	grandpa->left = child;
    } else {
	assert (parent == grandpa->right);
	grandpa->right = child;
    }

    child->left = parent;
    parent->parent = child;
}


/*
 * This operation is the ``mirror'' image of rotate_left. It is
 * the same procedure, but with left and right interchanged.
 */

void udict_tree_rotate_right(udict_node_t *child, udict_node_t *parent)
{
    udict_node_t *rightgrandchild, *grandpa;

    assert (parent->left == child);

    parent->left = rightgrandchild = child->right;
    rightgrandchild->parent = parent;

    child->parent = grandpa = parent->parent;

    if (parent == grandpa->right) {
	grandpa->right = child;
    } else {
	assert (parent == grandpa->left);
	grandpa->left = child;
    }

    child->right = parent;
    parent->parent = child;
}

