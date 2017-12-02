/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlUpcaseUnicodeStringToCountedOemString
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlUpcaseUnicodeStringToCountedOemString)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    CHAR OemNameBuffer[13];
    OEM_STRING OemName;

    RtlInitUnicodeString(&Name, L"\x00ae");
    RtlFillMemory(OemNameBuffer, sizeof(OemNameBuffer), 0x55);
    OemName.Buffer = OemNameBuffer;
    OemName.Length = 0;
    OemName.MaximumLength = sizeof(OemNameBuffer);
    Status = RtlUpcaseUnicodeStringToCountedOemString(&OemName, &Name, FALSE);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(OemName.Length == 1, "OemName.Length = %u\n", OemName.Length);
    ok(OemNameBuffer[0] == 'R', "OemNameBuffer[0] = 0x%x\n", OemNameBuffer[0]);
    ok(OemNameBuffer[1] == 0x55, "OemNameBuffer[1] = 0x%x\n", OemNameBuffer[1]);
    ok(OemNameBuffer[2] == 0x55, "OemNameBuffer[2] = 0x%x\n", OemNameBuffer[2]);
}
