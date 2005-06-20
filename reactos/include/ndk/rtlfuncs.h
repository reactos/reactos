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

/* DEPENDENCIES **************************************************************/
#include <ndk/rtltypes.h>
#include <ndk/pstypes.h>

/* PROTOTYPES ****************************************************************/

/* FIXME: FILE NEEDS SOME ALPHABETIZING AND REGROUP */

/* List Macros */
static __inline 
VOID
InitializeListHead(
    IN PLIST_ENTRY  ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

static __inline 
VOID
InsertHeadList(
    IN PLIST_ENTRY  ListHead,
    IN PLIST_ENTRY  Entry)
{
    PLIST_ENTRY OldFlink;
    OldFlink = ListHead->Flink;
    Entry->Flink = OldFlink;
    Entry->Blink = ListHead;
    OldFlink->Blink = Entry;
    ListHead->Flink = Entry;
}

static __inline 
VOID
InsertTailList(
    IN PLIST_ENTRY  ListHead,
    IN PLIST_ENTRY  Entry)
{
    PLIST_ENTRY OldBlink;
    OldBlink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = OldBlink;
    OldBlink->Flink = Entry;
    ListHead->Blink = Entry;
}

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

#define PopEntryList(ListHead) \
    (ListHead)->Next; \
    { \
        PSINGLE_LIST_ENTRY _FirstEntry; \
        _FirstEntry = (ListHead)->Next; \
        if (_FirstEntry != NULL) \
            (ListHead)->Next = _FirstEntry->Next; \
    }

#define PushEntryList(_ListHead, _Entry) \
    (_Entry)->Next = (_ListHead)->Next; \
    (_ListHead)->Next = (_Entry); \

static __inline 
BOOLEAN
RemoveEntryList(
    IN PLIST_ENTRY  Entry)
{
    PLIST_ENTRY OldFlink;
    PLIST_ENTRY OldBlink;

    OldFlink = Entry->Flink;
    OldBlink = Entry->Blink;
    OldFlink->Blink = OldBlink;
    OldBlink->Flink = OldFlink;
    return (OldFlink == OldBlink);
}

static __inline 
PLIST_ENTRY 
RemoveHeadList(
    IN PLIST_ENTRY  ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

static __inline 
PLIST_ENTRY
RemoveTailList(
    IN PLIST_ENTRY  ListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

#define IsFirstEntry(ListHead, Entry) \
    ((ListHead)->Flink == Entry)
  
#define IsLastEntry(ListHead, Entry) \
    ((ListHead)->Blink == Entry)
    
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

/*
 * Constant String Macro
 */
#define RTL_CONSTANT_STRING(__SOURCE_STRING__) \
{ \
 sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
 sizeof(__SOURCE_STRING__), \
 (__SOURCE_STRING__) \
}

/*
 * Debug Functions 
 */

ULONG
CDECL
DbgPrint(
    IN PCH  Format,
    IN ...
);

VOID
STDCALL
DbgBreakPoint(VOID);

VOID
STDCALL
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
);

ULONG
STDCALL
RtlNtStatusToDosError(IN NTSTATUS Status);

VOID
STDCALL
RtlRaiseException(IN PEXCEPTION_RECORD ExceptionRecord);

VOID 
STDCALL 
RtlRaiseStatus(NTSTATUS Status);

VOID
STDCALL
RtlUnwind(
  PEXCEPTION_REGISTRATION RegistrationFrame,
  PVOID ReturnAddress,
  PEXCEPTION_RECORD ExceptionRecord,
  DWORD EaxValue
);

/*
 * Heap Functions 
 */
 
PVOID
STDCALL
RtlAllocateHeap(
    IN HANDLE HeapHandle,
    IN ULONG Flags,
    IN ULONG Size
);

PVOID
STDCALL
RtlCreateHeap(
    IN ULONG Flags,
    IN PVOID BaseAddress OPTIONAL,
    IN ULONG SizeToReserve OPTIONAL,
    IN ULONG SizeToCommit OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_DEFINITION Definition OPTIONAL
);

HANDLE 
STDCALL
RtlDestroyHeap(HANDLE hheap);

BOOLEAN
STDCALL
RtlFreeHeap(
    IN HANDLE HeapHandle,
    IN ULONG Flags,
    IN PVOID P
);

PVOID 
STDCALL
RtlReAllocateHeap(
    HANDLE Heap,
    ULONG Flags,
    PVOID Ptr,
    ULONG Size
);

BOOLEAN 
STDCALL
RtlLockHeap(IN HANDLE Heap);

BOOLEAN 
STDCALL
RtlUnlockHeap(IN HANDLE Heap);

ULONG 
STDCALL
RtlSizeHeap(
    IN PVOID HeapHandle, 
    IN ULONG Flags, 
    IN PVOID MemoryPointer
);
    
BOOLEAN 
STDCALL
RtlValidateHeap(
    HANDLE Heap,
    ULONG Flags,
    PVOID pmem
);

#define RtlGetProcessHeap() (NtCurrentPeb()->ProcessHeap)


/*
 * Security Functions
 */
NTSTATUS
STDCALL
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN PULONG BufferLength
);

NTSTATUS
STDCALL
RtlAddAccessAllowedAce(
    PACL Acl,
    ULONG Revision,
    ACCESS_MASK AccessMask,
    PSID Sid
);

NTSTATUS 
STDCALL 
RtlAddAccessAllowedAceEx(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN PSID pSid
);

NTSTATUS 
STDCALL
RtlAddAccessDeniedAce(
    PACL Acl,
    ULONG Revision,
    ACCESS_MASK AccessMask,
    PSID Sid
);

NTSTATUS 
STDCALL
RtlAddAccessDeniedAceEx(
    IN OUT PACL Acl,
    IN ULONG Revision,
    IN ULONG Flags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
);

NTSTATUS 
STDCALL
RtlAddAuditAccessAceEx(
    IN OUT PACL Acl,
    IN ULONG Revision,
    IN ULONG Flags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN BOOLEAN Success,
    IN BOOLEAN Failure
);

NTSTATUS 
STDCALL
RtlAddAce(
    PACL Acl,
    ULONG Revision,
    ULONG StartingIndex,
    PACE AceList,
    ULONG AceListLength
);

NTSTATUS 
STDCALL
RtlAddAuditAccessAce(
    PACL Acl,
    ULONG Revision,
    ACCESS_MASK AccessMask,
    PSID Sid,
    BOOLEAN Success,
    BOOLEAN Failure
);
          
NTSTATUS
STDCALL
RtlAllocateAndInitializeSid(
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount,
    IN ULONG SubAuthority0,
    IN ULONG SubAuthority1,
    IN ULONG SubAuthority2,
    IN ULONG SubAuthority3,
    IN ULONG SubAuthority4,
    IN ULONG SubAuthority5,
    IN ULONG SubAuthority6,
    IN ULONG SubAuthority7,
    OUT PSID *Sid
);

BOOLEAN 
STDCALL
RtlAreAllAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess
);

