/* Color setup
   Copyright (C) 1994 Miguel de Icaza.
   
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
#include "tty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mad.h"
#include "setup.h"		/* For the externs */
#include "color.h"
#include "x.h"

/* "$Id: color.c,v 1.1 2001/12/30 09:55:26 sedwards Exp $" */

/* To avoid excessive calls to ncurses' has_colors () */
int   hascolors = 0;

/* Set to force black and white display at program startup */
int   disable_colors = 0;

/* Set if we are actually using colors */
int use_colors = 0;

int dialog_colors [4];

#define ELEMENTS(arr) ( sizeof(arr) / sizeof((arr)[0]) )



#ifdef HAVE_GNOME
#	 define  CTYPE GdkColor *
void init_pair (int, CTYPE, CTYPE);
#else
#	  ifdef HAVE_SLANG
#	      define CTYPE char *
#	  else
#	      define CTYPE int
#	      define color_map_fg(n) (color_map[n].fg % COLORS)
#	      define color_map_bg(n) (color_map[n].bg % COLORS)
#	  endif
#endif

struct colorpair {
    char *name;			/* Name of the entry */
    CTYPE fg;			/* foreground color */
    CTYPE bg;			/* background color */
};

#ifndef color_map_fg
#   define color_map_fg(n) color_map[n].fg
#   define color_map_bg(n) color_map[n].bg
#endif

struct colorpair color_map [] = {
    { "normal=",     0, 0 },	/* normal */
    { "selected=",   0, 0 },	/* selected */
    { "marked=",     0, 0 },	/* marked */
    { "markselect=", 0, 0 },	/* marked/selected */
    { "errors=",     0, 0 },	/* errors */
    { "menu=",       0, 0 },	/* menu entry */
    { "reverse=",    0, 0 },	/* reverse */

    /* Dialog colors */
    { "dnormal=",    0, 0 },	/* Dialog normal */
    { "dfocus=",     0, 0 },	/* Dialog focused */
    { "dhotnormal=", 0, 0 },	/* Dialog normal/hot */
    { "dhotfocus=",  0, 0 },	/* Dialog focused/hot */
    
    { "viewunderline=", 0, 0 },	/* _\b? sequence in view */
    { "menusel=",    0, 0 },	/* Menu selected color */
    { "menuhot=",    0, 0 },    /* Color for menu hotkeys */
    { "menuhotsel=", 0, 0 },    /* Menu hotkeys/selected entry */
    
    { "helpnormal=", 0, 0 },    /* Help normal */
    { "helpitalic=", 0, 0 },    /* Italic in help */
    { "helpbold=",   0, 0 },    /* Bold in help */
    { "helplink=",   0, 0 },    /* Not selected hyperlink */
    { "helpslink=",  0, 0 },    /* Selected hyperlink */
    
    { "gauge=",      0, 0 },    /* Color of the progress bar (percentage) */
    { "input=",      0, 0 },
 
    /* Per file types colors */
    { "directory=",  0, 0 },
    { "execute=",    0, 0 },
    { "link=",       0, 0 },
    { "device=",     0, 0 },
    { "special=",    0, 0 },
    { "core=",       0, 0 },
};

struct color_table_s {
    char *name;
    int  value;
};


struct color_table_s color_table [] = {
    { "black",         COLOR_BLACK   },
    { "gray",          COLOR_BLACK   | A_BOLD },
    { "red",           COLOR_RED     },
    { "brightred",     COLOR_RED     | A_BOLD },
    { "green",         COLOR_GREEN   },
    { "brightgreen",   COLOR_GREEN   | A_BOLD },
    { "brown",         COLOR_YELLOW  },
    { "yellow",        COLOR_YELLOW  | A_BOLD },
    { "blue",          COLOR_BLUE    },
    { "brightblue",    COLOR_BLUE    | A_BOLD },
    { "magenta",       COLOR_MAGENTA },
    { "brightmagenta", COLOR_MAGENTA | A_BOLD },
    { "cyan",          COLOR_CYAN    },
    { "brightcyan",    COLOR_CYAN    | A_BOLD },
    { "lightgray",     COLOR_WHITE },
    { "white",         COLOR_WHITE   | A_BOLD }
};

#ifdef HAVE_GNOME
void get_color (char *cpp, CTYPE *colp);
#else
#   ifdef HAVE_SLANG
#    	define color_value(i) color_table [i].name
#    	define color_name(i)  color_table [i].name
#    else
#    	define color_value(i) color_table [i].value
#    	define color_name(i)  color_table [i].name
#    endif

