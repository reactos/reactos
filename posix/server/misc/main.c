/* $Id: main.c,v 1.1 2002/04/10 21:30:22 ea Exp $
 *
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/misc/main.c 
 * DESCRIPTION: POSIX+ server main.
 * DATE       : 2001-05-05
 * AUTHOR     : Emanuele Aliberti <eal@users.sf.net>
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 
 * 	19990605 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 *      20020323 (Emanuele Aliberti)
 *              Converted to Win32 for testing it using NT LPC.
 */
#include <windows.h>
#include <reactos/buildno.h>
#include <psxss.h>

/*** EXTERNAL ********************************************************/

NTSTATUS STDCALL
PsxServerInitialization (
    IN ULONG ArgumentCount,
    IN PWSTR *ArgumentArray
    );

/*** ENTRY POINT *****************************************************/

#ifdef __PSXSS_ON_W32__ // W32 PSXSS.EXE
int main (int argc, char * argv[])
{
    INT c;

    debug_print(L"POSIX+ Subsystem for ReactOS "KERNEL_RELEASE_STR);

    if (STATUS_SUCCESS == PsxServerInitialization(0,NULL))
    {
        debug_print(L"PSXSS: server active");
        while (TRUE)
        {
            c = getch();
            if (c == 1) break;
        }
    }
    else
    {
        debug_print(L"PSXSS: Subsystem initialization failed.\n");
    }
    return 0;
}
#else /* Native PSXSS.EXE */
VOID NtProcessStartup (PPEB Peb)
{
    UNICODE_STRING Banner;

    RtlInitUnicodeString (& Banner, L"POSIX+ Subsystem for ReactOS "KERNEL_RELEASE_STR);
    NtDisplayString(& Banner);

    if (STATUS_SUCCESS == PsxServerInitialization(0,NULL))
    {
        DbgPrint("PSXSS: server active\n");
        /* TODO */
    }
    else
    {
        DbgPrint("PSXSS: Subsystem initialization failed.\n");
    }
    NtTerminateProcess (NtCurrentProcess(), 0);
}
#endif
/* EOF */
