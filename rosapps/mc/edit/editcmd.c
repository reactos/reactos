/* editor high level editing commands.

   Copyright (C) 1996, 1997 the Free Software Foundation

   Authors: 1996, 1997 Paul Sheer

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* #define PIPE_BLOCKS_SO_READ_BYTE_BY_BYTE */

#include <config.h>
#ifdef OS2_NT
#include <io.h>
#include <fcntl.h>
#endif
#include <ctype.h>
#include "edit.h"
#include "editcmddef.h"

#ifndef MIDNIGHT
#include <X11/Xatom.h>
#include "loadfile.h"
#endif

/* globals: */

/* search and replace: */
int replace_scanf = 0;
int replace_regexp = 0;
int replace_all = 0;
int replace_prompt = 1;
int replace_whole = 0;
int replace_case = 0;
int replace_backwards = 0;

/* queries on a save */
#ifdef MIDNIGHT
int edit_confirm_save = 1;
#else
int edit_confirm_save = 0;
#endif

#define NUM_REPL_ARGS 16
#define MAX_REPL_LEN 1024

#ifdef MIDNIGHT

static inline int my_lower_case (int c)
{
    return tolower(c);
}

char *strcasechr (const unsigned char *s, int c)
{
    for (; my_lower_case ((int) *s) != my_lower_case (c); ++s)
	if (*s == '\0')
	    return 0;
    return (char *) s;
}


#include "../src/mad.h"

#ifndef HAVE_MEMMOVE
/* for Christophe */
static void *memmove (void *dest, const void *src, size_t n)
{
    char *t, *s;

    if (dest <= src) {
	t = (char *) dest;
	s = (char *) src;
	while (n--)
	    *t++ = *s++;
    } else {
	t = (char *) dest + n;
	s = (char *) src + n;
	while (n--)
	    *--t = *--s;
    }
    return dest;
}
#endif

/* #define itoa MY_itoa  <---- this line is now in edit.h */
char *itoa (int i)
{
    static char t[14];
    char *s = t + 13;
    int j = i;
    *s-- = 0;
    do {
	*s-- = i % 10 + '0';
    } while ((i = i / 10));
    if (j < 0)
	*s-- = '-';
    return ++s;
}

/*
   This joins strings end on end and allocates memory for the result.
   The result is later automatically free'd and must not be free'd
   by the caller.
 */
char *catstrs (const char *first,...)
{
    static char *stacked[16] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    static int i = 0;
    va_list ap;
    int len;
    char *data;

    if (!first)
	return 0;

    len = strlen (first);
    va_start (ap, first);

    while ((data = va_arg (ap, char *)) != 0)
	 len += strlen (data);

    len++;

    i = (i + 1) % 16;
    if (stacked[i])
	free (stacked[i]);

    stacked[i] = malloc (len);
    va_end (ap);
    va_start (ap, first);
    strcpy (stacked[i], first);
    while ((data = va_arg (ap, char *)) != 0)
	 strcat (stacked[i], data);
    va_end (ap);

    return stacked[i];
}
#endif

#ifdef MIDNIGHT

void edit_help_cmd (WEdit * edit)
{
    char *hlpdir = concat_dir_and_file (mc_home, "mc.hlp");
    interactive_display (hlpdir, "[Internal File Editor]");
    free (hlpdir);
    edit->force |= REDRAW_COMPLETELY;
}

void edit_refresh_cmd (WEdit * edit)
{
#ifndef HAVE_SLANG
    clr_scr();
    do_refresh();
#else
    {
	int fg, bg;
	edit_get_syntax_color (edit, -1, &fg, &bg);
    }
    touchwin(stdscr);
#endif
    mc_refresh();
    doupdate();
}

#else

void edit_help_cmd (WEdit * edit)
{
}

void edit_refresh_cmd (WEdit * edit)
{
}

void CRefreshEditor (WEdit * edit)
{
    edit_refresh_cmd (edit);
}

#endif

#ifndef MIDNIGHT

/* three argument open */
int my_open (const char *pathname, int flags,...)
{
    int file;
    va_list ap;

    file = open ((char *) pathname, O_RDONLY);
    if (file < 0 && (flags & O_CREAT)) {	/* must it be created ? */
	mode_t mode;
	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);
	return creat ((char *) pathname, mode);
    }
    close (file);
    return open ((char *) pathname, flags);
}

#define open my_open

#endif

/* "Oleg Yu. Repin" <repin@ssd.sscc.ru> added backup filenames
    ...thanks -paul */

/*  If 0 (quick save) then  a) create/truncate <filename> file,
			    b) save to <filename>;
    if 1 (safe save) then   a) save to <tempnam>,
			    b) rename <tempnam> to <filename>;
    if 2 (do backups) then  a) save to <tempnam>,
			    b) rename <filename> to <filename.backup_ext>,
			    c) rename <tempnam> to <filename>. */

/* returns 0 on error */
int edit_save_file (WEdit * edit, const char *filename)
{
    long buf;
    long filelen = 0;
    int file;
    char *savename = (char *) filename;
    int this_save_mode;

    if ((file = open (savename, O_WRONLY)) == -1) {
	this_save_mode = 0;		/* the file does not exists yet, so no safe save or backup necessary */
    } else {
	close (file);
	this_save_mode = option_save_mode;
    }

    if (this_save_mode > 0) {
	char *savedir = ".", *slashpos = strrchr (filename, '/');
	if (slashpos != 0) {
	    savedir = strdup (filename);
	    if (savedir == 0)
		return 0;
	    savedir[slashpos - filename + 1] = '\0';
	}
#ifdef HAVE_MAD
	savename = strdup (tempnam (savedir, "cooledit"));
#else
	savename = tempnam (savedir, "cooledit");
#endif
	if (slashpos)
	    free (savedir);
	if (!savename)
	    return 0;
    }
    if ((file = open (savename, O_CREAT | O_WRONLY | O_TRUNC | MY_O_TEXT, edit->stat.st_mode)) == -1) {
	if (this_save_mode > 0)
	    free (savename);
	return 0;
    }
    chown (savename, edit->stat.st_uid, edit->stat.st_gid);
    buf = 0;
    while (buf <= (edit->curs1 >> S_EDIT_BUF_SIZE) - 1) {
	filelen += write (file, (char *) edit->buffers1[buf], EDIT_BUF_SIZE);
	buf++;
    }
    filelen += write (file, (char *) edit->buffers1[buf], edit->curs1 & M_EDIT_BUF_SIZE);

    if (edit->curs2) {
	edit->curs2--;
	buf = (edit->curs2 >> S_EDIT_BUF_SIZE);
	filelen += write (file, (char *) edit->buffers2[buf] + EDIT_BUF_SIZE - (edit->curs2 & M_EDIT_BUF_SIZE) - 1, 1 + (edit->curs2 & M_EDIT_BUF_SIZE));
	buf--;
	while (buf >= 0) {
	    filelen += write (file, (char *) edit->buffers2[buf], EDIT_BUF_SIZE);
	    buf--;
	}
	edit->curs2++;
    }
    close (file);

    if (filelen == edit->last_byte) {
	if (this_save_mode == 2) {
	    if (rename (filename, catstrs (filename, option_backup_ext, 0)) == -1) {	/* catstrs free's automatically */
		free (savename);
		return 0;
	    }
	}
	if (this_save_mode > 0) {
	    if (rename (savename, filename) == -1) {
		free (savename);
		return 0;
	    }
	    free (savename);
	}
	return 1;
    } else {
	if (this_save_mode > 0)
	    free (savename);
	return 0;
    }
}

#ifdef MIDNIGHT
/*
   I changed this from Oleg's original routine so
   that option_backup_ext works with coolwidgets as well. This
   does mean there is a memory leak - paul.
 */
void menu_save_mode_cmd (void)
{
#define DLG_X 36
#define DLG_Y 10
    static char *str_result;
    static int save_mode_new;
    static char *str[] =
    {
	"Quick save ",
	"Safe save ",
	"Do backups -->"};
    static QuickWidget widgets[] =
    {
	{quick_button, 18, DLG_X, 7, DLG_Y, "&Cancel", 0,
	 B_CANCEL, 0, 0, XV_WLAY_DONTCARE, "c"},
	{quick_button, 6, DLG_X, 7, DLG_Y, "&Ok", 0,
	 B_ENTER, 0, 0, XV_WLAY_DONTCARE, "o"},
	{quick_input, 23, DLG_X, 5, DLG_Y, 0, 9,
	 0, 0, &str_result, XV_WLAY_DONTCARE, "i"},
	{quick_label, 22, DLG_X, 4, DLG_Y, "Extension:", 0,
	 0, 0, 0, XV_WLAY_DONTCARE, "savemext"},
	{quick_radio, 4, DLG_X, 3, DLG_Y, "", 3,
	 0, &save_mode_new, str, XV_WLAY_DONTCARE, "t"},
	{0}};
    static QuickDialog dialog =
/* NLS ? */
    {DLG_X, DLG_Y, -1, -1, " Edit Save Mode ", "[Edit Save Mode]",
     "esm", widgets};

    widgets[2].text = option_backup_ext;
    widgets[4].value = option_save_mode;
    if (quick_dialog (&dialog) != B_ENTER)
	return;
    option_save_mode = save_mode_new;
    option_backup_ext = str_result;	/* this is a memory leak */
    option_backup_ext_int = 0;
    str_result[min (strlen (str_result), sizeof (int))] = '\0';
    memcpy ((char *) &option_backup_ext_int, str_result, strlen (option_backup_ext));
}

#endif

#ifdef MIDNIGHT

void edit_split_filename (WEdit * edit, char *f)
{
    if (edit->filename)
	free (edit->filename);
    edit->filename = strdup (f);
    if (edit->dir)
	free (edit->dir);
    edit->dir = strdup ("");
}

#else

void edit_split_filename (WEdit * edit, char *longname)
{
    char *exp, *p;
    exp = canonicalize_pathname (longname);	/* this ensures a full path */
    if (edit->filename)
	free (edit->filename);
    if (edit->dir)
	free (edit->dir);
    p = strrchr (exp, '/');
    edit->filename = strdup (++p);
    *p = 0;
    edit->dir = strdup (exp);
    free (exp);
}

