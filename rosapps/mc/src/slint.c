/* Slang interface to the Midnight Commander
   This emulates some features of ncurses on top of slang
   
   Copyright (C) 1995, 1996 The Free Software Foundation.

   Author Miguel de Icaza
          Norbert Warmuth
   
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
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include "tty.h"
#include "mad.h"
#include "color.h"
#include "util.h"
#include "mouse.h"		/* Gpm_Event is required in key.h */
#include "key.h"		/* define_sequence */
#include "main.h"		/* extern: force_colors */
#include "win.h"		/* do_exit_ca_mode */
#include "background.h"		/* we_are_background definition */
#include "setup.h"

#ifdef HAVE_SLANG

/* Taken from S-Lang's slutty.c */
#ifdef ultrix   /* Ultrix gets _POSIX_VDISABLE wrong! */
# define NULL_VALUE -1
#else
# ifdef _POSIX_VDISABLE
#  define NULL_VALUE _POSIX_VDISABLE
# else
#  define NULL_VALUE 255
# endif
#endif

/* Taken from S-Lang's sldisply.c file */
#ifndef USE_TERMCAP
#   define tgetstr(a,b) SLtt_tgetstr (a)
#else
    extern char *tgetstr(char *, char **);
#endif

#ifndef SA_RESTART
#    define SA_RESTART 0
#endif

/* Various saved termios settings that we control here */
static struct termios boot_mode;
static struct termios new_mode;

/* Set if we get an interrupt */
static int slinterrupt;

/* Controls whether we should wait for input in getch */
static int no_slang_delay;

/* {{{  Copied from ../slang/slgetkey.c, removed the DEC_8Bit_HACK, */
extern unsigned int SLang_Input_Buffer_Len;
extern unsigned char SLang_Input_Buffer [];
#if SLANG_VERSION >= 10000
extern unsigned int _SLsys_getkey (void);
extern int _SLsys_input_pending (int);
#else
extern unsigned int SLsys_getkey (void);
extern int SLsys_input_pending (int);
#endif

static unsigned int SLang_getkey2 (void)
{
   unsigned int imax;
   unsigned int ch;
   
   if (SLang_Input_Buffer_Len)
     {
	ch = (unsigned int) *SLang_Input_Buffer;
	SLang_Input_Buffer_Len--;
	imax = SLang_Input_Buffer_Len;
   
	memcpy ((char *) SLang_Input_Buffer, 
		(char *) (SLang_Input_Buffer + 1), imax);
	return(ch);
     }
#if SLANG_VERSION >= 10000
   else return(_SLsys_getkey ());
#else
   else return(SLsys_getkey());
#endif
}

static int SLang_input_pending2 (int tsecs)
{
   int n;
   unsigned char c;
   if (SLang_Input_Buffer_Len) return (int) SLang_Input_Buffer_Len;
#if SLANG_VERSION >= 10000  
   n = _SLsys_input_pending (tsecs);
#else
   n = SLsys_input_pending (tsecs);
#endif
   if (n <= 0) return 0;
   
   c = (unsigned char) SLang_getkey2 ();
   SLang_ungetkey_string (&c, 1);
   
   return n;
}
/* }}} */

static void
slang_intr (int signo)
{
    slinterrupt = 1;
}

int
interrupts_enabled (void)
{
    struct sigaction current_act;

    sigaction (SIGINT, NULL, &current_act);
    return current_act.sa_handler == slang_intr;
}

void
enable_interrupt_key(void)
{
    struct sigaction act;
    
    act.sa_handler = slang_intr;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGINT, &act, NULL);
    slinterrupt = 0;
}

void
disable_interrupt_key(void)
{
    struct sigaction act;
    
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    sigemptyset (&act.sa_mask);
    sigaction (SIGINT, &act, NULL);
}

int
got_interrupt ()
{
    int t;

    t = slinterrupt;
    slinterrupt = 0;
    return t;
}

