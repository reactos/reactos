/*
 * Rtl string functions
 *
 * Copyright (C) 1996-1998 Marcus Meissner
 * Copyright (C) 2000      Alexandre Julliard
 * Copyright (C) 2003      Thomas Mertes
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#define __NTDRIVER__
#include <ddk/ntddk.h>

#include <ntdll/rtl.h>

#include <ntos/minmax.h>
#include <ctype.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define TAG_USTR  TAG('U', 'S', 'T', 'R')
#define TAG_ASTR  TAG('A', 'S', 'T', 'R')
#define TAG_OSTR  TAG('O', 'S', 'T', 'R')


extern BOOLEAN NlsMbCodePageTag;
extern BOOLEAN NlsMbOemCodePageTag;

/* FUNCTIONS *****************************************************************/



WCHAR STDCALL
RtlAnsiCharToUnicodeChar (IN CHAR AnsiChar)
{
   ULONG Size;
   WCHAR UnicodeChar;

   Size = 1;
#if 0

   Size = (NlsLeadByteInfo[AnsiChar] == 0) ? 1 : 2;
#endif

   RtlMultiByteToUnicodeN (&UnicodeChar,
                           sizeof(WCHAR),
                           NULL,
                           &AnsiChar,
                           Size);

   return UnicodeChar;
}


/*
 * @implemented
 *
 * RETURNS
 *  The calculated size in bytes including nullterm.
 */
ULONG
STDCALL
RtlAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
   ULONG Size;

   RtlMultiByteToUnicodeSize(&Size,
                             AnsiString->Buffer,
                             AnsiString->Length);

   return(Size);
}



/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is never nullterminated.
 */
NTSTATUS
STDCALL
RtlAppendStringToString(IN OUT PSTRING Destination,
                        IN PSTRING Source)
{
   PCHAR Ptr;

   if (Source->Length == 0)
      return(STATUS_SUCCESS);

   if (Destination->Length + Source->Length >= Destination->MaximumLength)
      return(STATUS_BUFFER_TOO_SMALL);

   Ptr = Destination->Buffer + Destination->Length;
   memmove(Ptr,
           Source->Buffer,
           Source->Length);
   Ptr += Source->Length;
   *Ptr = 0;

   Destination->Length += Source->Length;

   return(STATUS_SUCCESS);
}



/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is nullterminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the nullterm is ommitted.
 */
NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(
   IN OUT PUNICODE_STRING Destination,
   IN PUNICODE_STRING Source)
{

   if ((Source->Length + Destination->Length) > Destination->MaximumLength)
      return STATUS_BUFFER_TOO_SMALL;

   memcpy((char*)Destination->Buffer + Destination->Length, Source->Buffer, Source->Length);
   Destination->Length += Source->Length;
   /* append terminating '\0' if enough space */
   if( Destination->MaximumLength > Destination->Length )
      Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlCharToInteger(
   IN PCSZ String,
   IN ULONG Base,
   IN OUT PULONG Value)
{
   ULONG Val;

   *Value = 0;

   if (Base == 0)
   {
      Base = 10;
      if (*String == '0')
      {
         Base = 8;
         String++;
         if ((*String == 'x') && isxdigit (String[1]))
         {
            String++;
            Base = 16;
         }
      }
   }

   if (!isxdigit (*String))
      return STATUS_INVALID_PARAMETER;

   while (isxdigit (*String) &&
          (Val = isdigit (*String) ? * String - '0' : (islower (*String)
                 ? toupper (*String) : *String) - 'A' + 10) < Base)
   {
      *Value = *Value * Base + Val;
      String++;
   }

   return STATUS_SUCCESS;
}



/*
 * @implemented
 */
LONG
STDCALL
RtlCompareString(
   IN PSTRING String1,
   IN PSTRING String2,
   IN BOOLEAN CaseInsensitive)
{
   ULONG len1, len2;
   PCHAR s1, s2;
   CHAR  c1, c2;

   if (String1 && String2)
   {
      len1 = String1->Length;
      len2 = String2->Length;
      s1 = String1->Buffer;
      s2 = String2->Buffer;

      if (s1 && s2)
      {
         if (CaseInsensitive)
         {
            while (1)
            {
               c1 = len1-- ? RtlUpperChar (*s1++) : 0;
               c2 = len2-- ? RtlUpperChar (*s2++) : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
         else
         {
            while (1)
            {
               c1 = len1-- ? *s1++ : 0;
               c2 = len2-- ? *s2++ : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
      }
   }

   return 0;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
STDCALL
RtlEqualString(
   IN PSTRING String1,
   IN PSTRING String2,
   IN BOOLEAN CaseInsensitive)
{
   ULONG i;
   CHAR c1, c2;
   PCHAR p1, p2;

   if (String1->Length != String2->Length)
      return FALSE;

   p1 = String1->Buffer;
   p2 = String2->Buffer;
   for (i = 0; i < String1->Length; i++)
   {
      if (CaseInsensitive == TRUE)
      {
         c1 = RtlUpperChar (*p1);
         c2 = RtlUpperChar (*p2);
      }
      else
      {
         c1 = *p1;
         c2 = *p2;
      }

      if (c1 != c2)
         return FALSE;

      p1++;
      p2++;
   }

   return TRUE;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
STDCALL
RtlEqualUnicodeString(
   IN PUNICODE_STRING String1,
   IN PUNICODE_STRING String2,
   IN BOOLEAN  CaseInsensitive)
{
   ULONG i;
   WCHAR wc1, wc2;
   PWCHAR pw1, pw2;

   if (String1->Length != String2->Length)
      return FALSE;

   pw1 = String1->Buffer;
   pw2 = String2->Buffer;

   for (i = 0; i < String1->Length / sizeof(WCHAR); i++)
   {
      if (CaseInsensitive == TRUE)
      {
         wc1 = RtlUpcaseUnicodeChar (*pw1);
         wc2 = RtlUpcaseUnicodeChar (*pw2);
      }
      else
      {
         wc1 = *pw1;
         wc2 = *pw2;
      }

      if (wc1 != wc2)
         return FALSE;

      pw1++;
      pw2++;
   }

   return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
   if (AnsiString->Buffer == NULL)
      return;

   ExFreePool(AnsiString->Buffer);

   AnsiString->Buffer = NULL;
   AnsiString->Length = 0;
   AnsiString->MaximumLength = 0;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFreeOemString(IN POEM_STRING OemString)
{
   if (OemString->Buffer == NULL)
      return;

   ExFreePool(OemString->Buffer);

   OemString->Buffer = NULL;
   OemString->Length = 0;
   OemString->MaximumLength = 0;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
   if (UnicodeString->Buffer == NULL)
      return;

   ExFreePool(UnicodeString->Buffer);

   UnicodeString->Buffer = NULL;
   UnicodeString->Length = 0;
   UnicodeString->MaximumLength = 0;
}


/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
                  IN PCSZ SourceString)
{
   ULONG DestSize;

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
   }
   else
   {
      DestSize = strlen ((const char *)SourceString);
      DestinationString->Length = DestSize;
      DestinationString->MaximumLength = DestSize + sizeof(CHAR);
   }
   DestinationString->Buffer = (PCHAR)SourceString;
}



/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitString(
   IN OUT PSTRING DestinationString,
   IN PCSZ SourceString)
{
   ULONG DestSize;

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
   }
   else
   {
      DestSize = strlen((const char *)SourceString);
      DestinationString->Length = DestSize;
      DestinationString->MaximumLength = DestSize + sizeof(CHAR);
   }
   DestinationString->Buffer = (PCHAR)SourceString;
}


/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PCWSTR SourceString)
{
   ULONG DestSize;

   DPRINT("RtlInitUnicodeString(DestinationString %x, SourceString %x)\n",
          DestinationString,
          SourceString);

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
   }
   else
   {
      DestSize = wcslen((PWSTR)SourceString) * sizeof(WCHAR);
      DestinationString->Length = DestSize;
      DestinationString->MaximumLength = DestSize + sizeof(WCHAR);
   }
   DestinationString->Buffer = (PWSTR)SourceString;
}



/*
 * @implemented
 *
 * NOTES
 *  Writes at most length characters to the string str.
 *  Str is nullterminated when length allowes it.
 *  When str fits exactly in length characters the nullterm is ommitted.
 */
NTSTATUS
STDCALL
RtlIntegerToChar(
   IN ULONG Value,
   IN ULONG Base,
   IN ULONG Length,
   IN OUT PCHAR String)
{
   ULONG Radix;
   CHAR  temp[33];
   ULONG v = Value;
   ULONG i;
   PCHAR tp;
   PCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
         (Radix != 10) && (Radix != 16))
      return STATUS_INVALID_PARAMETER;

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + '0';
      else
         *tp = i + 'a' - 10;
      tp++;
   }

   if (tp - temp >= Length)
      return STATUS_BUFFER_TOO_SMALL;

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}



/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlIntegerToUnicodeString(
   IN ULONG  Value,
   IN ULONG  Base, /* optional */
   IN OUT PUNICODE_STRING String)
{
   ANSI_STRING AnsiString;
   CHAR Buffer[33];
   NTSTATUS Status;

   Status = RtlIntegerToChar (Value,
                              Base,
                              33,
                              Buffer);
   if (!NT_SUCCESS(Status))
      return Status;

   AnsiString.Buffer = Buffer;
   AnsiString.Length = strlen (Buffer);
   AnsiString.MaximumLength = 33;

   Status = RtlAnsiStringToUnicodeString (String,
                                          &AnsiString,
                                          FALSE);

   return Status;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
STDCALL
RtlPrefixString(
   PANSI_STRING String1,
   PANSI_STRING String2,
   BOOLEAN  CaseInsensitive)
{
   PCHAR pc1;
   PCHAR pc2;
   ULONG Length;

   if (String2->Length < String1->Length)
      return FALSE;

   Length = String1->Length;
   pc1 = String1->Buffer;
   pc2 = String2->Buffer;

   if (pc1 && pc2)
   {
      if (CaseInsensitive)
      {
         while (Length--)
         {
            if (RtlUpperChar (*pc1++) != RtlUpperChar (*pc2++))
               return FALSE;
         }
      }
      else
      {
         while (Length--)
         {
            if (*pc1++ != *pc2++)
               return FALSE;
         }
      }
      return TRUE;
   }
   return FALSE;
}


/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
STDCALL
RtlPrefixUnicodeString(
   PUNICODE_STRING String1,
   PUNICODE_STRING String2,
   BOOLEAN  CaseInsensitive)
{
   PWCHAR pc1;
   PWCHAR pc2;
   ULONG Length;

   if (String2->Length < String1->Length)
      return FALSE;

   Length = String1->Length / 2;
   pc1 = String1->Buffer;
   pc2  = String2->Buffer;

   if (pc1 && pc2)
   {
      if (CaseInsensitive)
      {
         while (Length--)
         {
            if (RtlUpcaseUnicodeChar (*pc1++)
                  != RtlUpcaseUnicodeChar (*pc2++))
               return FALSE;
         }
      }
      else
      {
         while (Length--)
         {
            if( *pc1++ != *pc2++ )
               return FALSE;
         }
      }
      return TRUE;
   }
   return FALSE;
}



/*
 * @implemented
 *
 * Note that regardless of success or failure status, we should leave the
 * partial value in Value.  An error is never returned based on the chars
 * in the string.
 *
 * This function does check the base.  Only 2, 8, 10, 16 are permitted,
 * else STATUS_INVALID_PARAMETER is returned.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToInteger(
   IN PUNICODE_STRING String,
   IN ULONG  Base,
   OUT PULONG  Value)
{
   PWCHAR Str;
   ULONG lenmin = 0;
   ULONG i;
   ULONG Val;
   BOOLEAN addneg = FALSE;
   NTSTATUS Status = STATUS_SUCCESS;

   *Value = 0;
   Str = String->Buffer;

   if( Base && Base != 2 && Base != 8 && Base != 10 && Base != 16 )
       return STATUS_INVALID_PARAMETER;

   for (i = 0; i < String->Length / sizeof(WCHAR); i++)
   {
      if (*Str == L'b')
      {
         Base = 2;
         lenmin++;
      }
      else if (*Str == L'o')
      {
         Base = 8;
         lenmin++;
      }
      else if (*Str == L'd')
      {
         Base = 10;
         lenmin++;
      }
      else if (*Str == L'x')
      {
         Base = 16;
         lenmin++;
      }
      else if (*Str == L'+')
      {
         lenmin++;
      }
      else if (*Str == L'-')
      {
         addneg = TRUE;
         lenmin++;
      }
      else if ((*Str > L'1') && (Base == 2))
      {
	  break;
      }
      else if (((*Str > L'7') || (*Str < L'0')) && (Base == 8))
      {
	  break;
      }
      else if (((*Str > L'9') || (*Str < L'0')) && (Base == 10))
      {
	  break;
      }
      else if (  ((*Str > L'9') || (*Str < L'0')) &&
                 ((towupper (*Str) > L'F') || (towupper (*Str) < L'A')) &&
                 (Base == 16))
      {
	  break;
      }
      Str++;
   }

   Str = String->Buffer + lenmin;

   if (Base == 0)
      Base = 10;

   while (iswxdigit (*Str) &&
          (Val = 
	   iswdigit (*Str) ? 
	   *Str - L'0' : 
	   (towupper (*Str) - L'A' + 10)) < Base)
   {
      *Value = *Value * Base + Val;
      Str++;
   }

   if (addneg == TRUE)
      *Value *= -1;

   return Status;
}



/*
 * @implemented
 *
 * RETURNS
 *  Bytes necessary for the conversion including nullterm.
 */
ULONG
STDCALL
RtlUnicodeStringToOemSize(
   IN PUNICODE_STRING UnicodeString)
{
   ULONG Size;

   RtlUnicodeToMultiByteSize (&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

   return Size+1; //NB: incl. nullterm
}


/*
 * @implemented
 *
 
 * NOTES
 *  See RtlpUnicodeStringToAnsiString
 */
NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpUnicodeStringToAnsiString(
             AnsiDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}


/*
 * private
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
FASTCALL
RtlpUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG Length; //including nullterm

   if (NlsMbCodePageTag == TRUE)
   {
      Length = RtlUnicodeStringToAnsiSize(UniSource);
   }
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   AnsiDest->Length = Length - sizeof(CHAR);

   if (AllocateDestinationString)
   {
      AnsiDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_ASTR);
      if (AnsiDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      AnsiDest->MaximumLength = Length;
   }
   else if (AnsiDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > AnsiDest->MaximumLength)
   {
      //make room for nullterm
      AnsiDest->Length = AnsiDest->MaximumLength - sizeof(CHAR);
   }

   Status = RtlUnicodeToMultiByteN (AnsiDest->Buffer,
                                    AnsiDest->Length,
                                    NULL,
                                    UniSource->Buffer,
                                    UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool (AnsiDest->Buffer);
      return Status;
   }

   AnsiDest->Buffer[AnsiDest->Length] = 0;
   return Status;
}


/*
 * @implemented
 *
 * NOTES
 *  See RtlpOemStringToUnicodeString 
 */
NTSTATUS
STDCALL
RtlOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpOemStringToUnicodeString(
             UniDest,
             OemSource,
             AllocateDestinationString,
             NonPagedPool);
}


/*
 * private
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  Does NOT perform a partial copy if unicode is too small!
 */
NTSTATUS
FASTCALL
RtlpOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status;
   ULONG Length; //including nullterm

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlOemStringToUnicodeSize(OemSource);
   else
      Length = (OemSource->Length * sizeof(WCHAR)) + sizeof(WCHAR);

   if (Length > 0xffff)
      return STATUS_INVALID_PARAMETER_2;

   UniDest->Length = (WORD)(Length - sizeof(WCHAR));

   if (AllocateDestinationString)
   {
      UniDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = Length;
   }
   else if (Length > UniDest->MaximumLength)
   {
      DPRINT("STATUS_BUFFER_TOO_SMALL\n");
      return STATUS_BUFFER_TOO_SMALL;
   }

   //FIXME: Do we need this????? -Gunnar
   RtlZeroMemory (UniDest->Buffer,
                  UniDest->Length);

   Status = RtlOemToUnicodeN (UniDest->Buffer,
                              UniDest->Length,
                              NULL,
                              OemSource->Buffer,
                              OemSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool (UniDest->Buffer);
      return Status;
   }

   UniDest->Buffer[UniDest->Length / sizeof(WCHAR)] = 0;
   return STATUS_SUCCESS;
}


/*
 * @implemented
 *
 * NOTES
 *  See RtlpUnicodeStringToOemString.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
   return RtlpUnicodeStringToOemString(
             OemDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}



/*
 * private
 *
 * NOTES
 *   This function always '\0' terminates the string returned.
 */
NTSTATUS
FASTCALL
RtlpUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG Length; //including nullterm

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize (UniSource);
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   OemDest->Length = (WORD)(Length - sizeof(CHAR));

   if (AllocateDestinationString)
   {
      OemDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      OemDest->MaximumLength = Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > OemDest->MaximumLength)
   {
      //make room for nullterm
      OemDest->Length = OemDest->MaximumLength - sizeof(CHAR);
   }

   Status = RtlUnicodeToOemN (OemDest->Buffer,
                              OemDest->Length,
                              NULL,
                              UniSource->Buffer,
                              UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool(OemDest->Buffer);
      return Status;
   }

   OemDest->Buffer[OemDest->Length] = 0;
   return Status;
}