BOOLEAN 
STDCALL
RtlAreAnyAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess
);

VOID
STDCALL
RtlCopyLuid(
    IN PLUID LuidDest,
    IN PLUID LuidSrc
);

VOID 
STDCALL
RtlCopyLuidAndAttributesArray(
    ULONG Count,
    PLUID_AND_ATTRIBUTES Src,
    PLUID_AND_ATTRIBUTES Dest
);
        
NTSTATUS 
STDCALL
RtlCopySidAndAttributesArray(
    ULONG Count,
    PSID_AND_ATTRIBUTES Src,
    ULONG SidAreaSize,
    PSID_AND_ATTRIBUTES Dest,
    PVOID SidArea,
    PVOID* RemainingSidArea,
    PULONG RemainingSidAreaSize
);

NTSTATUS
STDCALL
RtlConvertSidToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PSID Sid,
    IN BOOLEAN AllocateDestinationString
);

NTSTATUS
STDCALL
RtlCopySid(
    IN ULONG Length,
    IN PSID Destination,
    IN PSID Source
);

NTSTATUS
STDCALL
RtlCreateAcl(
    PACL Acl,
    ULONG AclSize,
    ULONG AclRevision
);

NTSTATUS
STDCALL
RtlCreateSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ULONG Revision
);

NTSTATUS 
STDCALL
RtlCreateSecurityDescriptorRelative(
    PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    ULONG Revision
);

NTSTATUS 
STDCALL
RtlDeleteAce(
    PACL Acl,
    ULONG AceIndex
);

BOOLEAN 
STDCALL
RtlEqualPrefixSid(
    PSID Sid1,
    PSID Sid2
);
         
BOOLEAN
STDCALL
RtlEqualSid (
    IN PSID Sid1,
    IN PSID Sid2
);

