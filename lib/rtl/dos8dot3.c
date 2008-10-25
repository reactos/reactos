/* COPYRIGHT:       See COPYING in the top level directory
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
   CHAR c;

   StrLength = Name->Length / sizeof(WCHAR);
   DPRINT("StrLength: %lu\n", StrLength);

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
   DPRINT("DotPos: %lu\n", DotPos);

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
   DPRINT("NameLength: %lu\n", NameLength);

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
      Context->CheckSumInserted = FALSE;
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
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(IN PCUNICODE_STRING UnicodeName,
                       IN OUT POEM_STRING AnsiName OPTIONAL,
                       IN OUT PBOOLEAN SpacesFound OPTIONAL)
{
    static const char Illegal[] = "*?<>|\"+=,;[]:/\\\345";
    int Dot = -1;
    int i;
    char Buffer[12];
    OEM_STRING OemString;
    BOOLEAN GotSpace = FALSE;

    if (!AnsiName)
    {
        OemString.Length = sizeof(Buffer);
        OemString.MaximumLength = sizeof(Buffer);
        OemString.Buffer = Buffer;
        AnsiName = &OemString;
    }
    if (RtlUpcaseUnicodeStringToCountedOemString( AnsiName, UnicodeName, FALSE ) != STATUS_SUCCESS)
        return FALSE;

    if ((AnsiName->Length > 12) || (AnsiName->Buffer == NULL)) return FALSE;

    /* a starting . is invalid, except for . and .. */
    if (AnsiName->Buffer[0] == '.')
    {
        if (AnsiName->Length != 1 && (AnsiName->Length != 2 || AnsiName->Buffer[1] != '.')) return FALSE;
        if (SpacesFound) *SpacesFound = FALSE;
        return TRUE;
    }

    for (i = 0; i < AnsiName->Length; i++)
    {
        switch (AnsiName->Buffer[i])
        {
        case ' ':
            /* leading/trailing spaces not allowed */
            if (!i || i == AnsiName->Length-1 || AnsiName->Buffer[i+1] == '.') return FALSE;
            GotSpace = TRUE;
            break;
        case '.':
            if (Dot != -1) return FALSE;
            Dot = i;
            break;
        default:
            if (strchr(Illegal, AnsiName->Buffer[i])) return FALSE;
            break;
        }
    }
    /* check file part is shorter than 8, extension shorter than 3
     * dot cannot be last in string
     */
    if (Dot == -1)
    {
        if (AnsiName->Length > 8) return FALSE;
    }
    else
    {
        if (Dot > 8 || (AnsiName->Length - Dot > 4) || Dot == AnsiName->Length - 1) return FALSE;
    }
    if (SpacesFound) *SpacesFound = GotSpace;
    return TRUE;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
	IN  PVOID VolumeDeviceObject,
	OUT PUNICODE_STRING DosName
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
