/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    rtlfuncs.h

Abstract:

    Function definitions for the Run-Time Library

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _RTLFUNCS_H
#define _RTLFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <ntnls.h>
#include <extypes.h>
#include <rtltypes.h>

#ifdef NTOS_MODE_USER

//
// List Functions
//
FORCEINLINE
VOID
InitializeListHead(
    IN PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE
VOID
InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldFlink;
    OldFlink = ListHead->Flink;
    Entry->Flink = OldFlink;
    Entry->Blink = ListHead;
    OldFlink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldBlink;
    OldBlink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = OldBlink;
    OldBlink->Flink = Entry;
    ListHead->Blink = Entry;
}

BOOLEAN
FORCEINLINE
IsListEmpty(
    IN const LIST_ENTRY * ListHead
)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
    PSINGLE_LIST_ENTRY ListHead
)
{
    PSINGLE_LIST_ENTRY FirstEntry;
    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }

    return FirstEntry;
}

FORCEINLINE
VOID
PushEntryList(
    PSINGLE_LIST_ENTRY ListHead,
    PSINGLE_LIST_ENTRY Entry
)
{
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
}

FORCEINLINE
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
    return (BOOLEAN)(OldFlink == OldBlink);
}

FORCEINLINE
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

FORCEINLINE
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

//
// LUID Macros
//
#define RtlEqualLuid(L1, L2) (((L1)->HighPart == (L2)->HighPart) && \
                              ((L1)->LowPart  == (L2)->LowPart))

//
// RTL Splay Tree Functions
//
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(PRTL_SPLAY_LINKS Links);

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
    PRTL_SPLAY_LINKS Links,
    PRTL_SPLAY_LINKS *Root
);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(PRTL_SPLAY_LINKS Links);

#define RtlIsLeftChild(Links) \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlIsRightChild(Links) \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlRightChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->RightChild

#define RtlIsRoot(Links) \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links))

#define RtlLeftChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->LeftChild

#define RtlParent(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->Parent

#define RtlInitializeSplayLinks(Links)                  \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayLinks;                   \
        _SplayLinks = (PRTL_SPLAY_LINKS)(Links);        \
        _SplayLinks->Parent = _SplayLinks;              \
        _SplayLinks->LeftChild = NULL;                  \
        _SplayLinks->RightChild = NULL;                 \
    }

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks)    \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->LeftChild = _SplayChild;          \
        _SplayChild->Parent = _SplayParent;             \
    }

#define RtlInsertAsRightChild(ParentLinks,ChildLinks)   \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->RightChild = _SplayChild;         \
        _SplayChild->Parent = _SplayParent;             \
    }
#endif

//
// Error and Exception Functions
//
NTSYSAPI
PVOID
NTAPI
RtlAddVectoredExceptionHandler(
    IN ULONG FirstHandler,
    IN PVECTORED_EXCEPTION_HANDLER VectoredHandler
);

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
);

NTSYSAPI
VOID
NTAPI
RtlCaptureContext(OUT PCONTEXT ContextRecord);

NTSYSAPI
PVOID
NTAPI
RtlEncodePointer(IN PVOID Pointer);

NTSYSAPI
PVOID
NTAPI
RtlDecodePointer(IN PVOID Pointer);

NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context
);

NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(IN NTSTATUS Status);

NTSYSAPI
VOID
NTAPI
RtlRaiseException(IN PEXCEPTION_RECORD ExceptionRecord);

NTSYSAPI
VOID
NTAPI
RtlRaiseStatus(NTSTATUS Status);

NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo);

NTSYSAPI
VOID
NTAPI
RtlUnwind(
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
);

//
// Heap Functions
//
NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
    IN HANDLE HeapHandle,
    IN ULONG Flags,
    IN ULONG Size
);

NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
    IN ULONG Flags,
    IN PVOID BaseAddress OPTIONAL,
    IN SIZE_T SizeToReserve OPTIONAL,
    IN SIZE_T SizeToCommit OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
);