#define ITU_IMPLEMENTED_TESTS (IS_TEXT_UNICODE_ODD_LENGTH|IS_TEXT_UNICODE_SIGNATURE)


/*
 * @implemented
 *
 * RETURNS
 *  The length of the string if all tests were passed, 0 otherwise.
 */
ULONG STDCALL
RtlIsTextUnicode (PVOID Buffer,
                  ULONG Length,
                  ULONG *Flags)
{
   PWSTR s = Buffer;
   ULONG in_flags = (ULONG)-1;
   ULONG out_flags = 0;

   if (Length == 0)
      goto done;

   if (Flags != 0)
      in_flags = *Flags;

   /*
    * Apply various tests to the text string. According to the
    * docs, each test "passed" sets the corresponding flag in
    * the output flags. But some of the tests are mutually
    * exclusive, so I don't see how you could pass all tests ...
    */

   /* Check for an odd length ... pass if even. */
   if (!(Length & 1))
      out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

   /* Check for the BOM (byte order mark). */
   if (*s == 0xFEFF)
      out_flags |= IS_TEXT_UNICODE_SIGNATURE;

#if 0
   /* Check for the reverse BOM (byte order mark). */
   if (*s == 0xFFFE)
      out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
#endif

   /* FIXME: Add more tests */

   /*
    * Check whether the string passed all of the tests.
    */
   in_flags &= ITU_IMPLEMENTED_TESTS;
   if ((out_flags & in_flags) != in_flags)
      Length = 0;

done:
   if (Flags != 0)
      *Flags = out_flags;

   return Length;
}


