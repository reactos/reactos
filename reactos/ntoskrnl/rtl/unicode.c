/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/unicode.c
 * PURPOSE:         String functions
 * PROGRAMMER:      Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
 *                  Created 10/08/98
 */

#include <base.h>
#include <wstring.h>

#include <ddk/ntddk.h>

#include <internal/string.h>
#include <internal/ke.h>
#include <internal/ctype.h>

#define NDEBUG
#include <internal/debug.h>

#define Aa_Difference ('A'-'a')

PUNICODE_STRING RtlDuplicateUnicodeString(PUNICODE_STRING Dest, 
					  PUNICODE_STRING Src)
{
   if (Dest==NULL)
     {
	Dest=ExAllocatePool(NonPagedPool,sizeof(UNICODE_STRING));       
     }   
}

VOID RtlUpperString(PSTRING DestinationString, PSTRING SourceString)
{
   UNIMPLEMENTED;
}

WCHAR wtoupper(WCHAR c)
{
        if((c>='a') && (c<='z')) return c+Aa_Difference;
        return c;
}

WCHAR wtolower(WCHAR c)
{
//   DPRINT("c %c (c-Aa_Difference) %c\n",(char)c,(char)(c-Aa_Difference));
        if((c>='A') && (c<='Z')) return c-Aa_Difference;
        return c;
}

ULONG RtlAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
        return AnsiString->Length*2;
}

NTSTATUS RtlAnsiStringToUnicodeString(IN OUT PUNICODE_STRING DestinationString,
        IN PANSI_STRING SourceString, IN BOOLEAN AllocateDestinationString)
{
        unsigned long i;

        if(AllocateDestinationString==TRUE) {
          DestinationString->Buffer=ExAllocatePool(NonPagedPool, (SourceString->Length+1)*2);
          DestinationString->MaximumLength=SourceString->Length;
        };

        DestinationString->Length=SourceString->Length;
        memset(DestinationString->Buffer, 0, SourceString->Length*2);

        for (i=0; i<SourceString->Length; i++)
        {
                *DestinationString->Buffer=*SourceString->Buffer;

                SourceString->Buffer++;
                DestinationString->Buffer++;
        };
        *DestinationString->Buffer=0;

        SourceString->Buffer-=SourceString->Length;
        DestinationString->Buffer-=SourceString->Length;

        return STATUS_SUCCESS;
};

NTSTATUS RtlAppendUnicodeStringToString(IN OUT PUNICODE_STRING Destination,
        IN PUNICODE_STRING Source)
{
        unsigned long i;

        if(Destination->MaximumLength-Destination->Length-Source->Length<0)
                return STATUS_BUFFER_TOO_SMALL;

        Destination->Buffer+=Destination->Length;
        for(i=0; i<Source->Length; i++) {
                *Destination->Buffer=*Source->Buffer;
                Destination->Buffer++;
                Source->Buffer++;
        };
        *Destination->Buffer=0;
        Destination->Buffer-=(Destination->Length+Source->Length);
        Source->Buffer-=Source->Length;

        Destination->Length+=Source->Length;
        return STATUS_SUCCESS;
};

NTSTATUS RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
        IN PWSTR Source)
{
        unsigned long i, slen=wstrlen(Source);

        if(Destination->MaximumLength-Destination->Length-slen<0)
                return STATUS_BUFFER_TOO_SMALL;

        Destination->Buffer+=Destination->Length;
        for(i=0; i<slen; i++) {
                *Destination->Buffer=*Source;
                Destination->Buffer++;
                Source++;
        };
        *Destination->Buffer=0;
        Destination->Buffer-=(Destination->Length+slen);
        Source-=slen;

        Destination->Length+=slen;
        return STATUS_SUCCESS;
};

NTSTATUS RtlCharToInteger(IN PCSZ String, IN ULONG Base, IN OUT PULONG Value)
{
        *Value=simple_strtoul((const char *)String, NULL, Base);
};

LONG RtlCompareString(PSTRING String1, PSTRING String2, BOOLEAN CaseInsensitive)
{
        unsigned long i;
        char c1, c2;

        if(String1->Length!=String2->Length) return String1->Length-String2->Length;

        for(i=0; i<String1->Length; i++) {
                if(CaseInsensitive==TRUE) {
                        c1=toupper(*String1->Buffer);
                        c2=toupper(*String2->Buffer);
                } else {
                        c1=*String1->Buffer;
                        c2=*String2->Buffer;
                };
                if(c1!=c2) {
                        String1->Buffer-=i;
                        String2->Buffer-=i;
                        return c1-c2;
                };
                String1->Buffer++;
                String2->Buffer++;
        };
        String1->Buffer-=i;
        String2->Buffer-=i;

        return 0;
};

LONG RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2,
                             BOOLEAN CaseInsensitive)
{
        unsigned long i;
        WCHAR wc1, wc2;

        if(String1->Length!=String2->Length) return
                String1->Length-String2->Length;

        for(i=0; i<String1->Length; i++) {
                if(CaseInsensitive==TRUE) {
                        wc1=wtoupper(*String1->Buffer);
                        wc2=wtoupper(*String2->Buffer);
                } else {
                        wc1=*String1->Buffer;
                        wc2=*String2->Buffer;
                };

                if(wc1!=wc2) {
                        String1->Buffer-=i;
                        String2->Buffer-=i;
                        return wc1-wc2;
                };

                String1->Buffer++;
                String2->Buffer++;
        };

        String1->Buffer-=i;
        String2->Buffer-=i;

        return 0;
};

