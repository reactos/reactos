/* {{{  */

/* {{{ Copyright Notice */

/* Widget based utility functions.
   Copyright (C) 1994, 1995 the Free Software Foundation
   
   Authors: 1994, 1995, 1996 Miguel de Icaza
            1994, 1995 Radek Doulik
	    1995  Jakub Jelinek
	    1995  Andrej Borsenkow

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 */

/* }}} */

/*  [] = "$Id: wtools.c,v 1.1 2001/12/30 09:55:20 sedwards Exp $" */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "tty.h"
#include <stdarg.h>
#include "mad.h"
#include "global.h"
#include "util.h"
#include "win.h"
#include "color.h"
#include "mouse.h"
#include "dlg.h"
#include "widget.h"
#include "menu.h"
#include "wtools.h"
#include "key.h"	/* For mi_getch() */
#include "dialog.h"	/* For do_refresh() and my_wputs() */
#include "complete.h"   /* INPUT_COMPLETE_CD */
#include "x.h"

/* }}} */

/* {{{ Common dialog callback */
#ifdef HAVE_X
void dialog_repaint (struct Dlg_head *h, int back, int title_fore)
{
}
#else
void
dialog_repaint (struct Dlg_head *h, int back, int title_fore)
{
    attrset (back);
    dlg_erase (h);
    draw_box (h, 1, 1, h->lines - 2, h->cols - 2);
    attrset (title_fore);
    if (h->title){
	dlg_move (h, 1, (h->cols-strlen (h->title))/2);
	addstr (h->title);
    }
}
#endif
void
common_dialog_repaint (struct Dlg_head *h)
{
    dialog_repaint (h, COLOR_NORMAL, COLOR_HOT_NORMAL);
}

int
common_dialog_callback (struct Dlg_head *h, int id, int msg)
{
    if (msg == DLG_DRAW)
	common_dialog_repaint (h);
    return 0;
}

/* }}} */
/* {{{ Listbox utility functions */

void
listbox_refresh (Dlg_head *h)
{
    dialog_repaint (h, COLOR_NORMAL, COLOR_HOT_NORMAL);
}

static int listbox_callback (Dlg_head *h, int id, int msg)
{
    switch (msg) {
    case DLG_DRAW:
#ifndef HAVE_X
        listbox_refresh(h);
#endif
        return 1;
    }
    return 0;
}

Listbox *create_listbox_window (int cols, int lines, char *title, char *help)
{
    int xpos, ypos, len;
    Listbox  *listbox = xmalloc (sizeof (Listbox), "create_listbox_window");
    char* cancel_string = _("&Cancel");

    /* Adjust sizes */
    lines = (lines > LINES-6) ? LINES - 6 : lines;

	if (title && (cols < (len = strlen(title) + 2)))
		cols = len;

	/* no &, but 4 spaces around button for brackets and such */
	if (cols < (len = strlen(cancel_string) + 3))
		cols = len;
	
    cols = cols > COLS-6 ? COLS-6 : cols;

    /* I'm not sure if this -2 is safe, should test it */
    xpos = (COLS-cols)/2;
    ypos = (LINES-lines)/2 - 2;

    /* Create components */
    listbox->dlg = create_dlg (ypos, xpos, lines+6, cols+4, dialog_colors,
			       listbox_callback, help, "listbox", DLG_CENTER|DLG_GRID);
    x_set_dialog_title (listbox->dlg, title);	       
    
    listbox->list = listbox_new (2, 2, cols, lines, listbox_finish, 0, "li");

    add_widget (listbox->dlg,
		button_new (lines+3, (cols/2 + 2) - len/2, 
		B_CANCEL, NORMAL_BUTTON, cancel_string, 0, 0, "c"));
    add_widget (listbox->dlg, listbox->list);
#ifndef HAVE_X
    listbox_refresh(listbox->dlg);
#endif /* !HAVE_X */
    return listbox;
}

/* Returns the number of the item selected */
int run_listbox (Listbox *l)
{
    int val;
    
    run_dlg (l->dlg);
    if (l->dlg->ret_value == B_CANCEL)
	val = -1;
    else
	val = l->list->pos;
    destroy_dlg (l->dlg);
    free (l);
    return val;
}

/* }}} */


/* {{{ Query Dialog functions */
#ifndef HAVE_GNOME
struct text_struct {
    char *text;
    char *header;
};