/*
 * @implemented
 *
 * NOTES
 *  See RtlpOemStringToCountedUnicodeString
 */
NTSTATUS
STDCALL
RtlOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpOemStringToCountedUnicodeString(
             UniDest,
             OemSource,
             AllocateDestinationString,
             NonPagedPool);
}


/*
 * private
 *
 
 * NOTES
 *  Same as RtlOemStringToUnicodeString but doesn't write terminating null
 *  A partial copy is NOT performed if the dest buffer is too small!
 */
NTSTATUS
FASTCALL
RtlpOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN POEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status;
   ULONG Length; //excluding nullterm

   if (NlsMbCodePageTag == TRUE)
      Length = RtlOemStringToUnicodeSize(OemSource) - sizeof(WCHAR);
   else
      Length = OemSource->Length * sizeof(WCHAR);

   if (Length > 65535)
      return STATUS_INVALID_PARAMETER_2;

   if (AllocateDestinationString == TRUE)
   {
      UniDest->Buffer = ExAllocatePoolWithTag (PoolType, Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = Length;
   }
   else if (Length > UniDest->MaximumLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = Length;

   Status = RtlOemToUnicodeN (UniDest->Buffer,
                              UniDest->Length,
                              NULL,
                              OemSource->Buffer,
                              OemSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool (UniDest->Buffer);
      return Status;
   }

   return Status;
}


//FIXME: sjekk returnverdi og returtype. Wine koflikter mht. returtype for EqualString og EqualComputer---
/*
 * @implemented
 *
 * RETURNS
 *  0 if the names are equal, non-zero otherwise.
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
STDCALL
RtlEqualComputerName(
   IN PUNICODE_STRING ComputerName1,
   IN PUNICODE_STRING ComputerName2)
{
   OEM_STRING OemString1;
   OEM_STRING OemString2;
   BOOLEAN Result = FALSE;

   if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString( &OemString1, ComputerName1, TRUE )))
   {
      if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString( &OemString2, ComputerName2, TRUE )))
      {
         Result = RtlEqualString( &OemString1, &OemString2, FALSE );
         RtlFreeOemString( &OemString2 );
      }
      RtlFreeOemString( &OemString1 );
   }

   return Result;
}

//FIXME: sjekk returnverdi og returtype. Wine koflikter mht. returtype for EqualString og EqualComputer---
/*
 * @implemented
 *
 * RETURNS
 *  0 if the names are equal, non-zero otherwise.
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
STDCALL
RtlEqualDomainName (
   IN PUNICODE_STRING DomainName1,
   IN PUNICODE_STRING DomainName2
)
{
   return RtlEqualComputerName(DomainName1, DomainName2);
}


/*
 * @implemented
 */
/*
BOOLEAN
STDCALL
RtlEqualDomainName (
   IN PUNICODE_STRING DomainName1,
   IN PUNICODE_STRING DomainName2
)
{
   OEM_STRING OemString1;
   OEM_STRING OemString2;
   BOOLEAN Result;
 
   RtlUpcaseUnicodeStringToOemString (&OemString1,
                                      DomainName1,
                                      TRUE);
   RtlUpcaseUnicodeStringToOemString (&OemString2,
                                      DomainName2,
                                      TRUE);
 
   Result = RtlEqualString (&OemString1,
                            &OemString2,
                            FALSE);
 
   RtlFreeOemString (&OemString1);
   RtlFreeOemString (&OemString2);
 
   return Result;
   
}
*/

/*
 * @implemented
 *
 * RIPPED FROM WINE's ntdll\rtlstr.c rev 1.45
 *
 * Convert a string representation of a GUID into a GUID.
 *
 * PARAMS
 *  str  [I] String representation in the format "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
 *  guid [O] Destination for the converted GUID
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. guid contains the converted value.
 *  Failure: STATUS_INVALID_PARAMETER, if str is not in the expected format.
 *
 * SEE ALSO
 *  See RtlStringFromGUID.
 */
NTSTATUS
STDCALL
RtlGUIDFromString(
   IN UNICODE_STRING *str,
   OUT GUID* guid
)
{
   int i = 0;
   const WCHAR *lpszCLSID = str->Buffer;
   BYTE* lpOut = (BYTE*)guid;

   //TRACE("(%s,%p)\n", debugstr_us(str), guid);

   /* Convert string: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
    * to memory:       DWORD... WORD WORD BYTES............
    */
   while (i < 37)
   {
      switch (i)
      {
         case 0:
            if (*lpszCLSID != '{')
               return STATUS_INVALID_PARAMETER;
            break;

         case 9:
         case 14:
         case 19:
         case 24:
            if (*lpszCLSID != '-')
               return STATUS_INVALID_PARAMETER;
            break;

         case 37:
            if (*lpszCLSID != '}')
               return STATUS_INVALID_PARAMETER;
            break;

         default:
            {
               WCHAR ch = *lpszCLSID, ch2 = lpszCLSID[1];
               unsigned char byte;

               /* Read two hex digits as a byte value */
               if      (ch >= '0' && ch <= '9')
                  ch = ch - '0';
               else if (ch >= 'a' && ch <= 'f')
                  ch = ch - 'a' + 10;
               else if (ch >= 'A' && ch <= 'F')
                  ch = ch - 'A' + 10;
               else
                  return STATUS_INVALID_PARAMETER;

               if      (ch2 >= '0' && ch2 <= '9')
                  ch2 = ch2 - '0';
               else if (ch2 >= 'a' && ch2 <= 'f')
                  ch2 = ch2 - 'a' + 10;
               else if (ch2 >= 'A' && ch2 <= 'F')
                  ch2 = ch2 - 'A' + 10;
               else
                  return STATUS_INVALID_PARAMETER;

               byte = ch << 4 | ch2;

               switch (i)
               {
#ifndef WORDS_BIGENDIAN
                     /* For Big Endian machines, we store the data such that the
                      * dword/word members can be read as DWORDS and WORDS correctly. */
                     /* Dword */
                  case 1:
                     lpOut[3] = byte;
                     break;
                  case 3:
                     lpOut[2] = byte;
                     break;
                  case 5:
                     lpOut[1] = byte;
                     break;
                  case 7:
                     lpOut[0] = byte;
                     lpOut += 4;
                     break;
                     /* Word */
                  case 10:
                  case 15:
                     lpOut[1] = byte;
                     break;
                  case 12:
                  case 17:
                     lpOut[0] = byte;
                     lpOut += 2;
                     break;
#endif
                     /* Byte */
                  default:
                     lpOut[0] = byte;
                     lpOut++;
                     break;
               }
               lpszCLSID++; /* Skip 2nd character of byte */
               i++;
            }
      }
      lpszCLSID++;
      i++;
   }

   return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlInt64ToUnicodeString (IN ULONGLONG Value,
                         IN ULONG Base,
                         OUT PUNICODE_STRING String)
{
   return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlEraseUnicodeString(
   IN PUNICODE_STRING String)
{
   if (String->Buffer == NULL)
      return;

   if (String->MaximumLength == 0)
      return;

   memset (String->Buffer,
           0,
           String->MaximumLength);

   String->Length = 0;
}



/*
 * @implemented
 *
 * NOTES
 *  See RtlpUnicodeStringToCountedOemString.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpUnicodeStringToCountedOemString(
             OemDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}

/*
 * private
 *
 * NOTES
 *  Same as RtlUnicodeStringToOemString but doesn't write terminating null
 *  Does a partial copy if the dest buffer is too small
 */
NTSTATUS
FASTCALL
RtlpUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status;
   ULONG Length; //excluding nullterm

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource) - sizeof(CHAR);
   else
      Length = (UniSource->Length / sizeof(WCHAR));

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   OemDest->Length = (WORD)(Length);

   if (AllocateDestinationString)
   {
      OemDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      OemDest->MaximumLength = Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > OemDest->MaximumLength)
   {
      OemDest->Length = OemDest->MaximumLength;
   }

   Status = RtlUnicodeToOemN (OemDest->Buffer,
                              OemDest->Length,
                              NULL,
                              UniSource->Buffer,
                              UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool(OemDest->Buffer);
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlLargeIntegerToChar(
   IN PLARGE_INTEGER Value,
   IN ULONG  Base,
   IN ULONG  Length,
   IN OUT PCHAR  String)
{
   ULONG Radix;
   CHAR  temp[65];
   ULONGLONG v = Value->QuadPart;
   ULONG i;
   PCHAR tp;
   PCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
         (Radix != 10) && (Radix != 16))
      return STATUS_INVALID_PARAMETER;

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + '0';
      else
         *tp = i + 'a' - 10;
      tp++;
   }

   if (tp - temp >= Length)
      return STATUS_BUFFER_TOO_SMALL;

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}



/*
 * @implemented
 *
 * NOTES
 *  See RtlpUpcaseUnicodeString
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{

   return RtlpUpcaseUnicodeString(
             UniDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}


/*
 * private
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
NTSTATUS
FASTCALL
RtlpUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   ULONG i;
   PWCHAR Src, Dest;

   if (AllocateDestinationString == TRUE)
   {
      UniDest->MaximumLength = UniSource->Length;
      UniDest->Buffer = ExAllocatePoolWithTag(PoolType, UniDest->MaximumLength, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;
   }
   else if (UniSource->Length > UniDest->MaximumLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = UniSource->Length;

   Src = UniSource->Buffer;
   Dest = UniDest->Buffer;
   for (i = 0; i < UniSource->Length / sizeof(WCHAR); i++)
   {
      *Dest = RtlUpcaseUnicodeChar (*Src);
      Dest++;
      Src++;
   }

   return STATUS_SUCCESS;
}



/*
 * @implemented
 *
 * NOTES
 *  See RtlpUpcaseUnicodeStringToAnsiString
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
   return RtlpUpcaseUnicodeStringToAnsiString(
             AnsiDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}

/*
 * private
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
FASTCALL
RtlpUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status;
   ULONG Length; //including nullterm

   if (NlsMbCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource);
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   AnsiDest->Length = (WORD)(Length - sizeof(CHAR));

   if (AllocateDestinationString)
   {
      AnsiDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_ASTR);
      if (AnsiDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      AnsiDest->MaximumLength = Length;
   }
   else if (AnsiDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   else if (Length > AnsiDest->MaximumLength)
   {
      //make room for nullterm
      AnsiDest->Length = AnsiDest->MaximumLength - sizeof(CHAR);
   }

   //FIXME: do we need this??????? -Gunnar
   RtlZeroMemory (AnsiDest->Buffer,
                  AnsiDest->Length);

   Status = RtlUpcaseUnicodeToMultiByteN (AnsiDest->Buffer,
                                          AnsiDest->Length,
                                          NULL,
                                          UniSource->Buffer,
                                          UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool(AnsiDest->Buffer);
      return Status;
   }

   AnsiDest->Buffer[AnsiDest->Length] = 0;
   return Status;
}



/*
 * @implemented
 *
 * NOTES
 *  See RtlpUpcaseUnicodeStringToCountedOemString
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpUpcaseUnicodeStringToCountedOemString(
             OemDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}



/*
 * private
 *
 * NOTES
 *  Same as RtlUpcaseUnicodeStringToOemString but doesn't write terminating null
 *  It performs a partial copy if oem is too small.
 */
NTSTATUS
FASTCALL
RtlpUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status;
   ULONG Length; //excluding nullterm

   if (NlsMbCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource) - sizeof(CHAR);
   else
      Length = UniSource->Length / sizeof(WCHAR);

   if (Length > 0x0000FFFF)
      return(STATUS_INVALID_PARAMETER_2);

   OemDest->Length = (WORD)(Length);

   if (AllocateDestinationString)
   {
      OemDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return(STATUS_NO_MEMORY);

      //FIXME: Do we need this?????
      RtlZeroMemory (OemDest->Buffer, Length);

      OemDest->MaximumLength = (WORD)Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return(STATUS_BUFFER_TOO_SMALL);
   }
   else if (Length > OemDest->MaximumLength)
   {
      OemDest->Length = OemDest->MaximumLength;
   }

   Status = RtlUpcaseUnicodeToOemN(OemDest->Buffer,
                                   OemDest->Length,
                                   NULL,
                                   UniSource->Buffer,
                                   UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool(OemDest->Buffer);
      return Status;
   }

   return Status;
}