BOOLEAN 
STDCALL
RtlFirstFreeAce(
    PACL Acl,
    PACE* Ace
);

PVOID
STDCALL
RtlFreeSid (
    IN PSID Sid
);

NTSTATUS 
STDCALL
RtlGetAce(
    PACL Acl,
    ULONG AceIndex,
    PACE *Ace
);

NTSTATUS 
STDCALL
RtlGetControlSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSECURITY_DESCRIPTOR_CONTROL Control,
    PULONG Revision
);
                 
NTSTATUS
STDCALL
RtlGetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN DaclPresent,
    OUT PACL *Dacl,
    OUT PBOOLEAN DaclDefaulted
);

NTSTATUS
STDCALL
RtlGetSaclSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PBOOLEAN SaclPresent,
    PACL* Sacl,
    PBOOLEAN SaclDefaulted
);

NTSTATUS
STDCALL
RtlGetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Group,
    OUT PBOOLEAN GroupDefaulted
);

NTSTATUS
STDCALL
RtlGetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Owner,
    OUT PBOOLEAN OwnerDefaulted
);

BOOLEAN 
STDCALL
RtlGetSecurityDescriptorRMControl(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PUCHAR RMControl
);
                  
PSID_IDENTIFIER_AUTHORITY 
STDCALL
RtlIdentifierAuthoritySid(PSID Sid);

NTSTATUS
STDCALL
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

NTSTATUS
STDCALL
RtlInitializeSid(
    IN OUT PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount
);

ULONG
STDCALL
RtlLengthRequiredSid(IN UCHAR SubAuthorityCount);

ULONG
STDCALL
RtlLengthSid(IN PSID Sid);

NTSTATUS 
STDCALL
RtlQueryInformationAcl(
    PACL Acl,
    PVOID Information,
    ULONG InformationLength,
    ACL_INFORMATION_CLASS InformationClass
);

NTSTATUS
STDCALL
RtlSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSD,
    OUT PSECURITY_DESCRIPTOR AbsoluteSD,
    IN PULONG AbsoluteSDSize,
    IN PACL Dacl,
    IN PULONG DaclSize,
    IN PACL Sacl,
    IN PULONG SaclSize,
    IN PSID Owner,
    IN PULONG OwnerSize,
    IN PSID PrimaryGroup,
    IN PULONG PrimaryGroupSize
);

NTSTATUS 
STDCALL
RtlSetControlSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
);

NTSTATUS
STDCALL
RtlSetDaclSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    BOOLEAN DaclPresent,
    PACL Dacl,
    BOOLEAN DaclDefaulted
);

NTSTATUS
STDCALL
RtlSetGroupSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID Group,
    IN BOOLEAN GroupDefaulted
);

NTSTATUS 
STDCALL
RtlSetInformationAcl(
    PACL Acl,
    PVOID Information,
    ULONG InformationLength,
    ACL_INFORMATION_CLASS InformationClass
);

NTSTATUS
STDCALL
RtlSetOwnerSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID Owner,
    IN BOOLEAN OwnerDefaulted
);

NTSTATUS
STDCALL
RtlSetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN SaclPresent,
    IN PACL Sacl,
    IN BOOLEAN SaclDefaulted
);

VOID 
STDCALL
RtlSetSecurityDescriptorRMControl(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PUCHAR RMControl
);

PUCHAR
STDCALL
RtlSubAuthorityCountSid(
    IN PSID Sid
);

PULONG
STDCALL
RtlSubAuthoritySid(
    IN PSID Sid,
    IN ULONG SubAuthority
);

BOOLEAN
STDCALL
RtlValidRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
);

BOOLEAN 
STDCALL
RtlValidSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

BOOLEAN
STDCALL
RtlValidSid(IN PSID Sid);

BOOLEAN 
STDCALL
RtlValidAcl(PACL Acl);

/*
 * Single-Character Functions
 */
NTSTATUS 
STDCALL
RtlLargeIntegerToChar(
    IN PLARGE_INTEGER Value,
    IN ULONG Base,
    IN ULONG Length,
    IN OUT PCHAR String
);
    
CHAR 
STDCALL
RtlUpperChar(CHAR Source);

WCHAR
STDCALL
RtlUpcaseUnicodeChar(WCHAR Source);
    
WCHAR 
STDCALL
RtlDowncaseUnicodeChar(IN WCHAR Source);

