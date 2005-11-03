#ifndef __HOTLIST_H
#define __HOTLIST_H

#define LIST_VFSLIST	0x01
#define LIST_HOTLIST	0x02
#define LIST_MOVELIST	0x04

void add2hotlist_cmd (void);
char *hotlist_cmd (int list_vfs);
void load_hotlist (void);
int  save_hotlist (void);
void done_hotlist (void);

#endif
