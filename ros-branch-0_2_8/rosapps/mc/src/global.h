#ifndef __GLOBAL_H
#define __GLOBAL_H

extern char *home_dir;

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define min(x,y) (x>y ? y: x)
#define max(x,y) (x>y ? x: y)

void refresh_screen (void *);

#ifndef HAVE_STRDUP
char *strdup (const char *);
#endif

/* AIX compiler doesn't understand '\e' */
#define ESC_CHAR '\033'
#define ESC_STR  "\033"

#ifdef USE_BSD_CURSES
#   define xgetch x_getch
#else
#   define xgetch getch
#endif

#endif