NTSTATUS
STDCALL
RtlIntegerToChar(
    IN ULONG Value,
    IN ULONG Base,
    IN ULONG Length,
    IN OUT PCHAR String
);

NTSTATUS 
STDCALL
RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN OUT LPWSTR String
);

NTSTATUS
STDCALL
RtlIntegerToUnicodeString(
    IN ULONG Value,
    IN ULONG Base,
    IN OUT PUNICODE_STRING String
);

NTSTATUS
STDCALL
RtlCharToInteger(
    PCSZ String,
    ULONG Base,
    PULONG Value
);
    
USHORT 
FASTCALL
RtlUshortByteSwap(IN USHORT Source);

/*
 * Unicode->Ansi String Functions 
 */     
ULONG
STDCALL
RtlUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString);
   
NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

/*
 * Unicode->OEM String Functions
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString(
    IN OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

NTSTATUS
STDCALL
RtlUpcaseUnicodeString(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSTATUS
STDCALL
RtlUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);
    
NTSTATUS
STDCALL
RtlUpcaseUnicodeToOemN(
    PCHAR OemString,
    ULONG OemSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);
     
ULONG
STDCALL
RtlUnicodeStringToOemSize(IN PUNICODE_STRING UnicodeString);

NTSTATUS
STDCALL
RtlUnicodeToOemN(
    PCHAR OemString,
    ULONG OemSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);
    
/*
 * Unicode->MultiByte String Functions 
 */
NTSTATUS
STDCALL
RtlUnicodeToMultiByteN(
    PCHAR MbString,
    ULONG MbSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

NTSTATUS
STDCALL
RtlUpcaseUnicodeToMultiByteN(
    PCHAR MbString,
    ULONG MbSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);
    
NTSTATUS
STDCALL
RtlUnicodeToMultiByteSize(
    PULONG MbSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

/*
 * OEM to Unicode Functions
 */    
ULONG
STDCALL
RtlOemStringToUnicodeSize(POEM_STRING AnsiString);
    
NTSTATUS
STDCALL
RtlOemStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    POEM_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSTATUS
STDCALL
RtlOemToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    IN PCHAR OemString,
    ULONG BytesInOemString
);
    
/*
 * Ansi->Multibyte String Functions
 */
 
/*
 * Ansi->Unicode String Functions
 */
NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
);
    
ULONG
STDCALL
RtlAnsiStringToUnicodeSize(
    PANSI_STRING AnsiString
);
   
BOOLEAN 
STDCALL
RtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING Destination,
    IN PCSZ Source
);

/*
 * Unicode String Functions
 */
NTSTATUS
STDCALL
RtlAppendUnicodeToString(
    PUNICODE_STRING Destination,
    PCWSTR Source
);

NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(
    PUNICODE_STRING Destination,
    PUNICODE_STRING Source
);

LONG
STDCALL
RtlCompareUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

VOID 
STDCALL
RtlCopyUnicodeString(
    PUNICODE_STRING DestinationString,
    PUNICODE_STRING SourceString
);

BOOLEAN
STDCALL
RtlCreateUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);
    
BOOLEAN
STDCALL
RtlEqualUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

VOID
STDCALL
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString);

VOID
STDCALL
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING  DestinationString,
  IN PCWSTR  SourceString);

BOOLEAN
STDCALL
RtlPrefixUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

NTSTATUS
STDCALL
RtlUnicodeStringToInteger(
    PUNICODE_STRING String,
    ULONG Base,
    PULONG Value
);

/*
 * Ansi String Functions
 */
VOID
STDCALL
RtlFreeAnsiString(IN PANSI_STRING AnsiString);

VOID
STDCALL
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSZ SourceString
);
 
/*
 * OEM String Functions
 */
VOID 
STDCALL
RtlFreeOemString(IN POEM_STRING OemString);

/* 
 * MultiByte->Unicode String Functions
 */
NTSTATUS
STDCALL
RtlMultiByteToUnicodeN(
    PWCHAR UnicodeString,
    ULONG UnicodeSize,
    PULONG ResultSize,
    const PCHAR MbString,
    ULONG MbSize
);

NTSTATUS
STDCALL
RtlMultiByteToUnicodeSize(
    PULONG UnicodeSize,
    PCHAR MbString,
    ULONG MbSize
);

/*
 * Atom Functions
 */