/*
 * @implemented
 * NOTES
 *  See RtlpUpcaseUnicodeStringToOemString
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString
)
{
   return RtlpUpcaseUnicodeStringToOemString(
             OemDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}


/*
 * private
 *
 * NOTES
 *  Oem string is allways nullterminated 
 *  It performs a partial copy if oem is too small.
 */
NTSTATUS
FASTCALL
RtlpUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString,
   IN POOL_TYPE PoolType
)
{
   NTSTATUS Status;
   ULONG Length; //including nullterm

   if (NlsMbOemCodePageTag == TRUE)
      Length = RtlUnicodeStringToAnsiSize(UniSource);
   else
      Length = (UniSource->Length / sizeof(WCHAR)) + sizeof(CHAR);

   if (Length > 0x0000FFFF)
      return STATUS_INVALID_PARAMETER_2;

   OemDest->Length = (WORD)(Length - sizeof(CHAR));

   if (AllocateDestinationString)
   {
      OemDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_OSTR);
      if (OemDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      //FIXME: Do we need this????
      RtlZeroMemory (OemDest->Buffer, Length);

      OemDest->MaximumLength = (WORD)Length;
   }
   else if (OemDest->MaximumLength == 0)
   {
      return STATUS_BUFFER_OVERFLOW;
   }
   else if (Length > OemDest->MaximumLength)
   {
      //make room for nullterm
      OemDest->Length = OemDest->MaximumLength - sizeof(CHAR);
   }

   Status = RtlUpcaseUnicodeToOemN (OemDest->Buffer,
                                    OemDest->Length,
                                    NULL,
                                    UniSource->Buffer,
                                    UniSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool(OemDest->Buffer);
      return Status;
   }

   OemDest->Buffer[OemDest->Length] = 0;
   return Status;
}


