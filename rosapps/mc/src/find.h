#ifndef __FIND_H
#define __FIND_H

#define MAX_FIND_MENU      4   /* Maximum Menu */
#define MAX_FIND_FLINES   11   /* Size of Window Where files are stored */ 
#define MAX_FIND_TLINES    4   /* Size of Window Where Menu and text .. */
#define MAX_FIND_COLS     48   /* Length of Windows */
#define FIND_DIALOG_SIZE  MAX_FIND_FLINES+MAX_FIND_TLINES
                          /* Total size of the whole dialog */

typedef struct  find_list{
    int               selected;  /* Selection field */
    int               ypos;      /* For Scrolling */
    int               isdir;     /* For adding a '\t' on FALSE */
    char              *fname;    /* Name of the file */
    char              *path;     /* For changing panel */
    struct find_list  *up;      
    struct find_list  *down;
}find_list;

typedef struct FStack{		/* The Stack will be used to store */
    char *dir_name;             /* the directories to search */
    struct FStack *next;        /*  single-linked */ 
}FStack;

void do_find(void);
#endif
