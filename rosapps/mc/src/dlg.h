#ifndef MC_DLG_H
#define MC_DLG_H
#include "mouse.h"
#include "util.h"

/* Color constants */
#define FOCUSC           h->color[1]
#define NORMALC          h->color[0]
#define HOT_NORMALC      h->color[2]
#define HOT_FOCUSC       h->color[3]

/* Possible directions */
#define DIR_FORWARD     1
#define DIR_BACKWARD    0

/* Common return values */
#define B_EXIT		0
#define B_CANCEL	1
#define B_ENTER		2
#define B_HELP		3
#define B_USER          100

/* Widget messages */
enum {
    WIDGET_INIT,		/* Initialize widget */
    WIDGET_FOCUS,		/* Draw widget in focused state */
    WIDGET_UNFOCUS,		/* Draw widget in unfocused state */
    WIDGET_DRAW,		/* Sent to widget to draw themselves */
    WIDGET_KEY,			/* Sent to widgets on key press */
    WIDGET_HOTKEY,		/* Sent to widget to catch preprocess key */
    WIDGET_DESTROY,		/* Sent to widget at destruction time */
    WIDGET_CURSOR,		/* Sent to widget to position the cursor */
    WIDGET_IDLE,		/* Send to widgets with options & W_WANT_IDLE*/
    WIDGET_USER  = 0x100000

} /* Widget_Messages */;

enum {
    MSG_NOT_HANDLED,
    MSG_HANDLED
} /* WRET */;

/* Widgets are expected to answer to the following messages:

   WIDGET_FOCUS:   1 if the accept the focus, 0 if they do not.
   WIDGET_UNFOCUS: 1 if they accept to release the focus, 0 if they don't.
   WIDGET_KEY:     1 if they actually used the key, 0 if not.
   WIDGET_HOTKEY:  1 if they actually used the key, 0 if not.
*/

/* Dialog messages */
enum {
    DLG_KEY,			/* Sent on keypress before sending to widget */
    DLG_INIT,			/* Sent on init */
    DLG_END,			/* Sent on shutdown */
    DLG_ACTION,			
    DLG_DRAW,			/* Sent for updating dialog managed area */
    DLG_FOCUS,			/* Sent on give focus to a widget */
    DLG_UNFOCUS,		/* Sent on remove focus from widget */
    DLG_ONE_UP,			/* Sent on selecting next */
    DLG_ONE_DOWN,		/* Sent on selecting prev */
    DLG_POST_KEY,		/* Sent after key has been sent */
    DLG_IDLE,			/* Sent if idle is active */
    DLG_UNHANDLED_KEY,		/* Send if no widget wanted the key */
    DLG_HOTKEY_HANDLED,		/* Send if a child got the hotkey */
    DLG_PRE_EVENT               /* Send before calling get_event */
} /* Dialog_Messages */;

typedef unsigned long widget_data;
typedef struct Dlg_head {
    int *color;			/* color set */
    int count;			/* number of widgets */
    int ret_value;

    /* mouse status */
    int mouse_status;		/* For the autorepeat status of the mouse */

    void *previous_dialog;	/* Pointer to the previously running Dlg_head */
    int  refresh_pushed;	/* Did the dialog actually run? */
    
    /* position */
    int x, y;			/* Position relative to screen origin */
    
    /* Flags */
    int running;
    int direction;
    int send_idle_msg;

    char *name;			/* Dialog name Tk code */
    char *help_ctx;

    /* Internal variables */
    struct Widget_Item *current, *first, *last;
    int (*callback) (struct Dlg_head *, int, int);
    
    struct Widget_Item *initfocus;

    /* Hacks */
    char *title;

    int cols;
    int lines;
    void *data;
    
    int  has_menubar;	/* GrossHack: Send events on row 1 to a menubar? */
    int  raw;		/* Should the tab key be sent to the dialog? */
    
    widget_data wdata;
    int  grided;	/* Does it use the automatic layout? */
#ifdef HAVE_GNOME
    int  idle_fn_tag;	/* Tag for the idle routine, -1 if none */
#endif
} Dlg_head;

/* XView widget layout */

