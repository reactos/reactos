/* $Id: unicode.c,v 1.8 1999/11/15 15:58:24 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/unicode.c
 * PURPOSE:         String functions
 * PROGRAMMER:      Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
 *                  Created 10/08/98
 */

#include <ddk/ntddk.h>
#include <ctype.h>

//#include <internal/nls.h>


#define NDEBUG
#include <internal/debug.h>


extern unsigned long simple_strtoul(const char *cp, char **endp,
				    unsigned int base);


/* FUNCTIONS *****************************************************************/

WCHAR
STDCALL
RtlAnsiCharToUnicodeChar(PCHAR AnsiChar)
{
        ULONG Size;
        WCHAR UnicodeChar;

        Size = 1;
#if 0
        Size = (NlsLeadByteInfo[*AnsiChar] == 0) ? 1 : 2;
#endif

        RtlMultiByteToUnicodeN (&UnicodeChar,
                                sizeof(WCHAR),
                                NULL,
                                AnsiChar,
                                Size);

        return UnicodeChar;
}


ULONG
STDCALL
RtlAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
        ULONG Size;

        RtlMultiByteToUnicodeSize (&Size,
                                   AnsiString->Buffer,
                                   AnsiString->Length);

        return Size;
}


NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(
        IN OUT PUNICODE_STRING DestinationString,
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

        DestinationString->Length = Length;

        if (AllocateDestinationString == TRUE)
        {
                DestinationString->MaximumLength = Length + sizeof(WCHAR);
                DestinationString->Buffer =
                        RtlAllocateHeap (RtlGetProcessHeap (),
                                         0,
                                         DestinationString->MaximumLength);
                if (DestinationString->Buffer == NULL)
                        return STATUS_NO_MEMORY;
        }
        else
        {
                if (Length > DestinationString->Length)
                        return STATUS_BUFFER_OVERFLOW;
        }

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
                        RtlFreeHeap (RtlGetProcessHeap (),
                                     0,
                                     DestinationString->Buffer);
                return Status;
        }

        DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;

        return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(IN OUT PUNICODE_STRING Destination,
        IN PUNICODE_STRING Source)
{
        PWCHAR Src;
        PWCHAR Dest;
        ULONG  i;

        if ((Source->Length + Destination->Length) >= Destination->MaximumLength)
                return STATUS_BUFFER_TOO_SMALL;

        Src  = Source->Buffer;
        Dest = Destination->Buffer + (Destination->Length / sizeof (WCHAR));
        for (i = 0; i < (Source->Length / sizeof(WCHAR)); i++)
        {
                *Dest = *Src;
                Dest++;
                Src++;
        }
        *Dest = 0;

        Destination->Length += Source->Length;

        return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
        IN PWSTR Source)
{
        PWCHAR Src;
        PWCHAR Dest;
        ULONG  i;
        ULONG  slen;

        slen = wcslen (Source);

        if (Destination->Length + slen >= Destination->MaximumLength)
                return STATUS_BUFFER_TOO_SMALL;

        Src = Source;
        Dest = Destination->Buffer + (Destination->Length / sizeof (WCHAR));

        for (i = 0; i < slen; i++)
        {
                *Dest = *Src;
                Dest++;
                Src++;
        };
        *Dest = 0;

        Destination->Length += (slen * sizeof(WCHAR));

        return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlCharToInteger(IN PCSZ String, IN ULONG Base, IN OUT PULONG Value)
{
   *Value=simple_strtoul((const char *)String, NULL, Base);
   return(STATUS_SUCCESS);
}

LONG
STDCALL
RtlCompareString(PSTRING String1,
                 PSTRING String2,
                 BOOLEAN CaseInsensitive)
{
        unsigned long i;
        char c1, c2;

        if (String1->Length!=String2->Length)
                return String1->Length-String2->Length;

        for (i=0; i<String1->Length; i++)
        {
                if(CaseInsensitive==TRUE)
                {
                        c1=toupper(*String1->Buffer);
                        c2=toupper(*String2->Buffer);
                }
                else
                {
                        c1=*String1->Buffer;
                        c2=*String2->Buffer;
                }

                if(c1!=c2)
                {
                        String1->Buffer-=i;
                        String2->Buffer-=i;
                        return c1-c2;
                }
                String1->Buffer++;
                String2->Buffer++;
        }
        String1->Buffer-=i;
        String2->Buffer-=i;

        return 0;
}

LONG
STDCALL
RtlCompareUnicodeString(PUNICODE_STRING String1,
        PUNICODE_STRING String2,
        BOOLEAN CaseInsensitive)
{
        unsigned long i;
        WCHAR wc1, wc2;
        PWCHAR pw1, pw2;

        if (String1->Length != String2->Length)
                return String1->Length-String2->Length;

        pw1 = String1->Buffer;
        pw2 = String2->Buffer;

        for(i = 0; i < (String1->Length / sizeof(WCHAR)); i++)
        {
                if(CaseInsensitive == TRUE)
                {
                        wc1 = towupper (*pw1);
                        wc2 = towupper (*pw2);
                }
                else
                {
                        wc1 = *pw1;
                        wc2 = *pw2;
                }

                if (wc1 != wc2)
                {
                        return wc1 - wc2;
                }

                pw1++;
                pw2++;
        }

        return 0;
}

VOID
STDCALL
RtlCopyString(IN OUT PSTRING DestinationString, IN PSTRING SourceString)
{
        unsigned long copylen, i;

        if(SourceString==NULL)
        {
                 DestinationString->Length=0;
        }
        else
        {
                 if(SourceString->Length<DestinationString->MaximumLength)
                 {
                         copylen=SourceString->Length;
                 }
                 else
                 {
                         copylen=DestinationString->MaximumLength;
                 }

                 for(i=0; i<copylen; i++)
                 {
                         *DestinationString->Buffer=*SourceString->Buffer;
                         DestinationString->Buffer++;
                         SourceString->Buffer++;
                 }

                 *DestinationString->Buffer=0;
                 DestinationString->Buffer-=copylen;
                 SourceString->Buffer-=copylen;
        }
}

VOID
STDCALL
RtlCopyUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PUNICODE_STRING SourceString)
{
        unsigned long copylen, i;
        PWCHAR Src, Dest;
   
        if(SourceString==NULL) 
        {
                DestinationString->Length=0;
        } 
        else 
        {
                copylen = min(DestinationString->MaximumLength - sizeof(WCHAR),
                              SourceString->Length);
                Src = SourceString->Buffer;
                Dest = DestinationString->Buffer;

                for (i = 0; i < (copylen / sizeof (WCHAR)); i++)
                {
                        *Dest = *Src;
                        Dest++;
                        Src++;
                }
                *Dest = 0;

                DestinationString->Length = copylen;
        }
}

