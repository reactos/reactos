/* $Id: rtlfuncs.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
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
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/rtlfuncs.h
 * PURPOSE:         Prototypes for Runtime Library Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _RTLFUNCS_H
#define _RTLFUNCS_H

#include <ndk/rtltypes.h>

/*FIXME: REORGANIZE THIS */

#define RtlGetProcessHeap() (NtCurrentPeb()->ProcessHeap)

#define InsertAscendingListFIFO(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


#define InsertDescendingListFIFO(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField <\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


#define InsertAscendingList(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}


#define InsertDescendingList(ListHead, Type, ListEntryField, NewEntry, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField <=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}

NTSTATUS
STDCALL
RtlAbsoluteToSelfRelativeSD (
    IN PSECURITY_DESCRIPTOR     AbsoluteSecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    IN PULONG                   BufferLength
);

PVOID
STDCALL
RtlAllocateHeap (
    IN HANDLE  HeapHandle,
    IN ULONG   Flags,
    IN ULONG   Size
);

VOID
STDCALL
RtlAcquirePebLock (
	VOID
	);

VOID
STDCALL
RtlReleasePebLock (
	VOID
	);

NTSTATUS
STDCALL
RtlCompressBuffer (
    IN USHORT   CompressionFormatAndEngine,
    IN PUCHAR   UncompressedBuffer,
    IN ULONG    UncompressedBufferSize,
    OUT PUCHAR  CompressedBuffer,
    IN ULONG    CompressedBufferSize,
    IN ULONG    UncompressedChunkSize,
    OUT PULONG  FinalCompressedSize,
    IN PVOID    WorkSpace
);

NTSTATUS
STDCALL
RtlConvertSidToUnicodeString (
    OUT PUNICODE_STRING DestinationString,
    IN PSID             Sid,
    IN BOOLEAN          AllocateDestinationString
);

NTSTATUS
STDCALL
RtlCopySid (
    IN ULONG   Length,
    IN PSID    Destination,
    IN PSID    Source
);

BOOLEAN
STDCALL
RtlCreateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR           SourceString
);

NTSTATUS
STDCALL
RtlDecompressBuffer (
    IN USHORT   CompressionFormat,
    OUT PUCHAR  UncompressedBuffer,
    IN ULONG    UncompressedBufferSize,
    IN PUCHAR   CompressedBuffer,
    IN ULONG    CompressedBufferSize,
    OUT PULONG  FinalUncompressedSize
);

VOID
STDCALL
RtlDeleteCriticalSection (
     PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlEnterCriticalSection (
     PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS 
STDCALL
RtlInitializeCriticalSection (
     PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlLeaveCriticalSection (
     PRTL_CRITICAL_SECTION CriticalSection
);

#ifndef  _NTIFS_
NTSTATUS
STDCALL
RtlEqualSid (
    IN PSID Sid1,
    IN PSID Sid2
);

NTSTATUS
STDCALL
RtlFillMemoryUlong (
    IN PVOID    Destination,
    IN ULONG    Length,
    IN ULONG    Fill
);

#endif

NTSTATUS
STDCALL
RtlFindMessage (
	IN	PVOID				BaseAddress,
	IN	ULONG				Type,
	IN	ULONG				Language,
	IN	ULONG				MessageId,
	OUT	PRTL_MESSAGE_RESOURCE_ENTRY	*MessageResourceEntry
	);
	
BOOLEAN
STDCALL
RtlFreeHeap (
    IN HANDLE  HeapHandle,
    IN ULONG   Flags,
    IN PVOID   P
);

VOID
STDCALL
RtlFreeUnicodeString(
  IN PUNICODE_STRING  UnicodeString);

NTSTATUS
STDCALL
RtlGetCompressionWorkSpaceSize (
    IN USHORT   CompressionFormatAndEngine,
    OUT PULONG  CompressBufferWorkSpaceSize,
    OUT PULONG  CompressFragmentWorkSpaceSize
);

NTSTATUS
STDCALL
RtlGetDaclSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN            DaclPresent,
    OUT PACL                *Dacl,
    OUT PBOOLEAN            DaclDefaulted
);

NTSTATUS
STDCALL
RtlGetGroupSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID                *Group,
    OUT PBOOLEAN            GroupDefaulted
);

NTSTATUS
STDCALL
RtlGetOwnerSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID                *Owner,
    OUT PBOOLEAN            OwnerDefaulted
);