NTSYSAPI
ULONG
NTAPI
RtlCompactHeap(
    HANDLE heap,
    ULONG flags
);

NTSYSAPI
HANDLE
NTAPI
RtlDestroyHeap(HANDLE hheap);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    IN HANDLE HeapHandle,
    IN ULONG Flags,
    IN PVOID P
);

NTSYSAPI
ULONG
NTAPI
RtlGetProcessHeaps(
    ULONG HeapCount,
    HANDLE *HeapArray
);

NTSYSAPI
PVOID
NTAPI
RtlReAllocateHeap(
    HANDLE Heap,
    ULONG Flags,
    PVOID Ptr,
    ULONG Size
);

NTSYSAPI
BOOLEAN
NTAPI
RtlLockHeap(IN HANDLE Heap);

NTSYSAPI
BOOLEAN
NTAPI
RtlUnlockHeap(IN HANDLE Heap);

NTSYSAPI
ULONG
NTAPI
RtlSizeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID MemoryPointer
);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateHeap(
    HANDLE Heap,
    ULONG Flags,
    PVOID pmem
);

#define RtlGetProcessHeap() (NtCurrentPeb()->ProcessHeap)

//
// Security Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    IN PULONG BufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
    PACL Acl,
    ULONG Revision,
    ACCESS_MASK AccessMask,
    PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
    IN OUT PACL pAcl,
    IN ULONG dwAceRevision,
    IN ULONG AceFlags,
    IN ULONG AccessMask,
    IN PSID pSid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAce(
    PACL Acl,
    ULONG Revision,
    ACCESS_MASK AccessMask,
    PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAceEx(
    IN OUT PACL Acl,
    IN ULONG Revision,
    IN ULONG Flags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAceEx(
    IN OUT PACL Acl,
    IN ULONG Revision,
    IN ULONG Flags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN BOOLEAN Success,
    IN BOOLEAN Failure
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
    PACL Acl,
    ULONG AceRevision,
    ULONG StartingAceIndex,
    PVOID AceList,
    ULONG AceListLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAce(
    PACL Acl,
    ULONG Revision,
    ACCESS_MASK AccessMask,
    PSID Sid,
    BOOLEAN Success,
    BOOLEAN Failure
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAdjustPrivilege(
    IN ULONG Privilege,
    IN BOOLEAN NewValue,
    IN BOOLEAN ForThread,
    OUT PBOOLEAN OldValue
);

NTSYSAPI
NTSTATUS
NTAPI
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

NTSYSAPI
BOOLEAN
NTAPI
RtlAreAllAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreAnyAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess
);

NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
    IN PLUID LuidDest,
    IN PLUID LuidSrc
);

NTSYSAPI
VOID
NTAPI
RtlCopyLuidAndAttributesArray(
    ULONG Count,
    PLUID_AND_ATTRIBUTES Src,
    PLUID_AND_ATTRIBUTES Dest
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySidAndAttributesArray(
    ULONG Count,
    PSID_AND_ATTRIBUTES Src,
    ULONG SidAreaSize,
    PSID_AND_ATTRIBUTES Dest,
    PVOID SidArea,
    PVOID* RemainingSidArea,
    PULONG RemainingSidAreaSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PSID Sid,
    IN BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
    IN ULONG Length,
    IN PSID Destination,
    IN PSID Source
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
    PACL Acl,
    ULONG AclSize,
    ULONG AclRevision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(
    OUT PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
    PACL Acl,
    ULONG AceIndex
);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
    PSID Sid1,
    PSID Sid2
);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid (
    IN PSID Sid1,
    IN PSID Sid2
);

NTSYSAPI
BOOLEAN
NTAPI
RtlFirstFreeAce(
    PACL Acl,
    PACE* Ace
);

NTSYSAPI
PVOID
NTAPI
RtlFreeSid (
    IN PSID Sid
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
    PACL Acl,
    ULONG AceIndex,
    PVOID *Ace
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetControlSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT PULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN DaclPresent,
    OUT PACL *Dacl,
    OUT PBOOLEAN DaclDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PBOOLEAN SaclPresent,
    OUT PACL* Sacl,
    OUT PBOOLEAN SaclDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Group,
    OUT PBOOLEAN GroupDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Owner,
    OUT PBOOLEAN OwnerDefaulted
);

NTSYSAPI
BOOLEAN
NTAPI
RtlGetSecurityDescriptorRMControl(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PUCHAR RMControl
);

NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelf(IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
    IN OUT PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount
);

NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(IN ULONG SubAuthorityCount);

NTSYSAPI
ULONG
NTAPI
RtlLengthSid(IN PSID Sid);

NTSYSAPI
NTSTATUS
NTAPI
RtlMakeSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR AbsoluteSD,
    OUT PSECURITY_DESCRIPTOR SelfRelativeSD,
    IN OUT PULONG BufferLength);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationAcl(
    PACL Acl,
    PVOID Information,
    ULONG InformationLength,
    ACL_INFORMATION_CLASS InformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR SelfRelativeSD,
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

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD2(
    IN OUT PSECURITY_DESCRIPTOR SelfRelativeSD,
    OUT PULONG BufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetAttributesSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL Control,
    OUT PULONG Revision
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetControlSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN DaclPresent,
    IN PACL Dacl,
    IN BOOLEAN DaclDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID Group,
    IN BOOLEAN GroupDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetInformationAcl(
    PACL Acl,
    PVOID Information,
    ULONG InformationLength,
    ACL_INFORMATION_CLASS InformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID Owner,
    IN BOOLEAN OwnerDefaulted
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN SaclPresent,
    IN PACL Sacl,
    IN BOOLEAN SaclDefaulted
);

NTSYSAPI
VOID
NTAPI
RtlSetSecurityDescriptorRMControl(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PUCHAR RMControl
);

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
    IN PSID Sid
);

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
    IN PSID Sid,
    IN ULONG SubAuthority
);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(IN PSID Sid);

NTSYSAPI
BOOLEAN
NTAPI
RtlValidAcl(PACL Acl);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *ObjectDescriptor
);

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObject(
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    IN BOOLEAN IsDirectoryObject,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQuerySecurityObject(
    IN PSECURITY_DESCRIPTOR ObjectDescriptor,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
    IN ULONG DescriptorLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObject(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token
);

//
// Single-Character Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlLargeIntegerToChar(
    IN PLARGE_INTEGER Value,
    IN ULONG Base,
    IN ULONG Length,
    IN OUT PCHAR String
);

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(CHAR Source);

NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(WCHAR Source);

NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(IN WCHAR Source);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToChar(
    IN ULONG Value,
    IN ULONG Base,
    IN ULONG Length,
    IN OUT PCHAR String
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN OUT LPWSTR String
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    IN ULONG Value,
    IN ULONG Base,
    IN OUT PUNICODE_STRING String
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
    PCSZ String,
    ULONG Base,
    PULONG Value
);

//
// Byte Swap Functions
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || \
    ((defined(_M_AMD64) || \
     defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))

unsigned short __cdecl _byteswap_ushort(unsigned short);
unsigned long  __cdecl _byteswap_ulong (unsigned long);
unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)
#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))

#else

USHORT
FASTCALL
RtlUshortByteSwap(IN USHORT Source);

ULONG
FASTCALL
RtlUlongByteSwap(IN ULONG Source);

ULONGLONG
FASTCALL
RtlUlonglongByteSwap(IN ULONGLONG Source);

#endif

//
// Unicode->Ansi String Functions
//
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(IN PCUNICODE_STRING UnicodeString);

#ifdef NTOS_MODE_USER

#define RtlUnicodeStringToAnsiSize(STRING) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(STRING) :                     \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

//
// Unicode->OEM String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    IN OUT POEM_STRING DestinationString,
    IN PCUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
    PCHAR OemString,
    ULONG OemSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToOemSize(IN PCUNICODE_STRING UnicodeString);

#ifdef NTOS_MODE_USER

#define RtlUnicodeStringToOemSize(STRING) (                             \
    NLS_MB_OEM_CODE_PAGE_TAG ?                                          \
    RtlxUnicodeStringToOemSize(STRING) :                                \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR)           \
)

