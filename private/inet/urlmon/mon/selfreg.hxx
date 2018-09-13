//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       selfreg.hxx
//
//  Contents:   Taken from Office96
//              Header file for the common self registration code used by all the
//              sub projects of Sweeper project. They are
//              UrlMon
//              UrlMnPrx
//
//  Classes:
//
//  Functions:
//
//  History:    5-03-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _SELFREG_HXX_DEFINED_
#define _SELFREG_HXX_DEFINED_

// Computes the number of elements in an array
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

// The dwType field of a REGENTRY is set to this if the string is a resource
#define REG_RESID                                               ( REG_RESOURCE_REQUIREMENTS_LIST + 1 )

#define KEYTYPE_STRING  1
#define KEYTYPE_RESID   2

// SELF REGISTRATION RELATED DECLARATIONS
// Note: this struct used to have 'char' fields, but this caused
// PMac Hlink dll to crash, so now the fields are 32 bit aligned.
typedef struct {
        DWORD iKeyType;   // pszKey contains localizable parts if this
                          // is set to KEYTYPE_RESID
        char *pszKey;
        char *pszValueName;
        DWORD dwType;
        BYTE *pbData;   // If dwType is REG_RESID, this is a resource ID
} REGENTRY;

typedef struct {
        HKEY                    hkRoot;
        const REGENTRY  *rgEntries;
        DWORD                   dwEntries;
} REGENTRYGROUP;

// Function
typedef BOOL  (__stdcall * PFNLOADSTRING)(HINSTANCE hinst, int ids, char* sz, int cch);

// Helper for DllRegisterServer
HRESULT HrDllRegisterServer(
        const REGENTRYGROUP *rgRegEntryGroups,
        HINSTANCE hinstDll,
        PFNLOADSTRING pfnLoadString, char *pszAppName = NULL);

// Helper for DllUnregisterServer
HRESULT HrDllUnregisterServer(
        const REGENTRYGROUP *rgRegEntryGroups,
        HINSTANCE hinstDll,
        PFNLOADSTRING pfnLoadString);

// Register a group of reg entries off a root key
BOOL FRegisterEntries(HKEY hkRoot, const REGENTRY rgEntries[],
        DWORD dwEntries, char *pszPath, char *pszBinderName);

// Register several groups of reg entries
BOOL FRegisterEntryGroups(const REGENTRYGROUP *rgRegEntryGroups,
        char *pszPath, char *pszBinderName);

// FDeleteEntries - Delete a group of reg entries off a root key.
BOOL FDeleteEntries(HKEY hkRoot, const REGENTRY rgEntries[], DWORD dwEntries);

// FDeleteEntryGroups - Delete the base keys of all the given groups.
BOOL FDeleteEntryGroups(const REGENTRYGROUP *rgRegEntryGroups);

// Given the potential full path szFileName, return the filename portion
LPSTR ParseAFileName( LPSTR szFileName, int *piRetLen);

#define STD_ENTRY(pszKey, pszValue) \
        { KEYTYPE_STRING, pszKey, NULL, REG_SZ, (BYTE*)pszValue }

#define STD_RES_ENTRY(pszKey, wResId) \
        {KEYTYPE_STRING, pszKey, NULL, REG_RESID, (BYTE*)wResId }

#define STD_INTERFACE_ENTRY(Name, Clsid, Dll) \
        STD_ENTRY("Interface\\" Clsid,  Name), \
        STD_ENTRY("Interface\\" Clsid "\\ProxyStubClsid" NotMac("32"), Dll)

#endif // _SELFREG_HXX_DEFINED_