typedef enum { 
    XV_WLAY_DONTCARE, /* Place the widget wherever it is reasonable */
    
    XV_WLAY_RIGHTOF,  /* Place the widget to the right of the last widget
		       * created - note: add_widget creates widgets from
		       *  the last to the first one.
		       */
    
    XV_WLAY_BELOWOF,  /* Place it in a column like style */
    
    XV_WLAY_BELOWCLOSE,/* The same, but without any gap between them */
    
    XV_WLAY_NEXTROW,  /* Place it on the left margin with Y bellow all the
		       * previous widgets
		       */
    
    XV_WLAY_CENTERROW,/* The same as previous, but when the dialog is
		       * ready to show, tries to center that row of widgets
		       */
    
    XV_WLAY_NEXTCOLUMN, /* Place it on the top margin with X behind all the
		       * previous widgets
		       */
		       
    XV_WLAY_RIGHTDOWN, /* Place the widget to the right of the last one with
    		        * y set so that both y + h and yold + hold are equal.
    		          This is usefull if the previous widget was a radio,
    		          which has multiple lines */
    XV_WLAY_EXTENDWIDTH  /* Like nextrow, but later on tries to extend the widget
                          * to fit in the frame (only for PANEL_LIST) */
} WLay;

/* Every Widget must have this as it's first element */
typedef struct Widget {
    int x, y;
    int cols, lines;
    int color;			/* If the widget uses it, the color */
    int options;
    int focused;		/* Tells if the widget is focused */
    int (*callback)(Dlg_head *, void *, int, int);  /* The callback function */
    void (*destroy)(void *);
    mouse_h mouse;
    struct Dlg_head *parent;
    widget_data wdata;
    widget_data wcontainer;   /* For children of midnight_dlg, identifies
                               * the frame in which they should reside
			       */
    char *frame;		/* Tk version: frame containing it */
    char *tkname;		/* Tk version: widget name */
    enum {
        AREA_TOP,
        AREA_LEFT,
        AREA_RIGHT,
        AREA_BOTTOM
    } area; /* Used by X platforms, should stay here always because the size
               of this structure has to be same everywhere :) */
    WLay  layout;
} Widget;

/* The options for the widgets */
#define  W_WANT_POST_KEY     1
#define  W_WANT_HOTKEY       2
#define  W_WANT_CURSOR       4
#define  W_WANT_IDLE         8
#define  W_IS_INPUT         16
#define  W_PANEL_HIDDEN     32

typedef struct Widget_Item {
    int dlg_id;
    struct Widget_Item *next;	/* next in circle buffer */
    struct Widget_Item *prev;	/* previous in circle buffer */
    Widget *widget;		/* point to the component */
} Widget_Item;

/* draw box in window */
void draw_box (Dlg_head *h, int y, int x, int ys, int xs);

/* doubled line if possible */
void draw_double_box (Dlg_head *h, int y, int x, int ys, int xs);

/* Creates a dialog head  */
Dlg_head *create_dlg (int y1, int x1, int lines, int cols,
		      int *col,
		      int (*callback) (struct Dlg_head *, int, int),
		      char *help_ctx, char *name, int flags);

/* The flags: */
#define DLG_NO_TOPLEVEL 32	/* GNOME only: Do not create a toplevel window, user provides it */
#define DLG_GNOME_APP   16      /* GNOME only: use a gnome-app for the toplevel window */
#define DLG_NO_TED       8      /* GNOME only: do not manage layout with a GNOME GtkTed widget */
#define DLG_GRID         4      /* Widgets should be created under .widgets */
#define DLG_TRYUP        2	/* Try to move two lines up the dialog */
#define DLG_CENTER       1	/* Center the dialog */
#define DLG_NONE         0	/* No options */
int  add_widget           (Dlg_head *dest, void *Widget);
int  add_widgetl          (Dlg_head *dest, void *Widget, WLay layout);
int  remove_widget        (Dlg_head *dest, void *Widget);
int  destroy_widget       (Widget *w);

/* Runs dialog d */       
void run_dlg              (Dlg_head *d);
		          
void dlg_run_done         (Dlg_head *h);
void dlg_process_event    (Dlg_head *h, int key, Gpm_Event *event);
void init_dlg             (Dlg_head *h);

