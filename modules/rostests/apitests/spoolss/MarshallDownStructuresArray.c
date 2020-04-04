/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for MarshallDownStructuresArray
 * COPYRIGHT:   Copyright 2018 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#include <ndk/rtlfuncs.h>

#include <spoolss.h>
#include <marshalling/marshalling.h>
#include <marshalling/ports.h>

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

START_TEST(MarshallDownStructuresArray)
{
    const DWORD cElements = 2;
    const DWORD dwPortInfo2Offsets[] = {
        FIELD_OFFSET(PORT_INFO_2W, pPortName),
        FIELD_OFFSET(PORT_INFO_2W, pMonitorName),
        FIELD_OFFSET(PORT_INFO_2W, pDescription),
        MAXDWORD
    };

    PPORT_INFO_2W pPortInfo2;
    PPORT_INFO_2W pPortInfo2Copy;
    PPORT_INFO_2W pPortInfo2Test;
    PBYTE pPortInfoEnd;
    PCWSTR pwszStrings[] = { L"PortName", L"MonitorName", L"Description" };
    SIZE_T cbPortInfo2Size = cElements * (sizeof(PORT_INFO_2W) + (wcslen(pwszStrings[0]) + 1 + wcslen(pwszStrings[1]) + 1 + wcslen(pwszStrings[2]) + 1) * sizeof(WCHAR));
    DWORD fPortType = 1337;
    DWORD Reserved = 42;

    // Setting cElements to zero should yield success.
    SetLastError(0xDEADBEEF);
    ok(MarshallDownStructuresArray(NULL, 0, NULL, 0, FALSE), "MarshallDownStructuresArray returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "GetLastError returns %lu!\n", GetLastError());

    // Setting cElements non-zero should fail with ERROR_INVALID_PARAMETER.
    SetLastError(0xDEADBEEF);
    ok(!MarshallDownStructuresArray(NULL, 1, NULL, 0, FALSE), "MarshallDownStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError returns %lu!\n", GetLastError());

    // This is triggered by both pStructuresArray and pInfo.
    SetLastError(0xDEADBEEF);
    ok(!MarshallDownStructuresArray(INVALID_POINTER, 1, NULL, 0, FALSE), "MarshallDownStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError returns %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(!MarshallDownStructuresArray(NULL, 1, (const MARSHALLING_INFO*)INVALID_POINTER, 0, FALSE), "MarshallDownStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError returns %lu!\n", GetLastError());

    // Now create two PORT_INFO_2W structures.
    pPortInfo2 = (PPORT_INFO_2W)HeapAlloc(GetProcessHeap(), 0, cbPortInfo2Size);
    pPortInfoEnd = (PBYTE)pPortInfo2 + cbPortInfo2Size;

    (&pPortInfo2[0])->fPortType = fPortType;
    (&pPortInfo2[0])->Reserved = Reserved;
    pPortInfoEnd = PackStrings(pwszStrings, (PBYTE)(&pPortInfo2[0]), dwPortInfo2Offsets, pPortInfoEnd);

    (&pPortInfo2[1])->fPortType = fPortType + 1;
    (&pPortInfo2[1])->Reserved = Reserved + 1;
    pPortInfoEnd = PackStrings(pwszStrings, (PBYTE)(&pPortInfo2[1]), dwPortInfo2Offsets, pPortInfoEnd);

    // Create a backup.
    pPortInfo2Copy = (PPORT_INFO_2W)HeapAlloc(GetProcessHeap(), 0, cbPortInfo2Size);
    CopyMemory(pPortInfo2Copy, pPortInfo2, cbPortInfo2Size);

    // Marshall them down.
    SetLastError(0xDEADBEEF);
    ok(MarshallDownStructuresArray(pPortInfo2, cElements, pPortInfoMarshalling[2]->pInfo, pPortInfoMarshalling[2]->cbStructureSize, TRUE), "MarshallDownStructuresArray returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "GetLastError returns %lu!\n", GetLastError());

    // DWORD values should be unchanged.
    ok((&pPortInfo2[0])->fPortType == fPortType, "fPortType is %lu!\n", (&pPortInfo2[0])->fPortType);
    ok((&pPortInfo2[0])->Reserved == Reserved, "Reserved is %lu!\n", (&pPortInfo2[0])->Reserved);
    ok((&pPortInfo2[1])->fPortType == fPortType + 1, "fPortType is %lu!\n", (&pPortInfo2[1])->fPortType);
    ok((&pPortInfo2[1])->Reserved == Reserved + 1, "Reserved is %lu!\n", (&pPortInfo2[1])->Reserved);

    // Pointers should now contain relative offsets.
    ok((ULONG_PTR)(&pPortInfo2[0])->pPortName == ((ULONG_PTR)(&pPortInfo2Copy[0])->pPortName - (ULONG_PTR)(&pPortInfo2[0])), "pPortName is %p!\n", (&pPortInfo2[0])->pPortName);
    ok((ULONG_PTR)(&pPortInfo2[0])->pMonitorName == ((ULONG_PTR)(&pPortInfo2Copy[0])->pMonitorName - (ULONG_PTR)(&pPortInfo2[0])), "pMonitorName is %p!\n", (&pPortInfo2[0])->pMonitorName);
    ok((ULONG_PTR)(&pPortInfo2[0])->pDescription == ((ULONG_PTR)(&pPortInfo2Copy[0])->pDescription - (ULONG_PTR)(&pPortInfo2[0])), "pDescription is %p!\n", (&pPortInfo2[0])->pDescription);
    ok((ULONG_PTR)(&pPortInfo2[1])->pPortName == ((ULONG_PTR)(&pPortInfo2Copy[1])->pPortName - (ULONG_PTR)(&pPortInfo2[1])), "pPortName is %p!\n", (&pPortInfo2[1])->pPortName);
    ok((ULONG_PTR)(&pPortInfo2[1])->pMonitorName == ((ULONG_PTR)(&pPortInfo2Copy[1])->pMonitorName - (ULONG_PTR)(&pPortInfo2[1])), "pMonitorName is %p!\n", (&pPortInfo2[1])->pMonitorName);
    ok((ULONG_PTR)(&pPortInfo2[1])->pDescription == ((ULONG_PTR)(&pPortInfo2Copy[1])->pDescription - (ULONG_PTR)(&pPortInfo2[1])), "pDescription is %p!\n", (&pPortInfo2[1])->pDescription);

    // Marshall them up again.
    // We need a backup of the marshalled down array to experiment with MarshallUpStructuresArray.
    pPortInfo2Test = (PPORT_INFO_2W)HeapAlloc(GetProcessHeap(), 0, cbPortInfo2Size);
    CopyMemory(pPortInfo2Test, pPortInfo2, cbPortInfo2Size);

    // Due to the implementation of PackStrings, (&pPortInfo2[0])->pPortName contains the highest offset.
    // Show that MarshallUpStructuresArray checks the offsets and bails out with ERROR_INVALID_DATA if cbSize <= highest offset.
    SetLastError(0xDEADBEEF);
    ok(!MarshallUpStructuresArray((DWORD_PTR)(&pPortInfo2[0])->pPortName, pPortInfo2Test, cElements, pPortInfoMarshalling[2]->pInfo, pPortInfoMarshalling[2]->cbStructureSize, TRUE), "MarshallUpStructuresArray returns TRUE!\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "GetLastError returns %lu!\n", GetLastError());

    // It works with cbSize > highest offset.
    // In real world cases, we would use cbPortInfo2Size for cbSize.
    SetLastError(0xDEADBEEF);
    ok(MarshallUpStructuresArray((DWORD_PTR)(&pPortInfo2[0])->pPortName + 1, pPortInfo2, cElements, pPortInfoMarshalling[2]->pInfo, pPortInfoMarshalling[2]->cbStructureSize, TRUE), "MarshallUpStructuresArray returns FALSE!\n");
    ok(GetLastError() == 0xDEADBEEF, "GetLastError returns %lu!\n", GetLastError());

    // pPortInfo2 should now be identical to the copy again.
    ok(RtlEqualMemory(pPortInfo2, pPortInfo2Copy, cbPortInfo2Size), "pPortInfo2 and pPortInfo2Copy are not equal after marshalling down and up!\n");

    // Free all memory.
    HeapFree(GetProcessHeap(), 0, pPortInfo2);
    HeapFree(GetProcessHeap(), 0, pPortInfo2Copy);
    HeapFree(GetProcessHeap(), 0, pPortInfo2Test);
}