#define RtlUnicodeStringToCountedOemSize(STRING) (                      \
    (ULONG)(RtlUnicodeStringToOemSize(STRING) - sizeof(ANSI_NULL))      \
)

#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
    PCHAR OemString,
    ULONG OemSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

//
// Unicode->MultiByte String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    PCHAR MbString,
    ULONG MbSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
    PCHAR MbString,
    ULONG MbSize,
    PULONG ResultSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
    PULONG MbSize,
    PWCHAR UnicodeString,
    ULONG UnicodeSize
);

NTSYSAPI
ULONG
NTAPI
RtlxOemStringToUnicodeSize(IN PCOEM_STRING OemString);

//
// OEM to Unicode Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    POEM_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    IN PCHAR OemString,
    ULONG BytesInOemString
);

#ifdef NTOS_MODE_USER

#define RtlOemStringToUnicodeSize(STRING) (                             \
    NLS_MB_OEM_CODE_PAGE_TAG ?                                          \
    RtlxOemStringToUnicodeSize(STRING) :                                \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)              \
)

#define RtlOemStringToCountedUnicodeSize(STRING) (                      \
    (ULONG)(RtlOemStringToUnicodeSize(STRING) - sizeof(UNICODE_NULL))   \
)

#endif

//
// Ansi->Unicode String Functions
//
NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
    PCANSI_STRING AnsiString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

