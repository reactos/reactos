/* --------------- system.h -------------- */
#ifndef SYSTEM_H
#define SYSTEM_H

//#if MSC | WATCOM
#include <direct.h>
//#else
//#include <dir.h>
//#endif

#define swap(a,b){int x=a;a=b;b=x;}
/* ------- platform-dependent values ------ */
#define KEYBOARDPORT 0x60
#define FREQUENCY 100
#define COUNT (1193280L / FREQUENCY)
#define ZEROFLAG 0x40
#define MAXSAVES 50

//#define SCREENWIDTH  (80)
//#define SCREENHEIGHT (25)

HANDLE hInput;
HANDLE hOutput;

SHORT sScreenHeight;
SHORT sScreenWidth;


/* ---------- keyboard prototypes -------- */
int AltConvert(int);
void GetKey(PINPUT_RECORD);
int getshift(void);
BOOL keyhit(void);
void beep(void);

/* ---------- cursor prototypes -------- */
void curr_cursor(int *x, int *y);
void cursor(int x, int y);
void hidecursor(void);
void unhidecursor(void);
void savecursor(void);
void restorecursor(void);
void normalcursor(void);
void set_cursor_size(unsigned t);
void videomode(void);
void SwapCursorStack(void);

/* ------------ timer macros -------------- */
#define timed_out(timer)         (timer==0)
#define set_timer(timer, secs)     timer=(secs)*182/10+1
#define disable_timer(timer)     timer = -1
#define timer_running(timer)     (timer > 0)
#define countdown(timer)         --timer
#define timer_disabled(timer)     (timer == -1)


#ifndef TURBOC
#ifndef BCPP
/* ============= Color Macros ============ */
#define BLACK         0
#define BLUE          1
#define GREEN         2
#define CYAN          3
#define RED           4
#define MAGENTA       5
#define BROWN         6
#define LIGHTGRAY     7
#define DARKGRAY      8
#define LIGHTBLUE     9
#define LIGHTGREEN   10
#define LIGHTCYAN    11
#define LIGHTRED     12
#define LIGHTMAGENTA 13
#define YELLOW       14
#define WHITE        15
#define keyhit       kbhit
#endif
#endif

typedef enum messages {
#ifdef WATCOM
	WATCOMFIX1 = -1,
#endif
	#undef DFlatMsg
	#define DFlatMsg(m) m,
	#include "dflatmsg.h"
	MESSAGECOUNT
} DFMESSAGE;

typedef enum window_class    {
#ifdef WATCOM
	WATCOMFIX2 = -1,
#endif
	#define ClassDef(c,b,p,a) c,
	#include "classes.h"
	CLASSCOUNT
} DFCLASS;

#endif