/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
STDCALL
RtlOemStringToUnicodeSize(IN POEM_STRING OemString)
{
   ULONG Size;

   //this function returns size including nullterm
   RtlMultiByteToUnicodeSize(&Size,
                             OemString->Buffer,
                             OemString->Length);

   return(Size);
}



/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlStringFromGUID (IN REFGUID Guid,
                   OUT PUNICODE_STRING GuidString)
{
   STATIC CONST PWCHAR Hex = L"0123456789ABCDEF";
   WCHAR Buffer[40];
   PWCHAR BufferPtr;
   ULONG i;

   if (Guid == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }

   swprintf (Buffer,
             L"{%08lX-%04X-%04X-%02X%02X-",
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1]);
   BufferPtr = Buffer + 25;

   /* 6 hex bytes */
   for (i = 2; i < 8; i++)
   {
      *BufferPtr++ = Hex[Guid->Data4[i] >> 4];
      *BufferPtr++ = Hex[Guid->Data4[i] & 0xf];
   }

   *BufferPtr++ = L'}';
   *BufferPtr++ = L'\0';

   return RtlCreateUnicodeString (GuidString, Buffer);
}


/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
STDCALL
RtlUnicodeStringToAnsiSize(
   IN PUNICODE_STRING UnicodeString)
{
   ULONG Size;

   //this function return size without nullterm!
   RtlUnicodeToMultiByteSize (&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

   return Size + sizeof(CHAR); //NB: incl. nullterm
}




/*
 * @implemented
 */
LONG
STDCALL
RtlCompareUnicodeString(
   IN PUNICODE_STRING String1,
   IN PUNICODE_STRING String2,
   IN BOOLEAN  CaseInsensitive)
{
   ULONG len1, len2;
   PWCHAR s1, s2;
   WCHAR  c1, c2;

   if (String1 && String2)
   {
      len1 = String1->Length / sizeof(WCHAR);
      len2 = String2->Length / sizeof(WCHAR);
      s1 = String1->Buffer;
      s2 = String2->Buffer;

      if (s1 && s2)
      {
         if (CaseInsensitive)
         {
            while (1)
            {
               c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
               c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
         else
         {
            while (1)
            {
               c1 = len1-- ? *s1++ : 0;
               c2 = len2-- ? *s2++ : 0;
               if (!c1 || !c2 || c1 != c2)
                  return c1 - c2;
            }
         }
      }
   }

   return 0;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlCopyString(
   IN OUT PSTRING DestinationString,
   IN PSTRING SourceString)
{
   ULONG copylen;

   if(SourceString == NULL)
   {
      DestinationString->Length = 0;
      return;
   }

   copylen = min (DestinationString->MaximumLength - sizeof(CHAR),
                  SourceString->Length);

   memcpy(DestinationString->Buffer, SourceString->Buffer, copylen);
   DestinationString->Buffer[copylen] = 0;
   DestinationString->Length = copylen;
}



/*
 * @implemented
 */
VOID
STDCALL
RtlCopyUnicodeString(
   IN OUT PUNICODE_STRING DestinationString,
   IN PUNICODE_STRING SourceString)
{
   ULONG copylen;

   if (SourceString == NULL)
   {
      DestinationString->Length = 0;
      return;
   }

   copylen = min (DestinationString->MaximumLength - sizeof(WCHAR),
                  SourceString->Length);
   memcpy(DestinationString->Buffer, SourceString->Buffer, copylen);
   DestinationString->Buffer[copylen / sizeof(WCHAR)] = 0;
   DestinationString->Length = copylen;
}


/*
 * @implemented
 *
 * NOTES
 *  See RtlpCreateUnicodeString
 */
BOOLEAN
STDCALL
RtlCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PWSTR  Source)
{

   DPRINT("RtlCreateUnicodeString\n");
   return RtlpCreateUnicodeString(UniDest, Source, NonPagedPool);
}


/*
 * private
 */
BOOLEAN
FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PWSTR  Source,
   IN POOL_TYPE PoolType)
{
   ULONG Length;

   Length = (wcslen (Source) + 1) * sizeof(WCHAR);

   UniDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_USTR);
   if (UniDest->Buffer == NULL)
      return FALSE;

   memmove (UniDest->Buffer,
            Source,
            Length);

   UniDest->MaximumLength = Length;
   UniDest->Length = Length - sizeof (WCHAR);

   return TRUE;
}



/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz(
   OUT PUNICODE_STRING Destination,
   IN PCSZ  Source)
{
   ANSI_STRING AnsiString;
   NTSTATUS Status;

   RtlInitAnsiString (&AnsiString,
                      Source);

   Status = RtlAnsiStringToUnicodeString (Destination,
                                          &AnsiString,
                                          TRUE);

   return NT_SUCCESS(Status);
}



/*
 * @implemented
 *
 * NOTES
 *  See RtlpDowncaseUnicodeString
 */
NTSTATUS STDCALL
RtlDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpDowncaseUnicodeString(
             UniDest,
             UniSource,
             AllocateDestinationString,
             NonPagedPool);
}


