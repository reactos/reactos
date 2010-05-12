/*
 * File cpu_ppc.c
 *
 * Copyright (C) 2009-2009, Eric Pouech.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static unsigned ppc_get_addr(HANDLE hThread, const CONTEXT* ctx,
                             enum cpu_addr ca, ADDRESS64* addr)
{
   switch (ca)
    {
#if defined(__powerpc__)
    case cpu_addr_pc:
        addr->Mode    = AddrModeFlat;
        addr->Segment = 0; /* don't need segment */
        addr->Offset  = ctx->Iar;
        return TRUE;
#endif
    default:
    case cpu_addr_stack:
    case cpu_addr_frame:
        FIXME("not done\n");
    }
    return FALSE;
}

static BOOL ppc_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame)
{
    FIXME("not done\n");
    return FALSE;
}

struct cpu cpu_ppc = {
    IMAGE_FILE_MACHINE_POWERPC,
    4,
    ppc_get_addr,
    ppc_stack_walk,
};