BOOLEAN
STDCALL
RtlEqualString(PSTRING String1, PSTRING String2, BOOLEAN CaseInsensitive)
{
        unsigned long s1l=String1->Length;
        unsigned long s2l=String2->Length;
        unsigned long i;
        char c1, c2;

        if(s1l!=s2l) return FALSE;

        for(i=0; i<s1l; i++) {
                c1=*String1->Buffer;
                c2=*String2->Buffer;

                if(CaseInsensitive==TRUE) {
                        c1=toupper(c1);
                        c2=toupper(c2);
                };

                if(c1!=c2) {
                        String1->Buffer-=i;
                        String2->Buffer-=i;
                        return FALSE;
                };

                String1->Buffer++;
                String2->Buffer++;
        };

        String1->Buffer-=i;
        String2->Buffer-=i;

        return TRUE;
}

BOOLEAN
STDCALL
RtlEqualUnicodeString(PUNICODE_STRING String1,
                      PUNICODE_STRING String2,
                      BOOLEAN CaseInsensitive)
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
                        wc1 = towupper (*pw1);
                        wc2 = towupper (*pw2);
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

VOID
STDCALL
RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     AnsiString->Buffer);
}

VOID
STDCALL
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     UnicodeString->Buffer);
}

VOID
STDCALL
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
                  IN PCSZ SourceString)
{
        unsigned long DestSize;

        if(SourceString==NULL) {
                DestinationString->Length=0;
                DestinationString->MaximumLength=0;
        } else {
                DestSize=strlen((const char *)SourceString);
                DestinationString->Length=DestSize;
                DestinationString->MaximumLength=DestSize+1;
        };
        DestinationString->Buffer=(PCHAR)SourceString;
};

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
                DestinationString->Buffer = NULL;
        }
        else
        {
                DestSize = strlen((const char *)SourceString);
                DestinationString->Length = DestSize;
                DestinationString->MaximumLength = DestSize + sizeof(CHAR);
                DestinationString->Buffer = (PCHAR)SourceString;
        }
}

