/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    regclass.h

Abstract:

    This file contains declarations needed for manipulating
    the portion of the registry that contains class registrations

Author:

    Adam Edwards (adamed) 14-Nov-1997

Notes:

--*/

#if defined( LOCAL )

//
// Declarations
//

#define LENGTH( str )   ( sizeof( str ) - sizeof( UNICODE_NULL ))
#define INIT_SPECIALKEY(x) {LENGTH(x), LENGTH(x), x}

#define REG_CLASSES_FIRST_DISTINCT_ICH 10

#define REG_CHAR_SIZE sizeof(WCHAR)

#define REG_USER_HIVE_NAME L"\\Registry\\User"
#define REG_USER_HIVE_NAMELEN LENGTH(REG_USER_HIVE_NAME)
#define REG_USER_HIVE_NAMECCH (REG_USER_HIVE_NAMELEN / REG_CHAR_SIZE)

#define REG_USER_HIVE_CLASSES_SUFFIX L"_Classes"
#define REG_USER_HIVE_CLASSES_SUFFIXLEN LENGTH(REG_USER_HIVE_CLASSES_SUFFIX)
#define REG_USER_HIVE_CLASSES_SUFFIXCCH (REG_USER_HIVE_CLASSES_SUFFIXLEN / REG_CHAR_SIZE)

#define REG_MACHINE_CLASSES_HIVE_NAME L"\\Registry\\Machine\\Software\\Classes"
#define REG_MACHINE_CLASSES_HIVE_NAMELEN LENGTH(REG_MACHINE_CLASSES_HIVE_NAME)
#define REG_MACHINE_CLASSES_HIVE_NAMECCH (REG_MACHINE_CLASSES_HIVE_NAMELEN / REG_CHAR_SIZE)

#define REG_USER_HIVE_LINK_TREE L"\\Software\\Classes"

#define REG_CLASSES_HIVE_MIN_NAMELEN REG_USER_HIVE_CLASSES_SUFFIXLEN + REG_USER_HIVE_NAMELEN

//
// The difference between these two paths
// \Registry\User\<sid>_Classes and
// \Registry\User\<siid>\Software\Classes
//
// plus extra for the translation from machine to user -- take into account the sid
//
#define REG_CLASSES_SUBTREE_PADDING 128

#define REG_MAX_CLASSKEY_LEN 384
#define REG_MAX_CLASSKEY_CCH (REG_MAX_CLASSKEY_LEN / REG_CHAR_SIZE)

#define REG_MAX_KEY_LEN 512
#define REG_MAX_KEY_CCH (REG_MAX_KEY_LEN / REG_CHAR_SIZE)

#define REG_MAX_KEY_PATHLEN 65535


//
// HKCR Handle Tags for Per-user Class Registration.
//
// Subkeys of HKCR up to and including a class registration parent key are tagged
// by setting two free bits in their handle value (the lower two bits of a handle
// are free to be used as tags).  This makes it easy to tell if a key is in
// HKCR and needs special treatment.  After the class registration part of a path,
// this marker is not needed since such keys do not require special treatment
// for enumeration, opening, and deletion.
//

//
// Note that for the sake of speed we are using 1 bit instead of a specific pattern of
// two bits.  Currently, bit 0 is used to mark remote handles.  Bit 2 is used in the
// server only to mark restricted keys.  Locally, we use it to mark hkcr keys.  More
// Here is a list of valid combinations -- unused bits must be 0. Invalid means that
// in the current implementation, you should never see it in that part of the registry.
//

//
//  Local                       Server                               Client (application sees these)
//  00  (non HKCR, unused)      00 (unrestricted, unused)            00 (non HKCR, local)
//  01  Invalid (HKCR, unused)  01 Invalid (unrestricted, unused)    01 (non HKCR, remote)
//  10  (HKCR, unused)          10 (restricted, unused)              10 (HKCR, local)
//  11  Invalid (HKCR, unused)  11 Invalid (restricted, unused)      11 Invalid (HKCR, remote)
//

//
//  Note that we could use either 10 or 11 to mark HKCR handles -- we chose 10 for simplicity's
//  sake since it simply involves oring in a bit. This can be changed in the future
//  if yet another bit pattern is needed. Otherwise, clients never see 11 -- they only see
//  00, 01, and 10. Note that these bits must be propagated to the local portion. This is done
//  simply by leaving the bits as-is, because local doesn't use any of the bits.  Note that
//  we would be broken if the bits needed to propagate to server for some reason, since it
//  is using bit 2 already.  We do not allow HKCR as a remote handle, however, so this is
//  not a problem.
//

#define REG_CLASS_HANDLE_MASK 0x3

#define REG_CLASS_HANDLE_VALUE 0x2
#define REG_CLASS_IS_SPECIAL_KEY( Handle )     ( (LONG) ( ( (ULONG_PTR) (Handle) ) & REG_CLASS_HANDLE_VALUE ) )
#define REG_CLASS_SET_SPECIAL_KEY( Handle )    ( (HANDLE) ( (  (ULONG_PTR) (Handle) ) | \
                                                            REG_CLASS_HANDLE_VALUE ) )

