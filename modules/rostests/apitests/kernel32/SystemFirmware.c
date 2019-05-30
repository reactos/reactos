/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for System Firmware functions
 * COPYRIGHT:   Copyright 2018 Stanislav Motylkov
 *              Copyright 2018 Mark Jansen
 */

#include "precomp.h"

static UINT (WINAPI * pEnumSystemFirmwareTables)(DWORD, PVOID, DWORD);
static UINT (WINAPI * pGetSystemFirmwareTable)(DWORD, DWORD, PVOID, DWORD);

typedef struct ENTRY
{
    DWORD Signature;
    DWORD ErrInsuff;
    DWORD ErrSuccess;
} ENTRY;

static UINT
CallNt(IN DWORD FirmwareTableProviderSignature,
       IN DWORD FirmwareTableID,
       OUT PVOID pFirmwareTableBuffer,
       IN DWORD BufferSize,
       IN SYSTEM_FIRMWARE_TABLE_ACTION Action)
{
    SYSTEM_FIRMWARE_TABLE_INFORMATION* SysFirmwareInfo;
    ULONG Result = 0, ReturnedSize;
    ULONG TotalSize = BufferSize + sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);
    NTSTATUS Status;

    SysFirmwareInfo = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, TotalSize);
    if (!SysFirmwareInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    _SEH2_TRY
    {
        SysFirmwareInfo->ProviderSignature = FirmwareTableProviderSignature;
        SysFirmwareInfo->TableID = FirmwareTableID;
        SysFirmwareInfo->Action = Action;
        SysFirmwareInfo->TableBufferLength = BufferSize;

        Status = NtQuerySystemInformation(SystemFirmwareTableInformation, SysFirmwareInfo, TotalSize, &ReturnedSize);

        if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
            Result = SysFirmwareInfo->TableBufferLength;

        if (NT_SUCCESS(Status) && pFirmwareTableBuffer)
        {
            memcpy(pFirmwareTableBuffer, SysFirmwareInfo->TableBuffer, SysFirmwareInfo->TableBufferLength);
        }
    }
    _SEH2_FINALLY
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, SysFirmwareInfo);
    }
    _SEH2_END;

    SetLastError(RtlNtStatusToDosError(Status));
    return Result;
}

UINT
WINAPI
fEnumSystemFirmwareTables(IN DWORD FirmwareTableProviderSignature,
                          OUT PVOID pFirmwareTableBuffer,
                          IN DWORD BufferSize)
{
    return CallNt(FirmwareTableProviderSignature, 0, pFirmwareTableBuffer, BufferSize, SystemFirmwareTable_Enumerate);
}

UINT
WINAPI
fGetSystemFirmwareTable(IN DWORD FirmwareTableProviderSignature,
                        IN DWORD FirmwareTableID,
                        OUT PVOID pFirmwareTableBuffer,
                        IN DWORD BufferSize)
{
    return CallNt(FirmwareTableProviderSignature, FirmwareTableID, pFirmwareTableBuffer, BufferSize, SystemFirmwareTable_Get);
}

