/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: dos8dot3.c,v 1.6 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/dos8dot3.c
 * PURPOSE:         Short name (8.3 name) functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntos/minmax.h>

//#define NDEBUG
#include <ntdll/ntdll.h>


/* CONSTANTS *****************************************************************/

const PWCHAR RtlpShortIllegals = L" ;+=[]',\"*\\<>/?:|";


/* FUNCTIONS *****************************************************************/

static BOOLEAN
RtlpIsShortIllegal(WCHAR Char)
{
  int i;

  for (i = 0; RtlpShortIllegals[i]; i++)
    {
      if (Char == RtlpShortIllegals[i])
	return(TRUE);
    }

  return(FALSE);
}


/*
 * @implemented
 */
VOID STDCALL
RtlGenerate8dot3Name(IN PUNICODE_STRING Name,
		     IN BOOLEAN AllowExtendedCharacters,
		     IN OUT PGENERATE_NAME_CONTEXT Context,
		     OUT PUNICODE_STRING Name8dot3)
{
  WCHAR NameBuffer[8];
  WCHAR ExtBuffer[4];
  USHORT StrLength;
  USHORT NameLength;
  USHORT ExtLength;
  USHORT CopyLength;
  USHORT DotPos;
  USHORT i, j;
  USHORT CurrentIndex;

  memset(NameBuffer, 0, sizeof(NameBuffer));
  memset(ExtBuffer, 0, sizeof(ExtBuffer));

  StrLength = Name->Length / sizeof(WCHAR);

  DPRINT("StrLength: %hu\n", StrLength);

  /* Find last dot in Name */
  DotPos = 0;
  for (i = 0; i < StrLength; i++)
    {
      if (Name->Buffer[i] == L'.')
	{
	  DotPos = i;
	}
    }

  if (DotPos == 0)
    {
      DotPos = i;
    }

  DPRINT("DotPos: %hu\n", DotPos);

  /* Copy name (6 valid characters max) */
  for (i = 0, NameLength = 0; NameLength < 6 && i < DotPos; i++)
    {
      if ((!RtlpIsShortIllegal(Name->Buffer[i])) &&
	  (Name->Buffer[i] != L'.'))
	{
	  NameBuffer[NameLength++] = RtlUpcaseUnicodeChar(Name->Buffer[i]);
	}
    }
  DPRINT("NameBuffer: '%.08S'\n", NameBuffer);
  DPRINT("NameLength: %hu\n", NameLength);

  /* Copy extension (4 valid characters max) */
  if (DotPos < StrLength)
    {
      for (i = DotPos, ExtLength = 0; ExtLength < 4 && i < StrLength; i++)
	{
	  if (!RtlpIsShortIllegal(Name->Buffer[i]))
	    {
	      ExtBuffer[ExtLength++] = RtlUpcaseUnicodeChar(Name->Buffer[i]);
	    }
	}
    }

  DPRINT("ExtBuffer: '%.04S'\n", ExtBuffer);
  DPRINT("ExtLength: %hu\n", ExtLength);

  /* Determine next index */
  CurrentIndex = Context->LastIndexValue;
  CopyLength = min(NameLength, (CurrentIndex < 10) ? 6 : 5);

  if ((Context->NameLength == CopyLength) &&
      (wcsncmp(Context->NameBuffer, NameBuffer, CopyLength) == 0) &&
      (Context->ExtensionLength == ExtLength) &&
      (wcsncmp(Context->ExtensionBuffer, ExtBuffer, ExtLength) == 0))
    CurrentIndex++;
  else
    CurrentIndex = 1;

  DPRINT("CurrentIndex: %hu\n", CurrentIndex);

  /* Build the short name */
  for (i = 0; i < CopyLength; i++)
    {
      Name8dot3->Buffer[i] = NameBuffer[i];
    }

  Name8dot3->Buffer[i++] = L'~';
  if (CurrentIndex >= 10)
    Name8dot3->Buffer[i++] = (CurrentIndex / 10) + L'0';
  Name8dot3->Buffer[i++] = (CurrentIndex % 10) + L'0';

  for (j = 0; j < ExtLength; i++, j++)
    {
      Name8dot3->Buffer[i] = ExtBuffer[j];
    }

  Name8dot3->Length = i * sizeof(WCHAR);

  DPRINT("Name8dot3: '%wZ'\n", Name8dot3);

  /* Update context */
  Context->NameLength = CopyLength;
  for (i = 0; i < CopyLength; i++)
    {
      Context->NameBuffer[i] = NameBuffer[i];
    }

  Context->ExtensionLength = ExtLength;
  for (i = 0; i < ExtLength; i++)
    {
      Context->ExtensionBuffer[i] = ExtBuffer[i];
    }

  Context->LastIndexValue = CurrentIndex;
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlIsNameLegalDOS8Dot3(IN PUNICODE_STRING UnicodeName,
		       IN PANSI_STRING AnsiName,
		       OUT PBOOLEAN SpacesFound)
{
  PANSI_STRING name = AnsiName;
  ANSI_STRING DummyString;
  CHAR Buffer[12];
  char *str;
  ULONG Length;
  ULONG i;
  NTSTATUS Status;
  BOOLEAN HasSpace = FALSE;
  BOOLEAN HasDot = FALSE;

  if (UnicodeName->Length > 24)
    {
      return(FALSE); /* name too long */
    }

  if (!name)
    {
      name = &DummyString;
      name->Length = 0;
      name->MaximumLength = 12;
      name->Buffer = Buffer;
    }

  Status = RtlUpcaseUnicodeStringToCountedOemString(name,
						    UnicodeName,
						    FALSE);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  Length = name->Length;
  str = name->Buffer;

  if (!(Length == 1 && *str == '.') &&
      !(Length == 2 && *str == '.' && *(str + 1) == '.'))
    {
      for (i = 0; i < Length; i++, str++)
	{
	  switch (*str)
	    {
	      case ' ':
		HasSpace = TRUE;
		break;

	      case '.':
		if ((HasDot) ||			/* two or more dots */
		    (i == 0) ||			/* dot is first char */
		    (i + 1 == Length) ||	/* dot is last char */
		    (Length - i > 4) ||		/* more than 3 chars of extension */
		    (HasDot == FALSE && i > 8))	/* name is longer than 8 chars */
		  return(FALSE);
		HasDot = TRUE;
		break;
	    }
	}
    }

  /* Name is longer than 8 chars and does not have an extension */
  if (Length > 8 && HasDot == FALSE)
    {
      return(FALSE);
    }

  if (SpacesFound)
    *SpacesFound = HasSpace;

  return(TRUE);
}

/* EOF */