NTSTATUS
STDCALL
RtlAddAtomToAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN PWSTR AtomName,
    OUT PRTL_ATOM Atom
);

NTSTATUS
STDCALL
RtlCreateAtomTable(
    IN ULONG TableSize,
    IN OUT PRTL_ATOM_TABLE *AtomTable
);

NTSTATUS
STDCALL
RtlDeleteAtomFromAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN RTL_ATOM Atom
);

NTSTATUS 
STDCALL
RtlDestroyAtomTable(IN PRTL_ATOM_TABLE AtomTable);

NTSTATUS
STDCALL
RtlQueryAtomInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN RTL_ATOM Atom,
    IN OUT PULONG RefCount OPTIONAL,
    IN OUT PULONG PinCount OPTIONAL,
    IN OUT PWSTR AtomName OPTIONAL,
    IN OUT PULONG NameLength OPTIONAL
);
    
NTSTATUS
STDCALL
RtlLookupAtomInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN PWSTR AtomName,
    OUT PRTL_ATOM Atom
);

/*
 * Memory Functions
 */
SIZE_T 
STDCALL
RtlCompareMemory(
    IN const VOID *Source1,
    IN const VOID *Source2,
    IN SIZE_T Length
);
                 
VOID
STDCALL
RtlFillMemoryUlong(
    IN PVOID Destination,
    IN ULONG Length,
    IN ULONG Fill
);
                 
/*
 * Process Management Functions
 */      
VOID
STDCALL
RtlAcquirePebLock(VOID);

VOID
STDCALL
RtlReleasePebLock(VOID);

NTSTATUS
STDCALL
RtlCreateEnvironment(
    BOOLEAN Inherit,
    PWSTR *Environment
);

NTSTATUS
STDCALL
RtlCreateUserThread(
    IN HANDLE ProcessHandle,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN CreateSuspended,
    IN LONG StackZeroBits,
    IN OUT PULONG StackReserve,
    IN OUT PULONG StackCommit,
    IN PTHREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter,
    IN OUT PHANDLE ThreadHandle,
    IN OUT PCLIENT_ID ClientId
);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams(
    IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters
);

VOID
STDCALL
RtlDestroyEnvironment(
    PWSTR Environment
);
    
NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U(
    PWSTR Environment,
    PUNICODE_STRING Source,
    PUNICODE_STRING Destination,
    PULONG Length
);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams(
    IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters
);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U(
    PWSTR Environment,
    PUNICODE_STRING Name,
    PUNICODE_STRING Value
);

NTSTATUS
STDCALL
RtlSetEnvironmentVariable(
    PWSTR *Environment,
    PUNICODE_STRING Name,
    PUNICODE_STRING Value
);

/*
 * Critical Section/Resource Functions
 */
NTSTATUS
STDCALL
RtlDeleteCriticalSection (
     PRTL_CRITICAL_SECTION CriticalSection
);
 
NTSTATUS
STDCALL
RtlEnterCriticalSection(
     PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS 
STDCALL
RtlInitializeCriticalSection(
     PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS
STDCALL
RtlLeaveCriticalSection(
     PRTL_CRITICAL_SECTION CriticalSection
); 

/*
 * Compression Functions
 */
NTSTATUS
STDCALL
RtlCompressBuffer(
    IN USHORT CompressionFormatAndEngine,
    IN PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    OUT PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    IN ULONG UncompressedChunkSize,
    OUT PULONG FinalCompressedSize,
    IN PVOID WorkSpace
);

NTSTATUS
STDCALL
RtlDecompressBuffer(
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
);

NTSTATUS
STDCALL
RtlGetCompressionWorkSpaceSize(
    IN USHORT CompressionFormatAndEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
);

/*
 * Bitmap Functions
 */
BOOLEAN
STDCALL
RtlAreBitsClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length
);

BOOLEAN
STDCALL
RtlAreBitsSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length
);

VOID
STDCALL
RtlClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToClear
);

ULONG
STDCALL
RtlFindClearBits(
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
);

ULONG
STDCALL
RtlFindClearBitsAndSet(
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
);

VOID
STDCALL
RtlInitializeBitMap(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG BitMapBuffer,
    IN ULONG SizeOfBitMap
);

VOID
STDCALL
RtlSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToSet
);

/*
 * PE Functions
 */ 