static
VOID
test_EnumBuffer(
    DWORD Signature,
    PVOID Buffer,
    DWORD dwSize,
    UINT * pTableCount,
    DWORD * pFirstTableID,
    DWORD ErrInsuff,
    DWORD ErrSuccess
)
{
    DWORD dwError;
    DWORD dwBufferSize;
    DWORD dwException;
    UINT uResultSize;

    dwException = Buffer && IsBadWritePtr(Buffer, dwSize) ? STATUS_ACCESS_VIOLATION : STATUS_SUCCESS;

    // Test size = 0
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        FillMemory(Buffer, dwSize, 0xFF);
    }
    SetLastError(0xbeeffeed);
    dwError = GetLastError();
    dwBufferSize = 0;
    uResultSize = 0;
    StartSeh()
        uResultSize = pEnumSystemFirmwareTables(Signature, Buffer, dwBufferSize);
        dwError = GetLastError();
    EndSeh(STATUS_SUCCESS);

    if (uResultSize > 0)
    {
        ok(dwError == ErrInsuff,
           "GetLastError() returned %ld, expected %ld\n",
           dwError, ErrInsuff);
    }
    else
    {
        ok(dwError == ErrSuccess,
           "GetLastError() returned %ld, expected %ld\n",
           dwError, ErrSuccess);
    }
    if (ErrSuccess == ERROR_SUCCESS)
    {
        ok(uResultSize % sizeof(DWORD) == 0,
           "uResultSize is %u, expected %% sizeof(DWORD)\n",
           uResultSize);
    }
    else
    {
        ok(uResultSize == 0,
           "uResultSize is %u, expected == 0\n",
           uResultSize);
    }
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        ok(*(BYTE *)Buffer == 0xFF,
           "Buffer should be clean at offset 0, got %x\n",
           *(BYTE *)Buffer);
    }

    // Test size = 2
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        FillMemory(Buffer, dwSize, 0xFF);
    }
    SetLastError(0xbeeffeed);
    dwError = GetLastError();
    dwBufferSize = 2;
    uResultSize = 0;
    StartSeh()
        uResultSize = pEnumSystemFirmwareTables(Signature, Buffer, dwBufferSize);
        dwError = GetLastError();
    EndSeh(STATUS_SUCCESS);

    if (uResultSize > 0)
    {
        ok(dwError == ErrInsuff,
           "GetLastError() returned %ld, expected %ld\n",
           dwError, ErrInsuff);
    }
    else
    {
        ok(dwError == ErrSuccess,
           "GetLastError() returned %ld, expected %ld\n",
           dwError, ErrSuccess);
    }
    if (ErrSuccess == ERROR_SUCCESS)
    {
        ok(uResultSize % sizeof(DWORD) == 0,
           "uResultSize is %u, expected %% sizeof(DWORD)\n",
           uResultSize);
    }
    else
    {
        ok(uResultSize == 0,
           "uResultSize is %u, expected == 0\n",
           uResultSize);
    }
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        ok(*(WORD *)Buffer == 0xFFFF,
           "Buffer should be clean at offset 0, got %x\n",
           *(WORD *)Buffer);
    }

    // Test full size
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        FillMemory(Buffer, dwSize, 0xFF);
    }
    if (uResultSize > 0)
    {
        SetLastError(0xbeeffeed);
        dwError = GetLastError();
        dwBufferSize = uResultSize;
        uResultSize = 0;
        StartSeh()
            uResultSize = pEnumSystemFirmwareTables(Signature, Buffer, dwBufferSize);
            dwError = GetLastError();
        EndSeh(ErrSuccess == ERROR_SUCCESS ? dwException : STATUS_SUCCESS);
        // Windows 7: does not throw exception here

        if (dwException == STATUS_SUCCESS || ErrSuccess == ERROR_INVALID_FUNCTION)
        {
            ok(dwError == ErrSuccess,
               "GetLastError() returned %ld, expected %ld\n",
               dwError, ErrSuccess);
            if (ErrSuccess == ERROR_SUCCESS)
            {
                ok(uResultSize == dwBufferSize,
                   "uResultSize is not equal dwBufferSize, expected %ld\n",
                   dwBufferSize);
            }
            else
            {
                ok(uResultSize == 0,
                   "uResultSize is %u, expected == 0\n",
                   uResultSize);
            }
        }
        else
        {
            // Windows 7: returns ERROR_NOACCESS here
            ok(dwError == 0xbeeffeed,
               "GetLastError() returned %ld, expected %u\n",
               dwError, 0xbeeffeed);
            // Windows 7: returns correct size here
            ok(uResultSize == 0,
               "uResultSize is %u, expected == 0\n",
               uResultSize);
        }
    }

    if (pTableCount && pFirstTableID)
    {
        if (uResultSize > 0)
        {
            if (Signature == 'RSMB')
            {
                // Raw SMBIOS have only one table with ID 0
                ok(*(DWORD *)Buffer == 0,
                   "Buffer should be filled at offset 0, got %lx\n",
                   *(DWORD *)Buffer);
            }
            else
            {
                // In other cases ID can be different
                if (ErrSuccess == ERROR_SUCCESS)
                {
                    ok(*(DWORD *)Buffer != 0xFFFFFFFF,
                       "Buffer should be filled at offset 0\n");
                }
                else
                {
                    ok(*(DWORD *)Buffer == 0xFFFFFFFF,
                       "Buffer should be clean at offset 0\n");
                }
            }
        }
        *pTableCount = uResultSize / sizeof(DWORD);
        *pFirstTableID = *(DWORD *)Buffer;
    }
}

