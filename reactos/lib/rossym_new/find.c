/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/find.c
 * PURPOSE:         Find symbol info for an address
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */
/*
 * Parts of this file based on work Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ntddk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"

#define NDEBUG
#include <debug.h>

#include "rossym.h"
#include "dwarf.h"
#include "pe.h"

BOOLEAN
RosSymGetAddressInformation
(PROSSYM_INFO RosSymInfo,
 ULONG_PTR RelativeAddress,
 PROSSYM_LINEINFO RosSymLineInfo)
{
    ROSSYM_REGISTERS registers;
    DwarfParam params[sizeof(RosSymLineInfo->Parameters)/sizeof(RosSymLineInfo->Parameters[0])];
    DwarfSym proc = { };
    int i;
	int res = dwarfpctoline
		(RosSymInfo, 
         &proc,
		 RelativeAddress + RosSymInfo->pe->imagebase, 
		 &RosSymLineInfo->FileName,
		 &RosSymLineInfo->FunctionName,
		 &RosSymLineInfo->LineNumber);
	if (res == -1) {
        werrstr("Could not get basic function info");
		return FALSE;
    }

    if (!(RosSymLineInfo->Flags & ROSSYM_LINEINFO_HAS_REGISTERS))
        return TRUE;

    registers = RosSymLineInfo->Registers;

    DwarfExpr cfa = { };
    ulong cfaLocation;
    if (dwarfregunwind
        (RosSymInfo, 
         RelativeAddress + RosSymInfo->pe->imagebase, 
         proc.attrs.framebase.c, 
         &cfa,
         &registers) == -1) {
        werrstr("Can't get cfa location for %s", RosSymLineInfo->FunctionName);
        return TRUE;
    }

    res = dwarfgetparams
        (RosSymInfo,
         &proc,
         RelativeAddress + RosSymInfo->pe->imagebase,
         sizeof(params)/sizeof(params[0]),
         params);

    if (res == -1) {
        werrstr("%s: could not get params at all", RosSymLineInfo->FunctionName);
        RosSymLineInfo->NumParams = 0;
        return TRUE;
    }

    werrstr("%s: res %d", RosSymLineInfo->FunctionName, res);
    RosSymLineInfo->NumParams = res;

    res = dwarfcomputecfa(RosSymInfo, &cfa, &registers, &cfaLocation);
    if (res == -1) {
        werrstr("%s: could not get our own cfa", RosSymLineInfo->FunctionName);
        return TRUE;
    }

    for (i = 0; i < RosSymLineInfo->NumParams; i++) {
        werrstr("Getting arg %s, unit %x, type %x", 
                params[i].name, params[i].unit, params[i].type);
        res = dwarfargvalue
            (RosSymInfo, 
             &proc, 
             RelativeAddress + RosSymInfo->pe->imagebase,
             cfaLocation,
             &registers,
             &params[i]);
        if (res == -1) { RosSymLineInfo->NumParams = i; return TRUE; }
        werrstr("%s: %x", params[i].name, params[i].value);
        RosSymLineInfo->Parameters[i].ValueName = malloc(strlen(params[i].name)+1);
        strcpy(RosSymLineInfo->Parameters[i].ValueName, params[i].name);
        free(params[i].name);
        RosSymLineInfo->Parameters[i].Value = params[i].value;
    }

    return TRUE;
}

/* EOF */
