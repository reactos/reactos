/*
 * Red-black search tree support
 *
 * Copyright 2009 Henri Verbeet
 * Copyright 2009 Andrew Riedi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_RBTREE_H
#define __WINE_WINE_RBTREE_H

#define WINE_RB_ENTRY_VALUE(element, type, field) \
    ((type *)((char *)(element) - offsetof(type, field)))

struct wine_rb_entry
{
    struct wine_rb_entry *left;
    struct wine_rb_entry *right;
    unsigned int flags;
};

struct wine_rb_stack
{
    struct wine_rb_entry ***entries;
    size_t count;
    size_t size;
};

struct wine_rb_functions
{
    void *(*alloc)(size_t size);
    void *(*realloc)(void *ptr, size_t size);
    void (*free)(void *ptr);
    int (*compare)(const void *key, const struct wine_rb_entry *entry);
};

struct wine_rb_tree
{
    const struct wine_rb_functions *functions;
    struct wine_rb_entry *root;
    struct wine_rb_stack stack;
};

typedef void (wine_rb_traverse_func_t)(struct wine_rb_entry *entry, void *context);

#define WINE_RB_FLAG_RED                0x1
#define WINE_RB_FLAG_STOP               0x2
#define WINE_RB_FLAG_TRAVERSED_LEFT     0x4
#define WINE_RB_FLAG_TRAVERSED_RIGHT    0x8

static inline void wine_rb_stack_clear(struct wine_rb_stack *stack)
{
    stack->count = 0;
}

static inline void wine_rb_stack_push(struct wine_rb_stack *stack, struct wine_rb_entry **entry)
{
    stack->entries[stack->count++] = entry;
}

static inline int wine_rb_ensure_stack_size(struct wine_rb_tree *tree, size_t size)
{
    struct wine_rb_stack *stack = &tree->stack;

    if (size > stack->size)
    {
        size_t new_size = stack->size << 1;
        struct wine_rb_entry ***new_entries = tree->functions->realloc(stack->entries,
                new_size * sizeof(*stack->entries));

        if (!new_entries) return -1;

        stack->entries = new_entries;
        stack->size = new_size;
    }

    return 0;
}

static inline int wine_rb_is_red(struct wine_rb_entry *entry)
{
    return entry && (entry->flags & WINE_RB_FLAG_RED);
}

static inline void wine_rb_rotate_left(struct wine_rb_entry **entry)
{
    struct wine_rb_entry *e = *entry;
    struct wine_rb_entry *right = e->right;

    e->right = right->left;
    right->left = e;
    right->flags &= ~WINE_RB_FLAG_RED;
    right->flags |= e->flags & WINE_RB_FLAG_RED;
    e->flags |= WINE_RB_FLAG_RED;
    *entry = right;
}

static inline void wine_rb_rotate_right(struct wine_rb_entry **entry)
{
    struct wine_rb_entry *e = *entry;
    struct wine_rb_entry *left = e->left;

    e->left = left->right;
    left->right = e;
    left->flags &= ~WINE_RB_FLAG_RED;
    left->flags |= e->flags & WINE_RB_FLAG_RED;
    e->flags |= WINE_RB_FLAG_RED;
    *entry = left;
}

static inline void wine_rb_flip_color(struct wine_rb_entry *entry)
{
    entry->flags ^= WINE_RB_FLAG_RED;
    entry->left->flags ^= WINE_RB_FLAG_RED;
    entry->right->flags ^= WINE_RB_FLAG_RED;
}

static inline void wine_rb_fixup(struct wine_rb_stack *stack)
{
    while (stack->count)
    {
        struct wine_rb_entry **entry = stack->entries[stack->count - 1];

        if ((*entry)->flags & WINE_RB_FLAG_STOP)
        {
            (*entry)->flags &= ~WINE_RB_FLAG_STOP;
            return;
        }

        if (wine_rb_is_red((*entry)->right) && !wine_rb_is_red((*entry)->left)) wine_rb_rotate_left(entry);
        if (wine_rb_is_red((*entry)->left) && wine_rb_is_red((*entry)->left->left)) wine_rb_rotate_right(entry);
        if (wine_rb_is_red((*entry)->left) && wine_rb_is_red((*entry)->right)) wine_rb_flip_color(*entry);
        --stack->count;
    }
}

static inline void wine_rb_move_red_left(struct wine_rb_entry **entry)
{
    wine_rb_flip_color(*entry);
    if (wine_rb_is_red((*entry)->right->left))
    {
        wine_rb_rotate_right(&(*entry)->right);
        wine_rb_rotate_left(entry);
        wine_rb_flip_color(*entry);
    }
}

static inline void wine_rb_move_red_right(struct wine_rb_entry **entry)
{
    wine_rb_flip_color(*entry);
    if (wine_rb_is_red((*entry)->left->left))
    {
        wine_rb_rotate_right(entry);
        wine_rb_flip_color(*entry);
    }
}

static inline void wine_rb_postorder(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    struct wine_rb_entry **entry;

    if (!tree->root) return;

    for (entry = &tree->root;;)
    {
        struct wine_rb_entry *e = *entry;

        if (e->left && !(e->flags & WINE_RB_FLAG_TRAVERSED_LEFT))
        {
            wine_rb_stack_push(&tree->stack, entry);
            e->flags |= WINE_RB_FLAG_TRAVERSED_LEFT;
            entry = &e->left;
            continue;
        }

        if (e->right && !(e->flags & WINE_RB_FLAG_TRAVERSED_RIGHT))
        {
            wine_rb_stack_push(&tree->stack, entry);
            e->flags |= WINE_RB_FLAG_TRAVERSED_RIGHT;
            entry = &e->right;
            continue;
        }

        e->flags &= ~(WINE_RB_FLAG_TRAVERSED_LEFT | WINE_RB_FLAG_TRAVERSED_RIGHT);
        callback(e, context);

        if (!tree->stack.count) break;
        entry = tree->stack.entries[--tree->stack.count];
    }
}

static inline int wine_rb_init(struct wine_rb_tree *tree, const struct wine_rb_functions *functions)
{
    tree->functions = functions;
    tree->root = NULL;

    tree->stack.entries = functions->alloc(16 * sizeof(*tree->stack.entries));
    if (!tree->stack.entries) return -1;
    tree->stack.size = 16;
    tree->stack.count = 0;

    return 0;
}

static inline void wine_rb_for_each_entry(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    wine_rb_postorder(tree, callback, context);
}

static inline void wine_rb_clear(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    /* Note that we use postorder here because the callback will likely free the entry. */
    if (callback) wine_rb_postorder(tree, callback, context);
    tree->root = NULL;
}

