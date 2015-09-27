/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/dos8dot3.c
 * PURPOSE:         Short name (8.3 name) functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>


/* CONSTANTS *****************************************************************/

const PCHAR RtlpShortIllegals = ";+=[],\"*\\<>/?:|";


/* FUNCTIONS *****************************************************************/

static BOOLEAN
RtlpIsShortIllegal(CHAR Char)
{
   return strchr(RtlpShortIllegals, Char) ? TRUE : FALSE;
}

static USHORT
RtlpGetCheckSum(PUNICODE_STRING Name)
{
   USHORT Hash, Saved;
   WCHAR * CurChar;
   USHORT Len;

   if (Name->Length == 0)
   {
      return 0;
   }

   if (Name->Length == sizeof(WCHAR))
   {
      return Name->Buffer[0];
   }

   CurChar = Name->Buffer;
   Hash = (*CurChar << 8) + *(CurChar + 1);
   if (Name->Length == 2 * sizeof(WCHAR))
   {
      return Hash;
   }

   Saved = Hash;
   Len = 2;
   do
   {
      CurChar = CurChar + 2;
      Hash = (Hash << 7) + *CurChar;
      Hash = (Saved >> 1) + (Hash << 8);
      if (Len + 1 < Name->Length / sizeof(WCHAR))
      {
         Hash += *(CurChar + 1);
      }
      Saved = Hash;
      Len += 2;
   } while (Len < Name->Length / sizeof(WCHAR));

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
VOID NTAPI
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
   WCHAR c;

   StrLength = Name->Length / sizeof(WCHAR);
   DPRINT("StrLength: %lu\n", StrLength);

   /* Find last dot in Name */
   DotPos = StrLength;
   for (i = 0; i < StrLength; i++)
   {
      if (Name->Buffer[i] == L'.')
      {
         DotPos = i;
      }
   }

   DPRINT("DotPos: %lu\n", DotPos);

   /* Copy name (6 valid characters max) */
   for (i = 0, NameLength = 0; NameLength < 6 && i < DotPos; i++)
   {
      c = UNICODE_NULL;
      if (AllowExtendedCharacters)
      {
          c = RtlUpcaseUnicodeChar(Name->Buffer[i]);
          Count = 1;
      }
      else
      {
          RtlUpcaseUnicodeToOemN((CHAR *)&c, sizeof(CHAR), &Count, &Name->Buffer[i], sizeof(WCHAR));
      }

      if (Count != 1 || c == UNICODE_NULL || RtlpIsShortIllegal(c))
      {
         NameBuffer[NameLength++] = L'_';
      }
      else if (c != L'.' && c != L' ')
      {
         if (isgraph(c) || (AllowExtendedCharacters && iswgraph(c)))
         {
            NameBuffer[NameLength++] = c;
         }
      }
   }

   DPRINT("NameBuffer: '%.08S'\n", NameBuffer);
   DPRINT("NameLength: %lu\n", NameLength);

   /* Copy extension (4 valid characters max) */
   if (DotPos < StrLength)
   {
      for (i = DotPos, ExtLength = 0; ExtLength < 4 && i < StrLength; i++)
      {
          c = UNICODE_NULL;
          if (AllowExtendedCharacters)
          {
              c = RtlUpcaseUnicodeChar(Name->Buffer[i]);
              Count = 1;
          }
          else
          {
              RtlUpcaseUnicodeToOemN((CHAR *)&c, sizeof(CHAR), &Count, &Name->Buffer[i], sizeof(WCHAR));
          }

         if (Count != 1 || c == UNICODE_NULL || RtlpIsShortIllegal(c))
         {
            ExtBuffer[ExtLength++] = L'_';
         }
         else if (c != L' ')
         {
            if (isgraph(c) || c == L'.' || (AllowExtendedCharacters && iswgraph(c)))
            {
               ExtBuffer[ExtLength++] = c;
            }
         }
      }
   }
   else
   {
      ExtLength = 0;
   }
   DPRINT("ExtBuffer: '%.04S'\n", ExtBuffer);
   DPRINT("ExtLength: %lu\n", ExtLength);

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

   DPRINT("CopyLength: %lu\n", CopyLength);

   if ((Context->NameLength == CopyLength) &&
         (wcsncmp(Context->NameBuffer, NameBuffer, CopyLength) == 0) &&
         (Context->ExtensionLength == ExtLength) &&
         (wcsncmp(Context->ExtensionBuffer, ExtBuffer, ExtLength) == 0) &&
         (Checksum == Context->Checksum) &&
         (Context->LastIndexValue < 999))
   {
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
      Context->LastIndexValue = 1;
      if (NameLength == 0)
      {
         Context->CheckSumInserted = TRUE;
         Context->Checksum = RtlpGetCheckSum(Name);
      }
      else
      {
         Context->CheckSumInserted = FALSE;
      }
   }

   IndexLength = RtlpGetIndexLength(Context->LastIndexValue);

   DPRINT("CurrentIndex: %lu, IndexLength %lu\n", Context->LastIndexValue, IndexLength);

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
      Checksum = Context->Checksum;
      for (i = 0; i < 4; i++)
      {
         Name8dot3->Buffer[j++] = (Checksum & 0xF) > 9 ? (Checksum & 0xF) + L'A' - 10 : (Checksum & 0xF) + L'0';
         Checksum >>= 4;
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
   Name8dot3->Length = (USHORT)(j + ExtLength) * sizeof(WCHAR);

   DPRINT("Name8dot3: '%wZ'\n", Name8dot3);

   /* Update context */
   Context->NameLength = (UCHAR)CopyLength;
   Context->ExtensionLength = ExtLength;
   memcpy(Context->NameBuffer, NameBuffer, CopyLength * sizeof(WCHAR));
   memcpy(Context->ExtensionBuffer, ExtBuffer, ExtLength * sizeof(WCHAR));
}


/*
 * @implemented
 * Note: the function does not conform to the annotations.
 * SpacesFound is not always set!
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3 (
    _In_ PCUNICODE_STRING Name,
    _Inout_opt_ POEM_STRING OemName,
    _Out_opt_ PBOOLEAN NameContainsSpaces)
{
    static const char Illegal[] = "*?<>|\"+=,;[]:/\\\345";
    int Dot = -1;
    int i;
    char Buffer[12];
    OEM_STRING OemString;
    BOOLEAN GotSpace = FALSE;
    NTSTATUS Status;

    if (!OemName)
    {
        OemString.Length = sizeof(Buffer);
        OemString.MaximumLength = sizeof(Buffer);
        OemString.Buffer = Buffer;
        OemName = &OemString;
    }

    Status = RtlUpcaseUnicodeStringToCountedOemString(OemName, Name, FALSE);
    if (!NT_SUCCESS(Status))
        return FALSE;

    if ((OemName->Length > 12) || (OemName->Buffer == NULL)) return FALSE;

    /* a starting . is invalid, except for . and .. */
    if (OemName->Buffer[0] == '.')
    {
        if (OemName->Length != 1 && (OemName->Length != 2 || OemName->Buffer[1] != '.')) return FALSE;
        if (NameContainsSpaces) *NameContainsSpaces = FALSE;
        return TRUE;
    }

    for (i = 0; i < OemName->Length; i++)
    {
        switch (OemName->Buffer[i])
        {
        case ' ':
            /* leading/trailing spaces not allowed */
            if (!i || i == OemName->Length-1 || OemName->Buffer[i+1] == '.') return FALSE;
            GotSpace = TRUE;
            break;
        case '.':
            if (Dot != -1) return FALSE;
            Dot = i;
            break;
        default:
            if (strchr(Illegal, OemName->Buffer[i])) return FALSE;
            break;
        }
    }
    /* check file part is shorter than 8, extension shorter than 3
     * dot cannot be last in string
     */
    if (Dot == -1)
    {
        if (OemName->Length > 8) return FALSE;
    }
    else
    {
        if (Dot > 8 || (OemName->Length - Dot > 4) || Dot == OemName->Length - 1) return FALSE;
    }
    if (NameContainsSpaces) *NameContainsSpaces = GotSpace;
    return TRUE;
}

/* EOF */
