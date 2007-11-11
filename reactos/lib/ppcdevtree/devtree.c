/*
 *  FreeLoader
 *  Copyright (C) 2007       arty
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* A hardware describing device tree 
 * Upper layers will describe hardware they recognize in here.  Hal will need
 * this info to configure PICs, timers and such.
 *
 * When booting on openfirmware, we copy recognized devices from the device 
 * tree.  Other host types will configure it in different ways.
 *
 * Structure is a flat buffer stacked with PPC_DEVICE_NODEs, built as a tree.
 * Each node's child set is terminated by an empty (0) node.
 * Nodes are followed directly by properties and child nodes, which are
 * followed by their own child nodes, etc.
 *
 * A small management structure is used while the tree is being created, then
 * ignored when the tree is sent to upper layers.
 */

#include <string.h>
#include <ppcboot.h>

#define DT_ROUND_UP(x,round) (((x) + (round - 1)) & ~(round - 1))

PPPC_DEVICE_NODE PpcDevTreeGrow(PPPC_DEVICE_TREE tree, int newEntry)
{
    int newSize = 
        DT_ROUND_UP(tree->used_bytes + newEntry, tree->alloc_step);
    PPPC_DEVICE_NODE newArea;
    if (tree->alloc_size >= newSize) return tree->head;
    newArea = tree->allocFn(newSize);
    if (!newArea) return NULL;
    memcpy(newArea, tree->head, tree->alloc_size);
    tree->alloc_size = newSize;
    if (tree->active)
        tree->active = 
            (((char *)tree->active) - ((char *)tree->head)) + newArea;
    if (tree->head) tree->freeFn(tree->head);
    tree->head = newArea;
    return tree->head;
}

PPC_DT_BOOLEAN PpcDevTreeInitialize
(PPPC_DEVICE_TREE tree, int alloc_step, int align,
 PPC_DEVICE_ALLOC allocFn, PPC_DEVICE_FREE freeFn)
{
    tree->alloc_size = 0;
    tree->alloc_step = alloc_step;
    tree->used_bytes = 0;
    tree->align = align;
    tree->allocFn = allocFn;
    tree->freeFn = freeFn;
    /* Initialize */
    tree->active = tree->head = NULL;
    /* Add a root node */
    tree->head = PpcDevTreeGrow(tree, sizeof(PPC_DEVICE_NODE) + 1);
    if (!tree->head) return PPC_DT_FALSE;
    memset(tree->head, 0, sizeof(*tree->head) + 1);
    strcpy(tree->head->name, "/");
    tree->head->this_size = tree->head->total_size = 
        DT_ROUND_UP(sizeof(PPC_DEVICE_NODE)+1, tree->align);
    tree->active = tree->head;
    return tree->head != NULL;
}

PPPC_DEVICE_NODE PpcDevTreeGetRootNode(PPPC_DEVICE_TREE tree)
{
    return tree->head;
}

PPC_DT_BOOLEAN PpcDevTreeNodeIsChild
(PPPC_DEVICE_NODE parent, PPPC_DEVICE_NODE child)
{
    char *this_entry = (char *)parent;
    char *next_entry = ((char *)parent) + parent->total_size;
    char *want_entry = (char *)child;
    return want_entry > this_entry && want_entry < next_entry;
}

PPPC_DEVICE_NODE PpcDevTreeChildNode(PPPC_DEVICE_NODE parent)
{
    char *next_entry = ((char *)parent) + parent->this_size;
    PPPC_DEVICE_NODE next = (PPPC_DEVICE_NODE)next_entry;
    if (PpcDevTreeNodeIsChild(parent, next)) return next; else return NULL;
}

PPPC_DEVICE_NODE PpcDevTreeParentNode(PPPC_DEVICE_NODE child)
{
    char *parent = ((char *)child) - child->parent;
    if (!child->parent) return NULL; else return (PPPC_DEVICE_NODE)parent;
}