#ifdef NTOS_MODE_USER

#define RtlAnsiStringToUnicodeSize(STRING) (                        \
    NLS_MB_CODE_PAGE_TAG ?                                          \
    RtlxAnsiStringToUnicodeSize(STRING) :                           \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)          \
)

#endif

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING Destination,
    IN PCSZ Source
);

//
// Unicode String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
    PUNICODE_STRING Destination,
    PCWSTR Source
);

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
    PUNICODE_STRING Destination,
    PCUNICODE_STRING Source
);

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString
);

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

#ifdef NTOS_MODE_USER

NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN AllocateDestinationString
);

#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString
);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
    IN CONST UNICODE_STRING *String,
    IN BOOLEAN CaseInSensitive,
    IN ULONG HashAlgorithm,
    OUT PULONG HashValue
);

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString);

NTSYSAPI
ULONG
NTAPI
RtlIsTextUnicode(
    PVOID Buffer,
    ULONG Length,
    ULONG *Flags
);

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
    PCANSI_STRING String1,
    PCANSI_STRING String2,
    BOOLEAN CaseInsensitive
);

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
    PCUNICODE_STRING String,
    ULONG Base,
    PULONG Value
);

//
// Ansi String Functions
//
NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(IN PANSI_STRING AnsiString);

NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSZ SourceString
);

//
// OEM String Functions
//
NTSYSAPI
VOID
NTAPI
RtlFreeOemString(IN POEM_STRING OemString);

//
// MultiByte->Unicode String Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    PWCHAR UnicodeString,
    ULONG UnicodeSize,
    PULONG ResultSize,
    PCSTR MbString,
    ULONG MbSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
    PULONG UnicodeSize,
    PCSTR MbString,
    ULONG MbSize
);

//
// Atom Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAtomToAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN PWSTR AtomName,
    OUT PRTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAtomTable(
    IN ULONG TableSize,
    IN OUT PRTL_ATOM_TABLE *AtomTable
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAtomFromAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyAtomTable(IN PRTL_ATOM_TABLE AtomTable);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryAtomInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN RTL_ATOM Atom,
    IN OUT PULONG RefCount OPTIONAL,
    IN OUT PULONG PinCount OPTIONAL,
    IN OUT PWSTR AtomName OPTIONAL,
    IN OUT PULONG NameLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
RtlPinAtomInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
RtlLookupAtomInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN PWSTR AtomName,
    OUT PRTL_ATOM Atom
);

