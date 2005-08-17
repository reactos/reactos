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
#include <ntnls.h>

/* PROTOTYPES ****************************************************************/

/*
 * Error and Exception Functions
 */
PVOID
STDCALL
RtlAddVectoredExceptionHandler(
    IN ULONG FirstHandler,
    IN PVECTORED_EXCEPTION_HANDLER VectoredHandler
);

VOID
STDCALL
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
);

PVOID
STDCALL
RtlEncodePointer(IN PVOID Pointer);

PVOID
STDCALL
RtlDecodePointer(IN PVOID Pointer);

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
    IN SIZE_T SizeToReserve OPTIONAL,
    IN SIZE_T SizeToCommit OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
);

DWORD
STDCALL
RtlCompactHeap(
    HANDLE heap,
    DWORD flags
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

ULONG
STDCALL
RtlGetProcessHeaps(
    ULONG HeapCount,
    HANDLE *HeapArray
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
    IN OUT PISECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
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
    PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
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

VOID
STDCALL
RtlMapGenericMask(
    PACCESS_MASK AccessMask,
    PGENERIC_MAPPING GenericMapping
);

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
    IN PISECURITY_DESCRIPTOR_RELATIVE SelfRelativeSD,
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
    IN PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptorInput,
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

NTSTATUS
STDCALL
RtlDeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *ObjectDescriptor
);

NTSTATUS
STDCALL
RtlNewSecurityObject(
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    IN BOOLEAN IsDirectoryObject,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
);

NTSTATUS
STDCALL
RtlQuerySecurityObject(
    IN PSECURITY_DESCRIPTOR ObjectDescriptor,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
    IN ULONG DescriptorLength,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
RtlSetSecurityObject(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token
);

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

NTSTATUS
STDCALL
RtlDowncaseUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PUNICODE_STRING UniSource,
    IN BOOLEAN AllocateDestinationString
);

NTSTATUS
STDCALL
RtlDuplicateUnicodeString(
    IN INT AddNull,
    IN PUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString
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

NTSTATUS
STDCALL
RtlHashUnicodeString(
    IN CONST UNICODE_STRING *String,
    IN BOOLEAN CaseInSensitive,
    IN ULONG HashAlgorithm,
    OUT PULONG HashValue
);

VOID
STDCALL
RtlInitUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCWSTR SourceString);

ULONG
STDCALL
RtlIsTextUnicode(
    PVOID Buffer,
    ULONG Length,
    ULONG *Flags
);

BOOLEAN
STDCALL
RtlPrefixUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInsensitive
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

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams(IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

NTSTATUS
STDCALL
RtlDestroyProcessParameters(IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

VOID
STDCALL
RtlExitUserThread(NTSTATUS Status);

VOID
STDCALL
RtlInitializeContext(
    IN HANDLE ProcessHandle,
    OUT PCONTEXT ThreadContext,
    IN PVOID ThreadStartParam  OPTIONAL,
    IN PTHREAD_START_ROUTINE ThreadStartAddress,
    IN PINITIAL_TEB InitialTeb
);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams(IN PRTL_USER_PROCESS_PARAMETERS ProcessParameters);

VOID
STDCALL
RtlReleasePebLock(VOID);

/*
 * Environment/Path Functions
 */
NTSTATUS
STDCALL
RtlCreateEnvironment(
    BOOLEAN Inherit,
    PWSTR *Environment
);

VOID
STDCALL
RtlDestroyEnvironment(PWSTR Environment);

BOOLEAN
STDCALL
RtlDoesFileExists_U(PWSTR FileName);

ULONG
STDCALL
RtlDetermineDosPathNameType_U(PCWSTR Path);

ULONG
STDCALL
RtlDosSearchPath_U(
    WCHAR *sp,
    WCHAR *name,
    WCHAR *ext,
    ULONG buf_sz,
    WCHAR *buffer,
    WCHAR **shortname
);

BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U(
    PWSTR DosName,
    PUNICODE_STRING NtName,
    PWSTR *ShortName,
    PCURDIR CurrentDirectory
);

NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U(
    PWSTR Environment,
    PUNICODE_STRING Source,
    PUNICODE_STRING Destination,
    PULONG Length
);

ULONG
STDCALL
RtlGetCurrentDirectory_U(
    ULONG MaximumLength,
    PWSTR Buffer
);

ULONG
STDCALL
RtlGetFullPathName_U(
    const WCHAR *dosname,
    ULONG size,
    WCHAR *buf,
    WCHAR **shortname
);

BOOLEAN
STDCALL
RtlIsNameLegalDOS8Dot3(
    IN PUNICODE_STRING UnicodeName,
    IN PANSI_STRING AnsiName,
    PBOOLEAN Unknown
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
RtlSetCurrentDirectory_U(PUNICODE_STRING name);

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
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS
STDCALL
RtlEnterCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS
STDCALL
RtlInitializeCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS
STDCALL
RtlInitializeCriticalSectionAndSpinCount(
    IN PRTL_CRITICAL_SECTION CriticalSection,
    IN ULONG SpinCount
);

NTSTATUS
STDCALL
RtlLeaveCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpUnWaitCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

NTSTATUS
STDCALL
RtlpWaitForCriticalSection(
    IN PRTL_CRITICAL_SECTION CriticalSection
);

BOOLEAN
STDCALL
RtlAcquireResourceExclusive(
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
);

BOOLEAN
STDCALL
RtlAcquireResourceShared(
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
);

VOID
STDCALL
RtlConvertExclusiveToShared(
    IN PRTL_RESOURCE Resource
);

VOID
STDCALL
RtlConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
);

VOID
STDCALL
RtlDeleteResource(
    IN PRTL_RESOURCE Resource
);

VOID
STDCALL
RtlDumpResource(
    IN PRTL_RESOURCE Resource
);

VOID
STDCALL
RtlInitializeResource(
    IN PRTL_RESOURCE Resource
);

VOID
STDCALL
RtlReleaseResource(
    IN PRTL_RESOURCE Resource
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
 * Debug Info Functions
 */
PDEBUG_BUFFER
STDCALL
RtlCreateQueryDebugBuffer(
    IN ULONG Size,
    IN BOOLEAN EventPair
);

NTSTATUS
STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER DebugBuffer);

NTSTATUS
STDCALL
RtlQueryProcessDebugInformation(
    IN ULONG ProcessId,
    IN ULONG DebugInfoClassMask,
    IN OUT PDEBUG_BUFFER DebugBuffer
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
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex
);

ULONG
STDCALL
RtlFindClearBitsAndSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex
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
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToSet
);

/*
 * Timer Functions
 */
NTSTATUS
STDCALL
RtlCreateTimer(
    HANDLE TimerQueue,
    PHANDLE phNewTimer,
    WAITORTIMERCALLBACKFUNC Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
);

NTSTATUS
STDCALL
RtlCreateTimerQueue(PHANDLE TimerQueue);

NTSTATUS
STDCALL
RtlDeleteTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
);

NTSTATUS
STDCALL
RtlUpdateTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
);

