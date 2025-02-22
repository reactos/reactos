/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Test for NtCreateThread
 * PROGRAMMER:  Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "precomp.h"

START_TEST(NtCreateThread)
{
    NTSTATUS Status;
    INITIAL_TEB InitialTeb;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES Attributes;

    InitializeObjectAttributes(&Attributes, NULL, 0, NULL, NULL);
    ZeroMemory(&InitialTeb, sizeof(INITIAL_TEB));

    Status = NtCreateThread(&ThreadHandle,
                            0,
                            &Attributes,
                            NtCurrentProcess(),
                            NULL,
                            (PCONTEXT)0x70000000, /* Aligned usermode address */
                            &InitialTeb,
                            FALSE);

    ok_hex(Status, STATUS_ACCESS_VIOLATION);
}