VOID RtlCopyString(IN OUT PSTRING DestinationString, IN PSTRING SourceString)
{
        unsigned long copylen, i;

        if(SourceString==NULL) {
                 DestinationString->Length=0;
        } else {
                 if(SourceString->Length<DestinationString->MaximumLength) {
                         copylen=SourceString->Length;
                 } else {
                         copylen=DestinationString->MaximumLength;
                 };
                 for(i=0; i<copylen; i++)
                 {
                         *DestinationString->Buffer=*SourceString->Buffer;
                         DestinationString->Buffer++;
                         SourceString->Buffer++;
                 };
                 *DestinationString->Buffer=0;
                 DestinationString->Buffer-=copylen;
                 SourceString->Buffer-=copylen;
        };
};

VOID RtlCopyUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                          IN PUNICODE_STRING SourceString)
{
        unsigned long copylen, i;

        if(SourceString==NULL) {
                 DestinationString->Length=0;
        } else {
                 if(SourceString->Length<DestinationString->MaximumLength) {
                         copylen=SourceString->Length;
                 } else {
                         copylen=DestinationString->MaximumLength;
                 };
                 for(i=0; i<copylen; i++)
                 {
                         *DestinationString->Buffer=*SourceString->Buffer;
                         DestinationString->Buffer++;
                         SourceString->Buffer++;
                 };
                 *DestinationString->Buffer=0;
                 DestinationString->Buffer-=copylen;
                 SourceString->Buffer-=copylen;
        };
};

BOOLEAN RtlEqualString(PSTRING String1, PSTRING String2, BOOLEAN CaseInsensitive)
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
};

BOOLEAN RtlEqualUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2,
                              BOOLEAN CaseInsensitive)
{
        unsigned long s1l=String1->Length;
        unsigned long s2l=String2->Length;
        unsigned long i;
        char wc1, wc2;

        if(s1l!=s2l) return FALSE;

        for(i=0; i<s1l; i++) {
                if(CaseInsensitive==TRUE) {
                        wc1=wtoupper(*String1->Buffer);
                        wc2=wtoupper(*String2->Buffer);
                } else {
                        wc1=*String1->Buffer;
                        wc2=*String2->Buffer;
                };

                if(wc1!=wc2) {
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
};

VOID RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
        ExFreePool(AnsiString->Buffer);
};

VOID RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
        ExFreePool(UnicodeString->Buffer);
};

VOID RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
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

VOID RtlInitString(IN OUT PSTRING DestinationString,
                   IN PCSZ SourceString)
{
        DestinationString->Length=strlen((char *)SourceString);
        DestinationString->MaximumLength=strlen((char *)SourceString)+1;
        DestinationString->Buffer=SourceString;
};

VOID RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                          IN PCWSTR SourceString)
{
        unsigned long i, DestSize;
        UNICODE_STRING Dest=*DestinationString;

        if(SourceString==NULL) {
                DestinationString->Length=0;
                DestinationString->MaximumLength=0;
                DestinationString->Buffer=NULL;
        } else {
                DestSize=wstrlen((PWSTR)SourceString);
                DestinationString->Length=DestSize;
                DestinationString->MaximumLength=DestSize+1;

                DestinationString->Buffer=(PWSTR)SourceString;
        };
};

NTSTATUS RtlIntegerToUnicodeString(IN ULONG Value, IN ULONG Base,                    /* optional */
                                   IN OUT PUNICODE_STRING String)
{
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
                sprintf(str, "%u", Value);
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
};

NTSTATUS RtlUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
                                      IN PUNICODE_STRING SourceString,
                                      IN BOOLEAN AllocateDestinationString)
{
        unsigned long i;

        if(AllocateDestinationString==TRUE) {

                // Causes excetion 14(0) in _Validate_Free_List
                DestinationString->Buffer=ExAllocatePool(NonPagedPool, SourceString->Length+1);
                DestinationString->MaximumLength=SourceString->Length+1;
        };

        DestinationString->Length=SourceString->Length;

        for(i=0; i<SourceString->Length; i++) {
                *DestinationString->Buffer=*SourceString->Buffer;
                DestinationString->Buffer++;
                SourceString->Buffer++;
        };
        *DestinationString->Buffer=0;

        DestinationString->Buffer-=SourceString->Length;
        SourceString->Buffer-=SourceString->Length;

        return STATUS_SUCCESS;
};

NTSTATUS RtlUnicodeStringToInteger(IN PUNICODE_STRING String, IN ULONG Base,
                                   OUT PULONG Value)
{
        char *str;
        unsigned long i, lenmin=0;
        BOOLEAN addneg=FALSE;

        str=ExAllocatePool(NonPagedPool, String->Length+1);

        for(i=0; i<String->Length; i++) {
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

        ExFreePool(str);
};

NTSTATUS RtlUpcaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                                IN PUNICODE_STRING SourceString,
                                IN BOOLEAN AllocateDestinationString)
{
        unsigned long i;

        if(AllocateDestinationString==TRUE) {
                DestinationString->Buffer=ExAllocatePool(NonPagedPool, SourceString->Length*2+1);
                DestinationString->Length=SourceString->Length;
                DestinationString->MaximumLength=SourceString->Length+1;
        };

        for(i=0; i<SourceString->Length; i++) {
                *DestinationString->Buffer=wtoupper(*SourceString->Buffer);
                DestinationString->Buffer++;
                SourceString->Buffer++;
        };
        *DestinationString->Buffer=0;

        DestinationString->Buffer-=SourceString->Length;
        SourceString->Buffer-=SourceString->Length;

        return STATUS_SUCCESS;
};

VOID RtlUpcaseString(IN OUT PSTRING DestinationString,
                     IN PSTRING SourceString)
{
        unsigned long i, len;

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