static inline void wine_rb_destroy(struct wine_rb_tree *tree, wine_rb_traverse_func_t *callback, void *context)
{
    wine_rb_clear(tree, callback, context);
    tree->functions->free(tree->stack.entries);
}

static inline struct wine_rb_entry *wine_rb_get(const struct wine_rb_tree *tree, const void *key)
{
    struct wine_rb_entry *entry = tree->root;
    while (entry)
    {
        int c = tree->functions->compare(key, entry);
        if (!c) return entry;
        entry = c < 0 ? entry->left : entry->right;
    }
    return NULL;
}

static inline int wine_rb_put(struct wine_rb_tree *tree, const void *key, struct wine_rb_entry *entry)
{
    struct wine_rb_entry **parent = &tree->root;
    size_t black_height = 1;

    while (*parent)
    {
        int c;

        if (!wine_rb_is_red(*parent)) ++black_height;

        wine_rb_stack_push(&tree->stack, parent);

        c = tree->functions->compare(key, *parent);
        if (!c)
        {
            wine_rb_stack_clear(&tree->stack);
            return -1;
        }
        else if (c < 0) parent = &(*parent)->left;
        else parent = &(*parent)->right;
    }

    /* After insertion, the path length to any node should be <= (black_height + 1) * 2. */
    if (wine_rb_ensure_stack_size(tree, black_height << 1) == -1)
    {
        wine_rb_stack_clear(&tree->stack);
        return -1;
    }

    entry->flags = WINE_RB_FLAG_RED;
    entry->left = NULL;
    entry->right = NULL;
    *parent = entry;

    wine_rb_fixup(&tree->stack);
    tree->root->flags &= ~WINE_RB_FLAG_RED;

    return 0;
}

static inline void wine_rb_remove(struct wine_rb_tree *tree, const void *key)
{
    struct wine_rb_entry **entry = &tree->root;

    while (*entry)
    {
        if (tree->functions->compare(key, *entry) < 0)
        {
            wine_rb_stack_push(&tree->stack, entry);
            if (!wine_rb_is_red((*entry)->left) && !wine_rb_is_red((*entry)->left->left)) wine_rb_move_red_left(entry);
            entry = &(*entry)->left;
        }
        else
        {
            if (wine_rb_is_red((*entry)->left)) wine_rb_rotate_right(entry);
            if (!tree->functions->compare(key, *entry) && !(*entry)->right)
            {
                *entry = NULL;
                break;
            }
            if (!wine_rb_is_red((*entry)->right) && !wine_rb_is_red((*entry)->right->left))
                wine_rb_move_red_right(entry);
            if (!tree->functions->compare(key, *entry))
            {
                struct wine_rb_entry **e = &(*entry)->right;
                struct wine_rb_entry *m = *e;
                while (m->left) m = m->left;

                wine_rb_stack_push(&tree->stack, entry);
                (*entry)->flags |= WINE_RB_FLAG_STOP;

                while ((*e)->left)
                {
                    wine_rb_stack_push(&tree->stack, e);
                    if (!wine_rb_is_red((*e)->left) && !wine_rb_is_red((*e)->left->left)) wine_rb_move_red_left(e);
                    e = &(*e)->left;
                }
                *e = NULL;
                wine_rb_fixup(&tree->stack);

                *m = **entry;
                *entry = m;

                break;
            }
            else
            {
                wine_rb_stack_push(&tree->stack, entry);
                entry = &(*entry)->right;
            }
        }
    }

    wine_rb_fixup(&tree->stack);
    if (tree->root) tree->root->flags &= ~WINE_RB_FLAG_RED;
}

#endif  /* __WINE_WINE_RBTREE_H */
