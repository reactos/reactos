#ifndef __WTOOLS_H
#define __WTOOLS_H

/* Dialog default background repaint routines */
void dialog_repaint (struct Dlg_head *h, int back, int title_fore);
void common_dialog_repaint (struct Dlg_head *h);

/* For common dialogs, just repaint background */
int  common_dialog_callback (struct Dlg_head *h, int id, int msg);

/* Listbox utility functions */
typedef struct {
    Dlg_head *dlg;
    WListbox *list;
} Listbox;

Listbox *create_listbox_window (int cols, int lines, char *title, char *help);
#define LISTBOX_APPEND_TEXT(l,h,t,d) \
    listbox_add_item (l->list, 0, h, t, d);

int run_listbox (Listbox *l);

/* Quick Widgets */
enum {
    quick_end, quick_checkbox, 
    quick_button, quick_input,
    quick_label, quick_radio
} /* quick_t */;

/* The widget is placed on relative_?/divisions_? of the parent widget */
/* Please note that the contents of the fields in the union are just */
/* used for setting up the dialog.  They are a convenient place to put */
/* the values for a widget */

typedef struct {
    int widget_type;
    int relative_x;
    int x_divisions;
    int relative_y;
    int y_divisions;

    char *text;			/* Text */
    int  hotkey_pos;		/* the hotkey position */
    int  value;			/* Buttons only: value of button */
    int  *result;		/* Checkbutton: where to store result */
    char **str_result;		/* Input lines: destination  */
    WLay layout;		/* XView Layouting stuff */
    char *tkname;		/* Name of the widget used for Tk only */
    void *the_widget;		/* For the quick quick dialog manager */
} QuickWidget;
    
typedef struct {
    int  xlen, ylen;
    int  xpos, ypos; /* if -1, then center the dialog */
    char *title;
    char *help;
    char *class;		/* Used for Tk's class name */
    QuickWidget *widgets;
    int  i18n;			/* If true, internationalization has happened */
} QuickDialog;

int quick_dialog (QuickDialog *qd);
int quick_dialog_skip (QuickDialog *qd, int nskip);

/* Choosers */

#define CHOOSE_EDITABLE  1
#define CHOOSE_BROWSE    0

/* Chooser dialog boxes */
typedef struct {
    Dlg_head *dialog;
    WListbox *listbox;
} Chooser;

Chooser *new_chooser (int lines, int cols, char *help, int flags);
int run_chooser (Chooser *c);
void destroy_chooser (Chooser *c);

/* The input dialogs */
char *input_dialog (char *header, char *text, char *def_text);
int input_dialog_2 (char *header, char *text1, char *text2, char **r1, char **r2);
char *input_dialog_help (char *header, char *text, char *help, char *def_text);
char *input_expand_dialog (char *header, char *text, char *def_text);
char *real_input_dialog (char *header, char *text, char *def_text);
char *real_input_dialog_help (char *header, char *text, char *help, char *def_text);

void query_set_sel (int new_sel);
#endif	/* __WTOOLS_H */
