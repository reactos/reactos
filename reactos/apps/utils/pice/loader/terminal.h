/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    termínal.h

Abstract:
	
    HEADER for terminal.c

Environment:

    User mode only

Author:

    Klaus P. Gerlicher

Revision History:

    23-Jan-2001:	created
    
Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
BOOLEAN SetupSerial(ULONG port,ULONG baudrate);
void CloseSerial(void);


void DebuggerShell(void);
