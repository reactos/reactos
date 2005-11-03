/* ----------- console.c ---------- */

// #define WIN32_LEAN_AND_MEAN
#include <windows.h>


#include "dflat32/dflat.h"


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

static COORD cursorpos[MAXSAVES];
static CONSOLE_CURSOR_INFO cursorinfo[MAXSAVES];
static int cs = 0;


void SwapCursorStack(void)
{
	if (cs > 1)
	{
		COORD coord;
		CONSOLE_CURSOR_INFO csi;

		coord = cursorpos[cs-2];
		cursorpos[cs-2] = cursorpos[cs-1];
		cursorpos[cs-1] = coord;

		memcpy (&csi,
		        &cursorinfo[cs-2],
		        sizeof(CONSOLE_CURSOR_INFO));
		memcpy (&cursorinfo[cs-2],
		        &cursorinfo[cs-1],
		        sizeof(CONSOLE_CURSOR_INFO));
		memcpy (&cursorinfo[cs-1],
		        &csi,
		        sizeof(CONSOLE_CURSOR_INFO));
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
	if (cs < MAXSAVES)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;

		GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);
		cursorpos[cs].X = csbi.dwCursorPosition.X;
		cursorpos[cs].Y = csbi.dwCursorPosition.Y;

		GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
		                      &(cursorinfo[cs]));

		cs++;
	}
}

/* ---- restore the saved cursor configuration ---- */
void restorecursor(void)
{
	if (cs)
	{
		--cs;
		SetConsoleCursorPosition (GetStdHandle (STD_OUTPUT_HANDLE),
		                          cursorpos[cs]);
		SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
		                      &(cursorinfo[cs]));
	}
}

/* ------ make a normal cursor ------ */
void normalcursor(void)
{
	CONSOLE_CURSOR_INFO csi;

	csi.bVisible = TRUE;
	csi.dwSize = 5;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
}

/* ------ hide the cursor ------ */
void hidecursor(void)
{
	CONSOLE_CURSOR_INFO csi;

	GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
	csi.bVisible = FALSE;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
}

/* ------ unhide the cursor ------ */
void unhidecursor(void)
{
	CONSOLE_CURSOR_INFO csi;

	GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
	csi.bVisible = TRUE;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
}

/* set the cursor size (in percent) */
void set_cursor_size (unsigned t)
{
	CONSOLE_CURSOR_INFO csi;

	GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);

	if (t < 2)
		csi.dwSize = 2;
	else if (t > 90)
		csi.dwSize = 90;
	else
		csi.dwSize = t;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
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