static int query_callback (struct Dlg_head *h, int Id, int Msg)
{
    struct text_struct *info;

    info = (struct text_struct *) h->data;
    
    switch (Msg){
#ifndef HAVE_X    
    case DLG_DRAW:
	/* designate window */
	attrset (NORMALC);
	dlg_erase (h);
	draw_box (h, 1, 1, h->lines-2, h->cols-2);
	attrset (HOT_NORMALC);
	dlg_move (h, 1, (h->cols-strlen (info->header))/2);
	addstr (info->header);
	break;
#endif
    }
    return 0;
}


Dlg_head *last_query_dlg;

static int sel_pos = 0;

/* Used to ask questions to the user */
int query_dialog (char *header, char *text, int flags, int count, ...)
{
    va_list ap;
    Dlg_head *query_dlg;
    int win_len = 0;
    int i;
    int result = -1;
    int xpos, ypos;
    int cols, lines;
    char *cur_name;
    static int query_colors [4];
    static struct text_struct pass;
#ifdef HAVE_X    
    static char *buttonnames [10];
#endif
    
    /* set dialog colors */
    query_colors [0] = (flags & D_ERROR) ? ERROR_COLOR :  Q_UNSELECTED_COLOR;
    query_colors [1] = (flags & D_ERROR) ? REVERSE_COLOR : Q_SELECTED_COLOR;
    query_colors [2] = (flags & D_ERROR) ? ERROR_COLOR : COLOR_HOT_NORMAL;
    query_colors [3] = (flags & D_ERROR) ? COLOR_HOT_NORMAL :  COLOR_HOT_FOCUS;
    
    if (header == MSG_ERROR)
	    header = _(" Error ");
    
    if (count > 0){
	va_start (ap, count);
	for (i = 0; i < count; i++)
	{
		char* cp = va_arg (ap, char *);
	    win_len += strlen (cp) + 6;
		if (strchr (cp, '&') != NULL)
			win_len--;
	}
	va_end (ap);
    }

    /* count coordinates */
    cols = 6 + max (win_len, max (strlen (header), msglen (text, &lines)));
    lines += 4 + (count > 0 ? 2 : 0);
    xpos = COLS/2 - cols/2;
    ypos = LINES/3 - (lines-3)/2;
    pass.header = header;
    pass.text   = text;

    /* prepare dialog */
    query_dlg = create_dlg (ypos, xpos, lines, cols, query_colors,
			    query_callback, "[QueryBox]", "query", DLG_NONE);
    x_set_dialog_title (query_dlg, header);

    /* The data we need to pass to the callback */
    query_dlg->cols = cols; 
    query_dlg->lines = lines; 
    query_dlg->data  = &pass;
	
    query_dlg->direction = DIR_BACKWARD;

    if (count > 0){

	cols = (cols-win_len-2)/2+2;
	va_start (ap, count);
	for (i = 0; i < count; i++){
	    cur_name = va_arg (ap, char *);
	    xpos = strlen (cur_name)+6;
		if (strchr(cur_name, '&') != NULL)
			xpos--;
#ifndef HAVE_XVIEW
	    add_widget (query_dlg, button_new
			(lines-3, cols, B_USER+i, NORMAL_BUTTON, cur_name,
			 0, 0, NULL));
#else			 
	    buttonnames [i] = cur_name;
#endif			     
	    cols += xpos;
	    if (i == sel_pos)
		query_dlg->initfocus = query_dlg->current;
	}
	va_end (ap);

#ifdef HAVE_XVIEW
	for (i = count - 1; i >= 0; i--)
	    add_widgetl (query_dlg, button_new
			 (0, 0, B_USER+i, NORMAL_BUTTON, buttonnames [i], 0, 0, NULL), 
			 i ? XV_WLAY_RIGHTOF : XV_WLAY_CENTERROW);
#endif	
	add_widget (query_dlg, label_new (2, 3, text, NULL));
	
	/* run dialog and make result */
	run_dlg (query_dlg);
	switch (query_dlg->ret_value){
	case B_CANCEL:
	    break;
	default:
	    result = query_dlg->ret_value-B_USER;
	}

	/* free used memory */
	destroy_dlg (query_dlg);
    } else {
#ifdef HAVE_X
	add_widgetl (query_dlg, button_new(0, 0, B_EXIT, NORMAL_BUTTON, _("&Ok"), 0, 0, NULL),
	    XV_WLAY_CENTERROW);

	add_widget (query_dlg, label_new (2, 3, text, NULL));
#ifdef HAVE_TK
	if (flags & D_INSERT){
	} else
#endif
	{
	    run_dlg (query_dlg);
	    destroy_dlg (query_dlg);
	}
#else
	add_widget (query_dlg, label_new (2, 3, text, NULL));
	add_widget (query_dlg, button_new(0, 0, 0, HIDDEN_BUTTON, "-", 0, 0, NULL));
#endif /* HAVE_X */
	last_query_dlg = query_dlg;
    }
    sel_pos = 0;
    return result;
}

