/* ----------------- dialbox.h ---------------- */

#ifndef DIALOG_H
#define DIALOG_H

#include <stdio.h>

#define MAXCONTROLS 30
#define MAXRADIOS 20

#define OFF FALSE
#define ON  TRUE

/* -------- dialog box and control window structure ------- */
typedef struct  {
    char *title;    /* window title         */
    int x, y;       /* relative coordinates */
    int h, w;       /* size                 */
} DIALOGWINDOW;

/* ------ one of these for each control window ------- */
typedef struct {
    DIALOGWINDOW dwnd;
    DFCLASS class;    /* LISTBOX, BUTTON, etc */
    char *itext;    /* initialized text     */
    int command;    /* command code         */
    char *help;     /* help mnemonic        */
    BOOL isetting;  /* initially ON or OFF  */
    BOOL setting;   /* ON or OFF            */
    void *wnd;      /* window handle        */
} CTLWINDOW;

/* --------- one of these for each dialog box ------- */
typedef struct {
    char *HelpName;
    DIALOGWINDOW dwnd;
    CTLWINDOW ctl[MAXCONTROLS+1];
} DBOX;

/* -------- macros for dialog box resource compile -------- */
#define DIALOGBOX(db) DBOX db={ #db,
#define DB_TITLE(ttl,x,y,h,w) {ttl,x,y,h,w},{
#define CONTROL(ty,tx,x,y,h,w,c) 						\
				{{NULL,x,y,h,w},ty,						\
				(ty==EDITBOX||ty==COMBOBOX?NULL:tx),	\
				c,#c,(ty==BUTTON?ON:OFF),OFF,NULL},

#define ENDDB {{NULL}} }};

#define Cancel  " Cancel "
#define Ok      "   OK   "
#define Yes     "  Yes   "
#define No      "   No   "

#endif
