/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_conio_h_
#define __dj_include_conio_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE


extern int directvideo; /* ignored by gppconio */
extern int _wscroll;

#define _NOCURSOR      0
#define _SOLIDCURSOR   1
#define _NORMALCURSOR  2

struct text_info {
    unsigned char winleft;
    unsigned char wintop;
    unsigned char winright;
    unsigned char winbottom;
    unsigned char attribute;
    unsigned char normattr;
    unsigned char currmode;
    unsigned char screenheight;
    unsigned char screenwidth;
    unsigned char curx;
    unsigned char cury;
};

enum text_modes { LASTMODE=-1, BW40=0, C40, BW80, C80, MONO=7, C4350=64 };

enum COLORS {
    /*  dark colors     */
    BLACK,          
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    /*  light colors    */
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

#define BLINK   0x80    /*  blink bit   */

//void    blinkvideo(void);
char *  _cgets(char *_str); 
//void    clreol(void);
//void    clrscr(void);
int     _kbhit(void); /* checks for ungetch char */
int     _cprintf(const char *_format, ...) __attribute__((format(printf,1,2)));
int     _cputs(const char *_str);
int     _cscanf(const char *_format, ...) __attribute__((format(scanf,1,2)));
//void    delline(void);
int     _getch(void);
int     _getche(void);
//int     gettext(int _left, int _top, int _right, int _bottom, void *_destin);
//void    gettextinfo(struct text_info *_r);
//void    gotoxy(int _x, int _y);
//void    gppconio_init(void);
//void    highvideo(void);
//void    insline(void);
//void	intensevideo(void);
//void    lowvideo(void);
//int     movetext(int _left, int _top, int _right, int _bottom, int _destleft, int _desttop);
//void    normvideo(void);
int     _putch(int _c);
//int     puttext(int _left, int _top, int _right, int _bottom, void *_source);
//void    _setcursortype(int _type);
//void    _set_screen_lines(int _nlines);
//void    textattr(int _attr);
//void    textbackground(int _color);
//void    textcolor(int _color);
//void    textmode(int _mode);
int     _ungetch(int); 
//unsigned int     wherex(void);
//unsigned int    wherey(void);
//void    window(int _left, int _top, int _right, int _bottom);


int _inp(unsigned short p);
unsigned short _inpw(unsigned short p);
unsigned long _inpd(unsigned short p);
int _outp(unsigned short p, int i);
unsigned short _outpw(unsigned short p, unsigned short w);
unsigned long _outpd(unsigned short p, unsigned long d);

#define cgets		_cgets
#define cprintf 	_cprintf
#define cputs		_cputs
#define cscanf		_cscanf
#define getch		_getch
#define getche		_getche
#define kbhit		_kbhit
#define putch		_putch
#define ungetch 	_ungetch



#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* !__dj_include_conio_h_ */
