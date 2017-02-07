/*
 * File cpu_sparc.c
 *
 * Copyright (C) 2009-2009, Eric Pouech
 * Copyright (C) 2010, Austin English
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

#define IMAGE_FILE_MACHINE_SPARC   0x2000

static unsigned sparc_get_addr(HANDLE hThread, const CONTEXT* ctx,
                             enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    switch (ca)
    {
#ifdef __sparc__
    case cpu_addr_pc:    addr->Offset = ctx->pc; return TRUE;
    case cpu_addr_stack: addr->Offset = ctx->o6; return TRUE;
    case cpu_addr_frame: addr->Offset = ctx->i6; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

static BOOL sparc_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    FIXME("not done for Sparc\n");
    return FALSE;
}

static unsigned sparc_map_dwarf_register(unsigned regno)
{
    if (regno <= 7)
        return CV_SPARC_G0 + regno;
    else if (regno >= 8 && regno <= 15)
        return CV_SPARC_O0 + regno - 8;
    else if (regno >= 16 && regno <= 23)
        return CV_SPARC_L0 + regno - 16;
    else if (regno >= 24 && regno <= 31)
        return CV_SPARC_I0 + regno - 24;

    FIXME("Don't know how to map register %d\n", regno);
    return CV_SPARC_NOREG;
}

static void* sparc_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
    FIXME("not done for Sparc\n");
    return NULL;
}

static const char* sparc_fetch_regname(unsigned regno)
{
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

DECLSPEC_HIDDEN struct cpu cpu_sparc = {
    IMAGE_FILE_MACHINE_SPARC,
    4,
    CV_REG_NONE, /* FIXME */
    sparc_get_addr,
    sparc_stack_walk,
    NULL,
    sparc_map_dwarf_register,
    sparc_fetch_context_reg,
    sparc_fetch_regname,
};