void query_set_sel (int new_sel)
{
    sel_pos = new_sel;
}

/* }}} */

/* {{{ The message function */

/* To show nice messages to the users */
Dlg_head *message (int error, char *header, char *text, ...)
{
    va_list  args;
    char     buffer [4096];
    Dlg_head *d;

    /* Setup the display information */
    strcpy (buffer, "\n");
    va_start (args, text);
    vsprintf (&buffer [1], text, args);
    strcat (buffer, "\n");
    va_end (args);
    
    query_dialog (header, buffer, error, 0);
#ifndef HAVE_XVIEW
    d = last_query_dlg;
#ifdef HAVE_TK
    if (error & D_INSERT){
	init_dlg (d);
	tk_dispatch_all ();
	return d;
    }
#else	
    init_dlg (d);
    if (!(error & D_INSERT)){
	mi_getch ();
	dlg_run_done (d);
	destroy_dlg (d);
    } else
	return d;
#endif
#endif	
    return 0;
}
#endif
/* }}} */

/* {{{ The chooser routines */

static int  remove_callback (int i, void *data)
{
    Chooser *c = (Chooser *) data;
    
    listbox_remove_current (c->listbox, 0);

    dlg_select_widget (c->dialog, c->listbox);
    dlg_select_nth_widget (c->dialog, 0);
    
    /* Return: do not abort dialog */
    return 0;
}

Chooser *new_chooser (int lines, int cols, char *help, int flags)
{
    Chooser  *c;
    int      button_lines;

    c = (Chooser *) xmalloc (sizeof (Chooser), "new_chooser");
    c->dialog = create_dlg (0, 0, lines, cols, dialog_colors, common_dialog_callback,
			    help, "chooser", DLG_CENTER | DLG_GRID);
    
    c->dialog->lines = lines;
    c->dialog->cols  = cols;
    
    button_lines = flags & CHOOSE_EDITABLE ? 3 : 0;
    
    c->listbox = listbox_new (1, 1, cols-2, lines-button_lines,
			   listbox_finish, 0, "listbox");
    
    if (button_lines){
	add_widget (c->dialog, button_new (lines-button_lines+1,
				20, B_ENTER, DEFPUSH_BUTTON, _("&Remove"),
				remove_callback, c, "button-remove"));
	add_widget (c->dialog, button_new (lines-button_lines+1,
				    4, B_CANCEL, NORMAL_BUTTON, _("&Cancel"),
				    0, 0, "button-cancel"));
    }		    
    add_widget (c->dialog, c->listbox);
    return c;
}

int run_chooser (Chooser *c)
{
    run_dlg (c->dialog);
    return c->dialog->ret_value;
}

void destroy_chooser (Chooser *c)
{
    destroy_dlg (c->dialog);
}

/* }}} */

/* {{{ Quick dialog routines */

static int quick_callback (struct Dlg_head *h, int id, int Msg)
{
    switch (Msg){
#ifndef HAVE_X    
    case DLG_DRAW:
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 1, 1, h->lines-2, h->cols-2);
	
	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1,((h->cols-strlen (h->data))/2));
	addstr (h->data);
	break;
#endif	
    case DLG_KEY:
	if (id == '\n'){
	    h->ret_value = B_ENTER;
	    dlg_stop (h);
	    break;
	}
    }
    return 0;
}

#define I18N(x) (do_int && *x ? (x = _(x)): x)

