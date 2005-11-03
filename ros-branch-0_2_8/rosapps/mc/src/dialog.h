#ifndef __DIALOG_H
#define __DIALOG_H

#include "dlg.h"

#define MSG_ERROR ((char *) -1)
Dlg_head *message (int error, char *header, char *text, ...);

int query_dialog (char *header, char *text, int flags, int count, ...);

enum {
   D_NORMAL = 0,
   D_ERROR  = 1,
   D_INSERT = 2
} /* dialog options */;

/* The refresh stack */
typedef struct Refresh {
    void (*refresh_fn)(void *);
    void *parameter;
    int  flags;
    struct Refresh *next;
} Refresh;

/* We search under the stack until we find a refresh function that covers */
/* the complete screen, and from this point we go up refreshing the */
/* individual regions */

enum {
    REFRESH_COVERS_PART,	/* If the refresh fn convers only a part */
    REFRESH_COVERS_ALL		/* If the refresh fn convers all the screen */
};

void push_refresh (void (*new_refresh)(void *), void *data, int flags);
void pop_refresh (void);
void do_refresh (void);
void my_wputs (int y, int x, char *text);
char *input_dialog (char *header, char *text, char *def_text);
char *input_expand_dialog (char *header, char *text, char *def_text);

extern Refresh *refresh_list;

#endif	/* __DIALOG_H */