ULONG
STDCALL
RtlImageRvaToVa (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva,
	PIMAGE_SECTION_HEADER	*SectionHeader
	);

NTSTATUS
STDCALL
RtlInitializeSid (
    IN OUT PSID                     Sid,
    IN PSID_IDENTIFIER_AUTHORITY    IdentifierAuthority,
    IN UCHAR                        SubAuthorityCount
);

VOID
STDCALL
RtlRaiseException (
  IN PEXCEPTION_RECORD ExceptionRecord
);

VOID
STDCALL
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PCWSTR  SourceString);


BOOLEAN
STDCALL
RtlIsNameLegalDOS8Dot3 (
    IN PUNICODE_STRING UnicodeName,
    IN PANSI_STRING    AnsiName,
    PBOOLEAN           Unknown
);

ULONG
STDCALL
RtlLengthRequiredSid (
    IN UCHAR SubAuthorityCount
);

ULONG
STDCALL
RtlLengthSid (
    IN PSID Sid
);

ULONG
STDCALL
RtlNtStatusToDosError (
    IN NTSTATUS Status
);

NTSTATUS STDCALL
RtlFormatCurrentUserKeyPath (IN OUT PUNICODE_STRING KeyPath);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PWSTR		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

VOID STDCALL RtlRaiseStatus(NTSTATUS Status);
	
VOID
STDCALL
RtlSecondsSince1970ToTime (
    IN ULONG            SecondsSince1970,
    OUT PLARGE_INTEGER  Time
);

#if (VER_PRODUCTBUILD >= 2195)

NTSTATUS
STDCALL
RtlSelfRelativeToAbsoluteSD (
    IN PSECURITY_DESCRIPTOR     SelfRelativeSD,
    OUT PSECURITY_DESCRIPTOR    AbsoluteSD,
    IN PULONG                   AbsoluteSDSize,
    IN PACL                     Dacl,
    IN PULONG                   DaclSize,
    IN PACL                     Sacl,
    IN PULONG                   SaclSize,
    IN PSID                     Owner,
    IN PULONG                   OwnerSize,
    IN PSID                     PrimaryGroup,
    IN PULONG                   PrimaryGroupSize
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSTATUS
STDCALL
RtlSetGroupSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID                     Group,
    IN BOOLEAN                  GroupDefaulted
);

NTSTATUS
STDCALL
RtlSetOwnerSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID                     Owner,
    IN BOOLEAN                  OwnerDefaulted
);

NTSTATUS
STDCALL
RtlSetSaclSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN                  SaclPresent,
    IN PACL                     Sacl,
    IN BOOLEAN                  SaclDefaulted
);

PUCHAR
STDCALL
RtlSubAuthorityCountSid (
    IN PSID Sid
);

PULONG
STDCALL
RtlSubAuthoritySid (
    IN PSID    Sid,
    IN ULONG   SubAuthority
);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString (
	IN OUT	POEM_STRING	DestinationString,
	IN	PUNICODE_STRING	SourceString,
	IN	BOOLEAN		AllocateDestinationString
	);
	
NTSTATUS
STDCALL
RtlUpcaseUnicodeToOemN (
	PCHAR	OemString,
	ULONG	OemSize,
	PULONG	ResultSize,
	PWCHAR	UnicodeString,
	ULONG	UnicodeSize
	);
	
	
NTSTATUS STDCALL
RtlLargeIntegerToChar (
	IN	PLARGE_INTEGER	Value,
	IN	ULONG		Base,
	IN	ULONG		Length,
	IN OUT	PCHAR		String
	);
	
