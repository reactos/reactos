#ifndef __WIDGET_H
#define __WIDGET_H

#define C_BOOL		1
#define C_CHANGE	2

/* Please note that the first element in all the widgets is a     */
/* widget variable of type Widget.  We abuse this fact everywhere */
/* Widget_Items */

#define HIDDEN_BUTTON		0
#define NARROW_BUTTON		1
#define NORMAL_BUTTON		2
#define DEFPUSH_BUTTON		3

typedef struct WButton {
    Widget widget;
    int action;			/* what to do when pressed */
    int selected;		/* button state */
    unsigned int flags;		/* button flags */
    char *text;			/* text of button */
    int  hotkey;                /* hot KEY */
    int  hotpos;                /* offset hot KEY char in text */
    int  (*callback)(int, void*); /* Callback function */
    void *callback_data;
} WButton;

typedef struct WRadio {
    Widget widget;
    unsigned int state;		/* radio button state */
    int pos, sel;
    int count;			/* number of members */
    char **texts;		/* texts of labels */
    int upper_letter_is_hotkey; /* If true, then the capital letter is a hk */
#ifdef HAVE_GNOME
    void *first_gtk_radio;
#endif
} WRadio;

typedef struct WCheck {
    Widget widget;
    unsigned int state;		/* check button state */
    char *text;			/* text of check button */
    int hotkey;                 /* hot KEY */                    
    int hotpos;			/* offset hot KEY char in text */
} WCheck;

typedef struct WGauge {
    Widget widget;
    int shown;
    int max;
    int current;
    int pixels;			/* Only used for Tk:
				 * We keep the pixel size in the C code
				 * so that we can compute quickly compute
				 * the size of the rectangle in the Tk
				 * canvas.  Using Tcl would be too slow.
				 */
} WGauge;

typedef struct hist_entry {
    struct hist_entry *prev;
    struct hist_entry *next;
    char   *text;
} Hist;

Hist *history_get (char *input_name);
void history_put (char *input_name, Hist *h);
char *show_hist (Hist *history, int widget_y, int widget_x);

typedef struct {
    Widget widget;
    int  point;			/* cursor position in the input line */
    int  mark;			/* The mark position */
    int  first_shown;		/* Index of the first shown character */
    int  current_max_len;	/* Maximum length of input line */
    int  field_len;		/* Length of the editing field */
    int  color;			/* color used */
    int  first;			/* Is first keystroke? */
    int  disable_update;	/* Do we want to skip updates? */
    int  is_password;		/* Is this a password input line? */
    char *buffer;		/* pointer to editing buffer */
    Hist *history;		/* The history */
    int  need_push;		/* need to push the current Input on hist? */
    char **completions;		/* Possible completions array */
    int  completion_flags;	/* INPUT_COMPLETE* bitwise flags(complete.h) */
    int  inserted_one;		/* TK: just one char inserted, nothing fancy */
    char *history_name;		/* name of history for loading and saving */
} WInput;

/* For history load-save functions */
#define INPUT_LAST_TEXT ((char *) 2)
#define HISTORY_FILE_NAME ".mc/history"

typedef struct {
    Widget widget;
    int    auto_adjust_cols;	/* compute widget.cols from strlen(text)? */
    char   *text;
    int	   transparent;		/* Paint in the default color fg/bg */
} WLabel;

typedef struct WLEntry {
    char *text;			/* Text to display */
    int  hotkey;
    void *data;			/* Client information */
    struct WLEntry *next;
    struct WLEntry *prev;
} WLEntry;

enum {
    listbox_begin, listbox_end
} /* listbox_insert */;

/* Listbox actions when selecting an option: */
enum {
    listbox_nothing,
    listbox_finish,		/* finish dialog */
    listbox_cback		/* call the callback routine */
} /* listbox_action */;

typedef int (*lcback) (void *);

typedef struct {
    Widget widget;
    WLEntry *list;		/* Pointer to the circular double linked list. */
    WLEntry *top;		/* The first element displayed */
    WLEntry *current;		/* The current element displayed */
    int pos;			/* Cur. pos, must be kept in sync with current */
    int count;			/* Number of items in the listbox */
    int width;
    int height;			/* Size of the widget */
    int action;			/* Action type */
    int allow_duplicates;	/* Do we allow duplicates on the list? */
    int scrollbar;		/* Draw a scrollbar? */
    lcback cback;		/* The callback function */
    int cursor_x, cursor_y;	/* Cache the values */
} WListbox;

typedef struct {
    Widget widget;
    int    visible;		/* Is it visible? */
    struct {
	char   *text;
	void   (*function)(void *data);
	void   *data;
    } labels [10];
} WButtonBar;

