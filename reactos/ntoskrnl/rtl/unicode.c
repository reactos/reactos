/* $Id: unicode.c,v 1.37 2004/02/02 19:04:11 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/unicode.c
 * PURPOSE:         String functions
 * PROGRAMMERS:     Jason Filby (jasonfilby@yahoo.com)
 *                  Vizzini (vizzini@plasmic.com)
 * UPDATE HISTORY:
 *                  Created 10/08/98
 */

#include <ddk/ntddk.h>
#include <internal/ctype.h>
#include <ntos/minmax.h>
#include <internal/pool.h>
#include <internal/nls.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

#define TAG_USTR  TAG('U', 'S', 'T', 'R')
#define TAG_ASTR  TAG('A', 'S', 'T', 'R')
#define TAG_OSTR  TAG('O', 'S', 'T', 'R')


/* FUNCTIONS *****************************************************************/

WCHAR
STDCALL
RtlAnsiCharToUnicodeChar(IN CHAR AnsiChar)
{
  ULONG Size;
  WCHAR UnicodeChar;

  Size = 1;
#if 0
  Size = (NlsLeadByteInfo[AnsiChar] == 0) ? 1 : 2;
#endif

  RtlMultiByteToUnicodeN(&UnicodeChar,
			 sizeof(WCHAR),
			 NULL,
			 &AnsiChar,
			 Size);

  return(UnicodeChar);
}


/*
 * @implemented
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
 */
NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(IN OUT PUNICODE_STRING DestinationString,
			     IN PANSI_STRING SourceString,
			     IN BOOLEAN AllocateDestinationString)
{
	NTSTATUS Status;
	ULONG Length;

	if (NlsMbCodePageTag == TRUE)
		Length = RtlAnsiStringToUnicodeSize (SourceString);
	else
		Length = SourceString->Length * sizeof(WCHAR);

	if (Length > 65535)
		return STATUS_INVALID_PARAMETER_2;

	if (AllocateDestinationString == TRUE)
	{
		DestinationString->MaximumLength = Length + sizeof(WCHAR);
		DestinationString->Buffer =
		  ExAllocatePoolWithTag (NonPagedPool,
					 DestinationString->MaximumLength,
					 TAG_USTR);
		if (DestinationString->Buffer == NULL)
			return STATUS_NO_MEMORY;
	}
	else
	{
		if (Length + sizeof(WCHAR) > DestinationString->MaximumLength)
		{
			DPRINT("STATUS_BUFFER_TOO_SMALL\n");
			return STATUS_BUFFER_TOO_SMALL;
		}
	}
	DestinationString->Length = Length;

	RtlZeroMemory (DestinationString->Buffer,
	               DestinationString->Length);

	Status = RtlMultiByteToUnicodeN (DestinationString->Buffer,
	                                 DestinationString->Length,
	                                 NULL,
	                                 SourceString->Buffer,
	                                 SourceString->Length);
	if (!NT_SUCCESS(Status))
	{
		if (AllocateDestinationString)
			ExFreePool (DestinationString->Buffer);
		return Status;
	}

	DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;

	return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlAppendAsciizToString(IN OUT PSTRING Destination,
			IN PCSZ Source)
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
 */
NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(IN OUT PUNICODE_STRING Destination,
			       IN PUNICODE_STRING Source)
{

	if ((Source->Length + Destination->Length) >= Destination->MaximumLength)
		return STATUS_BUFFER_TOO_SMALL;

	memcpy((char*)Destination->Buffer + Destination->Length, Source->Buffer, Source->Length);
	Destination->Length += Source->Length;
	Destination->Buffer[Destination->Length / sizeof(WCHAR)] = 0;

	return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
			 IN PWSTR Source)
{
  ULONG  slen;

  slen = wcslen(Source) * sizeof(WCHAR);

  if (Destination->Length + slen >= Destination->MaximumLength)
    return(STATUS_BUFFER_TOO_SMALL);

  memcpy((char*)Destination->Buffer + Destination->Length, Source, slen + sizeof(WCHAR));
  Destination->Length += slen;

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlCharToInteger(IN PCSZ String,
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
    return(STATUS_INVALID_PARAMETER);

  while (isxdigit (*String) &&
	 (Val = isdigit (*String) ? * String - '0' : (islower (*String)
	  ? toupper (*String) : *String) - 'A' + 10) < Base)
    {
      *Value = *Value * Base + Val;
      String++;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
LONG
STDCALL
RtlCompareString(IN PSTRING String1,
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
 */
LONG
STDCALL
RtlCompareUnicodeString(IN PUNICODE_STRING String1,
			IN PUNICODE_STRING String2,
			IN BOOLEAN CaseInsensitive)
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
RtlCopyString(IN OUT PSTRING DestinationString,
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
RtlCopyUnicodeString(IN OUT PUNICODE_STRING DestinationString,
		     IN PUNICODE_STRING SourceString)
{
	ULONG copylen;

	if(SourceString==NULL)
	{
		DestinationString->Length=0;
		return;
	}

	copylen = min(DestinationString->MaximumLength - sizeof(WCHAR),
	              SourceString->Length);
	memcpy(DestinationString->Buffer, SourceString->Buffer, copylen);
	DestinationString->Buffer[copylen / sizeof(WCHAR)] = 0;
	DestinationString->Length = copylen;
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlCreateUnicodeString(IN OUT PUNICODE_STRING Destination,
		       IN PWSTR Source)
{
	ULONG Length;

	Length = (wcslen (Source) + 1) * sizeof(WCHAR);

	Destination->Buffer = ExAllocatePoolWithTag (NonPagedPool,
						     Length,
						     TAG_USTR);
	if (Destination->Buffer == NULL)
		return FALSE;

	memmove (Destination->Buffer,
	         Source,
	         Length);

	Destination->MaximumLength = Length;
	Destination->Length = Length - sizeof (WCHAR);

	return TRUE;
}


BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz(IN OUT PUNICODE_STRING Destination,
				 IN PCSZ Source)
{
  ANSI_STRING AnsiString;
  NTSTATUS Status;

  RtlInitAnsiString(&AnsiString,
		    Source);

  Status = RtlAnsiStringToUnicodeString(Destination,
					&AnsiString,
					TRUE);

  return(NT_SUCCESS(Status));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDowncaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
			 IN PUNICODE_STRING SourceString,
			 IN BOOLEAN AllocateDestinationString)
{
  ULONG i;
  PWCHAR Src, Dest;

  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
      DestinationString->Buffer = 
	ExAllocatePoolWithTag (NonPagedPool,
			       SourceString->Length + sizeof(WCHAR),
			       TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (SourceString->Length >= DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = SourceString->Length;

  Src = SourceString->Buffer;
  Dest = DestinationString->Buffer;
  for (i=0; i < SourceString->Length / sizeof(WCHAR); i++)
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
  *Dest = 0;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlEqualString(IN PSTRING String1,
	       IN PSTRING String2,
	       IN BOOLEAN CaseInsensitive)
{
	unsigned long s1l=String1->Length;
	unsigned long s2l=String2->Length;
	unsigned long i;
	char c1, c2;

	if (s1l != s2l)
		return FALSE;

	for (i = 0; i < s1l; i++)
	{
		c1 = *String1->Buffer;
		c2 = *String2->Buffer;

		if (CaseInsensitive == TRUE)
		{
			c1 = RtlUpperChar (c1);
			c2 = RtlUpperChar (c2);
		}

		if (c1 != c2)
		{
			String1->Buffer -= i;
			String2->Buffer -= i;
			return FALSE;
		}

		String1->Buffer++;
		String2->Buffer++;
	}

	String1->Buffer -= i;
	String2->Buffer -= i;

	return TRUE;
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlEqualUnicodeString(IN PUNICODE_STRING String1,
		      IN PUNICODE_STRING String2,
		      IN BOOLEAN CaseInsensitive)
{
	unsigned long s1l = String1->Length / sizeof(WCHAR);
	unsigned long s2l = String2->Length / sizeof(WCHAR);
	unsigned long i;
	WCHAR wc1, wc2;
	PWCHAR pw1, pw2;

	if (s1l != s2l)
		return FALSE;

	pw1 = String1->Buffer;
	pw2 = String2->Buffer;

	for (i = 0; i < s1l; i++)
	{
		if(CaseInsensitive == TRUE)
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

	ExFreePool (AnsiString->Buffer);

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

	ExFreePool (OemString->Buffer);

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

	ExFreePool (UnicodeString->Buffer);

	UnicodeString->Buffer = NULL;
	UnicodeString->Length = 0;
	UnicodeString->MaximumLength = 0;
}


/*
 * @implemented
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
 */
VOID
STDCALL
RtlInitString(IN OUT PSTRING DestinationString,
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
 */
NTSTATUS
STDCALL
RtlIntegerToChar(IN ULONG Value,
		 IN ULONG Base,
		 IN ULONG Length,
		 IN OUT PCHAR String)
{
	ULONG Radix;
	CHAR  temp[33];
	ULONG v = 0;
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

	if ((ULONG) (tp - temp) >= Length)
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
RtlIntegerToUnicodeString(IN ULONG Value,
			  IN ULONG Base,	/* optional */
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
 */
NTSTATUS
STDCALL
RtlOemStringToCountedUnicodeString(IN OUT PUNICODE_STRING DestinationString,
				   IN POEM_STRING SourceString,
				   IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE)
    Length = RtlAnsiStringToUnicodeSize (SourceString);
  else
    Length = SourceString->Length * sizeof(WCHAR);
  
  if (Length > 65535)
    return STATUS_INVALID_PARAMETER_2;
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(WCHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length > DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory (DestinationString->Buffer,
		 DestinationString->Length);
  
  Status = RtlOemToUnicodeN (DestinationString->Buffer,
			     DestinationString->Length,
			     NULL,
			     SourceString->Buffer,
			     SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);
      
      return Status;
    }

  DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;
  
  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
ULONG
STDCALL
RtlOemStringToUnicodeSize(IN POEM_STRING OemString)
{
  ULONG Size;

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
RtlOemStringToUnicodeString(IN OUT PUNICODE_STRING DestinationString,
			    IN POEM_STRING SourceString,
			    IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE)
    Length = RtlAnsiStringToUnicodeSize (SourceString);
  else
    Length = SourceString->Length * sizeof(WCHAR);
  
  if (Length > 65535)
    return STATUS_INVALID_PARAMETER_2;
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(WCHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length + sizeof(WCHAR) > DestinationString->MaximumLength)
      {
	DPRINT("STATUS_BUFFER_TOO_SMALL\n");
	return STATUS_BUFFER_TOO_SMALL;
      }
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory (DestinationString->Buffer,
		 DestinationString->Length);
  
  Status = RtlOemToUnicodeN (DestinationString->Buffer,
			     DestinationString->Length,
			     NULL,
			     SourceString->Buffer,
	                           SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);
      return Status;
    }

  DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;
  
  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlPrefixString(IN PANSI_STRING String1,
		IN PANSI_STRING String2,
		IN BOOLEAN CaseInsensitive)
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
 */
BOOLEAN
STDCALL
RtlPrefixUnicodeString(IN PUNICODE_STRING String1,
		       IN PUNICODE_STRING String2,
		       IN BOOLEAN CaseInsensitive)
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
 */
NTSTATUS
STDCALL
RtlStringFromGUID(
    IN REFGUID Guid,
    OUT PUNICODE_STRING GuidString
    )
{
	STATIC CONST PWCHAR Hex = L"0123456789ABCDEF";
	WCHAR Buffer[40];
	PWCHAR BufferPtr;
	INT i;
	
	if( Guid == NULL )
	{
		return STATUS_INVALID_PARAMETER;
	}

	swprintf( Buffer, L"{%08lX-%04X-%04X-%02X%02X-",
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

	*BufferPtr++ = '}';
	*BufferPtr++ = '\0';
		
	return RtlCreateUnicodeString( GuidString, Buffer );
}


/*
 * @implemented
 */
ULONG
STDCALL
RtlUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString)
{
  ULONG Size;

  RtlUnicodeToMultiByteSize(&Size,
			    UnicodeString->Buffer,
			    UnicodeString->Length);

  return(Size+1);
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
			     IN PUNICODE_STRING SourceString,
			     IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE){
    Length = RtlUnicodeStringToAnsiSize (SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_ASTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory (DestinationString->Buffer,
		 DestinationString->Length);
  
  Status = RtlUnicodeToMultiByteN (DestinationString->Buffer,
				   DestinationString->Length,
				   NULL,
				   SourceString->Buffer,
				   SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);
		return Status;
    }
  
  DestinationString->Buffer[Length] = 0;
  
  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlUnicodeStringToCountedOemString(IN OUT POEM_STRING DestinationString,
				   IN PUNICODE_STRING SourceString,
				   IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  ULONG Size;
  
  if (NlsMbOemCodePageTag == TRUE)
    Length = RtlUnicodeStringToAnsiSize (SourceString)/* + 1*/;
  else
    Length = SourceString->Length / sizeof(WCHAR) + 1;
  
  if (Length > 0x0000FFFF)
    return STATUS_INVALID_PARAMETER_2;
  
  DestinationString->Length = (WORD)(Length - 1);
  
  if (AllocateDestinationString)
    {
      DestinationString->Buffer = ExAllocatePoolWithTag (NonPagedPool,
							 Length,
							 TAG_ASTR);
      
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
      
      RtlZeroMemory (DestinationString->Buffer,
		     Length);
      DestinationString->MaximumLength = (WORD)Length;
	}
  else
    {
      if (Length > DestinationString->MaximumLength)
	{
	  if (DestinationString->MaximumLength == 0)
	    return STATUS_BUFFER_OVERFLOW;
	  DestinationString->Length =
	    DestinationString->MaximumLength - 1;
	}
    }
  
  Status = RtlUnicodeToOemN (DestinationString->Buffer,
			     DestinationString->Length,
			     &Size,
			     SourceString->Buffer,
			     SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);

      return Status;
    }
  
  DestinationString->Buffer[Size] = 0;
  
  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlUnicodeStringToInteger(IN PUNICODE_STRING InString,
			  IN ULONG Base,
			  OUT PULONG Value)
{
  BOOLEAN Negative = 0;
  PWCHAR String = InString->Buffer;
  ULONG MaxLen = InString->Length / sizeof(WCHAR);

  *Value = 0;

  if(*String == L'-')
    {
      Negative++;
      String++;
    }
  else if(*String == L'+')
    {
      Negative = 0;
      String++;
    }

  if(!Base)
    {
      if(*String == L'b')
        {
          Base = 2;
          String++;
        }
      else if(*String == L'o')
        {
          Base = 8;
          String++;
        }
      else if(*String == L'x')
        {
          Base = 16;
          String++;
        }
      else
        Base = 10;
    }

  while(*String && MaxLen)
    {
      short c = *String;;
      String++;
      MaxLen--;

      if(c >= 'a' && c <= 'f')
        c -= 'a' - 'A';

      /* validate chars for bases <= 10 */
      if( Base <= 10  && (c < '0' || c > '9') )
        return STATUS_INVALID_PARAMETER;

      /* validate chars for base 16 */
      else if( (c < '0' || c > '9') && (c < 'A' || c > 'F') )
        return STATUS_INVALID_PARAMETER;

      /* perhaps someday we'll validate additional bases */

      if(c >= 'A' && c <= 'F')
        *Value = *Value * Base + c - 'A' + 10;
      else
        *Value = *Value * Base + c - '0';
    }

  if(Negative)
    *Value *= -1;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlUnicodeStringToOemSize(IN PUNICODE_STRING UnicodeString)
{
  ULONG Size;

  RtlUnicodeToMultiByteSize(&Size,
			    UnicodeString->Buffer,
			    UnicodeString->Length);
  return(Size+1);
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlUnicodeStringToOemString(IN OUT POEM_STRING DestinationString,
			    IN PUNICODE_STRING SourceString,
			    IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbOemCodePageTag == TRUE){
    Length = RtlUnicodeStringToAnsiSize (SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_OSTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory(DestinationString->Buffer,
		DestinationString->Length);
  
  Status = RtlUnicodeToOemN(DestinationString->Buffer,
			    DestinationString->Length,
			    NULL,
			    SourceString->Buffer,
			    SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return Status;
    }
  
  DestinationString->Buffer[Length] = 0;
  
  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlUpcaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
		       IN PCUNICODE_STRING SourceString,
		       IN BOOLEAN AllocateDestinationString)
{
  ULONG i;
  PWCHAR Src, Dest;

  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
      DestinationString->Buffer = 
	ExAllocatePoolWithTag(NonPagedPool,
			      SourceString->Length + sizeof(WCHAR),
			      TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);
    }
  else
    {
      if (SourceString->Length >= DestinationString->MaximumLength)
	return(STATUS_BUFFER_TOO_SMALL);
    }
  DestinationString->Length = SourceString->Length;

  Src = SourceString->Buffer;
  Dest = DestinationString->Buffer;
  for (i=0; i < SourceString->Length / sizeof(WCHAR); i++)
    {
      *Dest = RtlUpcaseUnicodeChar(*Src);
      Dest++;
      Src++;
    }
  *Dest = 0;

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlUpcaseUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
				   IN PUNICODE_STRING SourceString,
				   IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE)
    {
      Length = RtlUnicodeStringToAnsiSize(SourceString);
      Length--;
    }
  else
    Length = SourceString->Length / sizeof(WCHAR);
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer = 
	ExAllocatePoolWithTag(NonPagedPool,
			      DestinationString->MaximumLength,
			      TAG_ASTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return(STATUS_BUFFER_TOO_SMALL);
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory(DestinationString->Buffer,
		DestinationString->Length);
  
  Status = RtlUpcaseUnicodeToMultiByteN(DestinationString->Buffer,
					DestinationString->Length,
					NULL,
					SourceString->Buffer,
					SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return(Status);
    }
  
  DestinationString->Buffer[Length] = 0;
  
  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlUpcaseUnicodeStringToCountedOemString(IN OUT POEM_STRING DestinationString,
					 IN PUNICODE_STRING SourceString,
					 IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  ULONG Size;

  if (NlsMbCodePageTag == TRUE)
    Length = RtlUnicodeStringToAnsiSize (SourceString)/* + 1*/;
  else
    Length = SourceString->Length / sizeof(WCHAR) + 1;

  if (Length > 0x0000FFFF)
    return(STATUS_INVALID_PARAMETER_2);

  DestinationString->Length = (WORD)(Length - 1);

  if (AllocateDestinationString == TRUE)
    {
      DestinationString->Buffer =
	ExAllocatePoolWithTag(NonPagedPool,
			      Length,
			      TAG_OSTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);

      RtlZeroMemory(DestinationString->Buffer,
		    Length);
      DestinationString->MaximumLength = (WORD)Length;
    }
  else
    {
      if (Length > DestinationString->MaximumLength)
	{
	  if (DestinationString->MaximumLength == 0)
	    return(STATUS_BUFFER_OVERFLOW);
	  DestinationString->Length =
	    DestinationString->MaximumLength - 1;
	}
    }

  Status = RtlUpcaseUnicodeToOemN(DestinationString->Buffer,
				  DestinationString->Length,
				  &Size,
				  SourceString->Buffer,
				  SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return(Status);
    }

  DestinationString->Buffer[Size] = 0;

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlUpcaseUnicodeStringToOemString(IN OUT POEM_STRING DestinationString,
				  IN PUNICODE_STRING SourceString,
				  IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;

  if (NlsMbOemCodePageTag == TRUE) {
    Length = RtlUnicodeStringToOemSize(SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);

  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag(NonPagedPool,
			      DestinationString->MaximumLength,
			      TAG_OSTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return(STATUS_BUFFER_TOO_SMALL);
    }
  DestinationString->Length = Length;

  RtlZeroMemory(DestinationString->Buffer,
		DestinationString->Length);

  Status = RtlUpcaseUnicodeToOemN(DestinationString->Buffer,
				  DestinationString->Length,
				  NULL,
				  SourceString->Buffer,
				  SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return(Status);
    }

  DestinationString->Buffer[Length] = 0;

  return(STATUS_SUCCESS);
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

/* EOF */
