/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    pgflt.h

Abstract:

    HEADER for pgflt.c

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
void InstallIntEHook(void);
void DeInstallIntEHook(void);

extern ULONG error_code;
