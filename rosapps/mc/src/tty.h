/* This file takes care of loading ncurses or slang */

int got_interrupt (void);
void mc_refresh (void);

#ifdef HAVE_SLANG
#   include "myslang.h"

#   define TTY_H_DONE
#else
#   define enable_interrupt_key()
#   define disable_interrupt_key()
#   define slang_shutdown()
#   define slang_done_screen()
#   define slang_init()
#   define slang_init_screen()
#   define slang_init_tty()
#   define slang_done_tty();
#   define acs()
#   define noacs()
#   define one_vline() addch (slow_terminal ? ' ' : ACS_VLINE)
#   define one_hline() addch (slow_terminal ? ' ' : ACS_HLINE)
#endif

#if !defined(TTY_H_DONE) && defined(USE_NCURSES)
    /* This is required since ncurses 1.8.6 and newer changed the name of */
    /* the include files (July 1994) */
#    ifdef RENAMED_NCURSES
#        include <curses.h>
#    else
#        include <ncurses.h>
#    endif
#    ifdef INCLUDE_TERM
#        include <term.h>
#        define TERM_INCLUDED 1
#    endif
#    define TTY_H_DONE
#endif

#if !defined(TTY_H_DONE) && defined(USE_BSD_CURSES)

    /* This is only to let people that don't want to install ncurses */
    /* run this nice program; they get what they deserve.            */

    /* Ultrix has a better curses: cursesX */
#   ifdef ultrix
#       include <cursesX.h>
#   else
#       include <curses.h>
#   endif

#   ifndef ACS_VLINE
#       define ACS_VLINE '|'
#   endif

#   ifndef ACS_HLINE
#       define ACS_HLINE '-'
#   endif

#   ifndef ACS_ULCORNER
#       define ACS_ULCORNER '+'
#   endif

#   ifndef ACS_LLCORNER
#       define ACS_LLCORNER '+'
#   endif

#   ifndef ACS_URCORNER
#       define ACS_URCORNER '+'
#   endif

#   ifndef ACS_LRCORNER
#       define ACS_LRCORNER '+'
#   endif

#   ifndef ACS_LTEE
#       define ACS_LTEE '+'
#   endif

#   ifndef KEY_BACKSPACE
#       define KEY_BACKSPACE 0
#   endif

#   ifndef KEY_END
#       define KEY_END 0
#   endif

#   define ACS_MAP(x) '*'

#   define NO_COLOR_SUPPORT
#   define untouchwin(win) 
#   define xgetch x_getch
#   define wtouchln(win,b,c,d) touchwin(win)
#   define derwin(win,x,y,z,w) win
#   define wscrl(win,n)
#   define TTY_H_DONE
#endif

#if !defined(TTY_H_DONE) && defined(USE_SYSV_CURSES)
#   include <curses.h>
#   ifdef INCLUDE_TERM
#       include <term.h>
        /* Ugly hack to avoid name space pollution */
#       undef cols
#       undef lines
#       undef buttons

#       define TERM_INCLUDED 1
#   endif

#   if defined(sparc) || defined(__sgi) || defined(_SGI_SOURCE)
        /* We are dealing with Solaris or SGI buggy curses :-) */
#       define BUGGY_CURSES 1
#   endif
#   if defined(mips) && defined(sgi)
        /* GNU C compiler, buggy sgi */
#       define BUGGY_CURSES 1
#   endif

#   ifdef __osf__
#       define untouchwin(win)
#   endif

#endif /* USE_SYSV_CURSES */

#ifdef NO_COLOR_SUPPORT
#   define COLOR_PAIR(x) 1

enum {
  COLOR_BLACK, COLOR_RED,     COLOR_GREEN, COLOR_YELLOW,
  COLOR_BLUE,  COLOR_MAGENTA, COLOR_CYAN,  COLOR_WHITE
};

int init_pair (int, int, int);

#endif

#define KEY_KP_ADD	4001
#define KEY_KP_SUBTRACT	4002
#define KEY_KP_MULTIPLY	4003

