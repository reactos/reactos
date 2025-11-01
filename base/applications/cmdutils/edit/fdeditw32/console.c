/* ----------- console.c ---------- */

#include "dflat.h"

/* ----- table of alt keys for finding shortcut keys ----- */
static int altconvert[] = {
    ALT_A,ALT_B,ALT_C,ALT_D,ALT_E,ALT_F,ALT_G,ALT_H,
    ALT_I,ALT_J,ALT_K,ALT_L,ALT_M,ALT_N,ALT_O,ALT_P,
    ALT_Q,ALT_R,ALT_S,ALT_T,ALT_U,ALT_V,ALT_W,ALT_X,
    ALT_Y,ALT_Z,ALT_0,ALT_1,ALT_2,ALT_3,ALT_4,ALT_5,
    ALT_6,ALT_7,ALT_8,ALT_9
};

unsigned video_mode;
unsigned video_page;
static int near cursorpos[MAXSAVES];
static int near cursorshape[MAXSAVES];
static union REGS regs;
static int cs;
static int far *clk = MK_FP(0x40,0x6c);

/* ------- macro to wait one clock tick -------- */
#define wait()           \
{                        \
    int now = *clk;      \
    while (now == *clk); \
}

/* ------------- clear the screen -------------- */
void clearscreen(void)
{
    int ht=SCREENHEIGHT;
    int wd=SCREENWIDTH;

    cursor(0, 0);
    regs.h.al=' ';
    regs.h.ah=9;
    regs.x.bx=7;
    regs.x.cx=ht * wd;
    int86(VIDEO, &regs, &regs);

}

void SwapCursorStack(void)
{
    if (cs > 1)
        {
        swap(cursorpos[cs-2], cursorpos[cs-1]);
        swap(cursorshape[cs-2], cursorshape[cs-1]);
	}

}

/* ---- BIOS keyboard routines with 84 and 102 key keyboard support ---- */
int Xbioskey(int cmd)
{
    static int keybase=-1;
    union REGS kregs;

    if (keybase < 0)
        {
        volatile char far *kbtype = MK_FP(0x40,0x96); /* BIOS data flag */
        keybase=(((*kbtype) & 0x10) != 0) ? 0x10 : 0;
	/* 0 for 84 key XT mode, 0x10 for 102 key AT mode. */
	/* (0x20 for 122 key mode, which is not used here) */
        }

    kregs.h.ah = (char) (keybase + cmd);
    kregs.h.al = 0;
    int86(0x16, &kregs, &kregs);
    if ((cmd == 1) && (kregs.x.flags & 0x40 /* zero flag */))
        return 0;

    return kregs.x.ax;

}

/* ---- Test for keystroke ---- */
BOOL keyhit(void)
{
#if MSC | WATCOM
    return (kbhit() ? TRUE : FALSE);
#else
    return (Xbioskey(1) != 0) ? TRUE : FALSE;
#endif
}

/* ---- Read a keystroke ---- */
int getkey(void)
{
    unsigned int c;
    unsigned int theShift;
    unsigned int theScan;

    while (keyhit() == FALSE);          /* Wait for a key */
    c=Xbioskey(0);                      /* Fetch key */
    theShift=getshift();
    theScan=c >> 8;                     /* Scan code */
    c=c & 0xff;                         /* ASCII code or 0 of 0xe0 */

    if (theShift & (LEFTSHIFT|RIGHTSHIFT))
        {
        /* BIOS normally calls shift-ins "ins" and shift-del "del" */
        if (theScan == 0x52)            /* INS */
            return CTRL_V;              /* SHIFT_INS */

        if (theScan == 0x53)            /* DEL */
            return CTRL_X;              /* SHIFT_DEL */

        }

    if ((theShift & ALTKEY) && (theScan == 0x0e)) /* Alt-BS  */
        return CTRL_Z;                  /* ALT_BS */

    if (theShift & CTRLKEY)
        {
        if (theScan == 0x92)            /* ^ins */
            {
            return CTRL_C;              /* CTRL_INS */
            }

        }

    if ((c != 0) && (c != 0xe0))        /* Nonzero / nonnumpad ASCII part? */
        return c;                       /* Then return only the ASCII part */

    /* Watch out: Special case for Russian non-numpad "0xe0 ASCII" */
    if ((c == 0xe0) && (theScan == 0))
        return 0xe0;

    return (FKEY | theScan);

}

/* ---------- read the keyboard shift status --------- */
int getshift(void)
{
    static int enhkeyb=-1;              /* 1 for an enhanced keyboard */
    static char far *kbtype = MK_FP(0x40,0x96);

    if (enhkeyb == -1)                  /* If we do not yet know... */
        {
        enhkeyb=(((*kbtype) & 0x10) != 0) ? 1 : 0; /* read BIOS data flag! */
        }                               /* Now enhkeyb is either 0 or 1 - Eric */

    if (!enhkeyb)                       /* Old/new by Eric */
        {
        regs.h.ah = 2;
        int86(KEYBRD, &regs, &regs);
        return regs.h.al;
        }
    else
        {
        regs.h.ah=0x12;                 /* Extended shift: AL as above... */
        int86(KEYBRD, &regs, &regs);

        /* Ignore SysRQ (Alt-PrtScr) and shift lock presses */
        regs.x.ax &= 0x0fff;
        if (regs.x.ax & RALTKEY)        /* Treat RALT as NO ALT (but as AltGr */
            regs.x.ax &= ~ALTKEY;

        return regs.x.ax;
        }

}

