/* Learn keys
   Copyright (C) 1995 The Free Software Foundation
   
   Written by: 1995 Jakub Jelinek

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

#include <config.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>		/* For malloc() */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "tty.h"
#include "mad.h"
#include "util.h"	/* Needed for the externs and convert_controls */
#include "win.h"
#include "color.h"
#include "dlg.h"
#include "widget.h"
#include "dialog.h"		/* For do_refresh() */
#include "profile.h"		/* Save profile */
#include "key.h"
#include "setup.h"
#include "main.h"
#define UX		4
#define UY		3

#define BY		UY + 17

#define ROWS		13
#define COLSHIFT	23

#define BUTTONS 	2

struct {
    int ret_cmd, flags, y, x;
	unsigned int hotkey;
    char *text;
} learn_but[BUTTONS] = {
    { B_CANCEL, NORMAL_BUTTON, 0, 39, 'C', N_("&Cancel") },
    { B_ENTER, DEFPUSH_BUTTON, 0, 25, 'S', N_("&Save") }
};

static Dlg_head *learn_dlg;
typedef struct {
    Widget *button;
    Widget *label;
    int ok;
    char *sequence;
} learnkey;
static learnkey *learnkeys = NULL;
static int learn_total;
static int learnok;
static int learnchanged;
static char* learn_title = N_(" Learn keys ");

#ifndef HAVE_X
static void learn_refresh (void)
{
    attrset (COLOR_NORMAL);
    dlg_erase (learn_dlg);
    
    draw_box (learn_dlg, 1, 2, learn_dlg->lines - 2, learn_dlg->cols - 4);
    
    attrset (COLOR_HOT_NORMAL);
    dlg_move (learn_dlg, 1, (learn_dlg->cols - strlen (learn_title)) / 2);
    addstr (learn_title);
}
#endif

static int learn_button (int action, void *param)
{
    unsigned char *seq;
    Dlg_head *d = message (D_INSERT | 1, _(" Teach me a key "),
_("Please press the %s\n"
"and then wait until this message disappears.\n\n"
"Then, press it again to see if OK appears\n"
"next to its button.\n\n"
"If you want to escape, press a single Escape key\n"
"and wait as well."), 
        _(key_name_conv_tab [action - B_USER].longname));
    mc_refresh ();
    if (learnkeys [action - B_USER].sequence != NULL) {
	free (learnkeys [action - B_USER].sequence);
	learnkeys [action - B_USER].sequence = NULL;
    }
    seq = learn_key ();

    if (seq){
	/* Esc hides the dialog and do not allow definitions of
	 * regular characters
	 */
	if (*seq && strcmp (seq, "\\e") && strcmp (seq, "\\e\\e")
	    && strcmp (seq, "^m" ) 
            && (seq [1] || (*seq < ' ' || *seq > '~'))){
	    
	    learnchanged = 1;
	    learnkeys [action - B_USER].sequence = seq;
	    seq = convert_controls (seq);
	    define_sequence (key_name_conv_tab [action - B_USER].code, seq, 
			     MCKEY_NOACTION);
	} else {
	    message (0, _(" Cannot accept this key "),
		_(" You have entered \"%s\""), seq);
	}
	
    	free (seq);
    }
    
    dlg_run_done (d);
    destroy_dlg (d);
    dlg_select_widget (learn_dlg, learnkeys [action - B_USER].button);
    return 0; /* Do not kill learn_dlg */
}

static int learn_move (int right)
{
    int i, totalcols;
    
    totalcols = (learn_total - 1) / ROWS + 1;
    for (i = 0; i < learn_total; i++)
        if (learnkeys [i].button == learn_dlg->current->widget) {
            if (right) {
                if (i < learn_total - ROWS)
                    i += ROWS;
                else 
                    i %= ROWS;
            } else {
                if (i / ROWS)
                    i -= ROWS;
                else if (i + (totalcols - 1) * ROWS >= learn_total)
                    i += (totalcols - 2) * ROWS;
                else
                    i += (totalcols - 1) * ROWS;
            }
            dlg_select_widget (learn_dlg, (void *) learnkeys [i].button);
            return 1;
        }
    return 0;
}

static int learn_check_key (int c)
{
    int i;

    for (i = 0; i < learn_total; i++) {
        if (key_name_conv_tab [i].code == c) {
	    if (!learnkeys [i].ok) {
	    	dlg_select_widget (learn_dlg, learnkeys [i].button);
	        label_set_text ((WLabel *) learnkeys [i].label,
	            _("OK"));
	        learnkeys [i].ok = 1;
	        learnok++;
	        if (learnok >= learn_total) {
	            learn_dlg->ret_value = B_CANCEL;
	            if (learnchanged) {
	                if (query_dialog (learn_title, 
			    _("It seems that all your keys already\n"
			      "work fine. That's great."),
	                    1, 2, _("&Save"), _("&Discard")) == 0)
	                    learn_dlg->ret_value = B_ENTER;
	            } else {
	            	message (1, learn_title,
			_("Great! You have a complete terminal database!\n"
			  "All your keys work well."));
	            }
		    dlg_stop (learn_dlg);
	        }
	        return 1;
	    }
        }
    }
    switch (c) {
        case KEY_LEFT:
        case 'h':
            return learn_move (0);
        case KEY_RIGHT:
        case 'l':
            return learn_move (1);
        case 'j':
            dlg_one_down (learn_dlg);
            return 1;
        case 'k':
            dlg_one_up (learn_dlg);
            return 1;
    }    

    /* Prevent from disappearing if a non-defined sequence is pressed
       and contains s or c. Use ALT('s') or ALT('c'). */
	if (c < 255 && isalpha(c))
	{
		c = toupper(c);
		for (i = 0; i < BUTTONS; i++)
			if (c == learn_but [i].hotkey)
				return 1;
	}

    return 0;
}

