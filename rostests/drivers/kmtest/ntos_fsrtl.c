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
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NtoskrnlFsRtlTest(HANDLE KeyHandle)
{
    FsRtlIsNameInExpressionTest();

    FinishTest(KeyHandle, L"FsRtlTest");
}
