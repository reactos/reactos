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
/* $Id: dos8dot3.c,v 1.1 2004/05/31 19:29:02 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rtl/dos8dot3.c
 * PURPOSE:         Short name (8.3 name) functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>


/* CONSTANTS *****************************************************************/

const PCHAR RtlpShortIllegals = " ;+=[],\"*\\<>/?:|";


/* FUNCTIONS *****************************************************************/

static BOOLEAN
RtlpIsShortIllegal(CHAR Char)
{
   return strchr(RtlpShortIllegals, Char) ? TRUE : FALSE;
}

static USHORT
RtlpGetCheckSum(PUNICODE_STRING Name)
{
   USHORT Hash = 0;
   ULONG Length;
   PWCHAR c;

   Length = Name->Length / sizeof(WCHAR);
   c = Name->Buffer;
   while(Length--)
   {
      Hash = (Hash + (*c << 4) + (*c >> 4)) * 11;
      c++;
   }
   return Hash;
}

static ULONG
RtlpGetIndexLength(ULONG Index)
{
   ULONG Length = 0;
   while (Index)
   {
      Index /= 10;
      Length++;
   }
   return Length ? Length : 1;
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
   ULONG Count;
   WCHAR NameBuffer[8];
   WCHAR ExtBuffer[4];
   ULONG StrLength;
   ULONG NameLength;
   ULONG ExtLength;
   ULONG CopyLength;
   ULONG DotPos;
   ULONG i, j;
   ULONG IndexLength;
   ULONG CurrentIndex;
   USHORT Checksum;
   CHAR c;

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
      c = 0;
      RtlUpcaseUnicodeToOemN(&c, sizeof(CHAR), &Count, &Name->Buffer[i], sizeof(WCHAR));
      if (Count != 1 || c == 0 || RtlpIsShortIllegal(c))
      {
         NameBuffer[NameLength++] = L'_';
      }
      else if (c != '.')
      {
         NameBuffer[NameLength++] = (WCHAR)c;
      }
   }

   DPRINT("NameBuffer: '%.08S'\n", NameBuffer);
   DPRINT("NameLength: %hu\n", NameLength);

   /* Copy extension (4 valid characters max) */
   if (DotPos < StrLength)
   {
      for (i = DotPos, ExtLength = 0; ExtLength < 4 && i < StrLength; i++)
      {
         c = 0;
         RtlUpcaseUnicodeToOemN(&c, sizeof(CHAR), &Count, &Name->Buffer[i], sizeof(WCHAR));
         if (Count != 1 || c == 0 || RtlpIsShortIllegal(Name->Buffer[i]))
         {
            ExtBuffer[ExtLength++] = L'_';
         }
         else
         {
            ExtBuffer[ExtLength++] = c;
         }
      }
   }
   else
   {
      ExtLength = 0;
   }
   DPRINT("ExtBuffer: '%.04S'\n", ExtBuffer);
   DPRINT("ExtLength: %hu\n", ExtLength);

   /* Determine next index */
   IndexLength = RtlpGetIndexLength(Context->LastIndexValue);
   if (Context->CheckSumInserted)
   {
      CopyLength = min(NameLength, 8 - 4 - 1 - IndexLength);
      Checksum = RtlpGetCheckSum(Name);
   }
   else
   {
      CopyLength = min(NameLength, 8 - 1 - IndexLength);
      Checksum = 0;
   }

   DPRINT("CopyLength: %hu\n", CopyLength);

   if ((Context->NameLength == CopyLength) &&
         (wcsncmp(Context->NameBuffer, NameBuffer, CopyLength) == 0) &&
         (Context->ExtensionLength == ExtLength) &&
         (wcsncmp(Context->ExtensionBuffer, ExtBuffer, ExtLength) == 0) &&
         (Checksum == Context->Checksum) &&
         (Context->LastIndexValue < 999))
   {
      CHECKPOINT;
      Context->LastIndexValue++;
      if (Context->CheckSumInserted == FALSE &&
            Context->LastIndexValue > 9)
      {
         Context->CheckSumInserted = TRUE;
         Context->LastIndexValue = 1;
         Context->Checksum = RtlpGetCheckSum(Name);
      }
   }
   else
   {
      CHECKPOINT;
      Context->LastIndexValue = 1;
      Context->CheckSumInserted = FALSE;
   }

   IndexLength = RtlpGetIndexLength(Context->LastIndexValue);

   DPRINT("CurrentIndex: %hu, IndexLength %hu\n", Context->LastIndexValue, IndexLength);

   if (Context->CheckSumInserted)
   {
      CopyLength = min(NameLength, 8 - 4 - 1 - IndexLength);
   }
   else
   {
      CopyLength = min(NameLength, 8 - 1 - IndexLength);
   }

   /* Build the short name */
   memcpy(Name8dot3->Buffer, NameBuffer, CopyLength * sizeof(WCHAR));
   j = CopyLength;
   if (Context->CheckSumInserted)
   {
      j += 3;
      Checksum = Context->Checksum;
      for (i = 0; i < 4; i++)
      {
         Name8dot3->Buffer[j--] = (Checksum % 16) > 9 ? (Checksum % 16) + L'A' - 10 : (Checksum % 16) + L'0';
         Checksum /= 16;
      }
      j = CopyLength + 4;
   }
   Name8dot3->Buffer[j++] = L'~';
   j += IndexLength - 1;
   CurrentIndex = Context->LastIndexValue;
   for (i = 0; i < IndexLength; i++)
   {
      Name8dot3->Buffer[j--] = (CurrentIndex % 10) + L'0';
      CurrentIndex /= 10;
   }
   j += IndexLength + 1;

   memcpy(Name8dot3->Buffer + j, ExtBuffer, ExtLength * sizeof(WCHAR));
   Name8dot3->Length = (j + ExtLength) * sizeof(WCHAR);

   DPRINT("Name8dot3: '%wZ'\n", Name8dot3);

   /* Update context */
   Context->NameLength = CopyLength;
   Context->ExtensionLength = ExtLength;
   memcpy(Context->NameBuffer, NameBuffer, CopyLength * sizeof(WCHAR));
   memcpy(Context->ExtensionBuffer, ExtBuffer, ExtLength * sizeof(WCHAR));
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
               if ((HasDot) ||   /* two or more dots */
                     (i == 0) ||   /* dot is first char */
                     (i + 1 == Length) || /* dot is last char */
                     (Length - i > 4) ||  /* more than 3 chars of extension */
                     (HasDot == FALSE && i > 8)) /* name is longer than 8 chars */
                  return(FALSE);
               HasDot = TRUE;
               break;
            default:
               if (RtlpIsShortIllegal(*str))
               {
                  return(FALSE);
               }
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
