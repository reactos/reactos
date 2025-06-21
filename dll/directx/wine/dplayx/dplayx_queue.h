/* A queue definition based on sys/queue.h TAILQ definitions
 *
 * Copyright 2000 Peter Hunnisett
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
 *
 * NOTES
 *  o Linked list implementation for dplay/dplobby. Based off of the BSD
 *    version found in <sys/queue.h>
 *  o Port it to <wine/list.h> ?
 *
 */

#ifndef __WINE_DPLAYX_QUEUE_H
#define __WINE_DPLAYX_QUEUE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#define DPQ_INSERT(a,b,c) DPQ_INSERT_IN_TAIL(a,b,c)

/*
 * Tail queue definitions.
 */
#define DPQ_HEAD(type)                                           \
struct {                                                         \
        struct type *lpQHFirst; /* first element */              \
        struct type **lpQHLast; /* addr of last next element */  \
}

#define DPQ_ENTRY(type)                                               \
struct {                                                              \
        struct type *lpQNext;  /* next element */                     \
        struct type **lpQPrev; /* address of previous next element */ \
}

/*
 * Tail queue functions.
 */
#define DPQ_INIT(head)                       \
do{                                          \
        (head).lpQHFirst = NULL;             \
        (head).lpQHLast = &(head).lpQHFirst; \
} while(0)

/* Front of the queue */
#define DPQ_FIRST( head ) ( (head).lpQHFirst )

/* Check if the queue has any elements */
#define DPQ_IS_EMPTY( head ) ( DPQ_FIRST(head) == NULL )

/* Next entry -- FIXME: Convert everything over to this macro ... */
#define DPQ_NEXT( elem ) (elem).lpQNext

#define DPQ_IS_ENDOFLIST( elem ) \
    ( DPQ_NEXT(elem) == NULL )

/* Insert element at end of queue */
#define DPQ_INSERT_IN_TAIL(head, elm, field)     \
do {                                             \
        (elm)->field.lpQNext = NULL;             \
        (elm)->field.lpQPrev = (head).lpQHLast;  \
        *(head).lpQHLast = (elm);                \
        (head).lpQHLast = &(elm)->field.lpQNext; \
} while(0)

/* Remove element from the queue */
#define DPQ_REMOVE(head, elm, field)                    \
do {                                                    \
        if (((elm)->field.lpQNext) != NULL)             \
                (elm)->field.lpQNext->field.lpQPrev =   \
                    (elm)->field.lpQPrev;               \
        else                                            \
                (head).lpQHLast = (elm)->field.lpQPrev; \
        *(elm)->field.lpQPrev = (elm)->field.lpQNext;   \
} while(0)

/* head - pointer to DPQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToCompare
 * fieldToCompare - The value that we're comparing against
 * fieldCompareOperator - The logical operator to compare field and
 *                        fieldToCompare.
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_FIND_ENTRY( head, elm, field, fieldCompareOperator, fieldToCompare, rc )\
do {                                                           \
  (rc) = DPQ_FIRST(head); /* NULL head? */                     \
                                                               \
  while( rc )                                                  \
  {                                                            \
      /* What we're searching for? */                          \
      if( (rc)->field fieldCompareOperator (fieldToCompare) )  \
      {                                                        \
        break; /* rc == correct element */                     \
      }                                                        \
                                                               \
      /* End of list check */                                  \
      if( ( (rc) = (rc)->elm.lpQNext ) == (head).lpQHFirst )   \
      {                                                        \
        rc = NULL;                                             \
        break;                                                 \
      }                                                        \
  }                                                            \
} while(0)

/* head - pointer to DPQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToCompare
 * fieldToCompare - The value that we're comparing against
 * compare_cb - Callback to invoke to determine if comparison should continue.
 *              Callback must be defined with DPQ_DECL_COMPARECB.
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_FIND_ENTRY_CB( head, elm, field, compare_cb, fieldToCompare, rc )\
do {                                                           \
  (rc) = DPQ_FIRST(head); /* NULL head? */                     \
                                                               \
  while( rc )                                                  \
  {                                                            \
      /* What we're searching for? */                          \
      if( compare_cb( &((rc)->field), &(fieldToCompare) ) )    \
      {                                                        \
        break; /* no more */                                   \
      }                                                        \
                                                               \
      /* End of list check */                                  \
      if( ( (rc) = (rc)->elm.lpQNext ) == (head).lpQHFirst )   \
      {                                                        \
        rc = NULL;                                             \
        break;                                                 \
      }                                                        \
  }                                                            \
} while(0)

/* How to define the method to be passed to DPQ_DELETEQ */
#define DPQ_DECL_COMPARECB( name, type ) BOOL name( const type* elem1, const type* elem2 )


/* head - pointer to DPQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToEqual
 * fieldToCompare - The value that we're comparing against
 * fieldCompareOperator - The logical operator to compare field and
 *                        fieldToCompare.
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_REMOVE_ENTRY( head, elm, field, fieldCompareOperator, fieldToCompare, rc )\
do {                                                           \
  DPQ_FIND_ENTRY( head, elm, field, fieldCompareOperator, fieldToCompare, rc );\
                                                               \
  /* Was the element found? */                                 \
  if( rc )                                                     \
  {                                                            \
    DPQ_REMOVE( head, rc, elm );                               \
  }                                                            \
} while(0)

/* head - pointer to DPQ_HEAD struct
 * elm  - how to find the next element
 * field - to be concatenated to rc to compare with fieldToCompare
 * fieldToCompare - The value that we're comparing against
 * compare_cb - Callback to invoke to determine if comparison should continue.
 *              Callback must be defined with DPQ_DECL_COMPARECB.
 * rc - Variable to put the return code. Same type as (head).lpQHFirst
 */
#define DPQ_REMOVE_ENTRY_CB( head, elm, field, compare_cb, fieldToCompare, rc )\
do {                                                           \
  DPQ_FIND_ENTRY_CB( head, elm, field, compare_cb, fieldToCompare, rc );\
                                                               \
  /* Was the element found? */                                 \
  if( rc )                                                     \
  {                                                            \
    DPQ_REMOVE( head, rc, elm );                               \
  }                                                            \
} while(0)


/* Delete the entire queue
 * head - pointer to the head of the queue
 * field - field to access the next elements of the queue
 * type - type of the pointer to the element element
 * df - a delete function to be called. Declared with DPQ_DECL_DELETECB.
 */
#define DPQ_DELETEQ( head, field, type, df )     \
do                                               \
{                                                \
  while( !DPQ_IS_EMPTY(head) )                   \
  {                                              \
    type holder = DPQ_FIRST(head);               \
    DPQ_REMOVE( head, holder, field );           \
    df( holder );                                \
  }                                              \
} while(0)

/* How to define the method to be passed to DPQ_DELETEQ */
#define DPQ_DECL_DELETECB( name, type ) void name( type elem )

/* Prototype of a method which just performs a HeapFree on the elem */
DPQ_DECL_DELETECB( cbDeleteElemFromHeap, LPVOID );

#endif /* __WINE_DPLAYX_QUEUE_H */
