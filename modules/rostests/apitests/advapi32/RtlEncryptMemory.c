/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlEncrypt/DecryptMemory
 * PROGRAMMER:      Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

#include <ntsecapi.h>

START_TEST(RtlEncryptMemory)
{
    static const CHAR TestData[32] = "This is some test Message!!!";
    CHAR Buffer[32];
    NTSTATUS Status;

    /* Size must be aligned to 8 bytes */
    Status = RtlEncryptMemory(Buffer, 7, RTL_ENCRYPT_OPTION_SAME_PROCESS);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    /* Buffer must not be aligned to 8 bytes */
    Status = RtlEncryptMemory(&Buffer[1], 8, RTL_ENCRYPT_OPTION_SAME_PROCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);

    RtlCopyMemory(Buffer, TestData, sizeof(Buffer));
    Status = RtlEncryptMemory(Buffer, sizeof(Buffer), RTL_ENCRYPT_OPTION_SAME_PROCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_int(RtlEqualMemory(Buffer, TestData, sizeof(Buffer)), 0);
    Status = RtlDecryptMemory(Buffer, sizeof(Buffer), RTL_ENCRYPT_OPTION_SAME_PROCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_int(RtlEqualMemory(Buffer, TestData, sizeof(Buffer)), 1);

}
