/* ---------- dfalloc.c ---------- */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dflat.h"

static void AllocationError(void)
{
	static BOOL OnceIn = FALSE;
	extern jmp_buf AllocError;
	extern BOOL AllocTesting;
	static char *ErrMsg[] = {
		"旼컴컴컴컴컴컴컴커",
		"� Out of Memory! �",
		"R컴컴컴컴컴컴컴컴U"
	};
	int x, y;
	CHAR_INFO savbuf[54];
	DFRECT rc = {30,11,47,13};
	INPUT_RECORD ir;

	if (!OnceIn)
	{
		OnceIn = TRUE;
		/* ------ close all windows ------ */
		DfSendMessage(ApplicationWindow, CLOSE_WINDOW, 0, 0);
		GetVideo(rc, savbuf);
		for (x = 0; x < 18; x++)
		{
			for (y = 0; y < 3; y++)
			{
				int c = (255 & (*(*(ErrMsg+y)+x))) | 0x7000;
				PutVideoChar(x+rc.lf, y+rc.tp, c);
			}
		}
		GetKey(&ir);
		StoreVideo(rc, savbuf);
		if (AllocTesting)
			longjmp(AllocError, 1);
	}
}

void *DFcalloc(size_t nitems, size_t size)
{
	void *rtn = calloc(nitems, size);
	if (size && rtn == NULL)
		AllocationError();
	return rtn;
}

void *DFmalloc(size_t size)
{
	void *rtn = malloc(size);
	if (size && rtn == NULL)
		AllocationError();
	return rtn;
}

void *DFrealloc(void *block, size_t size)
{
	void *rtn;

	rtn = realloc(block, size);
	if (size && rtn == NULL)
		AllocationError();
	return rtn;
}

/* EOF */