//
// Memory Functions
//
NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlong(
    IN PVOID Destination,
    IN ULONG Length,
    IN ULONG Fill
);

//
// Process Management Functions
//
NTSYSAPI
VOID
NTAPI
RtlAcquirePebLock(VOID);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParameters (
    OUT PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    IN PUNICODE_STRING ImagePathName OPTIONAL,
    IN PUNICODE_STRING DllPath OPTIONAL,
    IN PUNICODE_STRING CurrentDirectory OPTIONAL,
    IN PUNICODE_STRING CommandLine OPTIONAL,
    IN PWSTR Environment OPTIONAL,
    IN PUNICODE_STRING WindowTitle OPTIONAL,
    IN PUNICODE_STRING DesktopInfo OPTIONAL,
    IN PUNICODE_STRING ShellInfo OPTIONAL,
    IN PUNICODE_STRING RuntimeInfo OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserProcess(
    IN PUNICODE_STRING ImageFileName,
    IN ULONG Attributes,
    IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    IN PSECURITY_DESCRIPTOR ProcessSecutityDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor OPTIONAL,
    IN HANDLE ParentProcess OPTIONAL,
    IN BOOLEAN CurrentDirectory,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    OUT PRTL_USER_PROCESS_INFORMATION ProcessInfo
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserThread(
    IN HANDLE ProcessHandle,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN CreateSuspended,
    IN LONG StackZeroBits,
    IN ULONG StackReserve,
    IN ULONG StackCommit,
    IN PTHREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter,
    IN OUT PHANDLE ThreadHandle,
    IN OUT PCLIENT_ID ClientId
);

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlDeNormalizeProcessParams(IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyProcessParameters(IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSYSAPI
VOID
NTAPI
RtlExitUserThread(NTSTATUS Status);

NTSYSAPI
VOID
NTAPI
RtlInitializeContext(
    IN HANDLE ProcessHandle,
    OUT PCONTEXT ThreadContext,
    IN PVOID ThreadStartParam  OPTIONAL,
    IN PTHREAD_START_ROUTINE ThreadStartAddress,
    IN PINITIAL_TEB InitialTeb
);

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlNormalizeProcessParams(IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSYSAPI
VOID
NTAPI
RtlReleasePebLock(VOID);

NTSYSAPI
VOID
NTAPI
RtlSetProcessIsCritical(
    IN BOOLEAN NewValue,
    OUT PBOOLEAN OldValue OPTIONAL,
    IN BOOLEAN IsWinlogon
);

#define NtCurrentPeb() (NtCurrentTeb()->ProcessEnvironmentBlock)

//
// Environment/Path Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironment(
    BOOLEAN Inherit,
    PWSTR *Environment
);

NTSYSAPI
VOID
NTAPI
RtlDestroyEnvironment(PWSTR Environment);

NTSYSAPI
BOOLEAN
NTAPI
RtlDoesFileExists_U(PWSTR FileName);

NTSYSAPI
ULONG
NTAPI
RtlDetermineDosPathNameType_U(PCWSTR Path);

NTSYSAPI
ULONG
NTAPI
RtlDosSearchPath_U(
    WCHAR *sp,
    WCHAR *name,
    WCHAR *ext,
    ULONG buf_sz,
    WCHAR *buffer,
    WCHAR **shortname
);

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    IN PCWSTR DosPathName,
    OUT PUNICODE_STRING NtPathName,
    OUT PCWSTR *NtFileNamePart,
    OUT CURDIR *DirectoryInfo
);

NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings_U(
    PWSTR Environment,
    PUNICODE_STRING Source,
    PUNICODE_STRING Destination,
    PULONG Length
);

NTSYSAPI
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    ULONG MaximumLength,
    PWSTR Buffer
);

NTSYSAPI
ULONG
NTAPI
RtlGetFullPathName_U(
    const WCHAR *dosname,
    ULONG size,
    WCHAR *buf,
    WCHAR **shortname
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
    IN PCUNICODE_STRING Name,
    IN OUT POEM_STRING OemName OPTIONAL,
    IN OUT PBOOLEAN NameContainsSpaces OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    PWSTR Environment,
    PUNICODE_STRING Name,
    PUNICODE_STRING Value
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(PUNICODE_STRING name);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    PWSTR *Environment,
    PUNICODE_STRING Name,
    PUNICODE_STRING Value
);

//
// Critical Section/Resource Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection (
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    IN PRTL_CRITICAL_SECTION CriticalSection,
    IN ULONG SpinCount
);

NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
VOID
NTAPI
RtlpUnWaitCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpWaitForCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
);

