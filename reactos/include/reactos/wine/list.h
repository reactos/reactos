/*
 * Linked lists support
 *
 * Copyright (C) 2002 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_SERVER_LIST_H
#define __WINE_SERVER_LIST_H

struct list
{
    struct list *next;
    struct list *prev;
};

/* Define a list like so:
 *
 *   struct gadget
 *   {
 *       struct list  entry;   <-- doesn't have to be the first item in the struct
 *       int          a, b;
 *   };
 *
 *   static struct list global_gadgets = LIST_INIT( global_gadgets );
 *
 * or
 *
 *   struct some_global_thing
 *   {
 *       struct list gadgets;
 *   };
 *
 *   list_init( &some_global_thing->gadgets );
 *
 * Manipulate it like this:
 *
 *   list_add_head( &global_gadgets, &new_gadget->entry );
 *   list_remove( &new_gadget->entry );
 *   list_add_after( &some_random_gadget->entry, &new_gadget->entry );
 *
 * And to iterate over it:
 *
 *   struct list *cursor;
 *   LIST_FOR_EACH( cursor, &global_gadgets )
 *   {
 *       struct gadget *gadget = LIST_ENTRY( cursor, struct gadget, entry );
 *   }
 *
 */

/* add an element after the specified one */
__inline static void list_add_after( struct list *elem, struct list *to_add )
{
    to_add->next = elem->next;
    to_add->prev = elem;
    elem->next->prev = to_add;
    elem->next = to_add;
}

/* add an element before the specified one */
__inline static void list_add_before( struct list *elem, struct list *to_add )
{
    to_add->next = elem;
    to_add->prev = elem->prev;
    elem->prev->next = to_add;
    elem->prev = to_add;
}

/* add element at the head of the list */
__inline static void list_add_head( struct list *list, struct list *elem )
{
    list_add_after( list, elem );
}

/* add element at the tail of the list */
__inline static void list_add_tail( struct list *list, struct list *elem )
{
    list_add_before( list, elem );
}

/* remove an element from its list */
__inline static void list_remove( struct list *elem )
{
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;
}

/* get the next element */
__inline static struct list *list_next( struct list *list, struct list *elem )
{
    struct list *ret = elem->next;
    if (elem->next == list) ret = NULL;
    return ret;
}

/* get the previous element */
__inline static struct list *list_prev( struct list *list, struct list *elem )
{
    struct list *ret = elem->prev;
    if (elem->prev == list) ret = NULL;
    return ret;
}

/* get the first element */
__inline static struct list *list_head( struct list *list )
{
    return list_next( list, list );
}

/* get the last element */
__inline static struct list *list_tail( struct list *list )
{
    return list_prev( list, list );
}

/* check if a list is empty */
__inline static int list_empty( struct list *list )
{
    return list->next == list;
}

/* initialize a list */
__inline static void list_init( struct list *list )
{
    list->next = list->prev = list;
}

/* count the elements of a list */
__inline static unsigned int list_count( const struct list *list )
{
    unsigned count = 0;
    const struct list *ptr;
    for (ptr = list->next; ptr != list; ptr = ptr->next) count++;
    return count;
}

/* iterate through the list */
#define LIST_FOR_EACH(cursor,list) \
    for ((cursor) = (list)->next; (cursor) != (list); (cursor) = (cursor)->next)

/* iterate through the list, with safety against removal */
#define LIST_FOR_EACH_SAFE(cursor, cursor2, list) \
    for ((cursor) = (list)->next, (cursor2) = (cursor)->next; \
         (cursor) != (list); \
         (cursor) = (cursor2), (cursor2) = (cursor)->next)

/* iterate through the list using a list entry */
#define LIST_FOR_EACH_ENTRY(elem, list, type, field) \
    for ((elem) = LIST_ENTRY((list)->next, type, field); \
         &(elem)->field != (list); \
         (elem) = LIST_ENTRY((elem)->field.next, type, field))

#define LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, list, type, field) \
    for ((cursor) = LIST_ENTRY((list)->next, type, field), \
         (cursor2) = LIST_ENTRY((cursor)->field.next, type, field); \
         &(cursor)->field != (list); \
         (cursor) = (cursor2), \
         (cursor2) = LIST_ENTRY((cursor)->field.next, type, field))

/* iterate through the list in reverse order using a list entry */
#define LIST_FOR_EACH_ENTRY_REV(elem, list, type, field) \
    for ((elem) = LIST_ENTRY((list)->prev, type, field); \
         &(elem)->field != (list); \
         (elem) = LIST_ENTRY((elem)->field.prev, type, field)) 

/* macros for statically initialized lists */
#define LIST_INIT(list)  { &(list), &(list) }

/* get pointer to object containing list element */
#define LIST_ENTRY(elem, type, field) \
    ((type *)((char *)(elem) - (unsigned int)(&((type *)0)->field)))

#endif  /* __WINE_SERVER_LIST_H */
