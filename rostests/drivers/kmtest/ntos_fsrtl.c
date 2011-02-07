/*
 * FsRtl Test
 *
 * Copyright 2010 Pierre Schweitzer <pierre.schweitzer@reactos.org>
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

#include "kmtest.h"
#include <ntifs.h>

#define NDEBUG
#include "debug.h"

/* PRIVATE FUNCTIONS **********************************************************/

VOID FsRtlIsNameInExpressionTest()
{
    UNICODE_STRING Expression, Name;

    RtlInitUnicodeString(&Expression, L"*");
    RtlInitUnicodeString(&Name, L"");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"ntdll.dll");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"~1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"ntdll.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"smss.exe");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"~1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"ntdll.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"NTDLL.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"nt??krnl.???");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"he*o");
    RtlInitUnicodeString(&Name, L"hello");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"helo");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"hella");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"he*");
    RtlInitUnicodeString(&Name, L"hello");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"helo");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"hella");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"*.cpl");
    RtlInitUnicodeString(&Name, L"kdcom.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"bootvid.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L".");
    RtlInitUnicodeString(&Name, L"NTDLL.DLL");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"F0_*.*");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"SETUP.EXE");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"F0_001");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"*.TTF");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"SETUP.INI");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"*");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"SETUP.INI");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"\"ntoskrnl.exe");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrnl\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"ntoskrn\".exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrn\"\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrnl.\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrnl.exe\"");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe.");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"*.c.d");
    RtlInitUnicodeString(&Name, L"a.b.c.d");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
}

VOID FsRtlIsDbcsInExpressionTest()
{
    ANSI_STRING Expression, Name;

    RtlInitAnsiString(&Expression, "*");
    RtlInitAnsiString(&Name, "");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "ntdll.dll");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "~1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "ntdll.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "smss.exe");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "~1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "ntdll.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "NTDLL.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "nt??krnl.???");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "he*o");
    RtlInitAnsiString(&Name, "hello");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "helo");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "hella");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "he*");
    RtlInitAnsiString(&Name, "hello");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "helo");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "hella");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "*.cpl");
    RtlInitAnsiString(&Name, "kdcom.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "bootvid.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, ".");
    RtlInitAnsiString(&Name, "NTDLL.DLL");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "F0_*.*");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "SETUP.EXE");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "F0_001");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "*.TTF");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "SETUP.INI");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "*");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "SETUP.INI");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "\"ntoskrnl.exe");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrnl\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "ntoskrn\".exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrn\"\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrnl.\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrnl.exe\"");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "ntoskrnl.exe.");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "*.c.d");
    RtlInitAnsiString(&Name, "a.b.c.d");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NtoskrnlFsRtlTest(HANDLE KeyHandle)
{
    FsRtlIsNameInExpressionTest();
    FsRtlIsDbcsInExpressionTest();

    FinishTest(KeyHandle, L"FsRtlTest");
}