BOOLEAN 
STDCALL
RtlCreateUnicodeStringFromAsciiz (OUT PUNICODE_STRING Destination,
				  IN PCSZ Source);
	
VOID
STDCALL
RtlUnwind (
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PVOID ReturnAddress,
  PEXCEPTION_RECORD ExceptionRecord,
  DWORD EaxValue
);
	
BOOLEAN
STDCALL
RtlValidSid (
    IN PSID Sid
);

BOOLEAN 
STDCALL
RtlValidAcl (
    PACL Acl
);

/*  functions exported from NTOSKRNL.EXE which are considered RTL  */
char *_itoa (int value, char *string, int radix);
wchar_t *_itow (int value, wchar_t *string, int radix);
int _snprintf(char * buf, size_t cnt, const char *fmt, ...);
int _snwprintf(wchar_t *buf, size_t cnt, const wchar_t *fmt, ...);
int _stricmp(const char *s1, const char *s2);
char * _strlwr(char *x);
int _strnicmp(const char *s1, const char *s2, size_t n);
char * _strnset(char* szToFill, int szFill, size_t sizeMaxFill);
char * _strrev(char *s);
char * _strset(char* szToFill, int szFill);
char * _strupr(char *x);
int _vsnprintf(char *buf, size_t cnt, const char *fmt, va_list args);
int _wcsicmp (const wchar_t* cs, const wchar_t* ct);
wchar_t * _wcslwr (wchar_t *x);
int _wcsnicmp (const wchar_t * cs,const wchar_t * ct,size_t count);
wchar_t* _wcsnset (wchar_t* wsToFill, wchar_t wcFill, size_t sizeMaxFill);
wchar_t * _wcsrev(wchar_t *s);
wchar_t *_wcsupr(wchar_t *x);

int atoi(const char *str);
long atol(const char *str);
int isdigit(int c);
int islower(int c);
int isprint(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
size_t mbstowcs (wchar_t *wcstr, const char *mbstr, size_t count);
int mbtowc (wchar_t *wchar, const char *mbchar, size_t count);
void * memchr(const void *s, int c, size_t n);
void * memcpy(void *to, const void *from, size_t count);
void * memmove(void *dest,const void *src, size_t count);
void * memset(void *src, int val, size_t count);

int rand(void);
int sprintf(char * buf, const char *fmt, ...);
void srand(unsigned seed);
char * strcat(char *s, const char *append);
char * strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char * strcpy(char *to, const char *from);
size_t strlen(const char *str);
char * strncat(char *dst, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
char *strrchr(const char *s, int c);
size_t strspn(const char *s1, const char *s2);
char *strstr(const char *s, const char *find);
int swprintf(wchar_t *buf, const wchar_t *fmt, ...);
int tolower(int c);
int toupper(int c);
wchar_t towlower(wchar_t c);
wchar_t towupper(wchar_t c);
int vsprintf(char *buf, const char *fmt, va_list args);
wchar_t * wcscat(wchar_t *dest, const wchar_t *src);
wchar_t * wcschr(const wchar_t *str, wchar_t ch);
int wcscmp(const wchar_t *cs, const wchar_t *ct);
wchar_t* wcscpy(wchar_t* str1, const wchar_t* str2);
size_t wcscspn(const wchar_t *str,const wchar_t *reject);
size_t wcslen(const wchar_t *s);
wchar_t * wcsncat(wchar_t *dest, const wchar_t *src, size_t count);
int wcsncmp(const wchar_t *cs, const wchar_t *ct, size_t count);
wchar_t * wcsncpy(wchar_t *dest, const wchar_t *src, size_t count);
wchar_t * wcsrchr(const wchar_t *str, wchar_t ch);
size_t wcsspn(const wchar_t *str,const wchar_t *accept);
wchar_t *wcsstr(const wchar_t *s,const wchar_t *b);
size_t wcstombs (char *mbstr, const wchar_t *wcstr, size_t count);
int wctomb (char *mbchar, wchar_t wchar);

#endif