/* To activate/deactivate the idle message generation */
void set_idle_proc        (Dlg_head *d, int state);
		          
void dlg_redraw           (Dlg_head *h);
void dlg_refresh          (void *parameter);
void destroy_dlg          (Dlg_head *h);
		          
void widget_set_size      (Widget *widget, int x1, int y1, int x2, int y2);

void dlg_broadcast_msg_to (Dlg_head *h, int message, int reverse, int flags);
void dlg_broadcast_msg    (Dlg_head *h, int message, int reverse);
void dlg_mouse            (Dlg_head *h, Gpm_Event *event);

typedef void  (*destroy_fn)(void *);
typedef int   (*callback_fn)(Dlg_head *, void *, int, int);

void init_widget (Widget *w, int y, int x, int lines, int cols,
		  callback_fn callback, destroy_fn destroy,
		  mouse_h mouse_handler, char *tkname);

/* Various default service provision */
int default_dlg_callback  (Dlg_head *h, int id, int msg);
int std_callback          (Dlg_head *h, int Msg, int Par);
int default_proc          (Dlg_head *h, int Msg, int Par);

#define real_widget_move(w, _y, _x) move((w)->y + _y, (w)->x + _x)
#define dlg_move(h, _y, _x) move(((Dlg_head *) h)->y + _y, \
			     ((Dlg_head *) h)->x + _x)

#define widget_move(w,y,x) real_widget_move((Widget*)w,y,x)


extern Dlg_head *current_dlg;
extern Hook *idle_hook;

int  send_message         (Dlg_head *h, Widget *w, int msg, int par);
int  send_message_to      (Dlg_head *h, Widget *w, int msg, int par);
void dlg_replace_widget   (Dlg_head *h, Widget *old, Widget *new);
void widget_redraw        (Dlg_head *h, Widget_Item *w);
int  dlg_overlap          (Widget *a, Widget *b);
void widget_erase         (Widget *);
void dlg_erase            (Dlg_head *h);
void dlg_stop             (Dlg_head *h);

/* Widget selection */
int  dlg_select_widget     (Dlg_head *h, void *widget);
void dlg_one_up            (Dlg_head *h);
void dlg_one_down          (Dlg_head *h);
int  dlg_focus             (Dlg_head *h);
int  dlg_unfocus           (Dlg_head *h);
int  dlg_select_nth_widget (Dlg_head *h, int n);
int  dlg_item_number       (Dlg_head *h);
Widget *find_widget_type   (Dlg_head *h, callback_fn signature);

/* Sets/clear the specified flag in the options field */
#define widget_option(w,f,i) \
    w.options = ((i) ? (w.options | (f)) : (w.options & (~(f))))

#define widget_want_cursor(w,i) widget_option(w, W_WANT_CURSOR, i)
#define widget_want_hotkey(w,i) widget_option(w, W_WANT_HOTKEY, i)
#define widget_want_postkey(w,i) widget_option(w, W_WANT_POSTKEY, i)

typedef void (*movefn)(void *, int);

/* Layout definitions */

void xv_Layout (void *first_widget, ...);
void tk_layout (void *first_widget, ...);
void tk_new_frame (Dlg_head *, char *);
void tk_frame (Dlg_head *, char *);
void tk_end_frame ();
void x_set_dialog_title (Dlg_head *h, char *title);

/* The inner workings of run_dlg, exported for the Tk and XView toolkits */
void dlg_key_event (Dlg_head *h, int d_key);
void update_cursor (Dlg_head *h);

#ifdef HAVE_X
extern Dlg_head *midnight_dlg;
void x_focus_widget      (Widget_Item *p);
void x_unfocus_widget    (Widget_Item *p);
void x_init_dlg          (Dlg_head *h);
void x_destroy_dlg       (Dlg_head *h);
void x_destroy_dlg_start (Dlg_head *h);
void x_set_idle          (Dlg_head *h, int enable_idle);
void x_dialog_stop       (Dlg_head *h);
#else
#    define x_focus_widget(x) {}
#    define x_unfocus_widget(x) {}
#    define x_init_dlg(x)     {}
#    define x_destroy_dlg(x)  {}
#    define x_destroy_dlg_start(x) {}
#endif

#endif /* MC_DLG_H */
