/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    serial.h

Abstract:

    HEADER for serial.c

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
void SendString(LPSTR s);
void SetupSerial(ULONG port,ULONG baudrate);

BOOLEAN ConsoleInitSerial(void);
void ConsoleShutdownSerial(void); 

