/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    debug.h

Abstract:

    HEADER for debug.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
#ifdef DEBUG

#define ENTER_FUNC() DPRINT((0,"enter "__FUNCTION__"()\n"))

#define LEAVE_FUNC() DPRINT((0,"leave "__FUNCTION__"()\n"))

VOID Pice_dprintf(ULONG DebugLevel, PCHAR DebugMessage, ...); 
#define DPRINT(arg) Pice_dprintf arg 

#else // DEBUG

#define ENTER_FUNC()
#define LEAVE_FUNC()

#define DPRINT(arg)

#endif // DEBUG