static
VOID
test_GetBuffer(
    DWORD Signature,
    DWORD TableID,
    PVOID Buffer,
    DWORD dwSize,
    BOOL TestFakeID,
    DWORD ErrInsuff,
    DWORD ErrSuccess
)
{
    DWORD dwError;
    DWORD dwBufferSize;
    DWORD dwException;
    DWORD dwErrCase;
    UINT uResultSize;

    dwException = Buffer && IsBadWritePtr(Buffer, dwSize) ? STATUS_ACCESS_VIOLATION : STATUS_SUCCESS;
    switch (Signature)
    {
        case 'ACPI':
        {
            dwErrCase = ERROR_NOT_FOUND;
            break;
        }
        case 'FIRM':
        {
            dwErrCase = ERROR_INVALID_PARAMETER;
            break;
        }
        default:
        {
            dwErrCase = ErrInsuff;
        }
    }

    // Test size = 0
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        FillMemory(Buffer, dwSize, 0xFF);
    }
    SetLastError(0xbeeffeed);
    dwError = GetLastError();
    dwBufferSize = 0;
    uResultSize = 0;
    StartSeh()
        uResultSize = pGetSystemFirmwareTable(Signature, TableID, Buffer, dwBufferSize);
        dwError = GetLastError();
    EndSeh(STATUS_SUCCESS);

    ok(dwError == (TestFakeID ? dwErrCase : ErrInsuff),
       "GetLastError() returned %ld, expected %ld\n",
       dwError, (TestFakeID ? dwErrCase : ErrInsuff));
    if (ErrSuccess == ERROR_SUCCESS && (!TestFakeID || dwErrCase == ErrInsuff))
    {
        ok(uResultSize > 0,
           "uResultSize is %u, expected > 0\n",
           uResultSize);
    }
    else
    {
        ok(uResultSize == 0,
           "uResultSize is %u, expected == 0\n",
           uResultSize);
    }
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        ok(*(BYTE *)Buffer == 0xFF,
           "Buffer should be clean at offset 0, got %x\n",
           *(BYTE *)Buffer);
    }

    // Test size = 2
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        FillMemory(Buffer, dwSize, 0xFF);
    }
    SetLastError(0xbeeffeed);
    dwError = GetLastError();
    dwBufferSize = 2;
    uResultSize = 0;
    StartSeh()
        uResultSize = pGetSystemFirmwareTable(Signature, TableID, Buffer, dwBufferSize);
        dwError = GetLastError();
    EndSeh(STATUS_SUCCESS);

    ok(dwError == (TestFakeID ? dwErrCase : ErrInsuff),
       "GetLastError() returned %ld, expected %ld\n",
       dwError, (TestFakeID ? dwErrCase : ErrInsuff));
    if (ErrSuccess == ERROR_SUCCESS && (!TestFakeID || dwErrCase == ErrInsuff))
    {
        ok(uResultSize > 0,
           "uResultSize is %u, expected > 0\n",
           uResultSize);
    }
    else
    {
        ok(uResultSize == 0,
           "uResultSize is %u, expected == 0\n",
           uResultSize);
    }
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        ok(*(WORD *)Buffer == 0xFFFF,
           "Buffer should be clean at offset 0, got %x\n",
           *(WORD *)Buffer);
    }

    // Test full size
    if (Buffer && dwException == STATUS_SUCCESS)
    {
        FillMemory(Buffer, dwSize, 0xFF);
    }
    if (uResultSize == 0)
    {
        return;
    }
    SetLastError(0xbeeffeed);
    dwError = GetLastError();
    dwBufferSize = uResultSize;
    uResultSize = 0;
    StartSeh()
        uResultSize = pGetSystemFirmwareTable(Signature, TableID, Buffer, dwBufferSize);
        dwError = GetLastError();
    EndSeh(ErrSuccess == ERROR_SUCCESS ? dwException : STATUS_SUCCESS);
    // Windows 7: does not throw exception here

    if (dwException == STATUS_SUCCESS || ErrSuccess == ERROR_INVALID_FUNCTION)
    {
        ok(dwError == ErrSuccess,
           "GetLastError() returned %ld, expected %ld\n",
           dwError, ErrSuccess);
        if (ErrSuccess == ERROR_SUCCESS)
        {
            ok(uResultSize == dwBufferSize,
               "uResultSize is not equal dwBufferSize, expected %ld\n",
               dwBufferSize);
        }
        else
        {
            ok(uResultSize == 0,
               "uResultSize is %u, expected == 0\n",
               uResultSize);
        }
    }
    else
    {
        // Windows 7: returns ERROR_NOACCESS here
        ok(dwError == 0xbeeffeed,
           "GetLastError() returned %ld, expected %u\n",
           dwError, 0xbeeffeed);
        // Windows 7: returns correct size here
        ok(uResultSize == 0,
           "uResultSize is %u, expected == 0\n",
           uResultSize);
    }

    if (Buffer && dwException == STATUS_SUCCESS)
    {
        if (ErrSuccess == ERROR_SUCCESS)
        {
            ok(*(DWORD *)Buffer != 0xFFFFFFFF,
               "Buffer should be filled at offset 0\n");
        }
        else
        {
            ok(*(DWORD *)Buffer == 0xFFFFFFFF,
               "Buffer should be clean at offset 0\n");
        }
    }
}

