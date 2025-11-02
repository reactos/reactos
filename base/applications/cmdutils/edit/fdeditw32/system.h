/* --------------- system.h -------------- */
#ifndef SYSTEM_H
#define SYSTEM_H

/*#if MSC | WATCOM*/
#if defined(_MSC_VER) || defined(WATCOM)
#include <direct.h>
#else
#include <dir.h>
#endif

#define swap(a,b){int x=a;a=b;b=x;}
/* ------- platform-dependent values ------ */
#ifdef _WIN32
#include "dir_w32.h"
#include "country_w32.h"
int w32_screenwidth(void);
int w32_screenheight(void);
#define SCREENWIDTH         w32_screenwidth()
#define SCREENHEIGHT        w32_screenheight()
void init_videomode(void);
void uninit_videomode(void);
void clearBIOSbuffer(void);
void w32_trap_to_sched(void);
int w32_get_video(int x, int y);
void w32_put_video(int x, int y, int ch);
void w32_scroll_up(int x1,int y1,int x2,int y2,int attr);
void w32_scroll_dw(int x1,int y1,int x2,int y2,int attr);
#define waitforkeyboard()
#define TRAP_TO_SCHEDULER   w32_trap_to_sched();
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
void init_timer(void);
void exit_timer(void);
void update_key_buffer(void);
int w32_getdisks(void);
/* these must be placed somewhere... */
/*
void fix_popdown(struct PopDown *popdown);
void fix_menu(MENU *menu);
void fix_mbar(MBAR *mbar);
*/
#else
#define init_videomode()
#define uninit_videomode()
/* ----- interrupt vectors ----- */
#define TIMER  8
#define VIDEO  0x10
#define KEYBRD 0x16
#define DOS    0x21
#define CRIT   0x24
#define MOUSE  0x33
#define KEYBOARDVECT 9

#define KEYBOARDPORT 0x60
#define FREQUENCY    100
#define COUNT        (1193280L / FREQUENCY)
#define ZEROFLAG     0x40
#define MAXSAVES     50
#define SCREENWIDTH  (peekb(0x40,0x4a) & 255)
#define SCREENHEIGHT (isVGA() || isEGA() ? peekb(0x40,0x84)+1 : 25)
#define clearBIOSbuffer() *(int far *)(MK_FP(0x40,0x1a)) = \
		        	      *(int far *)(MK_FP(0x40,0x1c));

#define waitforkeyboard() while (Xbioskey(1)) (void)Xbioskey(0)
#define TRAP_TO_SCHEDULER
/* ----- keyboard BIOS (0x16) functions -------- */
#define READKB 0
#define KBSTAT 1
/* ------- video BIOS (0x10) functions --------- */
#define SETCURSORTYPE 1
#define SETCURSOR     2
#define READCURSOR    3
#define READATTRCHAR  8
#define WRITEATTRCHAR 9
#define HIDECURSOR    0x20
/* ------- the interrupt function registers -------- */
typedef struct {
    int bp,di,si,ds,es,dx,cx,bx,ax,ip,cs,fl;
} IREGS;
#endif
/* ---------- keyboard prototypes -------- */
int AltConvert(int);
int getkey(void);
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
void set_cursor_type(unsigned t);
void videomode(void);
void SwapCursorStack(void);
/* --------- screen prototpyes -------- */
void clearscreen(void);
/* ---------- mouse prototypes ---------- */
BOOL mouse_installed(void);
int mousebuttons(void);
void get_mouseposition(int *x, int *y);
void set_mouseposition(int x, int y);
void show_mousecursor(void);
void hide_mousecursor(void);
int button_releases(void);
void resetmouse(void);
void set_mousetravel(int, int, int, int);
#define leftbutton()     (mousebuttons()&1)
#define rightbutton()    (mousebuttons()&2)
#define waitformouse()   while(mousebuttons());
/* ------------ timer macros -------------- */
int timed_out(int timer);
void set_timer(int timer, int secs);
void set_timer_ticks(int timer, int ticks);
void disable_timer(int timer);
int timer_running(int timer);
/* void countdown(int timer); -- only used in IRQ handler anyway -- */
int timer_disabled(int timer);
/* ----------- video adaptor prototypes ----------- */
BOOL isEGA(void);
BOOL isVGA(void);
void Set25(void);
void Set43(void);
void Set50(void);

#ifndef BLACK
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
#endif

#if MSC | WATCOM
/*#define keyhit       ((kbhit()) ? TRUE : FALSE)*/
#endif

#if MSC | WATCOM
/* ============= Compatibility Macros ============ */
#define asm __asm
#undef FP_OFF
#undef FP_SEG
#undef MK_FP
#define FP_OFF(p)    ((unsigned)(p))
#define FP_SEG(p)    ((unsigned)((unsigned long)(p) >> 16))
#define MK_FP(s,o)   ((void far *) \
               (((unsigned long)(s) << 16) | (unsigned)(o)))
#define findfirst(p,f,a) _dos_findfirst(p,a,f)
#define findnext(f)      _dos_findnext(f)
#define ffblk            find_t
#define ff_name          name
#define ff_fsize         size
#define ff_attrib        attrib
#define poke(a,b,c)  (*((int  far*)MK_FP((a),(b))) = (int)(c))
#define pokeb(a,b,c) (*((char far*)MK_FP((a),(b))) = (char)(c))
#define peek(a,b)    (*((int  far*)MK_FP((a),(b))))
#define peekb(a,b)   (*((char far*)MK_FP((a),(b))))
#define getvect(v)   _dos_getvect(v)
#define setvect(v,f) _dos_setvect(v,f)
#define fnsplit          _splitpath
#define fnmerge          _makepath
#define EXTENSION         2
#define FILENAME          4
#define DIRECTORY         8
#define DRIVE            16
int getdisk(void);
#define setdisk(d)       _dos_setdrive((d)+1, NULL)
#define bioskey          _bios_keybrd
#define MAXPATH          80
#define MAXDRIVE          3
#define MAXDIR           66
#define MAXFILE           9
#define MAXEXT            5
#endif

typedef enum messages {
#ifdef WATCOM
	WATCOMFIX1 = -1,
#endif
	#undef DFlatMsg
	#define DFlatMsg(m) m,
	#include "dflatmsg.h"
	MESSAGECOUNT
} MESSAGE;

typedef enum window_class    {
#ifdef WATCOM
	WATCOMFIX2 = -1,
#endif
	#define ClassDef(c,b,p,a) c,
	#include "classes.h"
	CLASSCOUNT
} CLASS;

#endif
