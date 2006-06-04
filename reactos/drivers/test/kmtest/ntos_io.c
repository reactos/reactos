/*
 * NTOSKRNL Io Regressions KM-Test
 * ReactOS Device Interface functions implementation
 *
 * Copyright 2006 Aleksey Bragin <aleksey@reactos.org>
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

#include <ddk/ntddk.h>
#include "kmtest.h"

/* PUBLIC FUNCTIONS ***********************************************************/

VOID FASTCALL NtoskrnlIoMdlTest()
{
    PMDL Mdl;

    StartTest();

    // Try to alloc 2Gb MDL
    Mdl = IoAllocateMdl(NULL, 2048UL*0x100000, FALSE, FALSE, NULL);

    ok(Mdl == NULL, "IoAllocateMdl should fail allocation of 2Gb or more, but got Mdl=0x%X", (UINT)Mdl);
    if (Mdl)
        IoFreeMdl(Mdl);

    FinishTest("NTOSKRNL Io Mdl");
}