#define REG_CLASS_RESET_SPECIAL_KEY( Handle )  ( (HANDLE) ( ( ( (ULONG_PTR) (Handle) ) & ~REG_CLASS_HANDLE_MASK )))

#if defined(_REGCLASS_MALLOC_INSTRUMENTED_)

extern RTL_CRITICAL_SECTION gRegClassHeapCritSect;
extern DWORD                gcbAllocated;
extern DWORD                gcAllocs;
extern DWORD                gcbMaxAllocated;
extern DWORD                gcMaxAllocs;
extern PVOID                gpvAllocs;

__inline PVOID RegClassHeapAlloc(SIZE_T cbSize)
{
    PVOID pvAllocation;

    pvAllocation = RtlAllocateHeap(RtlProcessHeap(), 0, cbSize + sizeof(SIZE_T));

    RtlEnterCriticalSection(&gRegClassHeapCritSect);

    if (pvAllocation) {
        gcbAllocated += cbSize;
        gcAllocs ++;
        (ULONG_PTR) gpvAllocs ^= (ULONG_PTR) pvAllocation;

        if (gcAllocs > gcMaxAllocs) {
            gcMaxAllocs = gcAllocs;
        }

        if (gcbAllocated > gcbMaxAllocated) {
            gcbMaxAllocated = gcbAllocated;
        }
    }

    RtlLeaveCriticalSection(&gRegClassHeapCritSect);

    *((SIZE_T*) pvAllocation) = cbSize;

    ((SIZE_T*) pvAllocation) ++;

    return pvAllocation;
}

__inline BOOLEAN RegClassHeapFree(PVOID pvAllocation)
{
    BOOLEAN bRetVal;
    SIZE_T  cbSize;

    ((SIZE_T*) pvAllocation) --;

    cbSize = *((SIZE_T*) pvAllocation);

    bRetVal = RtlFreeHeap(RtlProcessHeap(), 0, pvAllocation);

    RtlEnterCriticalSection(&gRegClassHeapCritSect);

    gcbAllocated -= cbSize;
    gcAllocs --;

    (ULONG_PTR) gpvAllocs ^= (ULONG_PTR) pvAllocation;

    RtlLeaveCriticalSection(&gRegClassHeapCritSect);

    if (!bRetVal) {
        DbgBreakPoint();
    }

    return bRetVal;
}

#else // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

#define RegClassHeapAlloc(x) RtlAllocateHeap(RtlProcessHeap(), 0, x)
#define RegClassHeapFree(x) RtlFreeHeap(RtlProcessHeap(), 0, x)

#endif // defined(_REGCLASS_MALLOC_INSTRUMENTED_)

enum
{
    LOCATION_MACHINE =     0x1,
    LOCATION_USER =        0x2,
    LOCATION_BOTH =        0x3
};


//
// SKeySemantics
//
// This structure is the result of parsing a registry key full path
//
// BUGBUG: This structure, along with the current parsing code, needs to
// be overhauled.  Originally, it was designed to do one thing. Now, it
// identifies several parts of keys.  The original goal was speed --
// we attempted to touch the least amount of string (memory) possible.
// As more functionality was added to the parser, this became more complex.
// A better solution would pay more attention to a generic, straightforward
// way of parsing the key -- things have become far too convoluted in
// an attempt to be fast.
//

typedef struct _SKeySemantics
{
    /* out */     unsigned _fUser              : 1;     // this key is rooted in the user hive
    /* out */     unsigned _fMachine           : 1;     // this key is rooted in the machine hive
    /* out */     unsigned _fCombinedClasses   : 1;     // this key is rooted in the combined classes hive
    /* out */     unsigned _fClassRegistration : 1;     // this key is a class registration key
    /* out */     unsigned _fClassRegParent    : 1;     // this key is a special key (parent of a class reg key)
    /* out */     unsigned _fAllocedNameBuf    : 1;     // nonzero if _pFullPath was realloc'd and needs to be freed
    /* out */     USHORT   _ichKeyStart;                // index to start of a class reg after
                                                        //     \\software\\classes in the returned full path
    /* out */     USHORT   _cbPrefixLen;                // length of prefix
    /* out */     USHORT   _cbSpecialKey;               // length of special key
    /* out */     USHORT   _cbClassRegKey;              // length of class reg key name
    /* in, out */ ULONG    _cbFullPath;                 // size of the KEY_NAME_INFORMATION passed in
    /* out */     PKEY_NAME_INFORMATION _pFullPath;     // address of an OBJECT_NAME_INFORMATION structure
} SKeySemantics;


//
// External Prototypes
//

//
// Opens the HKCR predefined handle with the combined view
//
error_status_t OpenCombinedClassesRoot(
    IN REGSAM samDesired,
    OUT HANDLE * phKey);

//
// Parses a registry key and returns results
//
NTSTATUS BaseRegGetKeySemantics(
    HKEY hkParent,
    PUNICODE_STRING pSubKey,
    SKeySemantics* pKeySemantics);
//
// Frees resources associated with an SKeySemantics structure
//
void BaseRegReleaseKeySemantics(SKeySemantics* pKeySemantics);