/* -------- sound a buzz tone, using hardware directly ---------- */
void beep(void)
{
    wait();
    outp(0x43, 0xb6);                   /* program the frequency */
    outp(0x42, (int) (COUNT % 256));
    outp(0x42, (int) (COUNT / 256));
    outp(0x61, inp(0x61) | 3);          /* start the sound */
    wait();
    outp(0x61, inp(0x61) & ~3);         /* stop the sound  */

}

/* -------- get the video mode and page from BIOS -------- */
void videomode(void)
{
    regs.h.ah = 15;
    int86(VIDEO, &regs, &regs);
    video_mode = regs.h.al;
    video_page = regs.x.bx;
    video_page &= 0xff00;
    video_mode &= 0x7f;

}

/* ------ position the cursor ------ */
void cursor(int x, int y)
{
    videomode();
    regs.x.dx=((y << 8) & 0xff00)+x;
    regs.h.ah=SETCURSOR;
    regs.x.bx=video_page;
    int86(VIDEO, &regs, &regs);

}

/* ------ get cursor shape and position ------ */
static void near getcursor(void)
{
    videomode();
    regs.h.ah=READCURSOR;
    regs.x.bx=video_page;
    int86(VIDEO, &regs, &regs);

}

/* ------- get the current cursor position ------- */
void curr_cursor(int *x, int *y)
{
    getcursor();
    *x=regs.h.dl;
    *y=regs.h.dh;

}

/* ------ save the current cursor configuration ------ */
void savecursor(void)
{
    if (cs < MAXSAVES)
        {
        getcursor();
        cursorshape[cs]=regs.x.cx;
        cursorpos[cs]=regs.x.dx;
        cs++;
        }

}

/* ---- restore the saved cursor configuration ---- */
void restorecursor(void)
{
    if (cs)
        {
        --cs;
        videomode();
        regs.x.dx=cursorpos[cs];
        regs.h.ah=SETCURSOR;
        regs.x.bx=video_page;
        int86(VIDEO, &regs, &regs);
        set_cursor_type(cursorshape[cs]);
        }

}

/* ------ make a normal cursor ------ */
void normalcursor(void)
{
    set_cursor_type(0x0106);

}

/* ------ hide the cursor ------ */
void hidecursor(void)
{
    getcursor();
    regs.h.ch |= HIDECURSOR;
    regs.h.ah = SETCURSORTYPE;
    int86(VIDEO, &regs, &regs);

}

/* ------ unhide the cursor ------ */
void unhidecursor(void)
{
    getcursor();
    regs.h.ch &= ~HIDECURSOR;
    regs.h.ah = SETCURSORTYPE;
    int86(VIDEO, &regs, &regs);

}

/* ---- use BIOS to set the cursor type ---- */
void set_cursor_type(unsigned t)
{
    videomode();
    regs.h.ah=SETCURSORTYPE;
    regs.x.bx=video_page;
    regs.x.cx=t;
    int86(VIDEO, &regs, &regs);

}

/* ---- test for EGA -------- */
BOOL isEGA(void)
{
    if (isVGA())
        return FALSE;

    regs.h.ah=0x12;
    regs.h.bl=0x10;
    int86(VIDEO, &regs, &regs);
    return regs.h.bl != 0x10;

}

/* ---- test for VGA -------- */
BOOL isVGA(void)
{
    regs.x.ax=0x1a00;
    int86(VIDEO, &regs, &regs);
    return regs.h.al == 0x1a && regs.h.bl > 6;

}

static void Scan350(void)
{
    regs.x.ax=0x1201;
    regs.h.bl=0x30;
    int86(VIDEO, &regs, &regs);
    regs.h.ah=0x0f;
    int86(VIDEO, &regs, &regs);
    regs.h.ah=0x00;
    int86(VIDEO, &regs, &regs);

}

static void Scan400(void)
{
    regs.x.ax=0x1202;
    regs.h.bl=0x30;
    int86(VIDEO, &regs, &regs);
    regs.h.ah=0x0f;
    int86(VIDEO, &regs, &regs);
    regs.h.ah=0x00;
    int86(VIDEO, &regs, &regs);

}

/* ---------- set 25 line mode ------- */
void Set25(void)
{
    if (isVGA())
        {
        Scan400();
        regs.x.ax=0x1114;
	}
    else
        regs.x.ax=0x1111;

    regs.h.bl=0;
    int86(VIDEO, &regs, &regs);

}

/* ---------- set 43 line mode ------- */
void Set43(void)
{
    if (isVGA())
        Scan350();

    regs.x.ax=0x1112;
    regs.h.bl=0;
    int86(VIDEO, &regs, &regs);

}

/* ---------- set 50 line mode ------- */
void Set50(void)
{
    if (isVGA())
        Scan400();

    regs.x.ax=0x1112;
    regs.h.bl=0;
    int86(VIDEO, &regs, &regs);

}

/* ------ convert an Alt+ key to its letter equivalent ----- */
int AltConvert(int c)
{
    int i,a=0;

    for (i=0;i<36;i++)
        if (c == altconvert[i])
            break;

    if (i < 26)
        a='a'+i;
    else if (i < 36)
        a='0'+i-26;

    return a;

}

#if MSC | WATCOM
int getdisk(void)
{
    unsigned int cd;

    _dos_getdrive(&cd);
    cd -= 1;
    return cd;
}
#endif