/* Only done the first time */
void
slang_init (void)
{
    extern int SLtt_Blink_Mode;
    extern int SLtt_Has_Alt_Charset;
    extern int force_ugly_line_drawing;
    struct sigaction act, oact;
    
    SLtt_get_terminfo ();
    tcgetattr (fileno (stdin), &boot_mode);
    /* 255 = ignore abort char; XCTRL('g') for abort char = ^g */
    SLang_init_tty (XCTRL('c'), 1, 0);	
    
    /* If SLang uses fileno(stderr) for terminal input MC will hang
       if we call SLang_getkey between calls to open_error_pipe and
       close_error_pipe, e.g. when we do a growing view of an gzipped
       file. */
    if (SLang_TT_Read_FD == fileno (stderr))
	SLang_TT_Read_FD = fileno (stdin);

    if (force_ugly_line_drawing)
	SLtt_Has_Alt_Charset = 0;
    if (tcgetattr (SLang_TT_Read_FD, &new_mode) == 0) {
#ifdef VDSUSP
	new_mode.c_cc[VDSUSP] = NULL_VALUE;   /* to ignore ^Y */
#endif
#ifdef VLNEXT
	new_mode.c_cc[VLNEXT] = NULL_VALUE;   /* to ignore ^V */
#endif
	tcsetattr (SLang_TT_Read_FD, TCSADRAIN, &new_mode);
    }
    slang_prog_mode ();
    load_terminfo_keys ();
    act.sa_handler = slang_intr;
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_RESTART;
    sigaction (SIGINT, &act, &oact);
    SLtt_Blink_Mode = 0;
}

void
slang_set_raw_mode (void)
{
    tcsetattr (SLang_TT_Read_FD, TCSANOW, &new_mode);
}

/* Done each time we come back from done mode */
void
slang_prog_mode (void)
{
    tcsetattr (SLang_TT_Read_FD, TCSANOW, &new_mode);
    SLsmg_init_smg ();
    SLsmg_touch_lines (0, LINES);
}

/* Called each time we want to shutdown slang screen manager */
void
slang_shell_mode (void)
{
    tcsetattr (SLang_TT_Read_FD, TCSANOW, &boot_mode);
}

void
slang_shutdown ()
{
    char *op_cap;
    
    slang_shell_mode ();
    do_exit_ca_mode ();
    SLang_reset_tty ();

    /* Load the op capability to reset the colors to those that were
     * active when the program was started up
     */
    op_cap = SLtt_tgetstr ("op");
    if (op_cap){
	fprintf (stdout, "%s", op_cap);
	fflush (stdout);
    }
}

/* HP Terminals have capabilities (pfkey, pfloc, pfx) to program function keys.
   elm 2.4pl15 invoked with the -K option utilizes these softkeys and the
   consequence is that function keys don't work in MC sometimes. 
   Unfortunately I don't now the one and only escape sequence to turn off 
   softkeys (elm uses three different capabilities to turn on softkeys and two 
   capabilities to turn them off). 
   Among other things elm uses the pair we already use in slang_keypad. That's 
   the reason why I call slang_reset_softkeys from slang_keypad. In lack of
   something better the softkeys are programmed to their defaults from the
   termcap/terminfo database.
   The escape sequence to program the softkeys is taken from elm and it is 
   hardcoded because neither slang nor ncurses 4.1 know how to 'printf' this 
   sequence. -- Norbert
 */   
   
void
slang_reset_softkeys (void)
{
    int key;
    char *send;
    char *display = "                ";
    char tmp[100];
    
    for ( key = 1; key < 9; key++ ) {
	sprintf ( tmp, "k%d", key);
	send = (char *) SLtt_tgetstr (tmp);
	if (send) {
            sprintf(tmp, "\033&f%dk%dd%dL%s%s", key,
                    strlen(display), strlen(send), display, send);
    	    SLtt_write_string (tmp);
	}
    }
}

/* keypad routines */
void
slang_keypad (int set)
{
    char *keypad_string;
    extern int reset_hp_softkeys;
    
    keypad_string = (char *) SLtt_tgetstr (set ? "ks" : "ke");
    if (keypad_string)
	SLtt_write_string (keypad_string);
    if (set && reset_hp_softkeys)
	slang_reset_softkeys ();
}

void
set_slang_delay (int v)
{
    no_slang_delay = v;
}

void
hline (int ch, int len)
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

void
vline (int character, int len)
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
    if (!fg)
	fg = "white";
    if (!bg)
	bg = "blue";
    p->index = alloc_color_pair (fg, bg);
    return p->index;
}

char *color_terminals [] = {
#ifdef __linux__
    "console",
#endif
    "linux",
    "xterm-color",
    "color-xterm",
    "dtterm",
    "xtermc",
    "ansi",
/* 'qnx*' terminals have non-ANSI-compatible color sequences... */
    "qansi",
    "qansi-g",
    "qansi-m",
    "qansi-t",
    "qansi-w",
    0
};

