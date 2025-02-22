/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlIsNameLegalDOS8Dot3
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

START_TEST(RtlIsNameLegalDOS8Dot3)
{
    UNICODE_STRING Name;
    CHAR OemNameBuffer[13];
    OEM_STRING OemName;
    BOOLEAN NameContainsSpaces;
    BOOLEAN IsLegal;

    RtlInitUnicodeString(&Name, L"\x00ae");
    RtlFillMemory(OemNameBuffer, sizeof(OemNameBuffer), 0x55);
    OemName.Buffer = OemNameBuffer;
    OemName.Length = 0;
    OemName.MaximumLength = sizeof(OemNameBuffer);
    NameContainsSpaces = 0x55;
    IsLegal = RtlIsNameLegalDOS8Dot3(&Name, &OemName, &NameContainsSpaces);
    ok(IsLegal == TRUE, "IsLegal = %u\n", IsLegal);
    ok(NameContainsSpaces == FALSE, "NameContainsSpaces = %u\n", NameContainsSpaces);
    ok(OemName.Length == 1, "OemName.Length = %u\n", OemName.Length);
    ok(OemNameBuffer[0] == 'R', "OemNameBuffer[0] = 0x%x\n", OemNameBuffer[0]);
    ok(OemNameBuffer[1] == 0x55, "OemNameBuffer[1] = 0x%x\n", OemNameBuffer[1]);
    ok(OemNameBuffer[2] == 0x55, "OemNameBuffer[2] = 0x%x\n", OemNameBuffer[2]);

}