int quick_dialog_skip (QuickDialog *qd, int nskip)
{
    Dlg_head *dd;
    void     *widget;
    WRadio   *r;
    int      xpos;
    int      ypos;
    int      return_val;
    WInput   *input;
    QuickWidget *qw;
    int      do_int;

    if (!qd->i18n){
	qd->i18n = 1;
	do_int = 1;
	if (*qd->title)
	    qd->title = _(qd->title);
    } else
	do_int = 0;
    
    if (qd->xpos == -1)
        dd = create_dlg (0, 0, qd->ylen, qd->xlen, dialog_colors, quick_callback,
		         qd->help, qd->class, DLG_CENTER | DLG_TRYUP | DLG_GRID);
    else
        dd = create_dlg (qd->ypos, qd->xpos, qd->ylen, qd->xlen, dialog_colors, 
                         quick_callback,
		         qd->help, qd->class, DLG_GRID);

    x_set_dialog_title (dd, qd->title);
    
    /* We pass this to the callback */
    dd->cols  = qd->xlen;
    dd->lines = qd->ylen;
    dd->data  = qd->title;

    for (qw = qd->widgets; qw->widget_type; qw++){
#ifdef HAVE_X
	xpos = ypos = 0;
#else
	xpos = (qd->xlen * qw->relative_x)/qw->x_divisions;
	ypos = (qd->ylen * qw->relative_y)/qw->y_divisions;
#endif
	
	switch (qw->widget_type){
	case quick_checkbox:
	    widget = check_new (ypos, xpos, *qw->result, I18N (qw->text), qw->tkname);
	    break;

	case quick_radio:
	    r = radio_new (ypos, xpos, qw->hotkey_pos, qw->str_result, 1, qw->tkname);
	    r->pos = r->sel = qw->value;
	    widget = r;
	    break;
	    
	case quick_button:
	    widget = button_new (ypos, xpos, qw->value, (qw->value==B_ENTER) ? DEFPUSH_BUTTON : NORMAL_BUTTON,
	    I18N (qw->text), 0, 0, qw->tkname);
	    break;

	    /* We use the hotkey pos as the field length */
	case quick_input:
	    input = input_new (ypos, xpos, INPUT_COLOR,
			       qw->hotkey_pos, qw->text, qw->tkname);
	    input->is_password = qw->value == 1;
	    input->point = 0;
	    if (qw->value & 2)
	        input->completion_flags |= INPUT_COMPLETE_CD;
	    widget = input;
	    break;

	case quick_label:
	    widget = label_new (ypos, xpos, I18N(qw->text), qw->tkname);
	    break;
	    
	default:
	    widget = 0;
	    fprintf (stderr, "QuickWidget: unknown widget type\n");
	    break;
	}
	qw->the_widget = widget;
	add_widgetl (dd, widget, qw->layout);
    }

    while (nskip--)
	dd->current = dd->current->next;

    run_dlg (dd);

    /* Get the data if we found something interesting */
    if (dd->ret_value != B_CANCEL){
	for (qw = qd->widgets; qw->widget_type; qw++){
	    switch (qw->widget_type){
	    case quick_checkbox:
		*qw->result = ((WCheck *) qw->the_widget)->state & C_BOOL;
		break;

	    case quick_radio:
		*qw->result = ((WRadio *) qw->the_widget)->sel;
		break;
		
	    case quick_input:
		*qw->str_result = strdup (((WInput *) qw->the_widget)->buffer);
		break;
	    }
	}
    }
    return_val = dd->ret_value;
    destroy_dlg (dd);
    
    return return_val;
}

int quick_dialog (QuickDialog *qd)
{
    return quick_dialog_skip (qd, 0);
}

/* }}} */

/* {{{ Input routines */
#define INPUT_INDEX 2
char *real_input_dialog_help (char *header, char *text, char *help, char *def_text)
{
    QuickDialog Quick_input;
    QuickWidget quick_widgets [] = {
    { quick_button, 6, 10, 1, 0, N_("&Cancel"), 0, B_CANCEL, 0, 0,
	  XV_WLAY_RIGHTOF, "button-cancel" },
    { quick_button, 3, 10, 1, 0, N_("&Ok"), 0, B_ENTER, 0, 0,
	  XV_WLAY_CENTERROW, "button-ok" },
    { quick_input,  4, 80, 0, 0, "", 58, 0, 0, 0, XV_WLAY_NEXTROW, 0 },
    { quick_label,  3, 80, 2, 0, "", 0, 0, 0, 0, XV_WLAY_NEXTROW, "label" },
    { 0 } };
    
    int len;
    int i;
    int lines;
    char *my_str;
    char tk_name[64] = "inp|";
    
/* we need a unique name for tkname because widget.c:history_tool()
   needs a unique name for each dialog - using the header is ideal */

#ifdef HAVE_GNOME
    strncpy (tk_name + 4, header, 59);
#else
    strncpy (tk_name + 3, header, 60);
#endif
    tk_name[63] = '\0';
    quick_widgets[2].tkname = tk_name;

    len = max (strlen (header), msglen (text, &lines)) + 4;
    len = max (len, 64);

    if (strncmp (text, _("Password:"), strlen (_("Password"))) == 0){
	quick_widgets [INPUT_INDEX].value = 1;
	tk_name[3]=0;
    } else {
	quick_widgets [INPUT_INDEX].value = 0;
    }

#ifdef ENABLE_NLS
	/* 
	 * An attempt to place buttons symmetrically, based on actual i18n
	 * length of the string. It looks nicer with i18n (IMO) - alex
	 */
	quick_widgets [0].relative_x = len/2 + 4;
	quick_widgets [1].relative_x = 
		len/2 - (strlen (_(quick_widgets [1].text)) + 9);
	quick_widgets [0].x_divisions = quick_widgets [1].x_divisions = len;
#endif /* ENABLE_NLS */
    
    Quick_input.xlen  = len;
    Quick_input.xpos  = -1;
    Quick_input.title = header;
    Quick_input.help  = help;
    Quick_input.class = "quick_input";
    Quick_input.i18n  = 0;
    quick_widgets [INPUT_INDEX+1].text = text;
    quick_widgets [INPUT_INDEX].text = def_text;

    for (i = 0; i < 4; i++)
	quick_widgets [i].y_divisions = lines+6;
    Quick_input.ylen  = lines + 6;

    for (i = 0; i < 3; i++)
	quick_widgets [i].relative_y += 2 + lines;

    quick_widgets [INPUT_INDEX].str_result = &my_str;
    
    Quick_input.widgets = quick_widgets;
    if (quick_dialog (&Quick_input) != B_CANCEL){
	return *(quick_widgets [INPUT_INDEX].str_result);
    } else
	return 0;
}

