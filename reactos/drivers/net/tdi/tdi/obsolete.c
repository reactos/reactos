/* $Id: obsolete.c,v 1.7 2004/01/28 20:55:50 ekohl Exp $
 *
 */

#include <ddk/ntddk.h>

/*
 * @unimplemented
 */
VOID
STDCALL
TdiMapBuffer (
	DWORD	Unknown0
	)
{
	/* This function is absolete */
}


/*
 * @unimplemented
 */
VOID
STDCALL
TdiUnmapBuffer (
	DWORD	Unknown0
	)
{
	/* This function is obsolete */
}

/* EOF */
