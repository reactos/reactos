/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for GetComputerNameEx
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/rtltypes.h>

static
VOID
TestGetComputerNameEx(
    _In_ COMPUTER_NAME_FORMAT NameType)
{
    WCHAR Reference[128];
    DWORD ReferenceLen;
    WCHAR BufferW[128];
    CHAR BufferA[128];
    BOOL Ret;
    DWORD Size;
    DWORD Error;
    ULONG i;

    Size = RTL_NUMBER_OF(Reference);
    Ret = GetComputerNameExW(NameType, Reference, &Size);
    ok(Ret == TRUE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    if (!Ret)
    {
        skip("[%d] Failed to get reference string\n", NameType);
        return;
    }
    trace("[%d] Reference is %ls\n", NameType, Reference);
    ReferenceLen = wcslen(Reference);
    ok(ReferenceLen < RTL_NUMBER_OF(Reference),
       "[%d] Unexpected ReferenceLen %lu\n", NameType, ReferenceLen);
    if (NameType != ComputerNameDnsDomain && NameType != ComputerNamePhysicalDnsDomain)
    {
        ok(ReferenceLen != 0, "[%d] Unexpected ReferenceLen %lu\n", NameType, ReferenceLen);
    }
    ok(Size == ReferenceLen, "[%d] Size is %lu, expected %lu\n", NameType, Size, ReferenceLen);

    /* NULL buffer, NULL size */
    StartSeh()
        Ret = GetComputerNameExW(NameType, NULL, NULL);
        Error = GetLastError();
        ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
        ok(Error == ERROR_INVALID_PARAMETER, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Ret = GetComputerNameExA(NameType, NULL, NULL);
        Error = GetLastError();
        ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
        ok(Error == ERROR_INVALID_PARAMETER, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    EndSeh(STATUS_SUCCESS);

    /* NULL buffer, nonzero size */
    Size = 0x55555555;
    Ret = GetComputerNameExW(NameType, NULL, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    ok(Size == 0x55555555, "[%d] Got Size %lu\n", NameType, Size);

    Size = 0x55555555;
    Ret = GetComputerNameExA(NameType, NULL, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExA returned %d\n", NameType, Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "[%d] GetComputerNameExA returned error %lu\n", NameType, Error);
    ok(Size == 0x55555555, "[%d] Got Size %lu\n", NameType, Size);

    /* non-NULL buffer, NULL size */
    RtlFillMemory(BufferW, sizeof(BufferW), 0x55);
    Ret = GetComputerNameExW(NameType, BufferW, NULL);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    ok(BufferW[0] == 0x5555, "[%d] BufferW[0] = 0x%x\n", NameType, BufferW[0]);

    RtlFillMemory(BufferA, sizeof(BufferA), 0x55);
    Ret = GetComputerNameExA(NameType, BufferA, NULL);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExA returned %d\n", NameType, Ret);
    ok(Error == ERROR_INVALID_PARAMETER, "[%d] GetComputerNameExA returned error %lu\n", NameType, Error);
    ok(BufferA[0] == 0x55, "[%d] BufferA[0] = 0x%x\n", NameType, BufferA[0]);

    /* NULL buffer, zero size */
    Size = 0;
    Ret = GetComputerNameExW(NameType, NULL, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    ok(Error == ERROR_MORE_DATA, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    ok(Size == ReferenceLen + 1, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);

    Size = 0;
    Ret = GetComputerNameExA(NameType, NULL, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExA returned %d\n", NameType, Ret);
    ok(Error == ERROR_MORE_DATA, "[%d] GetComputerNameExA returned error %lu\n", NameType, Error);
    ok(Size == ReferenceLen + 1, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);

    /* non-NULL buffer, zero size */
    RtlFillMemory(BufferW, sizeof(BufferW), 0x55);
    Size = 0;
    Ret = GetComputerNameExW(NameType, BufferW, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    ok(Error == ERROR_MORE_DATA, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    ok(Size == ReferenceLen + 1, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);
    ok(BufferW[0] == 0x5555, "[%d] BufferW[0] = 0x%x\n", NameType, BufferW[0]);

    RtlFillMemory(BufferA, sizeof(BufferA), 0x55);
    Size = 0;
    Ret = GetComputerNameExA(NameType, BufferA, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExA returned %d\n", NameType, Ret);
    ok(Error == ERROR_MORE_DATA, "[%d] GetComputerNameExA returned error %lu\n", NameType, Error);
    ok(Size == ReferenceLen + 1, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);
    ok(BufferA[0] == 0x55, "[%d] BufferA[0] = 0x%x\n", NameType, BufferA[0]);

    /* non-NULL buffer, too small size */
    RtlFillMemory(BufferW, sizeof(BufferW), 0x55);
    Size = ReferenceLen;
    Ret = GetComputerNameExW(NameType, BufferW, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    ok(Error == ERROR_MORE_DATA, "[%d] GetComputerNameExW returned error %lu\n", NameType, Error);
    ok(Size == ReferenceLen + 1, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);
    if (NameType != ComputerNameNetBIOS && NameType != ComputerNamePhysicalNetBIOS)
    {
        if (ReferenceLen == 0)
        {
            ok(BufferW[0] == 0x5555, "[%d] BufferW[0] = 0x%x\n",
               NameType, BufferW[0]);
        }
        else
        {
            ok(BufferW[0] == 0, "[%d] BufferW[0] = 0x%x\n",
               NameType, BufferW[0]);
        }
    }
    ok(BufferW[1] == 0x5555, "[%d] BufferW[1] = 0x%x\n", NameType, BufferW[1]);

    RtlFillMemory(BufferA, sizeof(BufferA), 0x55);
    Size = ReferenceLen;
    Ret = GetComputerNameExA(NameType, BufferA, &Size);
    Error = GetLastError();
    ok(Ret == FALSE, "[%d] GetComputerNameExA returned %d\n", NameType, Ret);
    ok(Error == ERROR_MORE_DATA, "[%d] GetComputerNameExA returned error %lu\n", NameType, Error);
    ok(Size == ReferenceLen + 1, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);
    ok(BufferA[0] == 0x55, "[%d] BufferA[0] = 0x%x\n", NameType, BufferA[0]);

    /* non-NULL buffer, exact size */
    RtlFillMemory(BufferW, sizeof(BufferW), 0x55);
    Size = ReferenceLen + 1;
    Ret = GetComputerNameExW(NameType, BufferW, &Size);
    ok(Ret == TRUE, "[%d] GetComputerNameExW returned %d\n", NameType, Ret);
    ok(Size == ReferenceLen, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);
    ok(BufferW[ReferenceLen] == 0, "[%d] BufferW[ReferenceLen] = 0x%x\n", NameType, BufferW[ReferenceLen]);
    ok(BufferW[ReferenceLen + 1] == 0x5555, "[%d] BufferW[ReferenceLen + 1] = 0x%x\n", NameType, BufferW[ReferenceLen + 1]);
    ok(!wcscmp(BufferW, Reference), "[%d] '%ls' != '%ls'\n", NameType, BufferW, Reference);

    RtlFillMemory(BufferA, sizeof(BufferA), 0x55);
    Size = ReferenceLen + 1;
    Ret = GetComputerNameExA(NameType, BufferA, &Size);
    ok(Ret == TRUE, "[%d] GetComputerNameExA returned %d\n", NameType, Ret);
    ok(Size == ReferenceLen, "[%d] Got Size %lu, expected %lu\n", NameType, Size, ReferenceLen + 1);
    ok(BufferA[ReferenceLen] == 0, "[%d] BufferA[ReferenceLen] = 0x%x\n", NameType, BufferA[ReferenceLen]);
    ok(BufferA[ReferenceLen + 1] == 0x55, "[%d] BufferA[ReferenceLen + 1] = 0x%x\n", NameType, BufferA[ReferenceLen + 1]);
    for (i = 0; i < ReferenceLen; i++)
    {
        if (BufferA[i] != Reference[i])
        {
            ok(0, "[%d] BufferA[%lu] = 0x%x, expected 0x%x\n", NameType, i, BufferA[i], Reference[i]);
        }
    }
}

START_TEST(GetComputerNameEx)
{
    TestGetComputerNameEx(ComputerNameNetBIOS);
    TestGetComputerNameEx(ComputerNameDnsHostname);
    TestGetComputerNameEx(ComputerNameDnsDomain);
    //TestGetComputerNameEx(ComputerNameDnsFullyQualified);
    TestGetComputerNameEx(ComputerNamePhysicalNetBIOS);
    TestGetComputerNameEx(ComputerNamePhysicalDnsHostname);
    TestGetComputerNameEx(ComputerNamePhysicalDnsDomain);
    //TestGetComputerNameEx(ComputerNamePhysicalDnsFullyQualified);
}