#endif

/*  here we want to warn the user of overwriting an existing file, but only if they
   have made a change to the filename */
/* returns 1 on success */
int edit_save_as_cmd (WEdit * edit)
{
/* This heads the 'Save As' dialog box */
    char *exp = edit_get_save_file (edit->dir, edit->filename, _(" Save As "));
    int different_filename = 0;
    edit_push_action (edit, KEY_PRESS + edit->start_display);
    edit->force |= REDRAW_COMPLETELY;

    if (exp) {
	if (!*exp) {
	    free (exp);
	    return 0;
	} else {
	    if (strcmp(catstrs (edit->dir, edit->filename, 0), exp)) {
		int file;
		different_filename = 1;
		if ((file = open ((char *) exp, O_RDONLY)) != -1) {	/* the file exists */
		    close (file);
		    if (edit_query_dialog2 (_(" Warning "), 
		    _(" A file already exists with this name. "), 
/* Push buttons to over-write the current file, or cancel the operation */
		    _("Overwrite"), _("Cancel")))
			return 0;
		}
	    }
	    if (edit_save_file (edit, exp)) {
		edit_split_filename (edit, exp);
		free (exp);
		edit->modified = 0;
#ifdef MIDNIGHT
	        edit->delete_file = 0;
#endif		
		if (different_filename && !edit->explicit_syntax)
		    edit_load_syntax (edit, 0, 0);
		return 1;
	    } else {
		free (exp);
		edit_error_dialog (_(" Save as "), get_sys_error (_(" Error trying to save file. ")));
		return 0;
	    }
	}
    } else
	return 0;
}

/* {{{ Macro stuff starts here */

#ifdef MIDNIGHT
int raw_callback (struct Dlg_head *h, int key, int Msg)
{
    switch (Msg) {
    case DLG_DRAW:
	attrset (REVERSE_COLOR);
	dlg_erase (h);
	draw_box (h, 1, 1, h->lines - 2, h->cols - 2);

	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1, 2);
	printw (h->title);
	break;

    case DLG_KEY:
	h->running = 0;
	h->ret_value = key;
	return 1;
    }
    return 0;
}

/* gets a raw key from the keyboard. Passing cancel = 1 draws
   a cancel button thus allowing c-c etc.. Alternatively, cancel = 0 
   will return the next key pressed */
int edit_raw_key_query (char *heading, char *query, int cancel)
{
    int w = strlen (query) + 7;
    struct Dlg_head *raw_dlg = create_dlg (0, 0, 7, w, dialog_colors,
/* NLS ? */
					 raw_callback, "[Raw Key Query]",
					   "raw_key_input",
					   DLG_CENTER | DLG_TRYUP);
    x_set_dialog_title (raw_dlg, heading);
    raw_dlg->raw = 1;		/* to return even a tab key */
    if (cancel)
	add_widget (raw_dlg, button_new (4, w / 2 - 5, B_CANCEL, NORMAL_BUTTON, "Cancel", 0, 0, 0));
    add_widget (raw_dlg, label_new (3 - cancel, 2, query, 0));
    add_widget (raw_dlg, input_new (3 - cancel, w - 5, INPUT_COLOR, 2, "", 0));
    run_dlg (raw_dlg);
    w = raw_dlg->ret_value;
    destroy_dlg (raw_dlg);
    if (cancel)
	if (w == XCTRL ('g') || w == XCTRL ('c') || w == ESC_CHAR || w == B_CANCEL)
	    return 0;
/* hence ctrl-a (=B_CANCEL), ctrl-g, ctrl-c, and Esc are cannot returned */
    return w;
}

#else

int edit_raw_key_query (char *heading, char *query, int cancel)
{
    return CKeySymMod (CRawkeyQuery (0, 0, 0, heading, query));
}

#endif