static void get_color (char *cpp, CTYPE *colp)
{
    int i;
    
    for (i = 0; i < ELEMENTS(color_table); i++){
	if (strcmp (cpp, color_name (i)) == 0){
	    *colp = color_value (i);
            return;
	}
    }
}
#endif /* HAVE_GNOME */

static void get_two_colors (char **cpp, struct colorpair *colorpairp)
{
    char *p = *cpp;
    int state;

    state = 0;
    
    for (; *p; p++){
	if (*p == ':'){
	    *p = 0;
	    get_color (*cpp, state ? &colorpairp->bg : &colorpairp->fg);
	    *p = ':';
	    *cpp = p + 1;
	    return;
	}

	if (*p == ','){
	    state = 1;
	    *p = 0;
	    get_color (*cpp, &colorpairp->fg);
	    *p = ',';
	    *cpp = p + 1;
	}
    }
    get_color (*cpp, state ? &colorpairp->bg : &colorpairp->fg);
}

void configure_colors_string (char *the_color_string)
{
    char *color_string, *p;
    int  i, found;

    if (!the_color_string)
	return;

    p = color_string = strdup (the_color_string);
    while (color_string && *color_string){
	while (*color_string == ' ' || *color_string == '\t')
	    color_string++;

	found = 0;
	for (i = 0; i < ELEMENTS(color_map); i++){
	    int klen = strlen (color_map [i].name);

	    if (strncmp (color_string, color_map [i].name, klen) == 0){
		color_string += klen;
		get_two_colors (&color_string, &color_map [i]);
		found = 1;
	    }
	}
	if (!found){
		while (*color_string && *color_string != ':')
			color_string++;
		if (*color_string)
			color_string++;
	}
    }
    free (p);
}

static void configure_colors (void)
{
    extern char *command_line_colors;
#ifndef HAVE_TK
#ifndef HAVE_XVIEW
    extern char *default_edition_colors;
    
    configure_colors_string (default_edition_colors);
#endif
#endif
    configure_colors_string (setup_color_string);
    configure_colors_string (term_color_string);
    configure_colors_string (getenv ("MC_COLOR_TABLE"));
    configure_colors_string (command_line_colors);
}

#ifndef HAVE_SLANG
#define MAX_PAIRS 30
int attr_pairs [MAX_PAIRS];
#endif

static void
load_dialog_colors (void)
{
    dialog_colors [0] = COLOR_NORMAL;
    dialog_colors [1] = COLOR_FOCUS;
    dialog_colors [2] = COLOR_HOT_NORMAL;
    dialog_colors [3] = COLOR_HOT_FOCUS;
}

#ifdef HAVE_X
void
init_colors (void)
{
	int i;
	
	use_colors = 1;
	start_color ();
	configure_colors ();
	for (i = 0; i < ELEMENTS (color_map); i++)
	    init_pair (i+1, color_map_fg(i), color_map_bg(i));
	load_dialog_colors ();
}
#else
void init_colors (void)
{
    int i;
    
    hascolors = has_colors ();

    if (!disable_colors && hascolors){
	use_colors = 1;
    }

    if (use_colors){
	start_color ();
	configure_colors ();

#ifndef HAVE_SLANG
	if (ELEMENTS (color_map) > MAX_PAIRS){
	    /* This message should only be seen by the developers */
	    fprintf (stderr,
		     "Too many defined colors, resize MAX_PAIRS on color.c");
	    exit (1);
	}
#endif

#if defined HAVE_SLANG && !defined(OS2_NT)
	if (use_colors) { /* Hack to make COLOR_PAIR(31) be the default fg/bg
	                     of the terminal */
	    char *Norm_Vid = SLtt_tgetstr ("me");
	    
	    if (Norm_Vid == NULL)
	        Norm_Vid = SLtt_tgetstr ("se");
	    if (Norm_Vid == NULL)
	        Norm_Vid = "\033[0m";
	    SLtt_set_color_esc (31, Norm_Vid);
	}
#endif	

	for (i = 0; i < ELEMENTS (color_map); i++){
	    init_pair (i+1, color_map_fg(i), color_map_bg(i));

#ifndef HAVE_SLANG
	    /*
	     * As a convenience, for the SVr4 curses configuration, we have
	     * OR'd the A_BOLD attribute to the color number.  We'll need that
	     * later, to set the attribute with the colors.
	     */
	     attr_pairs [i+1] = color_map [i].fg & A_BOLD;
#endif
	}
    }
    load_dialog_colors ();
}
#endif

void toggle_color_mode (void)
{
    if (hascolors)
	use_colors = !use_colors;
}