VOID
STDCALL
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PCWSTR SourceString)
{
        ULONG DestSize;

        if (SourceString==NULL)
        {
                DestinationString->Length=0;
                DestinationString->MaximumLength=0;
                DestinationString->Buffer = NULL;
        }
        else
        {
                DestSize = wcslen((PWSTR)SourceString) * sizeof(WCHAR);
                DestinationString->Length = DestSize;
                DestinationString->MaximumLength = DestSize + sizeof(WCHAR);
                DestinationString->Buffer = (PWSTR)SourceString;
        }
}

NTSTATUS
STDCALL
RtlIntegerToUnicodeString(IN ULONG Value,
                          IN ULONG Base,                    /* optional */
                          IN OUT PUNICODE_STRING String)
{
//        UNIMPLEMENTED;

        return STATUS_NOT_IMPLEMENTED;
#if 0
        char *str;
        unsigned long len, i;

        str=ExAllocatePool(NonPagedPool, 1024);
        if(Base==16) {
                sprintf(str, "%x", Value);
        } else
        if(Base==8) {
                sprintf(str, "%o", Value);
        } else
        if(Base==2) {
                sprintf(str, "%b", Value);
        } else {
                sprintf(str, "%u", (unsigned int)Value);
        };

        len=strlen(str);
        if(String->MaximumLength<len) return STATUS_INVALID_PARAMETER;

        for(i=0; i<len; i++) {
                *String->Buffer=*str;
                String->Buffer++;
                str++;
        };
        *String->Buffer=0;
        String->Buffer-=len;
        String->Length=len;
        str-=len;
        ExFreePool(str);

        return STATUS_SUCCESS;
#endif
}


ULONG
STDCALL
RtlOemStringToUnicodeSize(IN PANSI_STRING OemString)
{
        ULONG Size;

        RtlMultiByteToUnicodeSize (&Size,
                                   OemString->Buffer,
                                   OemString->Length);

        return Size;
}

ULONG
STDCALL
RtlUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString)
{
        ULONG Size;

        RtlUnicodeToMultiByteSize (&Size,
                                   UnicodeString->Buffer,
                                   UnicodeString->Length);

        return Size;
}

NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
                             IN PUNICODE_STRING SourceString,
                             IN BOOLEAN AllocateDestinationString)
{
        NTSTATUS Status;
        ULONG Length;

        if (NlsMbCodePageTag == TRUE)
                Length = RtlUnicodeStringToAnsiSize (SourceString);
        else
                Length = SourceString->Length / sizeof(WCHAR);

        /* this doesn't make sense */
//        if (Length > 65535)
//                return STATUS_INVALID_PARAMETER_2;

        DestinationString->Length = Length;

        if (AllocateDestinationString == TRUE)
        {
                DestinationString->MaximumLength = Length + sizeof(CHAR);
                DestinationString->Buffer =
                        RtlAllocateHeap (RtlGetProcessHeap (),
                                         0,
                                         DestinationString->MaximumLength);
                if (DestinationString->Buffer == NULL)
                        return STATUS_NO_MEMORY;
        }
        else
        {
                if (Length >= DestinationString->Length)
                        return STATUS_BUFFER_OVERFLOW;
        }

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
                        RtlFreeHeap (RtlGetProcessHeap (),
                                     0,
                                     DestinationString->Buffer);
                return Status;
        }

        DestinationString->Buffer[Length] = 0;

        return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
