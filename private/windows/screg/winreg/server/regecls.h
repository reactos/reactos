/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regecls.h

Abstract:

    This file contains declarations for data structures
    needed for enumerating keys under HKEY_CLASSES_ROOT

Author:

    Adam Edwards (adamed) 14-Nov-1997

Notes:

--*/

#ifdef LOCAL

#if !defined(_REGECLS_H_)
#define _REGECLS_H_

#include "regstate.h"

#define ENUM_DEFAULT_KEY_NAME_SIZE         128
#define ENUM_DEFAULT_CLASS_SIZE            128

//
// Constants for controlling the direction of enumeration
//
enum
{
    ENUM_DIRECTION_BACKWARD = 0,
    ENUM_DIRECTION_FORWARD = 1,
    ENUM_DIRECTION_IGNORE = 2
};

//
// Constants specifying the physical location of a key being
// enumerated
//
enum 
{
    ENUM_LOCATION_USER = 1,
    ENUM_LOCATION_MACHINE = 2,
    ENUM_LOCATION_NONE = 3
};

//
// Structure for holding the state of an enumeration on a 
// user or machine subtree
//
typedef struct _EnumSubtreeState {
    PKEY_NODE_INFORMATION  pKeyInfo;  // structure holding information about a key
    ULONG                  cbKeyInfo; // size of pKeyInfo
    DWORD                  iSubKey;   // which key we need to ask the kernel for
    BOOL                   Finished;  // TRUE means we are done enumerating this subtree
    union {
        KEY_NODE_INFORMATION;         // Force the buffer to be aligned.
        BYTE                   KeyInfoBuffer[ sizeof( KEY_NODE_INFORMATION ) +
                                            ENUM_DEFAULT_KEY_NAME_SIZE +
                                            ENUM_DEFAULT_CLASS_SIZE ];

    };
} EnumSubtreeState;

//
// Structure for holding the state of enumeration for a registry key
// This structure persists in between calls to RegEnumKeyEx
//
typedef struct _EnumState {

    StateObject            Object;
    unsigned               Direction : 1;
    unsigned               LastLocation : 2;
    unsigned               fClassesRoot : 1;
    HKEY                   hKey;
    HKEY                   hkUserKey;
    HKEY                   hkMachineKey;
    DWORD                  dwLastRequest;
    DWORD                  dwThreadId;
    EnumSubtreeState       UserState;
    EnumSubtreeState       MachineState;

} EnumState;

typedef struct _KeyStateList {

    StateObject     Object;
    StateObjectList StateList;
    EnumState       RootState;

} KeyStateList;

typedef StateObjectList ThreadList;

VOID KeyStateListInit(KeyStateList* pStateList);
VOID KeyStateListDestroy(StateObject* pObject); 

//
// Hash table for storing enumeration states.  This table is indexed
// by (key handle, thread id).
//
typedef struct _EnumTable {

    BOOLEAN                bCriticalSectionInitialized;
    RTL_CRITICAL_SECTION   CriticalSection;
    ThreadList             ThreadEnumList;

} EnumTable;    

//
// Declaration of instance of enumeration table
//
extern EnumTable gClassesEnumTable;

//
// Prototypes for winreg client -- cleanup, init
//
BOOL InitializeClassesEnumTable();
BOOL CleanupClassesEnumTable(BOOL fThisThreadOnly);


//
// functions for managing enumeration state table
//
NTSTATUS EnumTableInit(EnumTable* pEnumTable);

enum
{
    ENUM_TABLE_REMOVEKEY_CRITERIA_THISTHREAD = 1,
    ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD = 2
};

NTSTATUS EnumTableClear(EnumTable* pEnumTable, DWORD dwCriteria);

DWORD EnumTableHashKey(
    EnumTable* pEnumTable,
    HKEY       hKey);

NTSTATUS EnumTableAddKey(
    EnumTable* pEnumTable,
    HKEY       hKey,
    DWORD      dwFirstSubKey,
    EnumState** ppEnumState,
    EnumState** ppRootState);

