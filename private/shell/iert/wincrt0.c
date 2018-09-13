/***
*wincrt0.c - C runtime Windows EXE start-up routine
*
*	Copyright (c) 1993-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This is the actual startup routine for Windows apps.  It calls the
*	user's main routine WinMain() after performing C Run-Time Library
*	initialization.
*
*Revision History:
*	??-??-??  ???	Module created.
*	09-01-94  SKS	Module commented.
*	10-28-94  SKS	Remove user32.lib as a default library -- it is now
*			specified in crt0init.obj along with kernel32.lib.
*
*******************************************************************************/

#ifndef _POSIX_

#define _WINMAIN_
#include "crt0.c"

#endif  /* ndef _POSIX_ */
