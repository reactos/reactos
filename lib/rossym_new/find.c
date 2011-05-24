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
RosSymGetAddressInformation(PROSSYM_INFO RosSymInfo,
                            ULONG_PTR RelativeAddress,
                            ULONG *LineNumber,
                            char *FileName,
                            char *FunctionName)
{
	char *cdir, *dir, *file, *function;
	ulong line, mtime, length;
	int res = dwarfpctoline
		(RosSymInfo, 
		 RelativeAddress + RosSymInfo->pe->imagebase, 
		 &cdir,
		 &dir,
		 &file,
		 &function,
		 &line,
		 &mtime,
		 &length);
	if (res != -1) {
		*LineNumber = line;
		FileName[0] = 0;
		if (dir) {
			strcpy(FileName, dir);
			strcat(FileName, "/");
		}
		if (file)
			strcat(FileName, file);
		FunctionName[0] = 0;
		if (function)
			strcpy(FunctionName, function);
		return TRUE;
	} else {
		return FALSE;
	}
}

/* EOF */