PPPC_DEVICE_NODE PpcDevTreeSiblingNode(PPPC_DEVICE_NODE this_entry)
{
    char *next_entry = ((char *)this_entry) + this_entry->total_size;
    PPPC_DEVICE_NODE next = (PPPC_DEVICE_NODE)next_entry;
    if (PpcDevTreeNodeIsChild(PpcDevTreeParentNode(this_entry), next)) 
        return next;
    else
        return NULL;
}

static
PPPC_DEVICE_NODE PpcDevTreeAllocChild(PPPC_DEVICE_TREE tree, int size)
{
    PPPC_DEVICE_NODE newHead = 
        PpcDevTreeGrow(tree, DT_ROUND_UP(size, tree->align));
    if (newHead == NULL) return NULL;
    newHead = (PPPC_DEVICE_NODE)
        (((char *)tree->active) + tree->active->total_size);
    memset(newHead, 0, size);
    return newHead;
}

PPC_DT_BOOLEAN PpcDevTreeAddProperty
(PPPC_DEVICE_TREE tree, int type, char *propname, char *propval, int proplen)
{
    int propname_len = DT_ROUND_UP(strlen(propname) + 1, tree->align);
    int entry_len = sizeof(PPC_DEVICE_NODE) + propname_len + proplen;
    PPPC_DEVICE_NODE newprop = PpcDevTreeAllocChild(tree, entry_len);
    if (!newprop) return PPC_DT_FALSE;
    newprop->type = type;
    newprop->parent = ((char *)newprop) - ((char *)tree->active);
    newprop->this_size = entry_len;
    newprop->value_offset = propname_len;
    newprop->value_size = proplen;
    strcpy(newprop->name, propname);
    memcpy(newprop->name + newprop->value_offset, propval, proplen);
    tree->active->total_size = 
        (((char *)newprop) + DT_ROUND_UP(newprop->this_size, tree->align)) - 
        ((char *)tree->active);
    return PPC_DT_TRUE;
}

PPC_DT_BOOLEAN PpcDevTreeAddDevice
(PPPC_DEVICE_TREE tree, int type, char *name)
{
    int entry_len = sizeof(PPC_DEVICE_NODE) + strlen(name);
    PPPC_DEVICE_NODE newprop = PpcDevTreeAllocChild(tree, entry_len);
    if (!newprop) return PPC_DT_FALSE;
    newprop->type = type;
    newprop->parent = ((char *)newprop) - ((char *)tree->active);
    newprop->this_size = newprop->total_size = 
        DT_ROUND_UP(entry_len, tree->align);
    strcpy(newprop->name, name);
    tree->active->total_size = 
        (((char *)newprop) + DT_ROUND_UP(newprop->this_size, tree->align)) - 
        ((char *)tree->active);
    tree->active = newprop;
    return PPC_DT_TRUE;
}
 
PPC_DT_BOOLEAN PpcDevTreeCloseDevice(PPPC_DEVICE_TREE tree)
{
    PPPC_DEVICE_NODE parent = PpcDevTreeParentNode(tree->active);
    if (!parent) return PPC_DT_FALSE;
    parent->total_size = tree->active->total_size + tree->active->parent;
    tree->active = parent;
    return PPC_DT_TRUE;
}

PPPC_DEVICE_NODE PpcDevTreeFindDevice
(PPPC_DEVICE_NODE root, int type, char *name)
{
    PPPC_DEVICE_NODE found = NULL;
    if (name && !strcmp(root->name, name)) return root;
    if (type && root->type == type) return root;
    for (root = PpcDevTreeChildNode(root); 
         root && !(found = PpcDevTreeFindDevice(root, type, name));
         root = PpcDevTreeSiblingNode(root));
    return found;
}

char *PpcDevTreeFindProperty
(PPPC_DEVICE_NODE root, int type, char *name, int *len)
{
    for (root = PpcDevTreeChildNode(root);
         root && 
             root->value_offset &&
             (!strcmp(root->name, name) && 
              (!type || root->type == type));
         root = PpcDevTreeSiblingNode(root));
    if (len) 
        *len = root->value_size;
    return root->name + root->value_offset;
}
