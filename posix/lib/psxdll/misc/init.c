/* $Id: init.c,v 1.1 2002/02/24 22:14:05 ea Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        reactos/subsys/psx/lib/psxdll/misc/init.c
 * PURPOSE:     Client initialization
 * PROGRAMMER:  Emanuele Aliberti
 * UPDATE HISTORY:
 *               2001-05-06
 */
#define NTOS_MODE_USER
#include <ntos.h>

/* DLL GLOBALS */
int * errno = NULL;
char *** _environ = NULL;
/*
 * Called by startup code in crt0.o, where real
 * errno and _environ are actually defined.
 */
VOID STDCALL __PdxInitializeData (int * errno_arg, char *** environ_arg)
{
	errno    = errno_arg;	
	_environ = environ_arg;	
}
/* EOF */

