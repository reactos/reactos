/* ----------- console.c ---------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#include "dflat.h"


/* ----- table of alt keys for finding shortcut keys ----- */
#if 0
static int altconvert[] = {
    DF_ALT_A,DF_ALT_B,DF_ALT_C,DF_ALT_D,DF_ALT_E,DF_ALT_F,DF_ALT_G,DF_ALT_H,
    DF_ALT_I,DF_ALT_J,DF_ALT_K,DF_ALT_L,DF_ALT_M,DF_ALT_N,DF_ALT_O,DF_ALT_P,
    DF_ALT_Q,DF_ALT_R,DF_ALT_S,DF_ALT_T,DF_ALT_U,DF_ALT_V,DF_ALT_W,DF_ALT_X,
    DF_ALT_Y,DF_ALT_Z,DF_ALT_0,DF_ALT_1,DF_ALT_2,DF_ALT_3,DF_ALT_4,DF_ALT_5,
    DF_ALT_6,DF_ALT_7,DF_ALT_8,DF_ALT_9
};
#endif

static COORD cursorpos[DF_MAXSAVES];
static CONSOLE_CURSOR_INFO cursorinfo[DF_MAXSAVES];
static int cs = 0;


void DfSwapCursorStack(void)
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
void DfGetKey (PINPUT_RECORD lpBuffer)
{
	HANDLE DfInput;
	DWORD dwRead;

	DfInput = GetStdHandle (STD_INPUT_HANDLE);

	do
	{
//		WaitForSingleObject (DfInput, INFINITE);
		ReadConsoleInput (DfInput, lpBuffer, 1, &dwRead);
		if ((lpBuffer->EventType == KEY_EVENT) &&
			(lpBuffer->Event.KeyEvent.bKeyDown == TRUE))
			break;
	}
	while (TRUE);
}


/* ---------- read the keyboard shift status --------- */

int DfGetShift(void)
{
//    regs.h.ah = 2;
//    int86(KEYBRD, &regs, &regs);
//    return regs.h.al;
/*	FIXME */

	return 0;
}


/* -------- sound a buzz tone ---------- */
void DfBeep(void)
{
	Beep(440, 50);
//	MessageBeep (-1);
}


/* ------ position the DfCursor ------ */
void DfCursor(int x, int y)
{
	COORD coPos;

	coPos.X = (USHORT)x;
	coPos.Y = (USHORT)y;
	SetConsoleCursorPosition (GetStdHandle (STD_OUTPUT_HANDLE), coPos);
}


/* ------- get the current DfCursor position ------- */
void DfCurrCursor(int *x, int *y)
//VOID GetCursorXY (PSHORT x, PSHORT y)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &csbi);

	*x = (int)csbi.dwCursorPosition.X;
	*y = (int)csbi.dwCursorPosition.Y;
}


/* ------ save the current DfCursor configuration ------ */
void DfSaveCursor(void)
{
	if (cs < DF_MAXSAVES)
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

/* ---- restore the saved DfCursor configuration ---- */
void DfRestoreCursor(void)
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

/* ------ make a normal DfCursor ------ */
void DfNormalCursor(void)
{
	CONSOLE_CURSOR_INFO csi;

	csi.bVisible = TRUE;
	csi.dwSize = 5;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
}

/* ------ hide the DfCursor ------ */
void DfHideCursor(void)
{
	CONSOLE_CURSOR_INFO csi;

	GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
	csi.bVisible = FALSE;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
}

/* ------ unhide the DfCursor ------ */
void DfUnhideCursor(void)
{
	CONSOLE_CURSOR_INFO csi;

	GetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
	csi.bVisible = TRUE;
	SetConsoleCursorInfo (GetStdHandle (STD_OUTPUT_HANDLE),
	                      &csi);
}

/* set the DfCursor size (in percent) */
void DfSetCursorSize (unsigned t)
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
int DfAltConvert(int c)
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
