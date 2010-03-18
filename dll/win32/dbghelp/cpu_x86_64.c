/*
 * File cpu_x86_64.c
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

static unsigned x86_64_get_addr(HANDLE hThread, const CONTEXT* ctx,
                                enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode = AddrModeFlat;
    switch (ca)
    {
#ifdef __x86_64__
    case cpu_addr_pc:    addr->Segment = ctx->SegCs; addr->Offset = ctx->Rip; return TRUE;
    case cpu_addr_stack: addr->Segment = ctx->SegSs; addr->Offset = ctx->Rsp; return TRUE;
    case cpu_addr_frame: addr->Segment = ctx->SegSs; addr->Offset = ctx->Rbp; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

enum st_mode {stm_start, stm_64bit, stm_done};

/* indexes in Reserved array */
#define __CurrentMode     0
#define __CurrentSwitch   1
#define __NextSwitch      2

#define curr_mode   (frame->Reserved[__CurrentMode])
#define curr_switch (frame->Reserved[__CurrentSwitch])
#define next_switch (frame->Reserved[__NextSwitch])

static BOOL x86_64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame)
{
    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;
    assert(!csw->is32);

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "64bit");

    if (curr_mode == stm_start)
    {
        if ((frame->AddrPC.Mode == AddrModeFlat) &&
            (frame->AddrFrame.Mode != AddrModeFlat))
        {
            WARN("Bad AddrPC.Mode / AddrFrame.Mode combination\n");
            goto done_err;
        }

        /* Init done */
        curr_mode = stm_64bit;
        curr_switch = 0;
        frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrModeFlat;
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (frame->AddrReturn.Offset == 0) goto done_err;
        frame->AddrPC = frame->AddrReturn;
    }

    if (!sw_read_mem(csw, frame->AddrStack.Offset,
                     &frame->AddrReturn.Offset, sizeof(DWORD64)))
    {
        WARN("Cannot read new frame offset %s\n",
             wine_dbgstr_longlong(frame->AddrFrame.Offset + sizeof(DWORD64)));
        goto done_err;
    }
    /* FIXME: simplistic stuff... need to handle both dwarf & PE stack information */
    frame->AddrStack.Offset += sizeof(DWORD64);
    memset(&frame->Params, 0, sizeof(frame->Params));

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    if (frame->AddrPC.Offset && sw_module_base(csw, frame->AddrPC.Offset))
        frame->FuncTableEntry = sw_table_access(csw, frame->AddrPC.Offset);
    else
        frame->FuncTableEntry = NULL;

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "64bit",
          frame->FuncTableEntry);

    return TRUE;
done_err:
    curr_mode = stm_done;
    return FALSE;
}

struct cpu cpu_x86_64 = {
    IMAGE_FILE_MACHINE_AMD64,
    8,
    x86_64_get_addr,
    x86_64_stack_walk,
};
