#ifndef __MYSLANG_H
#define __MYSLANG_H

#ifdef SLANG_H_INSIDE_SLANG_DIR
#    include <slang/slang.h>
#else
#    include "slang.h"
#endif

enum {
    KEY_BACKSPACE = 400,
    KEY_END, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME, KEY_A1, KEY_C1, KEY_NPAGE, KEY_PPAGE, KEY_IC,
    KEY_ENTER, KEY_DC, KEY_SCANCEL, KEY_BTAB
};

#define KEY_F(x) 1000+x

#define ACS_VLINE SLSMG_VLINE_CHAR
#define ACS_HLINE SLSMG_HLINE_CHAR
#define ACS_ULCORNER SLSMG_ULCORN_CHAR
#define ACS_LLCORNER SLSMG_LLCORN_CHAR
#define ACS_URCORNER SLSMG_URCORN_CHAR
#define ACS_LRCORNER SLSMG_LRCORN_CHAR

#ifdef OS2_NT
#    define ACS_LTEE 0xC3
#    define acs()   ;
#    define noacs() ;
#    define baudrate() 19200
#else
#    define ACS_LTEE 't'
#    define acs()   SLsmg_set_char_set(1)
#    define noacs() SLsmg_set_char_set (0)
#    define baudrate() SLang_TT_Baud_Rate
#endif

enum {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};

/* When using Slang with color, we have all the indexes free but
 * those defined here (A_BOLD, A_UNDERLINE, A_REVERSE, A_BOLD_REVERSE)
 */
#define A_BOLD      0x40
#define A_UNDERLINE 0x40
#define A_REVERSE   0x20
#define A_BOLD_REVERSE 0x21

#ifndef A_NORMAL
#    define A_NORMAL    0x00
#endif

#define COLOR_PAIR(x) x
#define ERR -1
#define TRUE 1
#define FALSE 0

void slang_set_raw_mode (void);

#define doupdate()
#define raw() slang_set_raw_mode()
#define noraw()
#define nodelay(x,val) set_slang_delay(val)
#define noecho()
#define beep() SLtt_beep ()
#define keypad(scr,value) slang_keypad (value)

#define ungetch(x) SLang_ungetkey(x)
#define start_color()
#define touchwin(x) SLsmg_touch_lines(0, LINES)
#define reset_shell_mode slang_shell_mode
#define reset_prog_mode slang_prog_mode
#define flushinp()

void slint_goto (int y, int x);
void attrset (int color);
void set_slang_delay (int);
void slang_init (void);
void slang_done_screen (void);
void slang_prog_mode (void);
void hline (int ch, int len);
void vline (int ch, int len);
int getch (void);
void slang_keypad (int set);
void slang_shell_mode (void);
void slang_shutdown (void);
int has_colors (void);
/* Internal function prototypes */
void load_terminfo_keys ();

/* FIXME Clean up this; gnome has nothing to do here */
#ifndef HAVE_GNOME
void init_pair (int, char *, char *);
#endif

/* copied from slcurses.h (MC version 4.0.7) */
#define move SLsmg_gotorc
#define clreol SLsmg_erase_eol
#define printw SLsmg_printf
#define mvprintw(x, y, z) SLsmg_gotorc(x, y); SLsmg_printf(z)
#define COLS SLtt_Screen_Cols
#define LINES SLtt_Screen_Rows
#define clrtobot SLsmg_erase_eos
#define clrtoeol SLsmg_erase_eol
#define standout SLsmg_reverse_video
#define standend  SLsmg_normal_video
#define addch SLsmg_write_char
#define addstr SLsmg_write_string
#define initscr() do { extern int force_ugly_line_drawing; \
	     extern int SLtt_Has_Alt_Charset; \
	     SLtt_get_terminfo (); \
	     if (force_ugly_line_drawing) \
		SLtt_Has_Alt_Charset = 0; \
             SLsmg_init_smg (); \
          } while(0)
#define refresh SLsmg_refresh
#define clear SLsmg_cls
#define erase SLsmg_cls
#define mvaddstr(y, x, s) SLsmg_gotorc(y, x); SLsmg_write_string(s)
#define touchline SLsmg_touch_lines
#define inch SLsmg_char_at
#define endwin SLsmg_reset_smg

#define SLsmg_draw_double_box(r,c,dr,dc) SLsmg_draw_box ((r), (c), (dr), (dc))

#ifdef OS2_NT
#   define one_vline() addch(ACS_VLINE)
#   define one_hline() addch(ACS_HLINE)
    /* This is fast, but unusefull if ! pc_system - doesn't use
       Alt_Char_Pairs [] :( */
#else
    /* This is slow, but works well :| */ 
#   define one_vline() SLsmg_draw_object (SLsmg_get_row(), SLsmg_get_column(), slow_terminal ? ' ' : ACS_VLINE) 
#   define one_hline() SLsmg_draw_object (SLsmg_get_row(), SLsmg_get_column(), slow_terminal ? ' ' : ACS_HLINE) 
#endif    

void enable_interrupt_key ();
void disable_interrupt_key ();
#endif
