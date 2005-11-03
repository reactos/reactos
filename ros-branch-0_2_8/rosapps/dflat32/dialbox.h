/* ----------------- dialbox.h ---------------- */

#ifndef DIALOG_H
#define DIALOG_H

#include <stdio.h>

#define DF_MAXCONTROLS 30
#define DF_MAXRADIOS 20

#define DF_OFF FALSE
#define DF_ON  TRUE

/* -------- dialog box and control window structure ------- */
typedef struct  {
    char *title;    /* window title         */
    int x, y;       /* relative coordinates */
    int h, w;       /* size                 */
} DF_DIALOGWINDOW;

/* ------ one of these for each control window ------- */
typedef struct {
    DF_DIALOGWINDOW dwnd;
    DFCLASS class;    /* DF_LISTBOX, DF_BUTTON, etc */
    char *itext;    /* initialized text     */
    int command;    /* command code         */
    char *help;     /* help mnemonic        */
    BOOL isetting;  /* initially DF_ON or DF_OFF  */
    BOOL setting;   /* DF_ON or DF_OFF            */
    void *wnd;      /* window handle        */
} DF_CTLWINDOW;

/* --------- one of these for each dialog box ------- */
typedef struct {
    char *HelpName;
    DF_DIALOGWINDOW dwnd;
    DF_CTLWINDOW ctl[DF_MAXCONTROLS+1];
} DF_DBOX;

/* -------- macros for dialog box resource compile -------- */
#define DF_DIALOGBOX(db) DF_DBOX db={ #db,
#define DF_DB_TITLE(ttl,x,y,h,w) {ttl,x,y,h,w},{
#define DF_CONTROL(ty,tx,x,y,h,w,c) 						\
				{{NULL,x,y,h,w},ty,						\
				(ty==DF_EDITBOX||ty==DF_COMBOBOX?NULL:tx),	\
				c,#c,(ty==DF_BUTTON?DF_ON:DF_OFF),DF_OFF,NULL},

#define DF_ENDDB {{NULL}} }};

#define DfCancel  " Cancel "
#define DfOk      "   OK   "
#define DfYes     "  Yes   "
#define DfNo      "   No   "

#endif
