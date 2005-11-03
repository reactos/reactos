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
		DfSendMessage(DfApplicationWindow, DFM_CLOSE_WINDOW, 0, 0);
		DfGetVideo(rc, savbuf);
		for (x = 0; x < 18; x++)
		{
			for (y = 0; y < 3; y++)
			{
				int c = (255 & (*(*(ErrMsg+y)+x))) | 0x7000;
				DfPutVideoChar(x+rc.lf, y+rc.tp, c);
			}
		}
		DfGetKey(&ir);
		DfStoreVideo(rc, savbuf);
		if (AllocTesting)
			longjmp(AllocError, 1);
	}
}

void *DfCalloc(size_t nitems, size_t size)
{
	void *rtn = calloc(nitems, size);
	if (size && rtn == NULL)
		AllocationError();
	return rtn;
}

void *DfMalloc(size_t size)
{
	void *rtn = malloc(size);
	if (size && rtn == NULL)
		AllocationError();
	return rtn;
}

void *DfRealloc(void *block, size_t size)
{
	void *rtn;

	rtn = realloc(block, size);
	if (size && rtn == NULL)
		AllocationError();
	return rtn;
}

/* EOF */
