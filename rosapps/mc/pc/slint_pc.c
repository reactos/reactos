/* Slang interface to the Midnight Commander for Windows NT and OS/2
   This emulates some features of ncurses on top of slang
   S-lang is not fully consistent between its Unix and non-Unix versions.
   
      
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

#include <config.h>
#include <stdio.h>
#include "../src/tty.h"
#include "../src/mad.h"
#include "../src/color.h"
#include "../src/util.h"
#include "../src/mouse.h"	/* Gpm_Event is required in key.h */
#include "../src/key.h"		/* define_sequence */
#include "../src/main.h"	/* extern: force_colors */
#include "../src/win.h"		/* do_exit_ca_mode */

#ifdef HAVE_SLANG


static void slang_sigterm ()
{
    SLsmg_reset_smg ();
}

static int slinterrupt;

void enable_interrupt_key(void)
{
    SLang_set_abort_signal(NULL);
    slinterrupt = 1;
}
void disable_interrupt_key(void)
{
    slinterrupt = 0;
}
int got_interrupt ()
{
    int t;
	int SLKeyboard_Quit=0;	/* FIXME!! */
    t = slinterrupt ? SLKeyboard_Quit : 0;
  /* SLKeyboard_Quit = 0; */
    return t;
}

/* Only done the first time */
void slang_init (void)
{
    SLtt_get_terminfo ();
    SLang_init_tty (XCTRL('c'), 1, 0);	
    slang_prog_mode ();
    load_terminfo_keys ();
}

/* Done each time we come back from done mode */
void slang_prog_mode (void)
{
    SLsmg_init_smg ();
    SLsmg_touch_lines (0, LINES);
}

/* Called each time we want to shutdown slang screen manager */
void slang_shell_mode (void)
{

}

void slang_shutdown ()
{
    slang_shell_mode ();
    do_exit_ca_mode ();
    SLang_reset_tty ();

    /* reset the colors to those that were
     * active when the program was started up
      (not written)
     */
}

/* keypad routines */
void slang_keypad (int set)
{
  /* enable keypad strings */
}

static int no_slang_delay;

void set_slang_delay (int v)
{
    no_slang_delay = v;
}

void hline (int ch, int len)
{
    int last_x, last_y;

    last_x = SLsmg_get_column ();
    last_y = SLsmg_get_row ();
    
    if (ch == 0)
	ch = ACS_HLINE;

    if (ch == ACS_HLINE){
	SLsmg_draw_hline (len);
    } else {
	while (len--)
	    addch (ch);
    }
    move (last_y, last_x);
}

void vline (int character, int len)
{
    if (!slow_terminal){
	SLsmg_draw_vline (len);
    } else {
	int last_x, last_y, pos = 0;

	last_x = SLsmg_get_column ();
	last_y = SLsmg_get_row ();

	while (len--){
	    move (last_y + pos++, last_x);
	    addch (' ');
	}
	move (last_x, last_y);
    }
}

int has_colors ()
{
    /* No terminals on NT, make default color */
    if (!disable_colors)
    	SLtt_Use_Ansi_Colors = 1;
    
    /* Setup emulated colors */
    if (SLtt_Use_Ansi_Colors){
        /* DO NOT TRANSLATE WITH gettext SYNTAX coloring will be broken */
	init_pair (A_REVERSE, "black", "white");
    } else {
/*	SLtt_set_mono (A_BOLD,    NULL, SLTT_BOLD_MASK);
	SLtt_set_mono (A_REVERSE, NULL, SLTT_REV_MASK);
	SLtt_set_mono (A_BOLD|A_REVERSE, NULL, SLTT_BOLD_MASK | SLTT_REV_MASK);
  */  }
    return SLtt_Use_Ansi_Colors;
}

void attrset (int color)
{
    if (!SLtt_Use_Ansi_Colors){
	SLsmg_set_color (color);
	return;
    }
    
    if (color & A_BOLD){
	if (color == A_BOLD)
	    SLsmg_set_color (A_BOLD);
	else
	    SLsmg_set_color ((color & (~A_BOLD)) + 8);
	return;
    }

    if (color == A_REVERSE)
	SLsmg_set_color (A_REVERSE);
    else
	SLsmg_set_color (color);
}
	
void load_terminfo_keys ()
{
}

int getch ()
{
    if (no_slang_delay)
	if (SLang_input_pending (0) == 0)
	    return -1;

    return SLang_getkey ();
}

extern int slow_terminal;

#else

/* Non slang builds do not understand got_interrupt */
int got_interrupt ()
{
    return 0;
}
#endif /* HAVE_SLANG */

void mc_refresh (void)
{
/*  if (!we_are_background) (no background mode yet) */
    refresh ();
}

void slang_set_raw_mode (void)
{
   return;
}

int max_index = 0;

void
init_pair (int index, char *foreground, char *background)
{

    SLtt_set_color (index, "", foreground, background);
    if (index > max_index)
	max_index = index;
}

int
alloc_color_pair (char *foreground, char *background)
{
    init_pair (++max_index, foreground, background);
    return max_index;
}

int
try_alloc_color_pair (char *fg, char *bg)
{
    static struct colors_avail {
	struct colors_avail *next;
	char *fg, *bg;
	int index;
    } *p, c =
    {
	0, 0, 0, 0
    };

    c.index = NORMAL_COLOR;
    p = &c;
    for (;;) {
	if (((fg && p->fg) ? !strcmp (fg, p->fg) : fg == p->fg) != 0
	    && ((bg && p->bg) ? !strcmp (bg, p->bg) : bg == p->bg) != 0)
	    return p->index;
	if (!p->next)
	    break;
	p = p->next;
    }
    p->next = malloc (sizeof (c));
    p = p->next;
    p->next = 0;
    p->fg = fg ? strdup (fg) : 0;
    p->bg = bg ? strdup (bg) : 0;
    /* DO NOT TRANSLATE WITH gettext SYNTAX coloring will be broken */
    if (!fg)
	fg = "white";
    if (!bg)
	bg = "blue";
    p->index = alloc_color_pair (fg, bg);
    return p->index;
}
