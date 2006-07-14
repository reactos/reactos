/*
 * Kernel Mode regression Test
 * Driver Core
 *
 * Copyright 2004 Filip Navara <xnavara@volny.cz>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include "kmtest.h"

LONG successes;
LONG failures;
tls_data glob_data;

/* PRIVATE FUNCTIONS ***********************************************************/
VOID
StartTest()
{
    successes = 0;
    failures = 0;
}

VOID
FinishTest(LPSTR TestName)
{
    DbgPrint("Test %s finished with %d succeses and %d failures\n", TestName, successes, failures);
}

void kmtest_set_location(const char* file, int line)
{
    glob_data.current_file=strrchr(file,'/');
    if (glob_data.current_file==NULL)
        glob_data.current_file=strrchr(file,'\\');
    if (glob_data.current_file==NULL)
        glob_data.current_file=file;
    else
        glob_data.current_file++;
    glob_data.current_line=line;
}

/*
 * Checks condition.
 * Parameters:
 *   - condition - condition to check;
 *   - msg test description;
 *   - file - test application source code file name of the check
 *   - line - test application source code file line number of the check
 * Return:
 *   0 if condition does not have the expected value, 1 otherwise
 */
int kmtest_ok(int condition, const char *msg, ... )
{
    va_list valist;

    if (!condition)
    {
        if (msg[0])
        {
            char string[1024];
            va_start(valist, msg);
            vsprintf(string, msg, valist);
            DbgPrint( "%s:%d: Test failed: %s\n",
                glob_data.current_file, glob_data.current_line, string );
            va_end(valist);
        }
        else
        {
            DbgPrint( "%s:%d: Test failed\n",
                glob_data.current_file, glob_data.current_line );
        }
        InterlockedIncrement(&failures);
        return 0;
    }
    else
    {/*
        if (report_success)
            fprintf( stdout, "%s:%d: Test succeeded\n",
            glob_data.current_file, glob_data.current_line);*/
        InterlockedIncrement(&successes);
    }
    return 1;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * Test Declarations
 */
VOID FASTCALL NtoskrnlIoMdlTest();
VOID FASTCALL NtoskrnlIoDeviceInterface();
VOID FASTCALL NtoskrnlObTest();
VOID FASTCALL NtoskrnlExecutiveTests();

/*
 * DriverEntry
 */
NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    DbgPrint("\n===============================================\nKernel Mode Regression Test driver starting...\n");
    NtoskrnlExecutiveTests();
    NtoskrnlIoDeviceInterface();
    NtoskrnlIoMdlTest();
    NtoskrnlObTest();

    return STATUS_UNSUCCESSFUL;
}
