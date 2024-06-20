
#ifndef LWIP_LIST_H
#define LWIP_LIST_H

struct elem;

struct list {
  struct elem *first, *last;
  int size, elems;
};

struct elem {
  struct elem *next;
  void *data;
};

struct list *list_new(int size);
int list_push(struct list *list, void *data);
void *list_pop(struct list *list);
void *list_first(struct list *list);
int list_elems(struct list *list);
void list_delete(struct list *list);
int list_remove(struct list *list, void *elem);
void list_map(struct list *list, void (* func)(void *arg));

#endif
