/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#ifndef NFS41_LIST_H
#define NFS41_LIST_H


/* doubly-linked list */
struct list_entry {
    struct list_entry *prev;
    struct list_entry *next;
};


#define list_container(entry, type, field) \
    ((type*)((const char*)(entry) - (const char*)(&((type*)0)->field)))

#define list_for_each(entry, head) \
    for (entry = (head)->next; entry != (head); entry = entry->next)

#define list_for_each_tmp(entry, tmp, head) \
    for (entry = (head)->next, tmp = entry->next; entry != (head); \
        entry = tmp, tmp = entry->next)

#define list_for_each_reverse(entry, head) \
    for (entry = (head)->prev; entry != (head); entry = entry->prev)

#define list_for_each_reverse_tmp(entry, tmp, head) \
    for (entry = (head)->next, tmp = entry->next; entry != (head); \
        entry = tmp, tmp = entry->next)


static void list_init(
    struct list_entry *head)
{
    head->prev = head;
    head->next = head;
}

static int list_empty(
    struct list_entry *head)
{
    return head->next == head;
}

static void list_add(
    struct list_entry *entry,
    struct list_entry *prev,
    struct list_entry *next)
{
    /* assert(prev->next == next && next->prev == prev); */
    entry->prev = prev;
    entry->next = next;
    prev->next = entry;
    next->prev = entry;
}

static void list_add_head(
    struct list_entry *head,
    struct list_entry *entry)
{
    list_add(entry, head, head->next);
}

static void list_add_tail(
    struct list_entry *head,
    struct list_entry *entry)
{
    list_add(entry, head->prev, head);
}

static void list_remove(
    struct list_entry *entry)
{
    if (!list_empty(entry)) {
        entry->next->prev = entry->prev;
        entry->prev->next = entry->next;
        list_init(entry);
    }
}

typedef int (*list_compare_fn)(const struct list_entry*, const void*);

static struct list_entry* list_search(
    const struct list_entry *head,
    const void *value,
    list_compare_fn compare)
{
    struct list_entry *entry;
    list_for_each(entry, head)
        if (compare(entry, value) == 0)
            return entry;
    return NULL;
}

#endif /* !NFS41_LIST_H */
