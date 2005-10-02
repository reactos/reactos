/* $Id$
 * 
 * ReactOS Operating System
 */
#include "ntrt0.h"

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME
 * 	NtRtParseCommandLine/2
 */
NTSTATUS STDCALL NtRtParseCommandLine (PPEB Peb, int * argc, char ** argv)
{
	*argc=0;
	argv[0]=NULL;
	//TODO
	return STATUS_SUCCESS;
}
/* EOF */