/*
 * private
 *
 * NOTES
 *  Dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. 
 *  Dest->Length is only set upon success.
 */
NTSTATUS
FASTCALL
RtlpDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   ULONG i;
   PWCHAR Src, Dest;

   if (AllocateDestinationString)
   {
      UniDest->Buffer = ExAllocatePoolWithTag(PoolType, UniSource->Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = UniSource->Length;
   }
   else if (UniSource->Length > UniDest->MaximumLength)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = UniSource->Length;

   Src = UniSource->Buffer;
   Dest = UniDest->Buffer;
   for (i=0; i < UniSource->Length / sizeof(WCHAR); i++)
   {
      if (*Src < L'A')
      {
         *Dest = *Src;
      }
      else if (*Src <= L'Z')
      {
         *Dest = (*Src + (L'a' - L'A'));
      }
      else
      {
         *Dest = RtlDowncaseUnicodeChar(*Src);
      }

      Dest++;
      Src++;
   }

   return STATUS_SUCCESS;
}



/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is '\0' terminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the '\0' is ommitted.
 */
NTSTATUS STDCALL
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
                         IN PWSTR Source)
{
   ULONG slen;

   slen = wcslen(Source) * sizeof(WCHAR);

   if (Destination->Length + slen > Destination->MaximumLength)
      return(STATUS_BUFFER_TOO_SMALL);

   memcpy((char*)Destination->Buffer + Destination->Length, Source, slen);
   Destination->Length += slen;
   /* append terminating '\0' if enough space */
   if( Destination->MaximumLength > Destination->Length )
      Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 *
 * NOTES
 *  See RtlpAnsiStringToUnicodeString
 */
NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PANSI_STRING AnsiSource,
   IN BOOLEAN AllocateDestinationString)
{
   return RtlpAnsiStringToUnicodeString(
             UniDest,
             AnsiSource,
             AllocateDestinationString,
             NonPagedPool);
}



