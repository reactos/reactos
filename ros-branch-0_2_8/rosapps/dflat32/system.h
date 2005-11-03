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
#define DF_KEYBOARDPORT 0x60
#define DF_FREQUENCY 100
#define DF_COUNT (1193280L / DF_FREQUENCY)
#define DF_ZEROFLAG 0x40
#define DF_MAXSAVES 50

//#define DF_SCREENWIDTH  (80)
//#define DF_SCREENHEIGHT (25)

HANDLE DfInput;
HANDLE DfOutput;

SHORT DfScreenHeight;
SHORT DfScreenWidth;


/* ---------- keyboard prototypes -------- */
int DfAltConvert(int);
void DfGetKey(PINPUT_RECORD);
int DfGetShift(void);
BOOL DfKeyHit(void);
void DfBeep(void);

/* ---------- DfCursor prototypes -------- */
void DfCurrCursor(int *x, int *y);
void DfCursor(int x, int y);
void DfHideCursor(void);
void DfUnhideCursor(void);
void DfSaveCursor(void);
void DfRestoreCursor(void);
void DfNormalCursor(void);
void DfSetCursorSize(unsigned t);
void DfVideoMode(void);
void DfSwapCursorStack(void);

/* ------------ timer macros -------------- */
#define DfTimedOut(timer)         (timer==0)
#define DfSetTimer(timer, secs)     timer=(secs)*182/10+1
#define DfDisableTimer(timer)     timer = -1
#define DfTimerRunning(timer)     (timer > 0)
#define DfCountdown(timer)         --timer
#define DfTimerDisabled(timer)     (timer == -1)


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
#define DfKeyHit       kbhit
#endif
#endif

typedef enum DfMessages {
#ifdef WATCOM
	DF_WATCOMFIX1 = -1,
#endif
	#undef DFlatMsg
	#define DFlatMsg(m) m,
	#include "dflatmsg.h"
	DF_MESSAGECOUNT
} DFMESSAGE;

typedef enum DfWindowClass    {
#ifdef WATCOM
	DF_WATCOMFIX2 = -1,
#endif
	#define DfClassDef(c,b,p,a) c,
	#include "classes.h"
	DF_CLASSCOUNT
} DFCLASS;

#endif
