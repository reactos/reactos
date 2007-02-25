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

static PROSSYM_ENTRY
FindEntry(IN PROSSYM_INFO RosSymInfo, IN ULONG_PTR RelativeAddress)
{
  /*
   * Perform a binary search.
   *
   * The code below is a bit sneaky.  After a comparison fails, we
   * divide the work in half by moving either left or right. If lim
   * is odd, moving left simply involves halving lim: e.g., when lim
   * is 5 we look at item 2, so we change lim to 2 so that we will
   * look at items 0 & 1.  If lim is even, the same applies.  If lim
   * is odd, moving right again involes halving lim, this time moving
   * the base up one item past p: e.g., when lim is 5 we change base
   * to item 3 and make lim 2 so that we will look at items 3 and 4.
   * If lim is even, however, we have to shrink it by one before
   * halving: e.g., when lim is 4, we still looked at item 2, so we
   * have to make lim 3, then halve, obtaining 1, so that we will only
   * look at item 3.
   */
  PROSSYM_ENTRY Base = RosSymInfo->Symbols;
  ULONG Lim;
  PROSSYM_ENTRY Mid, Low;

  if (RelativeAddress < Base->Address)
    {
      return NULL;
    }

  Low = Base;
  for (Lim = RosSymInfo->SymbolsCount; Lim != 0; Lim >>= 1)
    {
      Mid = Base + (Lim >> 1);
      if (RelativeAddress == Mid->Address)
        {
          return Mid;
        }
      if (Mid->Address < RelativeAddress)  /* key > mid: move right */
        {
          Low = Mid;
          Base = Mid + 1;
          Lim--;
        }               /* else move left */
    }

  return Low;
}


BOOLEAN
RosSymGetAddressInformation(PROSSYM_INFO RosSymInfo,
                            ULONG_PTR RelativeAddress,
                            ULONG *LineNumber,
                            char *FileName,
                            char *FunctionName)
{
  PROSSYM_ENTRY RosSymEntry;

  DPRINT("RelativeAddress = 0x%08x\n", RelativeAddress);

  if (RosSymInfo->Symbols == NULL || RosSymInfo->SymbolsCount == 0 ||
      RosSymInfo->Strings == NULL || RosSymInfo->StringsLength == 0)
    {
      DPRINT1("Uninitialized RosSymInfo\n");
      return FALSE;
    }

  ASSERT(LineNumber || FileName || FunctionName);

  /* find symbol entry for function */
  RosSymEntry = FindEntry(RosSymInfo, RelativeAddress);

  if (NULL == RosSymEntry)
    {
      DPRINT("None of the requested information was found!\n");
      return FALSE;
    }

  if (LineNumber != NULL)
    {
      *LineNumber = RosSymEntry->SourceLine;
    }
  if (FileName != NULL)
    {
      PCSTR Name = "";
      if (RosSymEntry->FileOffset != 0)
        {
          Name = (PCHAR) RosSymInfo->Strings + RosSymEntry->FileOffset;
        }
      strcpy(FileName, Name);
    }
  if (FunctionName != NULL)
    {
      PCSTR Name = "";
      if (RosSymEntry->FunctionOffset != 0)
        {
          Name = (PCHAR) RosSymInfo->Strings + RosSymEntry->FunctionOffset;
        }
      strcpy(FunctionName, Name);
    }

  return TRUE;
}

/* EOF */