RtlUnicodeStringToInteger(IN PUNICODE_STRING String,
                          IN ULONG Base,
                          OUT PULONG Value)
{
        return STATUS_NOT_IMPLEMENTED;
#if 0
        char *str;
        unsigned long i, lenmin=0;
        BOOLEAN addneg=FALSE;

        str = RtlAllocateHeap (RtlGetProcessHeap (),
                               0,
                               String->Length+1);

        for(i=0; i<String->Length; i++)
        {
                *str=*String->Buffer;

                if(*str=='b') { Base=2;  lenmin++; } else
                if(*str=='o') { Base=8;  lenmin++; } else
                if(*str=='d') { Base=10; lenmin++; } else
                if(*str=='x') { Base=16; lenmin++; } else
                if(*str=='+') { lenmin++; } else
                if(*str=='-') { addneg=TRUE; lenmin++; } else
                if((*str>'1') && (Base==2)) {
                        String->Buffer-=i;
                        *Value=0;
                        return STATUS_INVALID_PARAMETER;
                } else
                if(((*str>'7') || (*str<'0')) && (Base==8)) {
                        String->Buffer-=i;
                        *Value=0;
                        return STATUS_INVALID_PARAMETER;
                } else
                if(((*str>'9') || (*str<'0')) && (Base==10)) {
                        String->Buffer-=i;
                        *Value=0;
                        return STATUS_INVALID_PARAMETER;
                } else
                if((((*str>'9') || (*str<'0')) ||
                    ((toupper(*str)>'F') || (toupper(*str)<'A'))) && (Base==16))
                {
                        String->Buffer-=i;
                        *Value=0;
                        return STATUS_INVALID_PARAMETER;
                } else
                        str++;

                String->Buffer++;
        };

        *str=0;
        String->Buffer-=String->Length;
        str-=(String->Length-lenmin);

        if(addneg==TRUE) {
          *Value=simple_strtoul(str, NULL, Base)*-1;
        } else
          *Value=simple_strtoul(str, NULL, Base);

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     str);

        return(STATUS_SUCCESS);
#endif
}

NTSTATUS
STDCALL
RtlUpcaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                       IN PUNICODE_STRING SourceString,
                       IN BOOLEAN AllocateDestinationString)
{
        ULONG i;
        PWCHAR Src, Dest;

        if(AllocateDestinationString==TRUE)
        {
                DestinationString->Buffer =
                        RtlAllocateHeap (RtlGetProcessHeap (),
                                         0,
                                         SourceString->Length + sizeof(WCHAR));
                DestinationString->Length=SourceString->Length;
                DestinationString->MaximumLength=SourceString->Length+sizeof(WCHAR);
        }

        Src = SourceString->Buffer;
        Dest = DestinationString->Buffer;
        for (i=0; i < SourceString->Length / sizeof(WCHAR); i++)
        {
                *Dest = towupper (*Src);
                Dest++;
                Src++;
        }
        *Dest = 0;

        return STATUS_SUCCESS;
}

VOID
STDCALL
RtlUpcaseString(IN OUT PSTRING DestinationString,
                IN PSTRING SourceString)
{
        unsigned long i, len;
        PCHAR Src, Dest;

        if(SourceString->Length>DestinationString->MaximumLength) {
                len=DestinationString->MaximumLength;
        } else {
                len=SourceString->Length;
        };

        for(i=0; i<len; i++) {
                *DestinationString->Buffer=toupper(*SourceString->Buffer);
                DestinationString->Buffer++;
                SourceString->Buffer++;
        };
        *DestinationString->Buffer=0;

        DestinationString->Buffer-=len;
        SourceString->Buffer-=len;
}

VOID
STDCALL
RtlUpperString(PSTRING DestinationString, PSTRING SourceString)
{
//   UNIMPLEMENTED;
}

VOID
STDCALL
RtlUnwind (
	VOID
	)
{
}


VOID
STDCALL
RtlUpcaseUnicodeChar (
	VOID
	)
{
}


VOID
STDCALL
RtlUpcaseUnicodeToMultiByteN (
	VOID
	)
{
}



/* EOF */