NTSYSAPI
VOID
NTAPI
RtlConvertExclusiveToShared(
    IN PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlDeleteResource(
    IN PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlDumpResource(
    IN PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlInitializeResource(
    IN PRTL_RESOURCE Resource
);

NTSYSAPI
VOID
NTAPI
RtlReleaseResource(
    IN PRTL_RESOURCE Resource
);

//
// Compression Functions
//
NTSYSAPI
NTSTATUS
NTAPI
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

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer(
    IN USHORT CompressionFormat,
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
    IN USHORT CompressionFormatAndEngine,
    OUT PULONG CompressBufferWorkSpaceSize,
    OUT PULONG CompressFragmentWorkSpaceSize
);

//
// Debug Info Functions
//
NTSYSAPI
PRTL_DEBUG_BUFFER
NTAPI
RtlCreateQueryDebugBuffer(
    IN ULONG Size,
    IN BOOLEAN EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(IN PRTL_DEBUG_BUFFER DebugBuffer);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    IN ULONG ProcessId,
    IN ULONG DebugInfoClassMask,
    IN OUT PRTL_DEBUG_BUFFER DebugBuffer
);

//
// Bitmap Functions
//
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length
);

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG Length
);

NTSYSAPI
VOID
NTAPI
RtlClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToClear
);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex
);

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex
);

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG BitMapBuffer,
    IN ULONG SizeOfBitMap
);

NTSYSAPI
VOID
NTAPI
RtlSetBits (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToSet
);

//
// Timer Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimer(
    HANDLE TimerQueue,
    PHANDLE phNewTimer,
    WAITORTIMERCALLBACKFUNC Callback,
    PVOID Parameter,
    ULONG DueTime,
    ULONG Period,
    ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimerQueue(PHANDLE TimerQueue);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpdateTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
);

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueue(HANDLE TimerQueue);

//
// Debug Functions
//
ULONG
__cdecl
DbgPrint(
    IN PCH  Format,
    IN ...
);

VOID
NTAPI
DbgBreakPoint(VOID);

//
// Handle Table Functions
//
NTSYSAPI
PRTL_HANDLE_TABLE_ENTRY
NTAPI
RtlAllocateHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN OUT PULONG Index
);

NTSYSAPI
VOID
NTAPI
RtlDestroyHandleTable(IN PRTL_HANDLE_TABLE HandleTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
);

NTSYSAPI
VOID
NTAPI
RtlInitializeHandleTable(
    IN ULONG TableSize,
    IN ULONG HandleSize,
    IN PRTL_HANDLE_TABLE HandleTable
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidIndexHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN ULONG Index,
    OUT PRTL_HANDLE_TABLE_ENTRY *Handle
);

//
// PE Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlFindMessage(
    IN PVOID BaseAddress,
    IN ULONG Type,
    IN ULONG Language,
    IN ULONG MessageId,
    OUT PRTL_MESSAGE_RESOURCE_ENTRY *MessageResourceEntry
);

NTSYSAPI
ULONG
NTAPI
RtlGetNtGlobalFlags(VOID);

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
    PVOID  BaseAddress,
    BOOLEAN bFlag,
    ULONG Directory,
    PULONG Size
);

NTSYSAPI
PVOID
NTAPI
RtlImageRvaToVa(
    PIMAGE_NT_HEADERS NtHeader,
    PVOID BaseAddress,
    ULONG Rva,
    PIMAGE_SECTION_HEADER *SectionHeader
);

NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(IN PVOID BaseAddress);

NTSYSAPI
PIMAGE_SECTION_HEADER
NTAPI
RtlImageRvaToSection(
    PIMAGE_NT_HEADERS NtHeader,
    PVOID BaseAddress,
    ULONG Rva
);

//
// Registry Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
    ULONG RelativeTo,
    PWSTR Path
);

NTSYSAPI
NTSTATUS
NTAPI
RtlFormatCurrentUserKeyPath(IN OUT PUNICODE_STRING KeyPath);

NTSYSAPI
NTSTATUS
NTAPI
RtlpNtOpenKey(
    OUT HANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Unused
);

NTSYSAPI
NTSTATUS
NTAPI
RtlOpenCurrentUser(
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PVOID Context,
    IN PVOID Environment
);

NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
    ULONG RelativeTo,
    PCWSTR Path,
    PCWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength
);

//
// NLS Functions
//
NTSYSAPI
VOID
NTAPI
RtlInitNlsTables(
    IN PUSHORT AnsiTableBase,
    IN PUSHORT OemTableBase,
    IN PUSHORT CaseTableBase,
    OUT PNLSTABLEINFO NlsTable
);

NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
    IN PUSHORT TableBase,
    OUT PCPTABLEINFO CodePageTable
);

NTSYSAPI
VOID
NTAPI
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable);

#if defined(NTOS_MODE_USER) && !defined(NO_RTL_INLINES)

//
// Misc conversion functions
//
static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlConvertLongToLargeInteger(
    LONG SignedInteger
)
{
    LARGE_INTEGER Result;

    Result.QuadPart = SignedInteger;
    return Result;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedIntegerMultiply(
    LONG Multiplicand,
    LONG Multiplier
)
{
    LARGE_INTEGER Product;

    Product.QuadPart = (LONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return Product;
}

static __inline
ULONG
NTAPI_INLINE
RtlEnlargedUnsignedDivide(
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN PULONG Remainder OPTIONAL
)
{
    ULONG Quotient;

    Quotient = (ULONG)(Dividend.QuadPart / Divisor);
    if (Remainder) {
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    }

    return Quotient;
}

static __inline
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedUnsignedMultiply(
    ULONG Multiplicand,
    ULONG Multiplier
)
{
    LARGE_INTEGER Product;

    Product.QuadPart = (ULONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return Product;
}
#endif

NTSYSAPI
ULONG
NTAPI
RtlUniform(PULONG Seed);

//
// Network Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(
    IN LPWSTR IpString,
    IN ULONG Base,
    OUT PULONG PtrToIpAddr,
    OUT PULONG IpAddr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressA(
    IN LPSTR Name,
    OUT PULONG Unknown,
    OUT PVOID IpAddr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
    IN LPWSTR Name,
    OUT PULONG Unknown,
    OUT PVOID IpAddr
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExA(
    IN LPSTR AddressName,
    IN PVOID Address,
    IN PULONG ScopeId,
    IN PUSHORT Port
);

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(
    IN LPWSTR AddressName,
    IN PVOID Address,
    IN PULONG ScopeId,
    IN PUSHORT Port
);


//
// Time Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryTimeZoneInformation(LPTIME_ZONE_INFORMATION TimeZoneInformation);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
    IN ULONG SecondsSince1970,
    OUT PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetTimeZoneInformation(LPTIME_ZONE_INFORMATION TimeZoneInformation);

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
    PTIME_FIELDS TimeFields,
    PLARGE_INTEGER Time
);

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
    PLARGE_INTEGER Time,
    PTIME_FIELDS TimeFields
);

//
// Version Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
    IN PRTL_OSVERSIONINFOEXW VersionInfo,
    IN ULONG TypeMask,
    IN ULONGLONG ConditionMask
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation);

NTSYSAPI
BOOLEAN
NTAPI
RtlGetNtProductType(OUT PNT_PRODUCT_TYPE ProductType);

#endif
