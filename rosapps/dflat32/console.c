/* ----------- console.c ---------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#include "dflat.h"


/* ----- table of alt keys for finding shortcut keys ----- */
#if 0
static int altconvert[] = {
    ALT_A,ALT_B,ALT_C,ALT_D,ALT_E,ALT_F,ALT_G,ALT_H,
    ALT_I,ALT_J,ALT_K,ALT_L,ALT_M,ALT_N,ALT_O,ALT_P,
    ALT_Q,ALT_R,ALT_S,ALT_T,ALT_U,ALT_V,ALT_W,ALT_X,
    ALT_Y,ALT_Z,ALT_0,ALT_1,ALT_2,ALT_3,ALT_4,ALT_5,
    ALT_6,ALT_7,ALT_8,ALT_9
};
#endif

static int cursorpos[MAXSAVES];
static int cursorshape[MAXSAVES];
static int cs;


void SwapCursorStack(void)
{
	if (cs > 1)	{
		swap(cursorpos[cs-2], cursorpos[cs-1]);
		swap(cursorshape[cs-2], cursorshape[cs-1]);
	}
}


/* ---- Read a keystroke ---- */
void GetKey (PINPUT_RECORD lpBuffer)
{
	HANDLE hInput;
	DWORD dwRead;

	hInput = GetStdHandle (STD_INPUT_HANDLE);

	do
	{
//		WaitForSingleObject (hInput, INFINITE);
		ReadConsoleInput (hInput, lpBuffer, 1, &dwRead);
		if ((lpBuffer->EventType == KEY_EVENT) &&
			(lpBuffer->Event.KeyEvent.bKeyDown == TRUE))
			break;
	}
	while (TRUE);
}


/* ---------- read the keyboard shift status --------- */
int getshift(void)
{
//    regs.h.ah = 2;
//    int86(KEYBRD, &regs, &regs);
//    return regs.h.al;
/*	FIXME */

	return 0;
}


/* -------- sound a buzz tone ---------- */
void beep(void)
{
	Beep(440, 50);
//	MessageBeep (-1);
}


/* ------ position the cursor ------ */
void cursor(int x, int y)
{
	COORD coPos;

	coPos.X = (USHORT)x;
	coPos.Y = (USHORT)y;
	SetConsoleCursorPosition (GetStdHandle (STD_OUTPUT_HANDLE), coPos);
}

/* ------ get cursor shape and position ------ */
static void getcursor(void)
{
/*
    videomode();
    regs.h.ah = READCURSOR;
    regs.x.bx = video_page;
    int86(VIDEO, &regs, &regs);
*/
	/* FIXME */
}

/* ------- get the current cursor position ------- */

void curr_cursor(int *x, int *y)
//VOID GetCursorXY (PSHORT x, PSHORT y)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);

	*x = (int)csbi.dwCursorPosition.X;
	*y = (int)csbi.dwCursorPosition.Y;
}


/* ------ save the current cursor configuration ------ */
void savecursor(void)
{
/*
    if (cs < MAXSAVES)    {
        getcursor();
        cursorshape[cs] = regs.x.cx;
        cursorpos[cs] = regs.x.dx;
        cs++;
    }
*/
}

/* ---- restore the saved cursor configuration ---- */
void restorecursor(void)
{
/*
    if (cs)    {
        --cs;
        videomode();
        regs.x.dx = cursorpos[cs];
        regs.h.ah = SETCURSOR;
        regs.x.bx = video_page;
        int86(VIDEO, &regs, &regs);
        set_cursor_type(cursorshape[cs]);
    }
*/
}

/* ------ make a normal cursor ------ */
void normalcursor(void)
{
//    set_cursor_type(0x0607);
}

/* ------ hide the cursor ------ */
void hidecursor(void)
{
/*
    getcursor();
    regs.h.ch |= HIDECURSOR;
    regs.h.ah = SETCURSORTYPE;
    int86(VIDEO, &regs, &regs);
*/
}

/* ------ unhide the cursor ------ */
void unhidecursor(void)
{
/*
    getcursor();
    regs.h.ch &= ~HIDECURSOR;
    regs.h.ah = SETCURSORTYPE;
    int86(VIDEO, &regs, &regs);
*/
}

/* ---- use BIOS to set the cursor type ---- */
void set_cursor_type(unsigned t)
{
/*
    videomode();
    regs.h.ah = SETCURSORTYPE;
    regs.x.bx = video_page;
    regs.x.cx = t;
    int86(VIDEO, &regs, &regs);
*/
}


/* ------ convert an Alt+ key to its letter equivalent ----- */
int AltConvert(int c)
{
	return c;
#if 0
	int i, a = 0;
	for (i = 0; i < 36; i++)
		if (c == altconvert[i])
			break;
	if (i < 26)
		a = 'a' + i;
	else if (i < 36)
		a = '0' + i - 26;
	return a;
#endif
}

/* EOF */
