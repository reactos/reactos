/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    patch.h

Abstract:

    HEADER for patch.c

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
BOOLEAN PatchKeyboardDriver(ULONG AddrOfKbdEvent,ULONG AddrOfScancode);
void RestoreKeyboardDriver(void);

extern UCHAR ucBreakKey;