static int learn_callback (Dlg_head * h, int Par, int Msg)
{
    switch (Msg) {
    case DLG_DRAW:
	learn_refresh ();
	break;
    case DLG_KEY:
    	return learn_check_key (Par);
    }
    return 0;
}

static void init_learn (void)
{
    int x, y, i, j;
    key_code_name_t *key;
    char buffer [22];
	static int i18n_flag = 0;

    do_refresh ();

#ifdef ENABLE_NLS
	if (!i18n_flag)
	{
		char* cp;
		
		learn_but [0].text = _(learn_but [0].text);
		learn_but [0].x = 78 / 2 + 4;

		learn_but [1].text = _(learn_but [1].text);
		learn_but [1].x = 78 / 2 - (strlen (learn_but [1].text) + 9);

		for (i = 0; i < BUTTONS; i++)
		{
			cp = strchr(learn_but [i].text, '&');
			if (cp != NULL && *++cp != '\0')
				learn_but [i].hotkey = toupper(*cp);
		}

		learn_title = _(learn_title);
		i18n_flag = 1;
	}
#endif /* ENABLE_NLS */

    learn_dlg = create_dlg (0, 0, 23, 78, dialog_colors,
			      learn_callback, "[Learn keys]", "Learn keys",
			      DLG_CENTER);
    x_set_dialog_title (learn_dlg, _("Learn keys"));

#define XTRACT(i) BY+learn_but[i].y, learn_but[i].x, learn_but[i].ret_cmd, learn_but[i].flags, _(learn_but[i].text), 0, 0, NULL

    for (i = 0; i < BUTTONS; i++)
	add_widget (learn_dlg, button_new (XTRACT (i)));
    
    x = UX;
    y = UY;
    for (key = key_name_conv_tab, j = 0; key->name != NULL &&
        strcmp (key->name, "kpleft"); key++, j++);
    learnkeys = (learnkey *) xmalloc (sizeof (learnkey) * j, "Learn keys");
    x += ((j - 1) / ROWS) * COLSHIFT;
    y += (j - 1) % ROWS;
    learn_total = j;
    learnok = 0;
    learnchanged = 0;
    for (i = j - 1, key = key_name_conv_tab + j - 1; i >= 0; i--, key--) {
    	learnkeys [i].ok = 0;
    	learnkeys [i].sequence = NULL;
        sprintf (buffer, "%-16s", _(key->longname));
	add_widget (learn_dlg, learnkeys [i].button = (Widget *)
	    button_new (y, x, B_USER + i, NARROW_BUTTON, buffer, learn_button, 0, NULL));
	add_widget (learn_dlg, learnkeys [i].label = (Widget *)
	    label_new (y, x + 19, "", NULL));
	if (i % 13)
	    y--;
	else {
	    x -= COLSHIFT;
	    y = UY + ROWS - 1;
	}
    }
    add_widget (learn_dlg,
		label_new (UY+14, 5, _("Press all the keys mentioned here. After you have done it, check"), NULL));
    add_widget (learn_dlg,
		label_new (UY+15, 5, _("which keys are not marked with OK.  Press space on the missing"), NULL));
    add_widget (learn_dlg,
		label_new (UY+16, 5, _("key, or click with the mouse to define it. Move around with Tab."), NULL));
}

static void learn_done (void)
{
    destroy_dlg (learn_dlg);
    repaint_screen ();
}

void learn_save (void)
{
    int i;
    int profile_changed = 0;
    char *section = copy_strings ("terminal:", getenv ("TERM"), NULL);

    for (i = 0; i < learn_total; i++) {
	if (learnkeys [i].sequence != NULL) {
	    profile_changed = 1;
	    WritePrivateProfileString (section, key_name_conv_tab [i].name,
	        learnkeys [i].sequence, profile_name);
        }
    }

    /* On the one hand no good idea to save the complete setup but 
     * without 'Auto save setup' the new key-definitions will not be 
     * saved unless the user does an 'Options/Save Setup'. 
     * On the other hand a save-button that does not save anything to 
     * disk is much worse.
     */
    if (profile_changed)
        sync_profiles ();
}

void learn_keys (void)
{
    int save_old_esc_mode = old_esc_mode;
    int save_alternate_plus_minus = alternate_plus_minus;
    
    old_esc_mode = 0; /* old_esc_mode cannot work in learn keys dialog */
    alternate_plus_minus = 1; /* don't translate KP_ADD, KP_SUBTRACT and
                                 KP_MULTIPLY to '+', '-' and '*' in
                                 correct_key_code */
#ifndef HAVE_X
    application_keypad_mode ();
#endif
    init_learn ();

    run_dlg (learn_dlg);
    
    old_esc_mode = save_old_esc_mode;
    alternate_plus_minus = save_alternate_plus_minus;

#ifndef HAVE_X
    if (!alternate_plus_minus)
        numeric_keypad_mode ();

#endif
    
    switch (learn_dlg->ret_value) {
    case B_ENTER:
        learn_save ();
        break;
    }

    learn_done ();
}

