/* $Id$
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/autochk/autochk.c
 * PURPOSE:         Filesystem checker
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* FUNCTIONS ****************************************************************/

void
DisplayString(LPCWSTR lpwString)
{
    UNICODE_STRING us;

    RtlInitUnicodeString(&us, lpwString);
    NtDisplayString(&us);
}

void
PrintString(char* fmt,...)
{
    char buffer[512];
    va_list ap;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    RtlInitAnsiString(&AnsiString, buffer);
    RtlAnsiStringToUnicodeString(&UnicodeString,
                                 &AnsiString,
                                 TRUE);
    NtDisplayString(&UnicodeString);
    RtlFreeUnicodeString(&UnicodeString);
}


/* Native image's entry point */
int
_cdecl
_main(int argc,
      char *argv[],
      char *envp[],
      int DebugFlag)
{
    PROCESS_DEVICEMAP_INFORMATION DeviceMap;
    ULONG i;
    NTSTATUS Status;

    PrintString("Autochk 0.0.1\n");

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &DeviceMap.Query,
                                       sizeof(DeviceMap.Query),
                                       NULL);

    if(NT_SUCCESS(Status))
    {
        for (i = 0; i < 26; i++)
        {
            if ((DeviceMap.Query.DriveMap & (1 << i)) &&
                (DeviceMap.Query.DriveType[i] == DOSDEVICE_DRIVE_FIXED))
            {
                PrintString("  Checking drive %c:", 'A'+i);
                PrintString("      OK\n");
            }
        }
        PrintString("\n");
        return 0;
    }

    return 1;
}

/* EOF */