/* creates a macro file if it doesn't exist */
static FILE *edit_open_macro_file (const char *r)
{
    char *filename;
    int file;
    filename = catstrs (home_dir, MACRO_FILE, 0);
    if ((file = open (filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
	return 0;
    close (file);
    return fopen (filename, r);
}

#define MAX_MACROS 1024
static int saved_macro[MAX_MACROS + 1] =
{0, 0};
static int saved_macros_loaded = 0;

/*
   This is just to stop the macro file be loaded over and over for keys
   that aren't defined to anything. On slow systems this could be annoying.
 */
int macro_exists (int k)
{
    int i;
    for (i = 0; i < MAX_MACROS && saved_macro[i]; i++)
	if (saved_macro[i] == k)
	    return i;
    return -1;
}

/* returns 1 on error */
int edit_delete_macro (WEdit * edit, int k)
{
    struct macro macro[MAX_MACRO_LENGTH];
    FILE *f, *g;
    int s, i, n, j = 0;

    if (saved_macros_loaded)
	if ((j = macro_exists (k)) < 0)
	    return 0;
    g = fopen (catstrs (home_dir, TEMP_FILE, 0), "w");
    if (!g) {
/* This heads the delete macro error dialog box */
	edit_error_dialog (_(" Delete macro "),
/* 'Open' = load temp file */
		 get_sys_error (_(" Error trying to open temp file ")));
	return 1;
    }
    f = edit_open_macro_file ("r");
    if (!f) {
/* This heads the delete macro error dialog box */
	edit_error_dialog (_(" Delete macro "),
/* 'Open' = load temp file */
		get_sys_error (_(" Error trying to open macro file ")));
	fclose (g);
	return 1;
    }
    for (;;) {
	n = fscanf (f, _("key '%d 0': "), &s);
	if (!n || n == EOF)
	    break;
	n = 0;
	while (fscanf (f, "%hd %hd, ", &macro[n].command, &macro[n].ch))
	    n++;
	fscanf (f, ";\n");
	if (s != k) {
	    fprintf (g, _("key '%d 0': "), s);
	    for (i = 0; i < n; i++)
		fprintf (g, "%hd %hd, ", macro[i].command, macro[i].ch);
	    fprintf (g, ";\n");
	}
    };
    fclose (f);
    fclose (g);
    if (rename (catstrs (home_dir, TEMP_FILE, 0), catstrs (home_dir, MACRO_FILE, 0)) == -1) {
/* This heads the delete macro error dialog box */
	edit_error_dialog (_(" Delete macro "),
	   get_sys_error (_(" Error trying to overwrite macro file ")));
	return 1;
    }
    if (saved_macros_loaded)
	memmove (saved_macro + j, saved_macro + j + 1, sizeof (int) * (MAX_MACROS - j - 1));
    return 0;
}

/* returns 0 on error */
int edit_save_macro_cmd (WEdit * edit, struct macro macro[], int n)
{
    FILE *f;
    int s, i;

    edit->force |= REDRAW_COMPLETELY;
    edit_push_action (edit, KEY_PRESS + edit->start_display);
/* This heads the 'Macro' dialog box */
    s = edit_raw_key_query (_(" Macro "),
/* Input line for a single key press follows the ':' */
    _(" Press the macro's new hotkey: "), 1);
    if (s) {
	if (edit_delete_macro (edit, s))
	    return 0;
	f = edit_open_macro_file ("a+");
	if (f) {
	    fprintf (f, _("key '%d 0': "), s);
	    for (i = 0; i < n; i++)
		fprintf (f, "%hd %hd, ", macro[i].command, macro[i].ch);
	    fprintf (f, ";\n");
	    fclose (f);
	    if (saved_macros_loaded) {
		for (i = 0; i < MAX_MACROS && saved_macro[i]; i++);
		saved_macro[i] = s;
	    }
	    return 1;
	} else
/* This heads the 'Save Macro' dialog box */
	    edit_error_dialog (_(" Save macro "), get_sys_error (_(" Error trying to open macro file ")));
    }
    return 0;
}

void edit_delete_macro_cmd (WEdit * edit)
{
    int command;

#ifdef MIDNIGHT
    command = CK_Macro (edit_raw_key_query (_(" Delete Macro "), _(" Press macro hotkey: "), 1));
#else
/* This heads the 'Delete Macro' dialog box */
    command = CK_Macro (CKeySymMod (CRawkeyQuery (0, 0, 0, _(" Delete Macro "), 
/* Input line for a single key press follows the ':' */
    _(" Press macro hotkey: "))));
#endif

    if (command == CK_Macro (0))
	return;

    edit_delete_macro (edit, command - 2000);
}

/* return 0 on error */
int edit_load_macro_cmd (WEdit * edit, struct macro macro[], int *n, int k)
{
    FILE *f;
    int s, i = 0, found = 0;

    if (saved_macros_loaded)
	if (macro_exists (k) < 0)
	    return 0;

    if ((f = edit_open_macro_file ("r"))) {
	struct macro dummy;
	do {
	    int u;
	    u = fscanf (f, _("key '%d 0': "), &s);
	    if (!u || u == EOF)
		break;
	    if (!saved_macros_loaded)
		saved_macro[i++] = s;
	    if (!found) {
		*n = 0;
		while (*n < MAX_MACRO_LENGTH && 2 == fscanf (f, "%hd %hd, ", &macro[*n].command, &macro[*n].ch))
		    (*n)++;
	    } else {
		while (2 == fscanf (f, "%hd %hd, ", &dummy.command, &dummy.ch));
	    }
	    fscanf (f, ";\n");
	    if (s == k)
		found = 1;
	} while (!found || !saved_macros_loaded);
	if (!saved_macros_loaded) {
	    saved_macro[i] = 0;
	    saved_macros_loaded = 1;
	}
	fclose (f);
	return found;
    } else
/* This heads the 'Load Macro' dialog box */
	edit_error_dialog (_(" Load macro "),
		get_sys_error (_(" Error trying to open macro file ")));
    return 0;
}

/* }}} Macro stuff starts here */

/* returns 1 on success */
int edit_save_confirm_cmd (WEdit * edit)
{
    char *f;

    if (edit_confirm_save) {
#ifdef MIDNIGHT
	f = catstrs (_(" Confirm save file? : "), edit->filename, " ", 0);
#else
	f = catstrs (_(" Confirm save file? : "), edit->dir, edit->filename, " ", 0);
#endif
/* Buttons to 'Confirm save file' query */
	if (edit_query_dialog2 (_(" Save file "), f, _("Save"), _("Cancel")))
	    return 0;
    }
    return edit_save_cmd (edit);
}


/* returns 1 on success */
int edit_save_cmd (WEdit * edit)
{
    edit->force |= REDRAW_COMPLETELY;
    if (!edit_save_file (edit, catstrs (edit->dir, edit->filename, 0)))
	return edit_save_as_cmd (edit);
    edit->modified = 0;
#ifdef MIDNIGHT
    edit->delete_file = 0;
#endif		

    return 1;
}


/* returns 1 on success */
int edit_new_cmd (WEdit * edit)
{
    edit->force |= REDRAW_COMPLETELY;
    if (edit->modified)
	if (edit_query_dialog2 (_(" Warning "), _(" Current text was modified without a file save. \n Continue discards these changes. "), _("Continue"), _("Cancel")))
	    return 0;
    edit->modified = 0;
    return edit_renew (edit);	/* if this gives an error, something has really screwed up */
}

int edit_load_cmd (WEdit * edit)
{
    char *exp;
    edit->force |= REDRAW_COMPLETELY;

    if (edit->modified)
	if (edit_query_dialog2 (_(" Warning "), _(" Current text was modified without a file save. \n Continue discards these changes. "), _("Continue"), _("Cancel")))
	    return 0;

    exp = edit_get_load_file (edit->dir, edit->filename, _(" Load "));

    if (exp) {
	if (!*exp) {
	    free (exp);
	} else {
	    int file;
	    if ((file = open ((char *) exp, O_RDONLY, MY_O_TEXT)) != -1) {
		close (file);
		edit_reload (edit, exp, 0, "", 0);
		edit_split_filename (edit, exp);
		free (exp);
		edit->modified = 0;
		return 1;
	    } else {
		free (exp);
/* Heads the 'Load' file dialog box */
		edit_error_dialog (_(" Load "), get_sys_error (_(" Error trying to open file for reading ")));
	    }
	}
    }
    return 0;
}

/*
   if mark2 is -1 then marking is from mark1 to the cursor.
   Otherwise its between the markers. This handles this.
   Returns 1 if no text is marked.
 */
int eval_marks (WEdit * edit, long *start_mark, long *end_mark)
{
    if (edit->mark1 != edit->mark2) {
	if (edit->mark2 >= 0) {
	    *start_mark = min (edit->mark1, edit->mark2);
	    *end_mark = max (edit->mark1, edit->mark2);
	} else {
	    *start_mark = min (edit->mark1, edit->curs1);
	    *end_mark = max (edit->mark1, edit->curs1);
	    edit->column2 = edit->curs_col;
	}
	return 0;
    } else {
	*start_mark = *end_mark = 0;
	edit->column2 = edit->column1 = 0;
	return 1;
    }
}

/*Block copy, move and delete commands */

void edit_block_copy_cmd (WEdit * edit)
{
    long start_mark, end_mark, current = edit->curs1;
    long count;
    char *copy_buf;

    if (eval_marks (edit, &start_mark, &end_mark))
	return;


    copy_buf = malloc (end_mark - start_mark);

/* all that gets pushed are deletes hence little space is used on the stack */

    edit_push_markers (edit);

    count = start_mark;
    while (count < end_mark) {
	copy_buf[end_mark - count - 1] = edit_get_byte (edit, count);
	count++;
    }
    while (count-- > start_mark) {
	edit_insert_ahead (edit, copy_buf[end_mark - count - 1]);
    }
    free (copy_buf);
    edit_scroll_screen_over_cursor (edit);

    if (start_mark < current && end_mark > current)
	edit_set_markers (edit, start_mark, end_mark + end_mark - start_mark, 0, 0);

    edit->force |= REDRAW_PAGE;
}


void edit_block_move_cmd (WEdit * edit)
{
    long count;
    long current;
    char *copy_buf;
    long start_mark, end_mark;

    if (eval_marks (edit, &start_mark, &end_mark))
	return;

    if (start_mark <= edit->curs1 && end_mark >= edit->curs1)
	return;

    if ((end_mark - start_mark) > option_max_undo / 2)
	if (edit_query_dialog2 (_(" Warning "), _(" Block is large, you may not be able to undo this action. "), _("Continue"), _("Cancel")))
	    return;

    copy_buf = malloc (end_mark - start_mark);

    edit_push_markers (edit);

    current = edit->curs1;
    edit_cursor_move (edit, start_mark - edit->curs1);
    edit_scroll_screen_over_cursor (edit);

    count = start_mark;
    while (count < end_mark) {
	copy_buf[end_mark - count - 1] = edit_delete (edit);
	count++;
    }
    edit_scroll_screen_over_cursor (edit);

    edit_cursor_move (edit, current - edit->curs1 
	- (((current - edit->curs1) > 0) ? end_mark - start_mark : 0));
    edit_scroll_screen_over_cursor (edit);

    while (count-- > start_mark) {
	edit_insert_ahead (edit, copy_buf[end_mark - count - 1]);
	edit_set_markers (edit, edit->curs1, edit->curs1 + end_mark - start_mark, 0, 0);
    }
    edit_scroll_screen_over_cursor (edit);

    free (copy_buf);
    edit->force |= REDRAW_PAGE;
}

void edit_cursor_to_bol (WEdit * edit);

extern int column_highlighting;

void edit_delete_column_of_text (WEdit * edit)
{
    long p, q, r, m1, m2;
    int b, c, d, fin;

    eval_marks (edit, &m1, &m2);
    c = edit_move_forward3 (edit, edit_bol (edit, m1), 0, m1);
    d = edit_move_forward3 (edit, edit_bol (edit, m2), 0, m2);

    b = min (c, d);
    c = max (c, d);

    fin = 0;
    while (!fin) {
	eval_marks (edit, &m1, &m2);
	r = edit_bol (edit, edit->curs1);
	p = edit_move_forward3 (edit, r, b, 0);
	q = edit_move_forward3 (edit, r, c, 0);
	if (p < m1)
	    p = m1;
	if (q >= m2) {
	    q = m2;
	    fin = 1;
	}
	edit_cursor_move (edit, p - edit->curs1);
	while (p != q) {	/* delete line between margins */
	    if (edit_get_byte (edit, edit->curs1) != '\n')
		edit_delete (edit);
	    q--;
	}
	if (!fin)		/* next line */
	    edit_cursor_move (edit, edit_move_forward (edit, edit->curs1, 1, 0) - edit->curs1);
    }
}

/* returns 1 if canceelled by user */
int edit_block_delete_cmd (WEdit * edit)
{
    long count;
    long start_mark, end_mark;

    if (eval_marks (edit, &start_mark, &end_mark)) {
	start_mark = edit_bol (edit, edit->curs1);
	end_mark = edit_eol (edit, edit->curs1) + 1;
    }
    if ((end_mark - start_mark) > option_max_undo / 2)
/* Warning message with a query to continue or cancel the operation */
	if (edit_query_dialog2 (_(" Warning "), _(" Block is large, you may not be able to undo this action. "), _(" Continue "), _(" Cancel ")))
	    return 1;

    edit_push_markers (edit);

    edit_cursor_move (edit, start_mark - edit->curs1);
    edit_scroll_screen_over_cursor (edit);

    count = start_mark;
    if (start_mark < end_mark) {
	if (column_highlighting) {
	    edit_delete_column_of_text (edit);
	} else {
	    while (count < end_mark) {
		edit_delete (edit);
		count++;
	    }
	}
    }
    edit_set_markers (edit, 0, 0, 0, 0);
    edit->force |= REDRAW_PAGE;

    return 0;
}


#ifdef MIDNIGHT

#define INPUT_INDEX 9
#define SEARCH_DLG_HEIGHT 10
#define REPLACE_DLG_HEIGHT 15
#define B_REPLACE_ALL B_USER+1
#define B_SKIP_REPLACE B_USER+2

int edit_replace_prompt (WEdit * edit, char *replace_text, int xpos, int ypos)
{
    if (replace_prompt) {
	QuickWidget quick_widgets[] =
	{
/* NLS  for hotkeys? */
	    {quick_button, 14, 18, 3, 6, "&Cancel", 0, B_CANCEL, 0,
	     0, XV_WLAY_DONTCARE, NULL},
	    {quick_button, 9, 18, 3, 6, "Replace &all", 0, B_REPLACE_ALL, 0,
	     0, XV_WLAY_DONTCARE, NULL},
	    {quick_button, 6, 18, 3, 6, "&Skip", 0, B_SKIP_REPLACE, 0,
	     0, XV_WLAY_DONTCARE, NULL},
	    {quick_button, 2, 18, 3, 6, "&Replace", 0, B_ENTER, 0,
	     0, XV_WLAY_DONTCARE, NULL},
	    {quick_label, 2, 50, 2, 6, 0,
	     0, 0, 0, XV_WLAY_DONTCARE, 0},
	    {0}};

	quick_widgets[4].text = catstrs (_(" Replace with: "), replace_text, 0);

	{
	    QuickDialog Quick_input =
	    {66, 6, 0, 0, N_(" Replace "),
	     "[Input Line Keys]", "quick_input", 0 /*quick_widgets */ };

	    Quick_input.widgets = quick_widgets;

	    Quick_input.xpos = xpos;
	    Quick_input.ypos = ypos;
	    return quick_dialog (&Quick_input);
	}
    } else
	return 0;
}



void edit_replace_dialog (WEdit * edit, char **search_text, char **replace_text, char **arg_order)
{
    int treplace_scanf = replace_scanf;
    int treplace_regexp = replace_regexp;
    int treplace_all = replace_all;
    int treplace_prompt = replace_prompt;
    int treplace_backwards = replace_backwards;
    int treplace_whole = replace_whole;
    int treplace_case = replace_case;

    char *tsearch_text;
    char *treplace_text;
    char *targ_order;
    QuickWidget quick_widgets[] =
    {
	{quick_button, 6, 10, 12, REPLACE_DLG_HEIGHT, "&Cancel", 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 2, 10, 12, REPLACE_DLG_HEIGHT, "&Ok", 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 25, 50, 11, REPLACE_DLG_HEIGHT, "Scanf &expression", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 25, 50, 10, REPLACE_DLG_HEIGHT, "Replace &all", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 25, 50, 9, REPLACE_DLG_HEIGHT, "Pr&ompt on replace", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 11, REPLACE_DLG_HEIGHT, "&Backwards", 0, 0, 
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 10, REPLACE_DLG_HEIGHT, "&Regular exprssn", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 9, REPLACE_DLG_HEIGHT, "&Whole words only", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 8, REPLACE_DLG_HEIGHT, "Case &sensitive", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_input,    3, 50, 7, REPLACE_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-argord"},
	{quick_label, 2, 50, 6, REPLACE_DLG_HEIGHT, " Enter replacement argument order eg. 3,2,1,4 ", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, 50, 5, REPLACE_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-replace"},
	{quick_label, 2, 50, 4, REPLACE_DLG_HEIGHT, " Enter replacement string", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, 50, 3, REPLACE_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-search"},
	{quick_label, 2, 50, 2, REPLACE_DLG_HEIGHT, " Enter search string", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[2].result = &treplace_scanf;
    quick_widgets[3].result = &treplace_all;
    quick_widgets[4].result = &treplace_prompt;
    quick_widgets[5].result = &treplace_backwards;
    quick_widgets[6].result = &treplace_regexp;
    quick_widgets[7].result = &treplace_whole;
    quick_widgets[8].result = &treplace_case;
    quick_widgets[9].str_result = &targ_order;
    quick_widgets[9].text = *arg_order;
    quick_widgets[11].str_result = &treplace_text;
    quick_widgets[11].text = *replace_text;
    quick_widgets[13].str_result = &tsearch_text;
    quick_widgets[13].text = *search_text;
    {
	QuickDialog Quick_input =
	{50, REPLACE_DLG_HEIGHT, -1, 0, N_(" Replace "),
	 "[Input Line Keys]", "quick_input", 0 /*quick_widgets */ };

	Quick_input.widgets = quick_widgets;

	if (quick_dialog (&Quick_input) != B_CANCEL) {
	    *arg_order = *(quick_widgets[INPUT_INDEX].str_result);
	    *replace_text = *(quick_widgets[INPUT_INDEX + 2].str_result);
	    *search_text = *(quick_widgets[INPUT_INDEX + 4].str_result);
	    replace_scanf = treplace_scanf;
	    replace_backwards = treplace_backwards;
	    replace_regexp = treplace_regexp;
	    replace_all = treplace_all;
	    replace_prompt = treplace_prompt;
	    replace_whole = treplace_whole;
	    replace_case = treplace_case;
	    return;
	} else {
	    *arg_order = NULL;
	    *replace_text = NULL;
	    *search_text = NULL;
	    return;
	}
    }
}


void edit_search_dialog (WEdit * edit, char **search_text)
{
    int treplace_scanf = replace_scanf;
    int treplace_regexp = replace_regexp;
    int treplace_whole = replace_whole;
    int treplace_case = replace_case;
    int treplace_backwards = replace_backwards;

    char *tsearch_text;
    QuickWidget quick_widgets[] =
    {
	{quick_button, 6, 10, 7, SEARCH_DLG_HEIGHT, "&Cancel", 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 2, 10, 7, SEARCH_DLG_HEIGHT, "&Ok", 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 25, 50, 6, SEARCH_DLG_HEIGHT, "Scanf &expression", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL },
	{quick_checkbox, 25, 50, 5, SEARCH_DLG_HEIGHT, "&Backwards", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 6, SEARCH_DLG_HEIGHT, "&Regular exprssn", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 5, SEARCH_DLG_HEIGHT, "&Whole words only", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_checkbox, 4, 50, 4, SEARCH_DLG_HEIGHT, "Case &sensitive", 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{quick_input, 3, 50, 3, SEARCH_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "edit-search"},
	{quick_label, 2, 50, 2, SEARCH_DLG_HEIGHT, " Enter search string", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[2].result = &treplace_scanf;
    quick_widgets[3].result = &treplace_backwards;
    quick_widgets[4].result = &treplace_regexp;
    quick_widgets[5].result = &treplace_whole;
    quick_widgets[6].result = &treplace_case;
    quick_widgets[7].str_result = &tsearch_text;
    quick_widgets[7].text = *search_text;

    {
	QuickDialog Quick_input =
	{50, SEARCH_DLG_HEIGHT, -1, 0, N_(" Search "),
	 "[Input Line Keys]", "quick_input", 0 /*quick_widgets */ };

	Quick_input.widgets = quick_widgets;

	if (quick_dialog (&Quick_input) != B_CANCEL) {
	    *search_text = *(quick_widgets[7].str_result);
	    replace_scanf = treplace_scanf;
	    replace_backwards = treplace_backwards;
	    replace_regexp = treplace_regexp;
	    replace_whole = treplace_whole;
	    replace_case = treplace_case;
	    return;
	} else {
	    *search_text = NULL;
	    return;
	}
    }
}


#else

#define B_ENTER 0
#define B_SKIP_REPLACE 1
#define B_REPLACE_ALL 2
#define B_CANCEL 3

extern CWidget *wedit;

void edit_search_replace_dialog (Window parent, int x, int y, char **search_text, char **replace_text, char **arg_order, char *heading, int option)
{
    Window win;
    XEvent xev;
    CEvent cev;
    CState s;
    int xh, yh, h, xb, ys, yc, yb, yr;
    CWidget *m;

    CBackupState (&s);
    CDisable ("*");

    win = CDrawHeadedDialog ("replace", parent, x, y, heading);
    CGetHintPos (&xh, &h);

/* NLS hotkey ? */
    CIdent ("replace")->position = WINDOW_ALWAYS_RAISED;
/* An input line comes after the ':' */
    (CDrawText ("replace.t1", win, xh, h, _(" Enter search text : ")))->hotkey = 'E';

    CGetHintPos (0, &yh);
    (m = CDrawTextInput ("replace.sinp", win, xh, yh, 10, AUTO_HEIGHT, 256, *search_text))->hotkey = 'E';

    if (replace_text) {
	CGetHintPos (0, &yh);
	(CDrawText ("replace.t2", win, xh, yh, _(" Enter replace text : ")))->hotkey = 'n';
	CGetHintPos (0, &yh);
	(CDrawTextInput ("replace.rinp", win, xh, yh, 10, AUTO_HEIGHT, 256, *replace_text))->hotkey = 'n';
	CGetHintPos (0, &yh);
	(CDrawText ("replace.t3", win, xh, yh, _(" Enter argument order : ")))->hotkey = 'o';
	CGetHintPos (0, &yh);
	(CDrawTextInput ("replace.ainp", win, xh, yh, 10, AUTO_HEIGHT, 256, *arg_order))->hotkey = 'o';
/* Tool hint */
	CSetToolHint ("replace.ainp", _("Enter the order of replacement of your scanf format specifiers"));
	CSetToolHint ("replace.t3", _("Enter the order of replacement of your scanf format specifiers"));
    }
    CGetHintPos (0, &yh);
    ys = yh;
/* The following are check boxes */
    CDrawSwitch ("replace.ww", win, xh, yh, replace_whole, _(" Whole words only "), 0);
    CGetHintPos (0, &yh);
    CDrawSwitch ("replace.case", win, xh, yh, replace_case, _(" Case sensitive "), 0);
    yc = yh;
    CGetHintPos (0, &yh);
    CDrawSwitch ("replace.reg", win, xh, yh, replace_regexp, _(" Regular expression "), 1);
    CSetToolHint ("replace.reg", _("See the regex man page for how to compose a regular expression"));
    CSetToolHint ("replace.reg.label", _("See the regex man page for how to compose a regular expression"));
    yb = yh;
    CGetHintPos (0, &yh);
    CGetHintPos (&xb, 0);

    if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
	CDrawSwitch ("replace.bkwd", win, xh, yh, replace_backwards, _(" Backwards "), 0);
/* Tool hint */
	CSetToolHint ("replace.bkwd", _("Warning: Searching backward can be slow"));
	CSetToolHint ("replace.bkwd.label", _("Warning: Searching backward can be slow"));
	yb = yh;
    }
    if (replace_text) {
	if (option & SEARCH_DIALOG_OPTION_BACKWARDS)
	    yr = yc;
	else
	    yr = ys;
    } else {
	yr = yb;
    }

    if (replace_text) {
	CDrawSwitch ("replace.pr", win, xb, yr, replace_prompt, _(" Prompt on replace "), 0);
/* Tool hint */
	CSetToolHint ("replace.pr", _("Ask before making each replacement"));
	CGetHintPos (0, &yr);
	CDrawSwitch ("replace.all", win, xb, yr, replace_all, _(" Replace all "), 0);
	CGetHintPos (0, &yr);
    }
    CDrawSwitch ("replace.scanf", win, xb, yr, replace_scanf, _(" Scanf expression "), 1);
/* Tool hint */
    CSetToolHint ("replace.scanf", _("Allows entering of a C format string, see the scanf man page"));

    get_hint_limits (&x, &y);
    CDrawPixmapButton ("replace.ok", win, x - WIDGET_SPACING - TICK_BUTTON_WIDTH, h, PIXMAP_BUTTON_TICK);
/* Tool hint */
    CSetToolHint ("replace.ok", _("Begin search, Enter"));
    CDrawPixmapButton ("replace.cancel", win, x - WIDGET_SPACING - TICK_BUTTON_WIDTH, h + WIDGET_SPACING + TICK_BUTTON_WIDTH, PIXMAP_BUTTON_CROSS);
/* Tool hint */
    CSetToolHint ("replace.cancel", _("Abort this dialog, Esc"));
    CSetSizeHintPos ("replace");
    CMapDialog ("replace");

    m = CIdent ("replace");
    CSetWidgetSize ("replace.sinp", m->width - WIDGET_SPACING * 3 - 4 - TICK_BUTTON_WIDTH, (CIdent ("replace.sinp"))->height);
    if (replace_text) {
	CSetWidgetSize ("replace.rinp", m->width - WIDGET_SPACING * 3 - 4 - TICK_BUTTON_WIDTH, (CIdent ("replace.rinp"))->height);
	CSetWidgetSize ("replace.ainp", m->width - WIDGET_SPACING * 3 - 4 - TICK_BUTTON_WIDTH, (CIdent ("replace.ainp"))->height);
    }
    CFocus (CIdent ("replace.sinp"));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent ("replace")) {
	    *search_text = 0;
	    break;
	}
	if (!strcmp (cev.ident, "replace.cancel") || cev.command == CK_Cancel) {
	    *search_text = 0;
	    break;
	}
	if (!strcmp (cev.ident, "replace.reg") || !strcmp (cev.ident, "replace.scanf")) {
	    if (CIdent ("replace.reg")->keypressed || CIdent ("replace.scanf")->keypressed) {
		if (!(CIdent ("replace.case")->keypressed)) {
		    CIdent ("replace.case")->keypressed = 1;
		    CExpose ("replace.case");
		}
	    }
	}
	if (!strcmp (cev.ident, "replace.ok") || cev.command == CK_Enter) {
	    if (replace_text) {
		replace_all = CIdent ("replace.all")->keypressed;
		replace_prompt = CIdent ("replace.pr")->keypressed;
		*replace_text = strdup (CIdent ("replace.rinp")->text);
		*arg_order = strdup (CIdent ("replace.ainp")->text);
	    }
	    *search_text = strdup (CIdent ("replace.sinp")->text);
	    replace_whole = CIdent ("replace.ww")->keypressed;
	    replace_case = CIdent ("replace.case")->keypressed;
	    replace_scanf = CIdent ("replace.scanf")->keypressed;
	    replace_regexp = CIdent ("replace.reg")->keypressed;

	    if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
		replace_backwards = CIdent ("replace.bkwd")->keypressed;
	    } else {
		replace_backwards = 0;
	    }

	    break;
	}
    }
    CDestroyWidget ("replace");
    CRestoreState (&s);
}



void edit_search_dialog (WEdit * edit, char **search_text)
{
/* Heads the 'Search' dialog box */
    edit_search_replace_dialog (WIN_MESSAGES, search_text, 0, 0, _(" Search "), SEARCH_DIALOG_OPTION_BACKWARDS);
}

void edit_replace_dialog (WEdit * edit, char **search_text, char **replace_text, char **arg_order)
{
/* Heads the 'Replace' dialog box */
    edit_search_replace_dialog (WIN_MESSAGES, search_text, replace_text, arg_order, _(" Replace "), SEARCH_DIALOG_OPTION_BACKWARDS);
}

int edit_replace_prompt (WEdit * edit, char *replace_text, int xpos, int ypos)
{
    if (replace_prompt) {
	int q;
	char *p, *r = 0;
	r = p = malloc (strlen (replace_text) + NUM_REPL_ARGS * 2);
	strcpy (p, replace_text);
	while ((p = strchr (p, '%'))) {		/* convert "%" to "%%" so no convertion is attempted */
	    memmove (p + 2, p + 1, strlen (p) + 1);
	    *(++p) = '%';
	    p++;
	}
	edit->force |= REDRAW_COMPLETELY;
	q = edit_query_dialog4 (_(" Replace "), 
/* This is for the confirm replace dialog box. The replaced string comes after the ':' */
	catstrs (_(" Replace with: "), r, 0), 
/* Buttons for the confirm replace dialog box. */
	_("Replace"), _("Skip"), _("Replace all"), _("Cancel"));
	if (r)
	    free (r);
	switch (q) {
	case 0:
	    return B_ENTER;
	case 1:
	    return B_SKIP_REPLACE;
	case 2:
	    return B_REPLACE_ALL;
	case -1:
	case 3:
	    return B_CANCEL;
	}
    }
    return 0;
}


#endif

long sargs[NUM_REPL_ARGS][256 / sizeof (long)];

#define SCANF_ARGS sargs[0], sargs[1], sargs[2], sargs[3], \
		     sargs[4], sargs[5], sargs[6], sargs[7], \
		     sargs[8], sargs[9], sargs[10], sargs[11], \
		     sargs[12], sargs[13], sargs[14], sargs[15]

#define PRINTF_ARGS sargs[argord[0]], sargs[argord[1]], sargs[argord[2]], sargs[argord[3]], \
		     sargs[argord[4]], sargs[argord[5]], sargs[argord[6]], sargs[argord[7]], \
		     sargs[argord[8]], sargs[argord[9]], sargs[argord[10]], sargs[argord[11]], \
		     sargs[argord[12]], sargs[argord[13]], sargs[argord[14]], sargs[argord[15]]

/* This function is a modification of mc-3.2.10/src/view.c:regexp_view_search() */
/* returns -3 on error in pattern, -1 on not found, found_len = 0 if either */
int string_regexp_search (char *pattern, char *string, int len, int match_type, int match_bol, int icase, int *found_len)
{
    static regex_t r;
    regmatch_t pmatch[1];
    static char *old_pattern = NULL;
    static int old_type, old_icase;

    if (!old_pattern || strcmp (old_pattern, pattern) || old_type != match_type || old_icase != icase) {
	if (old_pattern) {
	    regfree (&r);
	    free (old_pattern);
	    old_pattern = 0;
	}
	if (regcomp (&r, pattern, REG_EXTENDED | (icase ? REG_ICASE : 0))) {
	    *found_len = 0;
	    return -3;
	}
	old_pattern = strdup (pattern);
	old_type = match_type;
	old_icase = icase;
    }
    if (regexec (&r, string, 1, pmatch, ((match_bol || match_type != match_normal) ? 0 : REG_NOTBOL)) != 0) {
	*found_len = 0;
	return -1;
    }
    *found_len = pmatch[0].rm_eo - pmatch[0].rm_so;
    return (pmatch[0].rm_so);
}

/* thanks to  Liviu Daia <daia@stoilow.imar.ro>  for getting this
   (and the above) routines to work properly - paul */

long edit_find_string (long start, unsigned char *exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, int once_only)
{
    long p, q = 0;
    long l = strlen ((char *) exp), f = 0;
    int n = 0;

    for (p = 0; p < l; p++)	/* count conversions... */
	if (exp[p] == '%')
	    if (exp[++p] != '%')	/* ...except for "%%" */
		n++;

    if (replace_scanf || replace_regexp) {
	int c;
	unsigned char *buf;
	unsigned char mbuf[MAX_REPL_LEN * 2 + 3];

	replace_scanf = (!replace_regexp);	/* can't have both */

	buf = mbuf;

	if (replace_scanf) {
	    unsigned char e[MAX_REPL_LEN];
	    if (n >= NUM_REPL_ARGS)
		return -3;

	    if (replace_case) {
		for (p = start; p < last_byte && p < start + MAX_REPL_LEN; p++)
		    buf[p - start] = (*get_byte) (data, p);
	    } else {
		for (p = 0; exp[p] != 0; p++)
		    exp[p] = my_lower_case (exp[p]);
		for (p = start; p < last_byte && p < start + MAX_REPL_LEN; p++) {
		    c = (*get_byte) (data, p);
		    buf[p - start] = my_lower_case (c);
		}
	    }

	    buf[(q = p - start)] = 0;
	    strcpy ((char *) e, (char *) exp);
	    strcat ((char *) e, "%n");
	    exp = e;

	    while (q) {
		*((int *) sargs[n]) = 0;	/* --> here was the problem - now fixed: good */
		if (n == sscanf ((char *) buf, (char *) exp, SCANF_ARGS)) {
		    if (*((int *) sargs[n])) {
			*len = *((int *) sargs[n]);
			return start;
		    }
		}
		if (once_only)
		    return -2;
		if (q + start < last_byte) {
		    if (replace_case) {
			buf[q] = (*get_byte) (data, q + start);
		    } else {
			c = (*get_byte) (data, q + start);
			buf[q] = my_lower_case (c);
		    }
		    q++;
		}
		buf[q] = 0;
		start++;
		buf++;		/* move the window along */
		if (buf == mbuf + MAX_REPL_LEN) {	/* the window is about to go past the end of array, so... */
		    memmove (mbuf, buf, strlen ((char *) buf) + 1);	/* reset it */
		    buf = mbuf;
		}
		q--;
	    }
	} else {	/* regexp matching */
	    long offset = 0;
	    int found_start, match_bol, move_win = 0; 

	    while (start + offset < last_byte) {
		match_bol = (offset == 0 || (*get_byte) (data, start + offset - 1) == '\n');
		if (!move_win) {
		    p = start + offset;
		    q = 0;
		}
		for (; p < last_byte && q < MAX_REPL_LEN; p++, q++) {
		    mbuf[q] = (*get_byte) (data, p);
		    if (mbuf[q] == '\n')
			break;
		}
		q++;
		offset += q;
		mbuf[q] = 0;

		buf = mbuf;
		while (q) {
		    found_start = string_regexp_search ((char *) exp, (char *) buf, q, match_normal, match_bol, !replace_case, len);

		    if (found_start <= -2) {	/* regcomp/regexec error */
			*len = 0;
			return -3;
		    }
		    else if (found_start == -1)	/* not found: try next line */
			break;
		    else if (*len == 0) { /* null pattern: try again at next character */
			q--;
			buf++;
			match_bol = 0;
			continue;
		    }
		    else	/* found */
			return (start + offset - q + found_start);
		}
		if (once_only)
		    return -2;

		if (buf[q - 1] != '\n') { /* incomplete line: try to recover */
		    buf = mbuf + MAX_REPL_LEN / 2;
		    q = strlen ((char *) buf);
		    memmove (mbuf, buf, q);
		    p = start + q;
		    move_win = 1;
		}
		else
		    move_win = 0;
	    }
	}
    } else {
 	*len = strlen ((char *) exp);
	if (replace_case) {
	    for (p = start; p <= last_byte - l; p++) {
 		if ((*get_byte) (data, p) == (unsigned char)exp[0]) {	/* check if first char matches */
		    for (f = 0, q = 0; q < l && f < 1; q++)
 			if ((*get_byte) (data, q + p) != (unsigned char)exp[q])
			    f = 1;
		    if (f == 0)
			return p;
		}
		if (once_only)
		    return -2;
	    }
	} else {
	    for (p = 0; exp[p] != 0; p++)
		exp[p] = my_lower_case (exp[p]);

	    for (p = start; p <= last_byte - l; p++) {
		if (my_lower_case ((*get_byte) (data, p)) == (unsigned char)exp[0]) {
		    for (f = 0, q = 0; q < l && f < 1; q++)
			if (my_lower_case ((*get_byte) (data, q + p)) != (unsigned char)exp[q])
			    f = 1;
		    if (f == 0)
			return p;
		}
		if (once_only)
		    return -2;
	    }
	}
    }
    return -2;
}


long edit_find_forwards (long search_start, unsigned char *exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data, int once_only)
{				/*front end to find_string to check for
				   whole words */
    long p;
    p = search_start;

    while ((p = edit_find_string (p, exp, len, last_byte, get_byte, data, once_only)) >= 0) {
	if (replace_whole) {
/*If the bordering chars are not in option_whole_chars_search then word is whole */
	    if (!strcasechr (option_whole_chars_search, (*get_byte) (data, p - 1))
		&& !strcasechr (option_whole_chars_search, (*get_byte) (data, p + *len)))
		return p;
	    if (once_only)
		return -2;
	} else
	    return p;
	if (once_only)
	    break;
	p++;			/*not a whole word so continue search. */
    }
    return p;
}

long edit_find (long search_start, unsigned char *exp, int *len, long last_byte, int (*get_byte) (void *, long), void *data)
{
    long p;
    if (replace_backwards) {
	while (search_start >= 0) {
	    p = edit_find_forwards (search_start, exp, len, last_byte, get_byte, data, 1);
	    if (p == search_start)
		return p;
	    search_start--;
	}
    } else {
	return edit_find_forwards (search_start, exp, len, last_byte, get_byte, data, 0);
    }
    return -2;
}

#define is_digit(x) ((x) >= '0' && (x) <= '9')

#define snprintf(v) { \
		*p1++ = *p++; \
		*p1++ = '%'; \
		*p1++ = 'n'; \
		*p1 = '\0'; \
		sprintf(s,q1,v,&n); \
		s += n; \
	    }

/* this function uses the sprintf command to do a vprintf */
/* it takes pointers to arguments instead of the arguments themselves */
int sprintf_p (char *str, const char *fmt,...)
{
    va_list ap;
    int n;
    char *q, *p, *s = str;
    char q1[32];
    char *p1;

    va_start (ap, fmt);
    p = q = (char *) fmt;

    while ((p = strchr (p, '%'))) {
	n = (int) ((unsigned long) p - (unsigned long) q);
	strncpy (s, q, n);	/* copy stuff between format specifiers */
	s += n;
	*s = 0;
	q = p;
	p1 = q1;
	*p1++ = *p++;
	if (*p == '%') {
	    p++;
	    *s++ = '%';
	    q = p;
	    continue;
	}
	if (*p == 'n') {
	    p++;
/* do nothing */
	    q = p;
	    continue;
	}
	if (*p == '#')
	    *p1++ = *p++;
	if (*p == '0')
	    *p1++ = *p++;
	if (*p == '-')
	    *p1++ = *p++;
	if (*p == '+')
	    *p1++ = *p++;
	if (*p == '*') {
	    p++;
	    strcpy (p1, itoa (*va_arg (ap, int *)));	/* replace field width with a number */
	    p1 += strlen (p1);
	} else {
	    while (is_digit (*p))
		*p1++ = *p++;
	}
	if (*p == '.')
	    *p1++ = *p++;
	if (*p == '*') {
	    p++;
	    strcpy (p1, itoa (*va_arg (ap, int *)));	/* replace precision with a number */
	    p1 += strlen (p1);
	} else {
	    while (is_digit (*p))
		*p1++ = *p++;
	}
/* flags done, now get argument */
	if (*p == 's') {
	    snprintf (va_arg (ap, char *));
	} else if (*p == 'h') {
	    if (strchr ("diouxX", *p))
		snprintf (*va_arg (ap, short *));
	} else if (*p == 'l') {
	    *p1++ = *p++;
	    if (strchr ("diouxX", *p))
		snprintf (*va_arg (ap, long *));
	} else if (strchr ("cdiouxX", *p)) {
	    snprintf (*va_arg (ap, int *));
	} else if (*p == 'L') {
	    *p1++ = *p++;
	    if (strchr ("EefgG", *p))
		snprintf (*va_arg (ap, double *));	/* should be long double */
	} else if (strchr ("EefgG", *p)) {
	    snprintf (*va_arg (ap, double *));
	} else if (strchr ("DOU", *p)) {
	    snprintf (*va_arg (ap, long *));
	} else if (*p == 'p') {
	    snprintf (*va_arg (ap, void **));
	}
	q = p;
    }
    va_end (ap);
    sprintf (s, q);		/* print trailing leftover */
    return (unsigned long) s - (unsigned long) str + strlen (s);
}

static void regexp_error (WEdit *edit)
{
/* "Error: Syntax error in regular expression, or scanf expression contained too many %'s */
    edit_error_dialog (_(" Error "), _(" Invalid regular expression, or scanf expression with to many conversions "));
}

/* call with edit = 0 before shutdown to close memory leaks */
void edit_replace_cmd (WEdit * edit, int again)
{
    static char *old1 = NULL;
    static char *old2 = NULL;
    static char *old3 = NULL;
    char *exp1 = "";
    char *exp2 = "";
    char *exp3 = "";
    int replace_yes;
    int replace_continue;
    int i = 0;
    long times_replaced = 0, last_search;
    char fin_string[32];
    int argord[NUM_REPL_ARGS];

    if (!edit) {
	if (old1) {
	    free (old1);
	    old1 = 0;
	}
	if (old2) {
	    free (old2);
	    old2 = 0;
	}
	if (old3) {
	    free (old3);
	    old3 = 0;
	}
	return;
    }

    last_search = edit->last_byte;

    edit->force |= REDRAW_COMPLETELY;

    exp1 = old1 ? old1 : exp1;
    exp2 = old2 ? old2 : exp2;
    exp3 = old3 ? old3 : exp3;

    if (again) {
	if (!old1 || !old2)
	    return;
	exp1 = strdup (old1);
	exp2 = strdup (old2);
	exp3 = strdup (old3);
    } else {
	edit_push_action (edit, KEY_PRESS + edit->start_display);
	edit_replace_dialog (edit, &exp1, &exp2, &exp3);
    }

    if (!exp1 || !*exp1) {
	edit->force = REDRAW_COMPLETELY;
	if (exp1) {
	    free (exp1);
	    free (exp2);
	    free (exp3);
	}
	return;
    }
    if (old1)
	free (old1);
    if (old2)
	free (old2);
    if (old3)
	free (old3);
    old1 = strdup (exp1);
    old2 = strdup (exp2);
    old3 = strdup (exp3);

    {
	char *s;
	int ord;
	while ((s = strchr (exp3, ' ')))
	    memmove (s, s + 1, strlen (s));
	s = exp3;
	for (i = 0; i < NUM_REPL_ARGS; i++) {
	    if ((unsigned long) s != 1 && s < exp3 + strlen (exp3)) {
		if ((ord = atoi (s)))
		    argord[i] = ord - 1;
		else
		    argord[i] = i;
		s = strchr (s, ',') + 1;
	    } else
		argord[i] = i;
	}
    }

    replace_continue = replace_all;

    if (edit->found_len && edit->search_start == edit->found_start + 1 && replace_backwards)
	edit->search_start--;

    if (edit->found_len && edit->search_start == edit->found_start - 1 && !replace_backwards)
	edit->search_start++;

    do {
	int len = 0;
	long new_start;
	new_start = edit_find (edit->search_start, (unsigned char *) exp1, &len, last_search,
			(int (*) (void *, long)) edit_get_byte, (void *) edit);
	if (new_start == -3) {
	    regexp_error (edit);
	    break;
	}
	edit->search_start = new_start;
	/*returns negative on not found or error in pattern */

	if (edit->search_start >= 0) {
	    edit->found_start = edit->search_start;
	    i = edit->found_len = len;

	    edit_cursor_move (edit, edit->search_start - edit->curs1);
	    edit_scroll_screen_over_cursor (edit);

	    replace_yes = 1;

	    if (replace_prompt) {
		int l;
		l = edit->curs_row - edit->num_widget_lines / 3;
		if (l > 0)
		    edit_scroll_downward (edit, l);
		if (l < 0)
		    edit_scroll_upward (edit, -l);

		edit_scroll_screen_over_cursor (edit);
		edit->force |= REDRAW_PAGE;
		edit_render_keypress (edit);

		/*so that undo stops at each query */
		edit_push_key_press (edit);

		switch (edit_replace_prompt (edit, exp2,	/*and prompt 2/3 down */
					     edit->num_widget_columns / 2 - 33, edit->num_widget_lines * 2 / 3)) {
		case B_ENTER:
		    break;
		case B_SKIP_REPLACE:
		    replace_yes = 0;
		    break;
		case B_REPLACE_ALL:
		    replace_prompt = 0;
		    replace_continue = 1;
		    break;
		case B_CANCEL:
		    replace_yes = 0;
		    replace_continue = 0;
		    break;
		}
	    }
	    if (replace_yes) {	/* delete then insert new */
		if (replace_scanf) {
		    char repl_str[MAX_REPL_LEN + 2];
		    if (sprintf_p (repl_str, exp2, PRINTF_ARGS) >= 0) {
			times_replaced++;
			while (i--)
			    edit_delete (edit);
			while (repl_str[++i])
			    edit_insert (edit, repl_str[i]);
		    } else {
			edit_error_dialog (_(" Replace "), 
/* "Invalid regexp string or scanf string" */
			_(" Error in replacement format string. "));
			replace_continue = 0;
		    }
		} else {
		    times_replaced++;
		    while (i--)
			edit_delete (edit);
		    while (exp2[++i])
			edit_insert (edit, exp2[i]);
		}
		edit->found_len = i;
	    }
/* so that we don't find the same string again */
	    if (replace_backwards) {
		last_search = edit->search_start;
		edit->search_start--;
	    } else {
		edit->search_start += i;
		last_search = edit->last_byte;
	    }
	    edit_scroll_screen_over_cursor (edit);
	} else {
	    edit->search_start = edit->curs1;	/* try and find from right here for next search */
	    edit_update_curs_col (edit);

	    edit->force |= REDRAW_PAGE;
	    edit_render_keypress (edit);
	    if (times_replaced) {
		sprintf (fin_string, _(" %ld replacements made. "), times_replaced);
		edit_message_dialog (_(" Replace "), fin_string);
	    } else
		edit_message_dialog (_(" Replace "), _(" Search string not found. "));
	    replace_continue = 0;
	}
    } while (replace_continue);

    free (exp1);
    free (exp2);
    free (exp3);
    edit->force = REDRAW_COMPLETELY;
    edit_scroll_screen_over_cursor (edit);
}




void edit_search_cmd (WEdit * edit, int again)
{
    static char *old = NULL;
    char *exp = "";

    if (!edit) {
	if (old) {
	    free (old);
	    old = 0;
	}
	return;
    }

    exp = old ? old : exp;
    if (again) {		/*ctrl-hotkey for search again. */
	if (!old)
	    return;
	exp = strdup (old);
    } else {
	edit_search_dialog (edit, &exp);
	edit_push_action (edit, KEY_PRESS + edit->start_display);
    }

    if (exp) {
	if (*exp) {
	    int len = 0;
	    if (old)
		free (old);
	    old = strdup (exp);

	    if (edit->found_len && edit->search_start == edit->found_start + 1 && replace_backwards)
		edit->search_start--;

	    if (edit->found_len && edit->search_start == edit->found_start - 1 && !replace_backwards)
		edit->search_start++;

	    edit->search_start = edit_find (edit->search_start, (unsigned char *) exp, &len, edit->last_byte,
		   (int (*)(void *, long)) edit_get_byte, (void *) edit);

	    if (edit->search_start >= 0) {
		edit->found_start = edit->search_start;
		edit->found_len = len;

		edit_cursor_move (edit, edit->search_start - edit->curs1);
		edit_scroll_screen_over_cursor (edit);
		if (replace_backwards)
		    edit->search_start--;
		else
		    edit->search_start++;
	    } else if (edit->search_start == -3) {
		edit->search_start = edit->curs1;
		regexp_error (edit);
	    } else {
		edit->search_start = edit->curs1;
		edit_error_dialog (_(" Search "), _(" Search string not found. "));
	    }
	}
	free (exp);
    }
    edit->force |= REDRAW_COMPLETELY;
    edit_scroll_screen_over_cursor (edit);
}


/* Real edit only */
void edit_quit_cmd (WEdit * edit)
{
    edit_push_action (edit, KEY_PRESS + edit->start_display);

    edit->force |= REDRAW_COMPLETELY;
    if (edit->modified) {
#ifdef MIDNIGHT
	switch (edit_query_dialog3 (_(" Quit "), _(" File was modified, Save with exit? "), _("Cancel quit"), _("&Yes"), _("&No"))) {
#else
/* Confirm 'Quit' dialog box */
	switch (edit_query_dialog3 (_(" Quit "),
	_(" Current text was modified without a file save. \n Save with exit? "), _(" &Cancel quit "), _(" &Yes "), _(" &No "))) {
#endif
	case 1:
	    edit_push_markers (edit);
	    edit_set_markers (edit, 0, 0, 0, 0);
	    if (!edit_save_cmd (edit))
		return;
	    break;
	case 2:
#ifdef MIDNIGHT
	    if (edit->delete_file)
		unlink (catstrs (edit->dir, edit->filename, 0));
#endif
	    break;
	case 0:
	case -1:
	    return;
	}
    } 
#ifdef MIDNIGHT
    else if (edit->delete_file)
	unlink (catstrs (edit->dir, edit->filename, 0));

    edit->widget.parent->running = 0;
#else
    edit->stopped = 1;
#endif
}

#define TEMP_BUF_LEN 1024

extern int column_highlighting;

/* returns a null terminated length of text. Result must be free'd */
unsigned char *edit_get_block (WEdit * edit, long start, long finish, int *l)
{
    unsigned char *s, *r;
    r = s = malloc (finish - start + 1);
    if (column_highlighting) {
	*l = 0;
	while (start < finish) {	/* copy from buffer, excluding chars that are out of the column 'margins' */
	    int c, x;
	    x = edit_move_forward3 (edit, edit_bol (edit, start), 0, start);
	    c = edit_get_byte (edit, start);
	    if ((x >= edit->column1 && x < edit->column2)
	     || (x >= edit->column2 && x < edit->column1) || c == '\n') {
		*s++ = c;
		(*l)++;
	    }
	    start++;
	}
    } else {
	*l = finish - start;
	while (start < finish)
	    *s++ = edit_get_byte (edit, start++);
    }
    *s = 0;
    return r;
}

/* save block, returns 1 on success */
int edit_save_block (WEdit * edit, const char *filename, long start, long finish)
{
    long i = start, end, filelen = finish - start;
    int file;
    unsigned char *buf;

    if ((file = open ((char *) filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
	return 0;

    buf = malloc (TEMP_BUF_LEN);
    while (start != finish) {
	end = min (finish, start + TEMP_BUF_LEN);
	for (; i < end; i++)
	    buf[i - start] = edit_get_byte (edit, i);
	filelen -= write (file, (char *) buf, end - start);
	start = end;
    }
    free (buf);
    close (file);
    if (filelen)
	return 0;
    return 1;
}

/* copies a block to clipboard file */
static int edit_save_block_to_clip_file (WEdit * edit, long start, long finish)
{
    return edit_save_block (edit, catstrs (home_dir, CLIP_FILE, 0), start, finish);
}

#ifndef MIDNIGHT

void paste_text (WEdit * edit, unsigned char *data, unsigned int nitems)
{
    if (data) {
	data += nitems - 1;
	while (nitems--)
	    edit_insert_ahead (edit, *data--);
    }
    edit->force |= REDRAW_COMPLETELY;
}

char *selection_get_line (void *data, int line)
{
    static unsigned char t[1024];
    struct selection *s;
    int i = 0;
    s = (struct selection *) data;
    line = (current_selection + line + 1) % NUM_SELECTION_HISTORY;
    if (s[line].text) {
	unsigned char *p = s[line].text;
	int c, j;
	for (j = 0; j < s[line].len; j++) {
	    c = *p++;
	    if ((c < ' ' || (c > '~' && c < 160)) && c != '\t') {
		t[i++] = '.';
		t[i++] = '\b';
		t[i++] = '.';
	    } else
		t[i++] = c;
	    if (i > 1020)
		break;
	}
    }
    t[i] = 0;
    return (char *) t;
}

void edit_paste_from_history (WEdit * edit)
{
    int i, c;

    edit_update_curs_col (edit);
    edit_update_curs_row (edit);

    c = max (20, edit->num_widget_columns - 5);

    i = CListboxDialog (WIN_MESSAGES, c, 10,
	       0, NUM_SELECTION_HISTORY - 10, NUM_SELECTION_HISTORY - 1, NUM_SELECTION_HISTORY,
	       selection_get_line, (void *) selection_history);

    if (i < 0)
	return;

    i = (current_selection + i + 1) % NUM_SELECTION_HISTORY;

    paste_text (edit, selection_history[i].text, selection_history[i].len);
    edit->force |= REDRAW_COMPLETELY;
}

/* copies a block to the XWindows buffer */
static int edit_XStore_block (WEdit * edit, long start, long finish)
{
    edit_get_selection (edit);
    if (selection.len <= 512 * 1024) {	/* we don't want to fill up the server */
	XStoreBytes (CDisplay, (char *) selection.text, (int) selection.len);
	return 0;
    } else
	return 1;
}

int edit_copy_to_X_buf_cmd (WEdit * edit)
{
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    edit_XStore_block (edit, start_mark, end_mark);
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Copy to clipboard "), get_sys_error (_(" Unable to save to file. ")));
	return 1;
    }
    XSetSelectionOwner (CDisplay, XA_PRIMARY, edit->widget->winid, CurrentTime);
    edit_mark_cmd (edit, 1);
    return 0;
}

int edit_cut_to_X_buf_cmd (WEdit * edit)
{
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    edit_XStore_block (edit, start_mark, end_mark);
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Cut to clipboard "), _(" Unable to save to file. "));
	return 1;
    }
    edit_block_delete_cmd (edit);
    XSetSelectionOwner (CDisplay, XA_PRIMARY, edit->widget->winid, CurrentTime);
    edit_mark_cmd (edit, 1);
    return 0;
}

void selection_paste (WEdit * edit, Window win, unsigned prop, int delete);

void edit_paste_from_X_buf_cmd (WEdit * edit)
{
    if (selection.text)
	paste_text (edit, selection.text, selection.len);
    else if (!XGetSelectionOwner (CDisplay, XA_PRIMARY))
	selection_paste (edit, CRoot, XA_CUT_BUFFER0, False);
    else
	XConvertSelection (CDisplay, XA_PRIMARY, XA_STRING,
			   XInternAtom (CDisplay, "VT_SELECTION", False),
			   edit->widget->winid, CurrentTime);
    edit->force |= REDRAW_PAGE;
}

#else				/* MIDNIGHT */

void edit_paste_from_history (WEdit *edit)
{
}

int edit_copy_to_X_buf_cmd (WEdit * edit)
{
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Copy to clipboard "), get_sys_error (_(" Unable to save to file. ")));
	return 1;
    }
    edit_mark_cmd (edit, 1);
    return 0;
}

int edit_cut_to_X_buf_cmd (WEdit * edit)
{
    long start_mark, end_mark;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 0;
    if (!edit_save_block_to_clip_file (edit, start_mark, end_mark)) {
	edit_error_dialog (_(" Cut to clipboard "), _(" Unable to save to file. "));
	return 1;
    }
    edit_block_delete_cmd (edit);
    edit_mark_cmd (edit, 1);
    return 0;
}

void edit_paste_from_X_buf_cmd (WEdit * edit)
{
    edit_insert_file (edit, catstrs (home_dir, CLIP_FILE, 0));
}

#endif				/* MIDMIGHT */

void edit_goto_cmd (WEdit *edit)
{
    char *f;
    static int l = 0;
#ifdef MIDNIGHT
    char s[12];
    sprintf (s, "%d", l);
    f = input_dialog (_(" Goto line "), _(" Enter line: "), l ? s : "");
#else
    f = CInputDialog ("goto", WIN_MESSAGES, 150, l ? itoa (l) : "", _(" Goto line "), _(" Enter line: "));
#endif
    if (f) {
	if (*f) {
	    l = atoi (f);
	    edit_move_display (edit, l - edit->num_widget_lines / 2 - 1);
	    edit_move_to_line (edit, l - 1);
	    edit->force |= REDRAW_COMPLETELY;
	    free (f);
	}
    }
}

/*returns 1 on success */
int edit_save_block_cmd (WEdit * edit) {
    long start_mark, end_mark;
    char *exp;
    if (eval_marks (edit, &start_mark, &end_mark))
	return 1;

    exp = edit_get_save_file (edit->dir, catstrs (home_dir, CLIP_FILE, 0), _(" Save Block "));

    edit->force |= REDRAW_COMPLETELY;
    edit_push_action (edit, KEY_PRESS + edit->start_display);

    if (exp) {
	if (!*exp) {
	    free (exp);
	    return 0;
	} else {
	    if (edit_save_block (edit, exp, start_mark, end_mark)) {
		free (exp);
		edit->force |= REDRAW_COMPLETELY;
		return 1;
	    } else {
		free (exp);
		edit->force |= REDRAW_COMPLETELY;
		edit_error_dialog (_(" Save Block "), get_sys_error (_(" Error trying to save file. ")));
		return 0;
	    }
	}
    } else
	return 0;
}


/* inserts a file at the cursor, returns 1 on success */
int edit_insert_file (WEdit * edit, const char *filename)
{
    int i, file, blocklen;
    long current = edit->curs1;
    unsigned char *buf;

    if ((file = open ((char *) filename, O_RDONLY)) == -1)
	return 0;
    buf = malloc (TEMP_BUF_LEN);
    while ((blocklen = read (file, (char *) buf, TEMP_BUF_LEN)) > 0) {
	for (i = 0; i < blocklen; i++)
	    edit_insert (edit, buf[i]);
    }
    edit_cursor_move (edit, current - edit->curs1);
    free (buf);
    close (file);
    if (blocklen)
	return 0;
    return 1;
}


/* returns 1 on success */
int edit_insert_file_cmd (WEdit * edit) {
    char *exp = edit_get_load_file (edit->dir, catstrs (home_dir, CLIP_FILE, 0), _(" Insert File "));
    edit->force |= REDRAW_COMPLETELY;

    edit_push_action (edit, KEY_PRESS + edit->start_display);

    if (exp) {
	if (!*exp) {
	    free (exp);
	    return 0;
	} else {
	    if (edit_insert_file (edit, exp)) {
		free (exp);
		return 1;
	    } else {
		free (exp);
		edit_error_dialog (_(" Insert file "), get_sys_error (_(" Error trying to insert file. ")));
		return 0;
	    }
	}
    } else
	return 0;
}

#ifdef MIDNIGHT

/* sorts a block, returns -1 on system fail, 1 on cancel and 0 on success */
int edit_sort_cmd (WEdit * edit)
{
    static char *old = 0;
    char *exp;
    long start_mark, end_mark;
    int e;

    if (eval_marks (edit, &start_mark, &end_mark)) {
/* Not essential to translate */
	edit_error_dialog (_(" Sort block "), _(" You must first highlight a block of text. "));
	return 0;
    }
    edit_save_block (edit, catstrs (home_dir, BLOCK_FILE, 0), start_mark, end_mark);

    exp = old ? old : "";

    exp = input_dialog (_(" Run Sort "), 
/* Not essential to translate */
    _(" Enter sort options (see manpage) separated by whitespace: "), "");

    if (!exp)
	return 1;
    if (old)
	free (old);
    old = exp;

    e = system (catstrs (" sort ", exp, " ", home_dir, BLOCK_FILE, " > ", home_dir, TEMP_FILE, 0));
    if (e) {
	if (e == -1 || e == 127) {
	    edit_error_dialog (_(" Sort "), 
/* Not essential to translate */
	    get_sys_error (_(" Error trying to execute sort command ")));
	} else {
	    char q[8];
	    sprintf (q, "%d ", e);
	    edit_error_dialog (_(" Sort "), 
/* Not essential to translate */
	    catstrs (_(" Sort returned non-zero: "), q, 0));
	}
	return -1;
    }

    edit->force |= REDRAW_COMPLETELY;

    if (edit_block_delete_cmd (edit))
	return 1;
    edit_insert_file (edit, catstrs (home_dir, TEMP_FILE, 0));
    return 0;
}

/* if block is 1, a block must be highlighted and the shell command
   processes it. If block is 0 the shell command is a straight system
   command, that just produces some output which is to be inserted */
void edit_block_process_cmd (WEdit * edit, const char *shell_cmd, int block)
{
    long start_mark, end_mark;
    struct stat s;
    char *f = NULL, *b = NULL;

    if (block) {
	if (eval_marks (edit, &start_mark, &end_mark)) {
	    edit_error_dialog (_(" Process block "), 
/* Not essential to translate */
		_(" You must first highlight a block of text. "));
	    return;
	}
	edit_save_block (edit, b = catstrs (home_dir, BLOCK_FILE, 0), start_mark, end_mark);
	my_system (0, shell, catstrs (home_dir, shell_cmd, 0));
	edit_refresh_cmd (edit);
    } else {
	my_system (0, shell, shell_cmd);
	edit_refresh_cmd (edit);
    }

    edit->force |= REDRAW_COMPLETELY;

    f = catstrs (home_dir, ERROR_FILE, 0);

    if (block) {
	if (stat (f, &s) == 0) {
	    if (!s.st_size) {	/* no error messages */
		if (edit_block_delete_cmd (edit))
		    return;
		edit_insert_file (edit, b);
		return;
	    } else {
		edit_insert_file (edit, f);
		return;
	    }
	} else {
/* Not essential to translate */
	    edit_error_dialog (_(" Process block "), 
/* Not essential to translate */
	    get_sys_error (_(" Error trying to stat file ")));
	    return;
	}
    }
}

#endif

int edit_execute_cmd (WEdit * edit, int command, int char_for_insertion);

/* prints at the cursor */
/* returns the number of chars printed */
int edit_print_string (WEdit * e, const char *s)
{
    int i = 0;
    while (s[i])
	edit_execute_cmd (e, -1, s[i++]);
    e->force |= REDRAW_COMPLETELY;
    edit_update_screen (e);
    return i;
}

int edit_printf (WEdit * e, const char *fmt,...)
{
    int i;
    va_list pa;
    char s[1024];
    va_start (pa, fmt);
    sprintf (s, fmt, pa);
    i = edit_print_string (e, s);
    va_end (pa);
    return i;
}

#ifdef MIDNIGHT

/* FIXME: does this function break NT_OS2 ? */

static void pipe_mail (WEdit *edit, char *to, char *subject, char *cc)
{
    FILE *p;
    char *s;
    s = malloc (4096);
    sprintf (s, "mail -s \"%s\" -c \"%s\" \"%s\"", subject, cc, to);
    p = popen (s, "w");
    if (!p) {
	free (s);
	return;
    } else {
	long i;
	for (i = 0; i < edit->last_byte; i++)
	    fputc (edit_get_byte (edit, i), p);
	pclose (p);
    }
    free (s);
}

#define MAIL_DLG_HEIGHT 12

void edit_mail_dialog (WEdit * edit)
{
    char *tmail_to;
    char *tmail_subject;
    char *tmail_cc;

    static char *mail_cc_last = 0;
    static char *mail_subject_last = 0;
    static char *mail_to_last = 0;

    QuickDialog Quick_input =
    {50, MAIL_DLG_HEIGHT, -1, 0, N_(" Mail "),
/* NLS ? */
     "[Input Line Keys]", "quick_input", 0};

    QuickWidget quick_widgets[] =
    {
/* NLS ? */
	{quick_button, 6, 10, 9, MAIL_DLG_HEIGHT, "&Cancel", 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_button, 2, 10, 9, MAIL_DLG_HEIGHT, "&Ok", 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
	{quick_input, 3, 50, 8, MAIL_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "mail-dlg-input"},
	{quick_label, 2, 50, 7, MAIL_DLG_HEIGHT, " Copies to", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, 50, 6, MAIL_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "mail-dlg-input-2"},
	{quick_label, 2, 50, 5, MAIL_DLG_HEIGHT, " Subject", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_input, 3, 50, 4, MAIL_DLG_HEIGHT, "", 44, 0, 0,
	 0, XV_WLAY_BELOWCLOSE, "mail-dlg-input-3"},
	{quick_label, 2, 50, 3, MAIL_DLG_HEIGHT, " To", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{quick_label, 2, 50, 2, MAIL_DLG_HEIGHT, " mail -s <subject> -c <cc> <to>", 0, 0, 0,
	 0, XV_WLAY_DONTCARE, 0},
	{0}};

    quick_widgets[2].str_result = &tmail_cc;
    quick_widgets[2].text = mail_cc_last ? mail_cc_last : "";
    quick_widgets[4].str_result = &tmail_subject;
    quick_widgets[4].text = mail_subject_last ? mail_subject_last : "";
    quick_widgets[6].str_result = &tmail_to;
    quick_widgets[6].text = mail_to_last ? mail_to_last : "";

    Quick_input.widgets = quick_widgets;

    if (quick_dialog (&Quick_input) != B_CANCEL) {
	if (mail_cc_last)
	    free (mail_cc_last);
	if (mail_subject_last)
	    free (mail_subject_last);
	if (mail_to_last)
	    free (mail_to_last);
	mail_cc_last = *(quick_widgets[2].str_result);
	mail_subject_last = *(quick_widgets[4].str_result);
	mail_to_last = *(quick_widgets[6].str_result);
	pipe_mail (edit, mail_to_last, mail_subject_last, mail_cc_last);
    }
}

#endif

