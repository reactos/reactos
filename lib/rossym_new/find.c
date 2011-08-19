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

#include <precomp.h>

#define NDEBUG
#include <debug.h>

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

VOID
RosSymFreeAggregate(PROSSYM_AGGREGATE Aggregate)
{
    int i;
    for (i = 0; i < Aggregate->NumElements; i++) {
        free(Aggregate->Elements[i].Name);
        free(Aggregate->Elements[i].Type);
    }
    free(Aggregate->Elements);
}

BOOLEAN
RosSymAggregate(PROSSYM_INFO RosSymInfo, PCHAR Type, PROSSYM_AGGREGATE Aggregate)
{
    char *tchar;
    ulong unit, typeoff = 0;
    DwarfSym type = { };
    // Get the first unit
    if (dwarfaddrtounit(RosSymInfo, RosSymInfo->pe->codestart + RosSymInfo->pe->imagebase, &unit) == -1)
        return FALSE;

    if (Type[0] == '#') {
        for (tchar = Type + 1; *tchar; tchar++) {
            typeoff *= 10;
            typeoff += *tchar - '0';
        }
        if (dwarfseeksym(RosSymInfo, unit, typeoff, &type) == -1)
            return FALSE;
    } else if (dwarflookupnameinunit(RosSymInfo, unit, Type, &type) != 0 ||
        (type.attrs.tag != TagStructType && type.attrs.tag != TagUnionType))
        return FALSE;
    
    DwarfSym element = { }, inner = { };
    int count = 0;
    
    werrstr("type %s (want %s) type %x\n", type.attrs.name, Type, type.attrs.type);
    
    if (type.attrs.have.type) {
        if (dwarfseeksym(RosSymInfo, unit, type.attrs.type, &inner) == -1)
            return FALSE;
        type = inner;
    }
    
    werrstr("finding members %d\n", type.attrs.haskids);
    while (dwarfnextsymat(RosSymInfo, &type, &element) != -1) {
        if (element.attrs.have.name)
            werrstr("%x %s\n", element.attrs.tag, element.attrs.name);
        if (element.attrs.tag == TagMember) count++;
    }
    
    werrstr("%d members\n", count);
    
    if (!count) return FALSE;
    memset(&element, 0, sizeof(element));
    Aggregate->NumElements = count;
    Aggregate->Elements = malloc(sizeof(ROSSYM_AGGREGATE_MEMBER) * count);
    count = 0;
    werrstr("Enumerating %s\n", Type);
    while (dwarfnextsymat(RosSymInfo, &type, &element) != -1) {
        memset(&Aggregate->Elements[count], 0, sizeof(*Aggregate->Elements));
        if (element.attrs.tag == TagMember) {
            if (element.attrs.have.name) {
                Aggregate->Elements[count].Name = malloc(strlen(element.attrs.name) + 1);
                strcpy(Aggregate->Elements[count].Name, element.attrs.name);
            }
            Aggregate->Elements[count].TypeId = element.attrs.type;
            // Seek our range in loc
            DwarfBuf locbuf;
            DwarfBuf instream = { };
            
            locbuf.d = RosSymInfo;
            locbuf.addrsize = RosSymInfo->addrsize;

            if (element.attrs.have.datamemberloc) {
                instream = locbuf;
                instream.p = element.attrs.datamemberloc.b.data;
                instream.ep = element.attrs.datamemberloc.b.data + element.attrs.datamemberloc.b.len;
                werrstr("datamemberloc type %x %p:%x\n", 
                        element.attrs.have.datamemberloc,
                        element.attrs.datamemberloc.b.data, element.attrs.datamemberloc.b.len);
            }

            if (dwarfgetarg(RosSymInfo, element.attrs.name, &instream, 0, NULL, &Aggregate->Elements[count].BaseOffset) == -1)
                Aggregate->Elements[count].BaseOffset = -1;
            werrstr("tag %x name %s base %x type %x\n", 
                    element.attrs.tag, element.attrs.name, 
                    Aggregate->Elements[count].BaseOffset,
                    Aggregate->Elements[count].TypeId);
            count++;
        }
    }
    for (count = 0; count < Aggregate->NumElements; count++) {
        memset(&type, 0, sizeof(type));
        memset(&inner, 0, sizeof(inner));
        werrstr("seeking type %x (%s) from %s\n", 
                Aggregate->Elements[count].TypeId,
                Aggregate->Elements[count].Type,
                Aggregate->Elements[count].Name);
        dwarfseeksym(RosSymInfo, unit, Aggregate->Elements[count].TypeId, &type);
        while (type.attrs.have.type && type.attrs.tag != TagPointerType) {
            if (dwarfseeksym(RosSymInfo, unit, type.attrs.type, &inner) == -1)
                return FALSE;
            type = inner;
        }
        //dwarfdumpsym(RosSymInfo, &type);
        if (type.attrs.have.name) {
            Aggregate->Elements[count].Type = malloc(strlen(type.attrs.name) + 1);
            strcpy(Aggregate->Elements[count].Type, type.attrs.name);
        } else {
            char strbuf[128] = {'#'}, *bufptr = strbuf + 1;
            ulong idcopy = Aggregate->Elements[count].TypeId;
            ulong mult = 1;
            while (mult * 10 < idcopy) mult *= 10;
            while (mult > 0) {
                *bufptr++ = '0' + ((idcopy / mult) % 10);
                mult /= 10;
            }
            Aggregate->Elements[count].Type = malloc(strlen(strbuf) + 1);
            strcpy(Aggregate->Elements[count].Type, strbuf);
        }
        if (type.attrs.tag == TagPointerType)
            Aggregate->Elements[count].Size = RosSymInfo->addrsize;
        else
            Aggregate->Elements[count].Size = type.attrs.bytesize;
        if (type.attrs.have.bitsize)
            Aggregate->Elements[count].Bits = type.attrs.bitsize;
        if (type.attrs.have.bitoffset)
            Aggregate->Elements[count].FirstBit = type.attrs.bitoffset;
    }
    return TRUE;
}

/* EOF */