/*
 * private
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  If the dest buffer is too small a partial copy is NOT performed!
 */
NTSTATUS
FASTCALL
RtlpAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PANSI_STRING AnsiSource,
   IN BOOLEAN AllocateDestinationString,
   IN POOL_TYPE PoolType)
{
   NTSTATUS Status;
   ULONG Length; //including nullterm

   if (NlsMbCodePageTag == TRUE)
      Length = RtlAnsiStringToUnicodeSize(AnsiSource);
   else
      Length = (AnsiSource->Length * sizeof(WCHAR)) + sizeof(WCHAR);

   if (Length > 0xffff)
      return STATUS_INVALID_PARAMETER_2;

   if (AllocateDestinationString == TRUE)
   {
      UniDest->Buffer = ExAllocatePoolWithTag(PoolType, Length, TAG_USTR);
      if (UniDest->Buffer == NULL)
         return STATUS_NO_MEMORY;

      UniDest->MaximumLength = Length;
   }
   else if (Length > UniDest->MaximumLength)
   {
      DPRINT("STATUS_BUFFER_TOO_SMALL\n");
      return STATUS_BUFFER_TOO_SMALL;
   }

   UniDest->Length = Length - sizeof(WCHAR);

   //FIXME: We don't need this??? -Gunnar
   RtlZeroMemory (UniDest->Buffer,
                  UniDest->Length);

   Status = RtlMultiByteToUnicodeN (UniDest->Buffer,
                                    UniDest->Length,
                                    NULL,
                                    AnsiSource->Buffer,
                                    AnsiSource->Length);

   if (!NT_SUCCESS(Status) && AllocateDestinationString)
   {
      ExFreePool(UniDest->Buffer);
      return Status;
   }

   UniDest->Buffer[UniDest->Length / sizeof(WCHAR)] = 0;
   return Status;
}



/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is never '\0' terminated.
 */
NTSTATUS
STDCALL
RtlAppendAsciizToString(
   IN OUT   PSTRING  Destination,
   IN PCSZ  Source)
{
   ULONG Length;
   PCHAR Ptr;

   if (Source == NULL)
      return STATUS_SUCCESS;

   Length = strlen (Source);
   if (Destination->Length + Length >= Destination->MaximumLength)
      return STATUS_BUFFER_TOO_SMALL;

   Ptr = Destination->Buffer + Destination->Length;
   memmove (Ptr,
            Source,
            Length);
   Ptr += Length;
   *Ptr = 0;

   Destination->Length += Length;

   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID STDCALL
RtlUpperString(PSTRING DestinationString,
               PSTRING SourceString)
{
   ULONG Length;
   ULONG i;
   PCHAR Src;
   PCHAR Dest;

   Length = min(SourceString->Length,
                DestinationString->MaximumLength - 1);

   Src = SourceString->Buffer;
   Dest = DestinationString->Buffer;
   for (i = 0; i < Length; i++)
   {
      *Dest = RtlUpperChar(*Src);
      Src++;
      Dest++;
   }
   *Dest = 0;

   DestinationString->Length = SourceString->Length;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlxAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
   return RtlAnsiStringToUnicodeSize(AnsiString);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlxOemStringToUnicodeSize(IN POEM_STRING OemString)
{
   return RtlOemStringToUnicodeSize((PANSI_STRING)OemString);
}



/*
 * @implemented
 */
ULONG STDCALL
RtlxUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString)
{
   return RtlUnicodeStringToAnsiSize(UnicodeString);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlxUnicodeStringToOemSize(IN PUNICODE_STRING UnicodeString)
{
   return RtlUnicodeStringToOemSize(UnicodeString);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDuplicateUnicodeString(
   INT AddNull,
   IN PUNICODE_STRING SourceString,
   PUNICODE_STRING DestinationString)
{
   if (SourceString == NULL || DestinationString == NULL)
      return STATUS_INVALID_PARAMETER;


   if (SourceString->Length == 0 && AddNull != 3)
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
      DestinationString->Buffer = NULL;
   }
   else
   {
      unsigned int DestMaxLength = SourceString->Length;

      if (AddNull)
         DestMaxLength += sizeof(UNICODE_NULL);

      DestinationString->Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, DestMaxLength);
      if (DestinationString->Buffer == NULL)
         return STATUS_NO_MEMORY;

      RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
      DestinationString->Length = SourceString->Length;
      DestinationString->MaximumLength = DestMaxLength;

      if (AddNull)
         DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
   }

   return STATUS_SUCCESS;
}

/* EOF */