char *input_dialog (char *header, char *text, char *def_text)
{
    return input_dialog_help (header, text, "[Input Line Keys]", def_text);
}

int input_dialog_help_2 (char *header, char *text1, char *text2, char *help, char **r1, char **r2)
{
    QuickDialog Quick_input;
    QuickWidget quick_widgets [] = {
    { quick_button, 6, 10, 4, 0, N_("&Cancel"), 0, B_CANCEL, 0, 0,
	  XV_WLAY_DONTCARE, "button-cancel" },
    { quick_button, 3, 10, 4, 0, N_("Ok")
, 0, B_ENTER, 0, 0,
	  XV_WLAY_DONTCARE, "button-ok" },
    { quick_input,  4, 80, 4, 0, "", 58, 0, 0, 0, XV_WLAY_BELOWCLOSE, "input-pth" },
    { quick_label,  3, 80, 3, 0, "", 0, 0, 0, 0, XV_WLAY_DONTCARE, "label-pth" },
    { quick_input,  4, 80, 3, 0, "", 58, 0, 0, 0, XV_WLAY_BELOWCLOSE, "input-lbl" },
    { quick_label,  3, 80, 2, 0, "", 0, 0, 0, 0, XV_WLAY_DONTCARE, "label-lbl" },
    { 0 } };
    
    int len;
    int i;
    int lines1, lines2;
    char *my_str1, *my_str2;
    
    len = max (strlen (header), msglen (text1, &lines1));
    len = max (len, msglen (text2, &lines2)) + 4;
    len = max (len, 64);

    Quick_input.xlen  = len;
    Quick_input.xpos  = -1;
    Quick_input.title = header;
    Quick_input.help  = help;
    Quick_input.class = "quick_input_2";
    Quick_input.i18n  = 0;
    quick_widgets [5].text = text1;
    quick_widgets [3].text = text2;

    for (i = 0; i < 6; i++)
	quick_widgets [i].y_divisions = lines1+lines2+7;
    Quick_input.ylen  = lines1 + lines2 + 7;

    quick_widgets [0].relative_y += (lines1 + lines2);
    quick_widgets [1].relative_y += (lines1 + lines2);
    quick_widgets [2].relative_y += (lines1);
    quick_widgets [3].relative_y += (lines1);

    quick_widgets [4].str_result = &my_str1;
    quick_widgets [2].str_result = &my_str2;
    
    Quick_input.widgets = quick_widgets;
    if (quick_dialog (&Quick_input) != B_CANCEL){
	 *r1 = *(quick_widgets [4].str_result);
	 *r2 = *(quick_widgets [2].str_result);
	 return 1;
    } else
	 return 0;
}

int input_dialog_2 (char *header, char *text1, char *text2, char **r1, char **r2)
{
    return input_dialog_help_2 (header, text1, text2,  "[Input Line Keys]", r1, r2);
}

char *input_expand_dialog (char *header, char *text, char *def_text)
{
    char *result;
    char *expanded;

    result = input_dialog (header, text, def_text);
    if (result){
	expanded = tilde_expand (result);
	if (expanded){
	    free (result);
	    return expanded;
	} else 
	    return result;
    }
    return result;
}

/* }}} */

/* }}} */
/*
  Cause emacs to enter folding mode for this file:
  Local variables:
  end:
*/
