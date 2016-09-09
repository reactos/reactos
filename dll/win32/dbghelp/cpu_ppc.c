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

#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static BOOL ppc_get_addr(HANDLE hThread, const CONTEXT* ctx,
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

static BOOL ppc_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    FIXME("not done\n");
    return FALSE;
}

static unsigned ppc_map_dwarf_register(unsigned regno, BOOL eh_frame)
{
    FIXME("not done\n");
    return 0;
}

static void* ppc_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
    FIXME("NIY\n");
    return NULL;
}

static const char* ppc_fetch_regname(unsigned regno)
{
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static BOOL ppc_fetch_minidump_thread(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx)
{
    FIXME("NIY\n");
    return FALSE;
}

static BOOL ppc_fetch_minidump_module(struct dump_context* dc, unsigned index, unsigned flags)
{
    FIXME("NIY\n");
    return FALSE;
}

DECLSPEC_HIDDEN struct cpu cpu_ppc = {
    IMAGE_FILE_MACHINE_POWERPC,
    4,
    CV_REG_NONE, /* FIXME */
    ppc_get_addr,
    ppc_stack_walk,
    NULL,
    ppc_map_dwarf_register,
    ppc_fetch_context_reg,
    ppc_fetch_regname,
    ppc_fetch_minidump_thread,
    ppc_fetch_minidump_module,
};