//
// Opens a class key that exists in either
// HKLM or HKCU
//
NTSTATUS BaseRegOpenClassKey(
    HKEY            hKey,
    PUNICODE_STRING lpSubKey,
    DWORD           dwOptions,
    REGSAM          samDesired,
    PHKEY           phkResult);

//
// Opens a class key from a specified set
// of locations
//
NTSTATUS BaseRegOpenClassKeyFromLocation(
    SKeySemantics*  pKeyInfo,
    HKEY            hKey,
    PUNICODE_STRING lpSubKey,
    REGSAM          samDesired,
    DWORD           dwLocation,
    HKEY*           phkResult);

//
// Returns key objects for the user and machine
// versions of a key
//
NTSTATUS BaseRegGetUserAndMachineClass(
    SKeySemantics*  pKeySemantics,
    HKEY            Key,
    REGSAM          samDesired,
    PHKEY           phkMachine,
    PHKEY           phkUser);


//
// Internal Prototypes
//

USHORT BaseRegGetUserPrefixLength(
    PUNICODE_STRING pFullPath);

USHORT BaseRegCchSpecialKeyLen(
    PUNICODE_STRING pFullPath,
    USHORT          ichSpecialKeyStart,
    SKeySemantics*  pKeySemantics);

NTSTATUS BaseRegTranslateToMachineClassKey(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pMachineClassKey,
    USHORT*         pPrefixLen);

NTSTATUS BaseRegTranslateToUserClassKey(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pUserClassKey,
    USHORT*         pPrefixLen);

NTSTATUS BaseRegOpenClassKeyRoot(
    SKeySemantics*  pKeyInfo,
    PHKEY           phkClassRoot,
    PUNICODE_STRING pClassKeyPath,
    BOOL            fMachine);

NTSTATUS BaseRegMapClassRegistrationKey(
    HKEY              hKey,
    PUNICODE_STRING   pSubKey,
    SKeySemantics*    pKeyInfo,
    PUNICODE_STRING   pDestSubKey,
    BOOL*             pfRetryOnAccessDenied,
    PHKEY             phkDestResult,
    PUNICODE_STRING*  ppSubKeyResult);

NTSTATUS BaseRegMapClassOnAccessDenied(
    SKeySemantics*    pKeySemantics,
    PHKEY             phkDest,
    PUNICODE_STRING   pDestSubKey,
    BOOL*             pfRetryOnAccessDenied);

NTSTATUS GetFixedKeyInfo(
    HKEY     hkUser,
    HKEY     hkMachine,
    LPDWORD  pdwUserValues,
    LPDWORD  pdwMachineValues,
    LPDWORD  pdwUserMaxDataLen,
    LPDWORD  pdwMachineMaxDataLen,
    LPDWORD  pdwMaxValueNameLen);

BOOL InitializeClassesNameSpace();

extern BOOL gbCombinedClasses;


//
// Inline functions
//

enum
{
    REMOVEPREFIX_DISCARD_INITIAL_PATHSEP = 0,
    REMOVEPREFIX_KEEP_INITIAL_PATHSEP = 1
};

__inline void KeySemanticsRemovePrefix(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pDestination,
    DWORD           dwFlags)
{
    BOOL fMoveBack;

    fMoveBack = (dwFlags & REMOVEPREFIX_KEEP_INITIAL_PATHSEP) &&
        (pKeyInfo->_pFullPath->Name[pKeyInfo->_ichKeyStart]);

    pDestination->Buffer = &(pKeyInfo->_pFullPath->Name[pKeyInfo->_ichKeyStart -
                                                              (fMoveBack ? 1 : 0)]);

    pDestination->Length = (USHORT) pKeyInfo->_pFullPath->NameLength -
        ((pKeyInfo->_ichKeyStart - (fMoveBack ? 1 : 0))  * REG_CHAR_SIZE);
}

__inline void KeySemanticsGetSid(
    SKeySemantics*  pKeyInfo,
    PUNICODE_STRING pSidString)
{
    pSidString->Buffer = &(pKeyInfo->_pFullPath->Name[REG_USER_HIVE_NAMECCH]);

    pSidString->Length = pKeyInfo->_cbPrefixLen -
            (REG_USER_HIVE_CLASSES_SUFFIXLEN + REG_USER_HIVE_NAMELEN);
}

__inline void KeySemanticsTruncatePrefixToClassReg(
    SKeySemantics*  pKeyInfo,
    USHORT          PrefixLen,
    PUNICODE_STRING pDestination)
{
    pDestination->Length = PrefixLen + (pKeyInfo->_fClassRegistration ? REG_CHAR_SIZE : 0) +
        pKeyInfo->_cbSpecialKey + pKeyInfo->_cbClassRegKey;
}

#else // LOCAL

#define REG_CLASS_IS_SPECIAL_KEY( Handle )     0
#define REG_CLASS_SET_SPECIAL_KEY( Handle )    (Handle)
#define REG_CLASS_RESET_SPECIAL_KEY( Handle )  (Handle)

#endif // LOCAL