/* Constructors */
WButton *button_new   (int y, int x, int action, int flags, char *text, 
			int (*callback)(int, void *), void *extra, char *tkname);
WRadio  *radio_new    (int y, int x, int count, char **text, int use_hotkey, char *tkname);
WCheck  *check_new    (int y, int x, int state,  char *text, char *tkname);
WInput  *input_new    (int y, int x, int color, int len, char *text, char *tkname);
WLabel  *label_new    (int y, int x, char *text, char *tkname);
WGauge  *gauge_new    (int y, int x, int shown, int max, int current, char *tkname);
WListbox *listbox_new (int x, int y, int width, int height, int action,
		        lcback, char *tkname);

/* Input lines */
void winput_set_origin (WInput *i, int x, int field_len);
int handle_char (WInput *in, int c_code);
int is_in_input_map (WInput *in, int c_code);
void update_input (WInput *in, int clear_first);
void new_input (WInput *in);
int push_history (WInput *in, char *text);
void stuff (WInput *in, char *text, int insert_extra_space);
void input_disable_update (WInput *in);
void input_set_prompt (WInput *in, int field_len, char *prompt);
void input_enable_update (WInput *in);
void input_set_point (WInput *in, int pos);
void input_show_cursor (WInput *in);
void assign_text (WInput *in, char *text);

/* Labels */
void label_set_text (WLabel *label, char *text);

/* Gauges */
void gauge_set_value (WGauge *g, int max, int current);
void gauge_show (WGauge *g, int shown);

/* Buttons */
void button_set_text (WButton *b, char *text);

/* Listbox manager */
WLEntry *listbox_get_data (WListbox *l, int pos);

/* search text int listbox entries */
WLEntry *listbox_search_text (WListbox *l, char *text);
void listbox_select_entry (WListbox *l, WLEntry *dest);
void listbox_select_by_number (WListbox *l, int n);
void listbox_select_last (WListbox *l, int set_top);
void listbox_remove_current (WListbox *l, int force);
void listbox_remove_list (WListbox *l);
void listbox_get_current (WListbox *l, char **string, char **extra);

enum append_pos {
    LISTBOX_APPEND_AT_END,	/* append at the end */
    LISTBOX_APPEND_BEFORE,	/* insert before current */
    LISTBOX_APPEND_AFTER	/* insert after current */
};

char *listbox_add_item (WListbox *l, enum append_pos pos, int
			hotkey, char *text, void *data);

/* Hintbar routines */

/* Buttonbar routines */
WButtonBar *buttonbar_new (int visible);
typedef void (*buttonbarfn )(void *);
typedef void (*voidfn)(void);
void define_label (Dlg_head *, Widget *paneletc, int index, char *text, voidfn);
void define_label_data (Dlg_head *h, Widget *paneletc, int idx, char *text,
			buttonbarfn cback, void *data);
void set_label_text (WButtonBar *, int, char *);
void redraw_labels (Dlg_head *h, Widget *paneletc);
WButtonBar *find_buttonbar (Dlg_head *h, Widget *paneletc);
void buttonbar_hint (WButtonBar *bb, char *s);

#ifdef HAVE_X
int  x_create_radio       	     (Dlg_head *h, widget_data parent, WRadio *r);
int  x_create_button                 (Dlg_head *h, widget_data parent, WButton *b);
int  x_create_check       	     (Dlg_head *h, widget_data parent, WCheck *c);
int  x_create_label       	     (Dlg_head *h, widget_data parent, WLabel *l);
int  x_create_input       	     (Dlg_head *h, widget_data parent, WInput *in);
int  x_create_listbox     	     (Dlg_head *h, widget_data parent, WListbox *l);
int  x_create_buttonbar   	     (Dlg_head *h, widget_data parent, WButtonBar *bb);

void x_button_set         (WButton *b, char *text);
void x_label_set_text     (WLabel *label, char *text);
void x_listbox_select_nth (WListbox *l, int nth);
void x_listbox_delete_nth (WListbox *l, int nth);
void x_label_set_text     (WLabel *label, char *text);
int  x_create_gauge       (Dlg_head *h, widget_data parent, WGauge *g);
void x_gauge_show         (WGauge *g);
void x_gauge_set_value    (WGauge *g, int max, int current);
void x_radio_toggle       (WRadio *);
void x_radio_focus_item   (WRadio *radio);
void x_listbox_select_nth (WListbox *, int);
void x_list_insert        (WListbox *, WLEntry *, WLEntry *);
void x_redefine_label     (WButtonBar *, int);
void x_update_input       (WInput *in);
#endif

#endif	/* __WIDGET_H */