static
VOID
test_Functions()
{
    static const ENTRY Entries[] =
    {
        { 'ACPI', ERROR_INSUFFICIENT_BUFFER, ERROR_SUCCESS },
        { 'FIRM', ERROR_INSUFFICIENT_BUFFER, ERROR_SUCCESS },
        { 'RSMB', ERROR_INSUFFICIENT_BUFFER, ERROR_SUCCESS },
        /* This entry should be last */
        { 0xDEAD, ERROR_INVALID_FUNCTION, ERROR_INVALID_FUNCTION },
    };
    CHAR Buffer[262144]; // 256 KiB should be enough
    CHAR Sign[sizeof(DWORD) + 1];
    UINT TableCount[_countof(Entries)];
    DWORD FirstTableID[_countof(Entries)];
    int i;

    // Test EnumSystemFirmwareTables
    for (i = 0; i < _countof(Entries); i++)
    {
        // Test with NULL buffer
        test_EnumBuffer(Entries[i].Signature, NULL, sizeof(Buffer), NULL, NULL,
                        Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        // Test with wrong buffer
        test_EnumBuffer(Entries[i].Signature, (PVOID *)(LONG_PTR)0xbeeffeed, sizeof(Buffer), NULL, NULL,
                        Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        // Test with correct buffer
        test_EnumBuffer(Entries[i].Signature, &Buffer, sizeof(Buffer), &TableCount[i], &FirstTableID[i],
                        Entries[i].ErrInsuff, Entries[i].ErrSuccess);
    }

    // Test GetSystemFirmwareTable
    for (i = 0; i < _countof(Entries); i++)
    {
        // Test with fake ID and NULL buffer
        test_GetBuffer(Entries[i].Signature, 0xbeeffeed, NULL, sizeof(Buffer),
                       TRUE, Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        // Test with fake ID and wrong buffer
        test_GetBuffer(Entries[i].Signature, 0xbeeffeed, (PVOID *)(LONG_PTR)0xbeeffeed, sizeof(Buffer),
                       TRUE, Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        // Test with fake ID and correct buffer
        test_GetBuffer(Entries[i].Signature, 0xbeeffeed, &Buffer, sizeof(Buffer),
                       TRUE, Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        if (TableCount[i] == 0)
        {
            if (i < _countof(Entries) - 1)
            {
                ZeroMemory(&Sign, sizeof(Sign));
                *(DWORD *)&Sign = _byteswap_ulong(Entries[i].Signature);
                skip("No tables for %s found. Skipping\n",
                     Sign);
            }
            continue;
        }
        // Test with correct ID and NULL buffer
        test_GetBuffer(Entries[i].Signature, FirstTableID[i], NULL, sizeof(Buffer),
                       FALSE, Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        // Test with correct ID and wrong buffer
        test_GetBuffer(Entries[i].Signature, FirstTableID[i], (PVOID *)(LONG_PTR)0xbeeffeed, sizeof(Buffer),
                       FALSE, Entries[i].ErrInsuff, Entries[i].ErrSuccess);
        // Test with correct ID and correct buffer
        test_GetBuffer(Entries[i].Signature, FirstTableID[i], &Buffer, sizeof(Buffer),
                       FALSE, Entries[i].ErrInsuff, Entries[i].ErrSuccess);
    }
}

START_TEST(SystemFirmware)
{
    HANDLE hKernel;

    hKernel = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel)
    {
        skip("kernel32.dll module not found. Can't proceed\n");
        return;
    }

    pEnumSystemFirmwareTables = (void *)fEnumSystemFirmwareTables;
    pGetSystemFirmwareTable = (void *)fGetSystemFirmwareTable;

    test_Functions();

    pEnumSystemFirmwareTables = (void *)GetProcAddress(hKernel, "EnumSystemFirmwareTables");
    pGetSystemFirmwareTable = (void *)GetProcAddress(hKernel, "GetSystemFirmwareTable");

    if (!pEnumSystemFirmwareTables)
    {
        skip("EnumSystemFirmwareTables not found. Can't proceed\n");
        return;
    }
    if (!pGetSystemFirmwareTable)
    {
        skip("GetSystemFirmwareTable not found. Can't proceed\n");
        return;
    }
    test_Functions();
}
