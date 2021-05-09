/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlEncrypt/DecryptMemory
 * PROGRAMMER:      Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

#include <ntsecapi.h>

static const CHAR TestData[32] = "This is some test Message!!!";

void TestEncrypt(ULONG OptionFlags)
{
    CHAR DECLSPEC_ALIGN(8) Buffer[32];
    NTSTATUS Status;

    /* Size must be aligned to 8 bytes (aka RTL_ENCRYPT_MEMORY_SIZE) */
    RtlCopyMemory(Buffer, TestData, sizeof(Buffer));
    Status = RtlEncryptMemory(Buffer, 7, OptionFlags);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    /* Buffer can be unaligned */
    RtlCopyMemory(Buffer, TestData, sizeof(Buffer));
    Status = RtlEncryptMemory(&Buffer[1], 8, OptionFlags);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = RtlDecryptMemory(&Buffer[1], 8, OptionFlags);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(RtlEqualMemory(Buffer, TestData, sizeof(Buffer)) == 1,
       "OptionFlags=%lu, Buffer='%s', TestData='%s'\n",
       OptionFlags, Buffer, TestData);

    RtlCopyMemory(Buffer, TestData, sizeof(Buffer));
    Status = RtlEncryptMemory(Buffer, sizeof(Buffer), OptionFlags);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_int(RtlEqualMemory(Buffer, TestData, sizeof(Buffer)), 0);
    Status = RtlDecryptMemory(Buffer, sizeof(Buffer), OptionFlags);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_int(RtlEqualMemory(Buffer, TestData, sizeof(Buffer)), 1);
}

void TestCrossProcessDecrypt(PSTR Param)
{
    UCHAR Buffer[32] = { 0 };
    ULONG OptionFlags;
    PSTR StrData;
    ULONG i;
    NTSTATUS Status;

    OptionFlags = Param[3] == '1' ? RTL_ENCRYPT_OPTION_CROSS_PROCESS : RTL_ENCRYPT_OPTION_SAME_LOGON;

    /* Convert the HEX string to binary ('-cp<1|2>:<data>') */
    StrData = Param + 5;
    for (i = 0; i < sizeof(Buffer); i++)
    {
        UINT x;
        sscanf(&StrData[2 * i], "%02X", &x);
        Buffer[i] = (UCHAR)x;
    }

    /* Decrypt the data */
    Status = RtlDecryptMemory(Buffer, sizeof(Buffer), OptionFlags);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_int(RtlEqualMemory(Buffer, TestData, sizeof(Buffer)), 1);
}

void TestCrossProcessEncrypt(ULONG OptionFlags)
{
    UCHAR Buffer[32];
    CHAR CmdLine[MAX_PATH];
    PSTR StrBuffer;
    NTSTATUS Status;
    ULONG i;
    INT result;

    /* Encrypt the test string */
    RtlCopyMemory(Buffer, TestData, sizeof(Buffer));
    Status = RtlEncryptMemory(Buffer, sizeof(Buffer), OptionFlags);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Start building a command line */
    sprintf(CmdLine, "advapi32_apitest.exe RtlEncryptMemory -cp%lu:", OptionFlags);
    StrBuffer = CmdLine + strlen(CmdLine);

    /* Convert encrypted data into a hex string */
    for (i = 0; i < sizeof(Buffer); i++)
    {
        sprintf(&StrBuffer[2 * i], "%02X", Buffer[i]);
    }

    result = system(CmdLine);
    ok_int(result, 0);
}

START_TEST(RtlEncryptMemory)
{
    CHAR Buffer[8] = { 0 };
    PSTR CommandLine, Param;
    NTSTATUS Status;

    /* Check recursive call */
    CommandLine = GetCommandLineA();
    Param = strstr(CommandLine, "-cp");
    if (Param != NULL)
    {
        TestCrossProcessDecrypt(Param);
        return;
    }

    TestEncrypt(RTL_ENCRYPT_OPTION_SAME_PROCESS);
    TestEncrypt(RTL_ENCRYPT_OPTION_CROSS_PROCESS);
    TestEncrypt(RTL_ENCRYPT_OPTION_SAME_LOGON);

    Status = RtlEncryptMemory(Buffer, sizeof(Buffer), 4);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    TestCrossProcessEncrypt(RTL_ENCRYPT_OPTION_CROSS_PROCESS);
    TestCrossProcessEncrypt(RTL_ENCRYPT_OPTION_SAME_LOGON);
}