NTSTATUS
STDCALL
RtlDeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
);

NTSTATUS
STDCALL
RtlDeleteTimerQueue(HANDLE TimerQueue);

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

/*
 * Handle Table Functions
 */
PRTL_HANDLE_TABLE_ENTRY
STDCALL
RtlAllocateHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN OUT PULONG Index
);

VOID
STDCALL
RtlDestroyHandleTable(IN PRTL_HANDLE_TABLE HandleTable);

BOOLEAN
STDCALL
RtlFreeHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
);

VOID
STDCALL
RtlInitializeHandleTable(
    IN ULONG TableSize,
    IN ULONG HandleSize,
    IN PRTL_HANDLE_TABLE HandleTable
);

BOOLEAN
STDCALL
RtlIsValidHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN PRTL_HANDLE_TABLE_ENTRY Handle
);

BOOLEAN
STDCALL
RtlIsValidIndexHandle(
    IN PRTL_HANDLE_TABLE HandleTable,
    IN ULONG Index,
    OUT PRTL_HANDLE_TABLE_ENTRY *Handle
);

/*
 * PE Functions
 */
NTSTATUS
STDCALL
RtlFindMessage(
    IN PVOID BaseAddress,
    IN ULONG Type,
    IN ULONG Language,
    IN ULONG MessageId,
    OUT PRTL_MESSAGE_RESOURCE_ENTRY *MessageResourceEntry
);

ULONG
STDCALL
RtlGetNtGlobalFlags(VOID);

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
RtlCheckRegistryKey(
    ULONG RelativeTo,
    PWSTR Path
);

NTSTATUS
STDCALL
RtlFormatCurrentUserKeyPath(IN OUT PUNICODE_STRING KeyPath);

NTSTATUS
STDCALL
RtlpNtOpenKey(
    OUT HANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Unused
);

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

ULONG
STDCALL
RtlUniform(PULONG Seed);

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

#endif
