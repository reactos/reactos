/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */



#include <stdlib.h>
#include <netif/list.h>


/*-----------------------------------------------------------------------------------*/
struct list *
list_new(int size)
{
  struct list *list;
  list = (struct list *)malloc(sizeof(struct list));
  list->first = list->last = NULL;
  list->size = size;
  list->elems = 0;
  return list;
}
/*-----------------------------------------------------------------------------------*/
int
list_push(struct list *list, void *data)
{
  struct elem *elem;

  if (list->elems < list->size) {
    elem = (struct elem *)malloc(sizeof(struct elem));
    elem->data = data;
    elem->next = NULL;
    if (list->last != NULL) {
      list->last->next = elem;
    }
    list->last = elem;
    if (list->first == NULL) {
      list->first = elem;
    }
    list->elems++;
    return 1;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void *
list_pop(struct list *list)
{
  struct elem *elem;
  void *data;

  if (list->elems > 0) {
    elem = list->first;
    if (elem == list->last) {
      list->last = elem->next;
    }
    list->first = elem->next;

    list->elems--;

    data = elem->data;
    free(elem);

    return data;
  }
  return NULL;
}
/*-----------------------------------------------------------------------------------*/
void *
list_first(struct list *list)
{
  return list->first;
}
/*-----------------------------------------------------------------------------------*/
int
list_elems(struct list *list)
{
  return list->elems;
}
/*-----------------------------------------------------------------------------------*/
void
list_delete(struct list *list)
{
  while (list_pop(list) != NULL);
  free(list);
}
/*-----------------------------------------------------------------------------------*/
int
list_remove(struct list *list, void *elem)
{
  struct elem *e, *p;

  p = NULL;
  for(e = list->first; e != NULL; e = e->next) {
    if (e->data == elem) {
      if (p != NULL) {
        p->next = e->next;
      } else {
        list->first = e->next;
      }
      if (list->last == e) {
        list->last = p;
        if (p != NULL) {
          p->next = NULL;
        }
      }
      free(e);
      list->elems--;
      return 1;
    }
    p = e;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void
list_map(struct list *list, void (* func)(void *arg))
{
  struct elem *e;

  for(e = list->first; e != NULL; e = e->next) {
    func(e->data);
  }
}
/*-----------------------------------------------------------------------------------*/