PVOID
STDCALL
RtlImageDirectoryEntryToData(
    PVOID  BaseAddress,
    BOOLEAN bFlag,
    ULONG Directory,
    PULONG Size
);

ULONG
STDCALL
RtlImageRvaToVa(
    PIMAGE_NT_HEADERS NtHeader,
    PVOID BaseAddress,
    ULONG Rva,
    PIMAGE_SECTION_HEADER *SectionHeader
);

PIMAGE_NT_HEADERS
STDCALL
RtlImageNtHeader(IN PVOID BaseAddress);

PIMAGE_SECTION_HEADER
STDCALL
RtlImageRvaToSection(
    PIMAGE_NT_HEADERS NtHeader,
    PVOID BaseAddress,
    ULONG Rva
);

/*
 * Registry Functions
 */
NTSTATUS 
STDCALL
RtlFormatCurrentUserKeyPath(IN OUT PUNICODE_STRING KeyPath);
    
NTSTATUS
STDCALL
RtlOpenCurrentUser(
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE KeyHandle
);

NTSTATUS
STDCALL
RtlQueryRegistryValues(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PVOID Context,
    IN PVOID Environment
);

NTSTATUS
STDCALL
RtlWriteRegistryValue(
    ULONG RelativeTo,
    PCWSTR Path,
    PCWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength
);

NTSTATUS
STDCALL
RtlFindMessage (
    IN    PVOID                BaseAddress,
    IN    ULONG                Type,
    IN    ULONG                Language,
    IN    ULONG                MessageId,
    OUT    PRTL_MESSAGE_RESOURCE_ENTRY    *MessageResourceEntry
    );
  
/*
 * NLS Functions
 */      
VOID
STDCALL
RtlInitNlsTables(
    IN PUSHORT AnsiTableBase,
    IN PUSHORT OemTableBase,
    IN PUSHORT CaseTableBase,
    OUT PNLSTABLEINFO NlsTable
);

VOID
STDCALL
RtlInitCodePageTable(
    IN PUSHORT TableBase,
    OUT PCPTABLEINFO CodePageTable
);
   
VOID
STDCALL
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable);

/*
 * Misc String Functions
 */     
BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U(
    PWSTR DosName,
    PUNICODE_STRING NtName,
    PWSTR *ShortName,
    PCURDIR CurrentDirectory
);
                 
BOOLEAN
STDCALL
RtlIsNameLegalDOS8Dot3(
    IN PUNICODE_STRING UnicodeName,
    IN PANSI_STRING AnsiName,
    PBOOLEAN Unknown
);

ULONG
STDCALL
RtlIsTextUnicode(
    PVOID Buffer,
    ULONG Length,
    ULONG *Flags
);

/*
 * Misc conversion functions
 */
LARGE_INTEGER 
STDCALL
RtlConvertLongToLargeInteger(IN LONG SignedInteger);

LARGE_INTEGER
STDCALL
RtlEnlargedIntegerMultiply(
    LONG Multiplicand,
    LONG Multiplier
);

ULONG
STDCALL
RtlEnlargedUnsignedDivide(
    ULARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
);

LARGE_INTEGER
STDCALL
RtlEnlargedUnsignedMultiply(
    ULONG Multiplicand,
    ULONG Multiplier
);
    
/*
 * Time Functions
 */
NTSTATUS
STDCALL
RtlQueryTimeZoneInformation(LPTIME_ZONE_INFORMATION TimeZoneInformation);
    
VOID
STDCALL
RtlSecondsSince1970ToTime(
    IN ULONG SecondsSince1970,
    OUT PLARGE_INTEGER Time
);

NTSTATUS 
STDCALL
RtlSetTimeZoneInformation(LPTIME_ZONE_INFORMATION TimeZoneInformation);
                      
BOOLEAN 
STDCALL
RtlTimeFieldsToTime(
    PTIME_FIELDS TimeFields,
    PLARGE_INTEGER Time
);

VOID
STDCALL
RtlTimeToTimeFields(
    PLARGE_INTEGER Time,
    PTIME_FIELDS TimeFields
);

/*
 * Version Functions
 */
NTSTATUS
STDCALL
RtlVerifyVersionInfo(
    IN PRTL_OSVERSIONINFOEXW VersionInfo,
    IN ULONG TypeMask,
    IN ULONGLONG ConditionMask
);
    
NTSTATUS
STDCALL
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation);

/*
 * C Runtime Library Functions
 */
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
int isalpha(int c);
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