int has_colors ()
{
    char *terminal = getenv ("TERM");
    char *cts = color_terminal_string, *s;
    int  i;

    SLtt_Use_Ansi_Colors = force_colors;
    if (NULL != getenv ("COLORTERM"))
	SLtt_Use_Ansi_Colors = 1;

    /* We want to allow overwriding */
    if (!disable_colors){
      if (!*cts)
      {
      /* check hard-coded terminals */
	for (i = 0; color_terminals [i]; i++)
	    if (strcmp (color_terminals [i], terminal) == 0)
		SLtt_Use_Ansi_Colors = 1;
      }
      else
      /* check color_terminal_string */
      {
        while (*cts)
        {
          while (*cts == ' ' || *cts == '\t') cts++;

          s = cts;
          i = 0;

          while (*cts && *cts != ',')
          {
            cts++;
            i++;
          }
          if (i && i == strlen(terminal) && strncmp(s, terminal, i) == 0)
            SLtt_Use_Ansi_Colors = 1;

          if (*cts == ',') cts++;
        }
      }
    }
    
    /* Setup emulated colors */
    if (SLtt_Use_Ansi_Colors){
        if (use_colors){
            init_pair (A_REVERSE, "black", "white");
	    init_pair (A_BOLD, "white", "black");
	} else {
	    init_pair (A_REVERSE, "black", "lightgray");
	    init_pair (A_BOLD, "white", "black");
	    init_pair (A_BOLD_REVERSE, "white", "lightgray");
	} 
    } else {
	SLtt_set_mono (A_BOLD,    NULL, SLTT_BOLD_MASK);
	SLtt_set_mono (A_REVERSE, NULL, SLTT_REV_MASK);
	SLtt_set_mono (A_BOLD|A_REVERSE, NULL, SLTT_BOLD_MASK | SLTT_REV_MASK);
    }
    return SLtt_Use_Ansi_Colors;
}

void
attrset (int color)
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

/* This table describes which capabilities we want and which values we
 * assign to them.
 */
struct {
    int  key_code;
    char *key_name;
} key_table [] = {
    { KEY_F(0),  "k0" },
    { KEY_F(1),  "k1" },
    { KEY_F(2),  "k2" },
    { KEY_F(3),  "k3" },
    { KEY_F(4),  "k4" },
    { KEY_F(5),  "k5" },
    { KEY_F(6),  "k6" },
    { KEY_F(7),  "k7" },
    { KEY_F(8),  "k8" },
    { KEY_F(9),  "k9" },
    { KEY_F(10), "k;" },
    { KEY_F(11), "F1" },
    { KEY_F(12), "F2" },
    { KEY_F(13), "F3" },
    { KEY_F(14), "F4" },
    { KEY_F(15), "F5" },
    { KEY_F(16), "F6" },
    { KEY_F(17), "F7" },
    { KEY_F(18), "F8" },
    { KEY_F(19), "F9" },
    { KEY_F(20), "FA" },	
    { KEY_IC,    "kI" },
    { KEY_NPAGE, "kN" },
    { KEY_PPAGE, "kP" },
    { KEY_LEFT,  "kl" },
    { KEY_RIGHT, "kr" },
    { KEY_UP,    "ku" },
    { KEY_DOWN,  "kd" },
    { KEY_DC,    "kD" },
    { KEY_BACKSPACE, "kb" },
    { KEY_HOME,  "kh" },
    { KEY_END,	 "@7" },
    { 0, 0}
};
	
void
do_define_key (int code, char *strcap)
{
    char    *seq;

    seq = (char *) SLtt_tgetstr (strcap);
    if (seq)
	define_sequence (code, seq, MCKEY_NOACTION);
}

void
load_terminfo_keys ()
{
    int i;

    for (i = 0; key_table [i].key_code; i++)
	do_define_key (key_table [i].key_code, key_table [i].key_name);
}

int getch ()
{
    if (no_slang_delay)
	if (SLang_input_pending2 (0) == 0)
	    return -1;

    return (SLang_getkey2 ());
}

extern int slow_terminal;

#else

/* Non slang builds do not understand got_interrupt */
int got_interrupt ()
{
    return 0;
}
#endif /* HAVE_SLANG */

void
mc_refresh (void)
{
    if (!we_are_background)
	refresh ();
}