NTSTATUS EnumTableRemoveKey(
    EnumTable* pEnumTable,
    HKEY       hKey,
    DWORD      dwCriteria);

NTSTATUS EnumTableGetNextEnum(
    EnumTable*            pEnumTable,
    HKEY                  hKey,
    DWORD                 dwSubkey,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID                 pKeyInfo,
    DWORD                 cbKeyInfo,
    LPDWORD               pcbKeyInfo);

NTSTATUS EnumTableGetKeyState(
    EnumTable*  pEnumTable,
    HKEY        hKey,
    DWORD       dwSubkey,
    EnumState** ppEnumState,
    EnumState** ppRootState,
    LPDWORD     pcbKeyInfo);

NTSTATUS EnumTableFindKeyState(
    EnumTable*     pEnumTable,
    HKEY           hKey,
    EnumState**    ppEnumState);

void EnumTableUpdateRootState(
    EnumTable* pEnumTable,
    EnumState* pRootState,
    EnumState* pEnumState,
    BOOL       fResetState);

NTSTATUS EnumTableGetRootState(
    EnumTable*  pEnumTable,
    EnumState** ppRootState);


//
// functions to manage enumeration subtrees
//
void EnumSubtreeStateClear(EnumSubtreeState* pTreeState);

NTSTATUS EnumSubtreeStateCopyKeyInfo(
    EnumSubtreeState*     pTreeState,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID                 pDestKeyinfo,
    ULONG                 cbDestKeyInfo,
    PULONG                pcbResult);

//
// functions for managing key enumeration state
//
NTSTATUS EnumStateInit(
    EnumState*     pEnumState,
    HKEY           hKey,
    DWORD          dwFirstSubKey,
    DWORD          dwDirection,
    SKeySemantics* pKeySemantics);

NTSTATUS EnumStateGetNextEnum(
    EnumState*            pEnumState,
    DWORD                 dwSubkey,
    KEY_INFORMATION_CLASS KeyInformationClass,
    PVOID                 pKeyInfo,
    DWORD                 cbKeyInfo,
    LPDWORD               pcbKeyInfo,
    BOOL*                 pfFreeState);

NTSTATUS EnumStateSetLimits(
    EnumState* pEnumState,
    DWORD      dwSubKey,
    LPDWORD    pdwStart,
    LPDWORD    pdwLimit,
    PLONG      plIncrement);

NTSTATUS EnumStateChooseNext(
    EnumState*         pEnumState,
    DWORD              dwSubKey,
    DWORD              dwStart,
    DWORD              dwLimit,
    LONG               lIncrement,
    EnumSubtreeState** ppTreeState);

NTSTATUS EnumStateCompareSubtrees(
    EnumState*         pEnumState,
    LONG               lIncrement,
    EnumSubtreeState** ppSubtree);

VOID EnumStateClear(EnumState* pEnumState);
VOID EnumStateDestroy(StateObject* pObject);

BOOL EnumStateIsEmpty(EnumState* pEnumState);

NTSTATUS EnumStateCopy(
    EnumState*            pDestState,
    EnumState*            pEnumState);

//
// Utility functions
//    
NTSTATUS EnumClassKey(
    HKEY              hKey,
    EnumSubtreeState* pTreeState);

NTSTATUS GetSubKeyCount(
    HKEY    hkClassKey,
    LPDWORD pdwUserSubKeys);

NTSTATUS ClassKeyCountSubKeys(
    HKEY    hKey,
    HKEY    hkUser,
    HKEY    hkMachine,
    DWORD   cMax,
    LPDWORD pcSubKeys);

__inline BOOL IsRootKey(SKeySemantics* pKeySemantics)
{
    return pKeySemantics->_fClassRegParent;
}

#endif // !defined(_REGECLS_H_)

#endif // LOCAL












