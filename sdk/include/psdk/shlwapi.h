/*
 * SHLWAPI.DLL functions
 *
 * Copyright (C) 2000 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SHLWAPI_H
#define __WINE_SHLWAPI_H

#include <specstrings.h>
#include <objbase.h>
#include <shtypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <pshpack8.h>

#ifndef NO_SHLWAPI_REG

/* Registry functions */

DWORD WINAPI SHDeleteEmptyKeyA(_In_ HKEY, _In_opt_ LPCSTR);
DWORD WINAPI SHDeleteEmptyKeyW(_In_ HKEY, _In_opt_ LPCWSTR);
#define SHDeleteEmptyKey WINELIB_NAME_AW(SHDeleteEmptyKey)

DWORD WINAPI SHDeleteKeyA(_In_ HKEY, _In_opt_ LPCSTR);
DWORD WINAPI SHDeleteKeyW(_In_ HKEY, _In_opt_ LPCWSTR);
#define SHDeleteKey WINELIB_NAME_AW(SHDeleteKey)

DWORD WINAPI SHDeleteValueA(_In_ HKEY, _In_opt_ LPCSTR, _In_ LPCSTR);
DWORD WINAPI SHDeleteValueW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ LPCWSTR);
#define SHDeleteValue WINELIB_NAME_AW(SHDeleteValue)

DWORD
WINAPI
SHGetValueA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_opt_(*pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData);

DWORD
WINAPI
SHGetValueW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_opt_(*pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData);

#define SHGetValue WINELIB_NAME_AW(SHGetValue)

DWORD
WINAPI
SHSetValueA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD,
  _In_reads_bytes_opt_(cbData) LPCVOID,
  _In_ DWORD cbData);

DWORD
WINAPI
SHSetValueW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD,
  _In_reads_bytes_opt_(cbData) LPCVOID,
  _In_ DWORD cbData);

#define SHSetValue WINELIB_NAME_AW(SHSetValue)

DWORD
WINAPI
SHQueryValueExA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _Reserved_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData);

DWORD
WINAPI
SHQueryValueExW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _Reserved_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData);

#define SHQueryValueEx WINELIB_NAME_AW(SHQueryValueEx)

LONG
WINAPI
SHEnumKeyExA(
  _In_ HKEY,
  _In_ DWORD,
  _Out_writes_(*pcchName) LPSTR,
  _Inout_ LPDWORD pcchName);

LONG
WINAPI
SHEnumKeyExW(
  _In_ HKEY,
  _In_ DWORD,
  _Out_writes_(*pcchName) LPWSTR,
  _Inout_ LPDWORD pcchName);

#define SHEnumKeyEx WINELIB_NAME_AW(SHEnumKeyEx)

LONG
WINAPI
SHEnumValueA(
  _In_ HKEY,
  _In_ DWORD,
  _Out_writes_opt_(*pcchValueName) LPSTR,
  _Inout_opt_ LPDWORD pcchValueName,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData);

LONG
WINAPI
SHEnumValueW(
  _In_ HKEY,
  _In_ DWORD,
  _Out_writes_opt_(*pcchValueName) LPWSTR,
  _Inout_opt_ LPDWORD pcchValueName,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData);

#define SHEnumValue WINELIB_NAME_AW(SHEnumValue)

LONG
WINAPI
SHQueryInfoKeyA(
  _In_ HKEY,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD);

LONG
WINAPI
SHQueryInfoKeyW(
  _In_ HKEY,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD);

#define SHQueryInfoKey WINELIB_NAME_AW(SHQueryInfoKey)

DWORD
WINAPI
SHRegGetPathA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _Out_writes_(MAX_PATH) LPSTR,
  _In_ DWORD);

DWORD
WINAPI
SHRegGetPathW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Out_writes_(MAX_PATH) LPWSTR,
  _In_ DWORD);

#define SHRegGetPath WINELIB_NAME_AW(SHRegGetPath)

DWORD
WINAPI
SHRegSetPathA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ LPCSTR,
  _In_ DWORD);

DWORD
WINAPI
SHRegSetPathW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ DWORD);

#define SHRegSetPath WINELIB_NAME_AW(SHRegSetPath)

DWORD
WINAPI
SHCopyKeyA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_ HKEY,
  _Reserved_ DWORD);

DWORD
WINAPI
SHCopyKeyW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_ HKEY,
  _Reserved_ DWORD);

#define SHCopyKey WINELIB_NAME_AW(SHCopyKey)

HKEY WINAPI  SHRegDuplicateHKey(_In_ HKEY);

/* SHRegGetValue flags */
typedef INT SRRF;

#define SRRF_RT_REG_NONE 0x1
#define SRRF_RT_REG_SZ 0x2
#define SRRF_RT_REG_EXPAND_SZ 0x4
#define SRRF_RT_REG_BINARY 0x8
#define SRRF_RT_REG_DWORD 0x10
#define SRRF_RT_REG_MULTI_SZ 0x20
#define SRRF_RT_REG_QWORD 0x40

#define SRRF_RT_DWORD (SRRF_RT_REG_BINARY|SRRF_RT_REG_DWORD)
#define SRRF_RT_QWORD (SRRF_RT_REG_BINARY|SRRF_RT_REG_QWORD)
#define SRRF_RT_ANY 0xffff

#define SRRF_RM_ANY 0
#define SRRF_RM_NORMAL 0x10000
#define SRRF_RM_SAFE 0x20000
#define SRRF_RM_SAFENETWORK 0x40000

#define SRRF_NOEXPAND 0x10000000
#define SRRF_ZEROONFAILURE 0x20000000
#define SRRF_NOVIRT 0x40000000

LSTATUS
WINAPI
SHRegGetValueA(
  _In_ HKEY hkey,
  _In_opt_ LPCSTR pszSubKey,
  _In_opt_ LPCSTR pszValue,
  _In_ SRRF srrfFlags,
  _Out_opt_ LPDWORD pdwType,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID pvData,
  _Inout_opt_ LPDWORD pcbData);

LSTATUS
WINAPI
SHRegGetValueW(
  _In_ HKEY hkey,
  _In_opt_ LPCWSTR pszSubKey,
  _In_opt_ LPCWSTR pszValue,
  _In_ SRRF srrfFlags,
  _Out_opt_ LPDWORD pdwType,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID pvData,
  _Inout_opt_ LPDWORD pcbData);

#define SHRegGetValue WINELIB_NAME_AW(SHRegGetValue)

/* Undocumented registry functions */

DWORD WINAPI SHDeleteOrphanKeyA(HKEY,LPCSTR);
DWORD WINAPI SHDeleteOrphanKeyW(HKEY,LPCWSTR);
#define SHDeleteOrphanKey WINELIB_NAME_AW(SHDeleteOrphanKey)


/* User registry functions */

typedef enum
{
  SHREGDEL_DEFAULT = 0,
  SHREGDEL_HKCU    = 0x1,
  SHREGDEL_HKLM    = 0x10,
  SHREGDEL_BOTH    = SHREGDEL_HKLM | SHREGDEL_HKCU
} SHREGDEL_FLAGS;

typedef enum
{
  SHREGENUM_DEFAULT = 0,
  SHREGENUM_HKCU    = 0x1,
  SHREGENUM_HKLM    = 0x10,
  SHREGENUM_BOTH    = SHREGENUM_HKLM | SHREGENUM_HKCU
} SHREGENUM_FLAGS;

#define SHREGSET_HKCU       0x1 /* Apply to HKCU if empty */
#define SHREGSET_FORCE_HKCU 0x2 /* Always apply to HKCU */
#define SHREGSET_HKLM       0x4 /* Apply to HKLM if empty */
#define SHREGSET_FORCE_HKLM 0x8 /* Always apply to HKLM */
#define SHREGSET_DEFAULT    (SHREGSET_FORCE_HKCU | SHREGSET_HKLM)

typedef HANDLE HUSKEY;
typedef HUSKEY *PHUSKEY;

LONG
WINAPI
SHRegCreateUSKeyA(
  _In_ LPCSTR,
  _In_ REGSAM,
  _In_opt_ HUSKEY,
  _Out_ PHUSKEY,
  _In_ DWORD);

LONG
WINAPI
SHRegCreateUSKeyW(
  _In_ LPCWSTR,
  _In_ REGSAM,
  _In_opt_ HUSKEY,
  _Out_ PHUSKEY,
  _In_ DWORD);

#define SHRegCreateUSKey WINELIB_NAME_AW(SHRegCreateUSKey)

LONG
WINAPI
SHRegOpenUSKeyA(
  _In_ LPCSTR,
  _In_ REGSAM,
  _In_opt_ HUSKEY,
  _Out_ PHUSKEY,
  _In_ BOOL);

LONG
WINAPI
SHRegOpenUSKeyW(
  _In_ LPCWSTR,
  _In_ REGSAM,
  _In_opt_ HUSKEY,
  _Out_ PHUSKEY,
  _In_ BOOL);

#define SHRegOpenUSKey WINELIB_NAME_AW(SHRegOpenUSKey)

LONG
WINAPI
SHRegQueryUSValueA(
  _In_ HUSKEY,
  _In_opt_ LPCSTR,
  _Inout_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData,
  _In_ BOOL,
  _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID,
  _In_opt_ DWORD dwDefaultDataSize);

LONG
WINAPI
SHRegQueryUSValueW(
  _In_ HUSKEY,
  _In_opt_ LPCWSTR,
  _Inout_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData,
  _In_ BOOL,
  _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID,
  _In_opt_ DWORD dwDefaultDataSize);

#define SHRegQueryUSValue WINELIB_NAME_AW(SHRegQueryUSValue)

LONG
WINAPI
SHRegWriteUSValueA(
  _In_ HUSKEY,
  _In_ LPCSTR,
  _In_ DWORD,
  _In_reads_bytes_(cbData) LPVOID,
  _In_ DWORD cbData,
  _In_ DWORD);

LONG
WINAPI
SHRegWriteUSValueW(
  _In_ HUSKEY,
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_reads_bytes_(cbData) LPVOID,
  _In_ DWORD cbData,
  _In_ DWORD);

#define SHRegWriteUSValue WINELIB_NAME_AW(SHRegWriteUSValue)

LONG
WINAPI
SHRegDeleteUSValueA(
  _In_ HUSKEY,
  _In_ LPCSTR,
  _In_ SHREGDEL_FLAGS);

LONG
WINAPI
SHRegDeleteUSValueW(
  _In_ HUSKEY,
  _In_ LPCWSTR,
  _In_ SHREGDEL_FLAGS);

#define SHRegDeleteUSValue WINELIB_NAME_AW(SHRegDeleteUSValue)

LONG
WINAPI
SHRegDeleteEmptyUSKeyA(
  _In_ HUSKEY,
  _In_ LPCSTR,
  _In_ SHREGDEL_FLAGS);

LONG
WINAPI
SHRegDeleteEmptyUSKeyW(
  _In_ HUSKEY,
  _In_ LPCWSTR,
  _In_ SHREGDEL_FLAGS);

#define SHRegDeleteEmptyUSKey WINELIB_NAME_AW(SHRegDeleteEmptyUSKey)

LONG
WINAPI
SHRegEnumUSKeyA(
  _In_ HUSKEY,
  _In_ DWORD,
  _Out_writes_to_(*pcchName, *pcchName) LPSTR,
  _Inout_ LPDWORD pcchName,
  _In_ SHREGENUM_FLAGS);

LONG
WINAPI
SHRegEnumUSKeyW(
  _In_ HUSKEY,
  _In_ DWORD,
  _Out_writes_to_(*pcchName, *pcchName) LPWSTR,
  _Inout_ LPDWORD pcchName,
  _In_ SHREGENUM_FLAGS);

#define SHRegEnumUSKey WINELIB_NAME_AW(SHRegEnumUSKey)

LONG
WINAPI
SHRegEnumUSValueA(
  _In_ HUSKEY,
  _In_ DWORD,
  _Out_writes_to_(*pcchValueName, *pcchValueName) LPSTR,
  _Inout_ LPDWORD pcchValueName,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData,
  _In_ SHREGENUM_FLAGS);

LONG
WINAPI
SHRegEnumUSValueW(
  _In_ HUSKEY,
  _In_ DWORD,
  _Out_writes_to_(*pcchValueName, *pcchValueName) LPWSTR,
  _Inout_ LPDWORD pcchValueName,
  _Out_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData,
  _In_ SHREGENUM_FLAGS);

#define SHRegEnumUSValue WINELIB_NAME_AW(SHRegEnumUSValue)

LONG
WINAPI
SHRegQueryInfoUSKeyA(
  _In_ HUSKEY,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _In_ SHREGENUM_FLAGS);

LONG
WINAPI
SHRegQueryInfoUSKeyW(
  _In_ HUSKEY,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPDWORD,
  _In_ SHREGENUM_FLAGS);

#define SHRegQueryInfoUSKey WINELIB_NAME_AW(SHRegQueryInfoUSKey)

LONG WINAPI SHRegCloseUSKey(_In_ HUSKEY);

LONG
WINAPI
SHRegGetUSValueA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _Inout_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData,
  _In_ BOOL,
  _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID,
  _In_ DWORD dwDefaultDataSize);

LONG
WINAPI
SHRegGetUSValueW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Inout_opt_ LPDWORD,
  _Out_writes_bytes_to_opt_(*pcbData, *pcbData) LPVOID,
  _Inout_opt_ LPDWORD pcbData,
  _In_ BOOL,
  _In_reads_bytes_opt_(dwDefaultDataSize) LPVOID,
  _In_ DWORD dwDefaultDataSize);

#define SHRegGetUSValue WINELIB_NAME_AW(SHRegGetUSValue)

LONG
WINAPI
SHRegSetUSValueA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ DWORD,
  _In_reads_bytes_opt_(cbData) LPVOID,
  _In_opt_ DWORD cbData,
  _In_opt_ DWORD);

LONG
WINAPI
SHRegSetUSValueW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_reads_bytes_opt_(cbData) LPVOID,
  _In_opt_ DWORD cbData,
  _In_opt_ DWORD);

#define SHRegSetUSValue WINELIB_NAME_AW(SHRegSetUSValue)

BOOL
WINAPI
SHRegGetBoolUSValueA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ BOOL,
  _In_ BOOL);

BOOL
WINAPI
SHRegGetBoolUSValueW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ BOOL,
  _In_ BOOL);

#define SHRegGetBoolUSValue WINELIB_NAME_AW(SHRegGetBoolUSValue)

int WINAPI SHRegGetIntW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ int);

/* IQueryAssociation and helpers */
enum
{
    ASSOCF_NONE                 = 0x0000,
    ASSOCF_INIT_NOREMAPCLSID    = 0x0001, /* Don't map clsid->progid */
    ASSOCF_INIT_BYEXENAME       = 0x0002, /* .exe name given */
    ASSOCF_OPEN_BYEXENAME       = 0x0002, /* Synonym */
    ASSOCF_INIT_DEFAULTTOSTAR   = 0x0004, /* Use * as base */
    ASSOCF_INIT_DEFAULTTOFOLDER = 0x0008, /* Use folder as base */
    ASSOCF_NOUSERSETTINGS       = 0x0010, /* No HKCU reads */
    ASSOCF_NOTRUNCATE           = 0x0020, /* Don't truncate return */
    ASSOCF_VERIFY               = 0x0040, /* Verify data */
    ASSOCF_REMAPRUNDLL          = 0x0080, /* Get rundll args */
    ASSOCF_NOFIXUPS             = 0x0100, /* Don't fixup errors */
    ASSOCF_IGNOREBASECLASS      = 0x0200, /* Don't read baseclass */
    ASSOCF_INIT_IGNOREUNKNOWN   = 0x0400, /* Fail for unknown progid */
    ASSOCF_INIT_FIXED_PROGID    = 0x0800, /* Used passed string as progid, don't try to map it */
    ASSOCF_IS_PROTOCOL          = 0x1000, /* Treat as protocol, that should be mapped */
    ASSOCF_INIT_FOR_FILE        = 0x2000, /* progid is for file extension association */
};

typedef DWORD ASSOCF;

typedef enum
{
    ASSOCSTR_COMMAND = 1,     /* Verb command */
    ASSOCSTR_EXECUTABLE,      /* .exe from command string */
    ASSOCSTR_FRIENDLYDOCNAME, /* Friendly doc type name */
    ASSOCSTR_FRIENDLYAPPNAME, /* Friendly .exe name */
    ASSOCSTR_NOOPEN,          /* noopen value */
    ASSOCSTR_SHELLNEWVALUE,   /* Use shellnew key */
    ASSOCSTR_DDECOMMAND,      /* DDE command template */
    ASSOCSTR_DDEIFEXEC,       /* DDE command for process create */
    ASSOCSTR_DDEAPPLICATION,  /* DDE app name */
    ASSOCSTR_DDETOPIC,        /* DDE topic */
    ASSOCSTR_INFOTIP,         /* Infotip */
    ASSOCSTR_QUICKTIP,        /* Quick infotip */
    ASSOCSTR_TILEINFO,        /* Properties for tileview */
    ASSOCSTR_CONTENTTYPE,     /* Mimetype */
    ASSOCSTR_DEFAULTICON,     /* Icon */
    ASSOCSTR_SHELLEXTENSION,  /* GUID for shell extension handler */
    ASSOCSTR_MAX
} ASSOCSTR;

typedef enum
{
    ASSOCKEY_SHELLEXECCLASS = 1, /* Key for ShellExec */
    ASSOCKEY_APP,                /* Application */
    ASSOCKEY_CLASS,              /* Progid or class */
    ASSOCKEY_BASECLASS,          /* Base class */
    ASSOCKEY_MAX
} ASSOCKEY;

typedef enum
{
    ASSOCDATA_MSIDESCRIPTOR = 1, /* Component descriptor */
    ASSOCDATA_NOACTIVATEHANDLER, /* Don't activate */
    ASSOCDATA_QUERYCLASSSTORE,   /* Look in Class Store */
    ASSOCDATA_HASPERUSERASSOC,   /* Use user association */
    ASSOCDATA_EDITFLAGS,         /* Edit flags */
    ASSOCDATA_VALUE,             /* pszExtra is value */
    ASSOCDATA_MAX
} ASSOCDATA;

typedef enum
{
    ASSOCENUM_NONE
} ASSOCENUM;

typedef enum
{
    FTA_None                  = 0x00000000,
    FTA_Exclude               = 0x00000001,
    FTA_Show                  = 0x00000002,
    FTA_HasExtension          = 0x00000004,
    FTA_NoEdit                = 0x00000008,
    FTA_NoRemove              = 0x00000010,
    FTA_NoNewVerb             = 0x00000020,
    FTA_NoEditVerb            = 0x00000040,
    FTA_NoRemoveVerb          = 0x00000080,
    FTA_NoEditDesc            = 0x00000100,
    FTA_NoEditIcon            = 0x00000200,
    FTA_NoEditDflt            = 0x00000400,
    FTA_NoEditVerbCmd         = 0x00000800,
    FTA_NoEditVerbExe         = 0x00001000,
    FTA_NoDDE                 = 0x00002000,
    FTA_NoEditMIME            = 0x00008000,
    FTA_OpenIsSafe            = 0x00010000,
    FTA_AlwaysUnsafe          = 0x00020000,
    FTA_NoRecentDocs          = 0x00100000,
    FTA_SafeForElevation      = 0x00200000, /* Win8+ */
    FTA_AlwaysUseDirectInvoke = 0x00400000  /* Win8+ */
} FILETYPEATTRIBUTEFLAGS;
DEFINE_ENUM_FLAG_OPERATORS(FILETYPEATTRIBUTEFLAGS)

typedef struct IQueryAssociations *LPQUERYASSOCIATIONS;

#define INTERFACE IQueryAssociations
DECLARE_INTERFACE_(IQueryAssociations,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IQueryAssociations methods ***/
    STDMETHOD(Init)(THIS_ _In_ ASSOCF flags, _In_opt_ LPCWSTR pszAssoc, _In_opt_ HKEY hkProgid, _In_opt_ HWND hwnd) PURE;
    STDMETHOD(GetString)(THIS_ _In_ ASSOCF flags, _In_ ASSOCSTR str, _In_opt_ LPCWSTR pszExtra, _Out_writes_opt_(*pcchOut) LPWSTR pszOut, _Inout_ DWORD *pcchOut) PURE;
    STDMETHOD(GetKey)(THIS_ _In_ ASSOCF flags, _In_ ASSOCKEY key, _In_opt_ LPCWSTR pszExtra, _Out_ HKEY *phkeyOut) PURE;
    STDMETHOD(GetData)(THIS_ _In_ ASSOCF flags, _In_ ASSOCDATA data, _In_opt_ LPCWSTR pszExtra, _Out_writes_bytes_opt_(*pcbOut) LPVOID pvOut, _Inout_opt_ DWORD *pcbOut) PURE;
    STDMETHOD(GetEnum)(THIS_ _In_ ASSOCF flags, _In_ ASSOCENUM assocenum, _In_opt_ LPCWSTR pszExtra, _In_ REFIID riid, _Outptr_ LPVOID *ppvOut) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IQueryAssociations_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define IQueryAssociations_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define IQueryAssociations_Release(p)              (p)->lpVtbl->Release(p)
#define IQueryAssociations_Init(p,a,b,c,d)         (p)->lpVtbl->Init(p,a,b,c,d)
#define IQueryAssociations_GetString(p,a,b,c,d,e)  (p)->lpVtbl->GetString(p,a,b,c,d,e)
#define IQueryAssociations_GetKey(p,a,b,c,d)       (p)->lpVtbl->GetKey(p,a,b,c,d)
#define IQueryAssociations_GetData(p,a,b,c,d,e)    (p)->lpVtbl->GetData(p,a,b,c,d,e)
#define IQueryAssociations_GetEnum(p,a,b,c,d,e)    (p)->lpVtbl->GetEnum(p,a,b,c,d,e)
#endif

HRESULT WINAPI AssocCreate(_In_ CLSID, _In_ REFIID, _Outptr_ LPVOID*);

HRESULT
WINAPI
AssocQueryStringA(
  _In_ ASSOCF,
  _In_ ASSOCSTR,
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _Out_writes_opt_(*pcchOut) LPSTR,
  _Inout_ LPDWORD pcchOut);

HRESULT
WINAPI
AssocQueryStringW(
  _In_ ASSOCF,
  _In_ ASSOCSTR,
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Out_writes_opt_(*pcchOut) LPWSTR,
  _Inout_ LPDWORD pcchOut);

#define AssocQueryString WINELIB_NAME_AW(AssocQueryString)

HRESULT
WINAPI
AssocQueryStringByKeyA(
  _In_ ASSOCF,
  _In_ ASSOCSTR,
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _Out_writes_opt_(*pcchOut) LPSTR,
  _Inout_ LPDWORD pcchOut);

HRESULT
WINAPI
AssocQueryStringByKeyW(
  _In_ ASSOCF,
  _In_ ASSOCSTR,
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _Out_writes_opt_(*pcchOut) LPWSTR,
  _Inout_ LPDWORD pcchOut);

#define AssocQueryStringByKey WINELIB_NAME_AW(AssocQueryStringByKey)

HRESULT
WINAPI
AssocQueryKeyA(
  _In_ ASSOCF,
  _In_ ASSOCKEY,
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _Out_ PHKEY);

HRESULT
WINAPI
AssocQueryKeyW(
  _In_ ASSOCF,
  _In_ ASSOCKEY,
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Out_ PHKEY);

#define AssocQueryKey WINELIB_NAME_AW(AssocQueryKey)

BOOL WINAPI AssocIsDangerous(_In_ LPCWSTR);

#endif /* NO_SHLWAPI_REG */

void WINAPI IUnknown_Set(_Inout_ IUnknown **ppunk, _In_opt_ IUnknown *punk);
void WINAPI IUnknown_AtomicRelease(_Inout_opt_ IUnknown **punk);
HRESULT WINAPI IUnknown_GetWindow(_In_ IUnknown *punk, _Out_ HWND *phwnd);

HRESULT
WINAPI
IUnknown_SetSite(
  _In_ IUnknown *punk,
  _In_opt_ IUnknown *punkSite);

HRESULT
WINAPI
IUnknown_GetSite(
  _In_ IUnknown *punk,
  _In_ REFIID riid,
  _Outptr_ void **ppv);

HRESULT
WINAPI
IUnknown_QueryService(
  _In_opt_ IUnknown *punk,
  _In_ REFGUID guidService,
  _In_ REFIID riid,
  _Outptr_ void **ppvOut);

/* Path functions */
#ifndef NO_SHLWAPI_PATH

/* GetPathCharType return flags */
#define GCT_INVALID     0x0
#define GCT_LFNCHAR     0x1
#define GCT_SHORTCHAR   0x2
#define GCT_WILD        0x4
#define GCT_SEPARATOR   0x8

LPSTR  WINAPI PathAddBackslashA(_Inout_updates_(MAX_PATH) LPSTR);
LPWSTR WINAPI PathAddBackslashW(_Inout_updates_(MAX_PATH) LPWSTR);
#define PathAddBackslash WINELIB_NAME_AW(PathAddBackslash)

BOOL
WINAPI
PathAddExtensionA(
  _Inout_updates_(MAX_PATH) LPSTR,
  _In_opt_ LPCSTR);

BOOL
WINAPI
PathAddExtensionW(
  _Inout_updates_(MAX_PATH) LPWSTR,
  _In_opt_ LPCWSTR);

#define PathAddExtension WINELIB_NAME_AW(PathAddExtension)

BOOL WINAPI PathAppendA(_Inout_updates_(MAX_PATH) LPSTR, _In_ LPCSTR);
BOOL WINAPI PathAppendW(_Inout_updates_(MAX_PATH) LPWSTR, _In_ LPCWSTR);
#define PathAppend WINELIB_NAME_AW(PathAppend)

LPSTR  WINAPI PathBuildRootA(_Out_writes_(4) LPSTR, int);
LPWSTR WINAPI PathBuildRootW(_Out_writes_(4) LPWSTR, int);
#define PathBuildRoot WINELIB_NAME_AW(PathBuiltRoot)

BOOL WINAPI PathCanonicalizeA(_Out_writes_(MAX_PATH) LPSTR, _In_ LPCSTR);
BOOL WINAPI PathCanonicalizeW(_Out_writes_(MAX_PATH) LPWSTR, _In_ LPCWSTR);
#define PathCanonicalize WINELIB_NAME_AW(PathCanonicalize)

LPSTR
WINAPI
PathCombineA(
  _Out_writes_(MAX_PATH) LPSTR,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR);

LPWSTR
WINAPI
PathCombineW(
  _Out_writes_(MAX_PATH) LPWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR);

#define PathCombine WINELIB_NAME_AW(PathCombine)

BOOL
WINAPI
PathCompactPathA(
  _In_opt_ HDC,
  _Inout_updates_(MAX_PATH) LPSTR,
  _In_ UINT);

BOOL
WINAPI
PathCompactPathW(
  _In_opt_ HDC,
  _Inout_updates_(MAX_PATH) LPWSTR,
  _In_ UINT);

#define PathCompactPath WINELIB_NAME_AW(PathCompactPath)

BOOL
WINAPI
PathCompactPathExA(
  _Out_writes_(cchMax) LPSTR,
  _In_ LPCSTR,
  _In_ UINT cchMax,
  _In_ DWORD);

BOOL
WINAPI
PathCompactPathExW(
  _Out_writes_(cchMax) LPWSTR,
  _In_ LPCWSTR,
  _In_ UINT cchMax,
  _In_ DWORD);

#define PathCompactPathEx WINELIB_NAME_AW(PathCompactPathEx)

int
WINAPI
PathCommonPrefixA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _Out_writes_opt_(MAX_PATH) LPSTR);

int
WINAPI
PathCommonPrefixW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _Out_writes_opt_(MAX_PATH) LPWSTR);

#define PathCommonPrefix WINELIB_NAME_AW(PathCommonPrefix)

HRESULT
WINAPI
PathCreateFromUrlA(
  _In_ LPCSTR,
  _Out_writes_to_(*pcchPath, *pcchPath) LPSTR,
  _Inout_ LPDWORD pcchPath,
  DWORD);

HRESULT
WINAPI
PathCreateFromUrlW(
  _In_ LPCWSTR pszUrl,
  _Out_writes_to_(*pcchPath, *pcchPath) LPWSTR pszPath,
  _Inout_ LPDWORD pcchPath,
  DWORD dwFlags);

#define PathCreateFromUrl WINELIB_NAME_AW(PathCreateFromUrl)

HRESULT WINAPI PathCreateFromUrlAlloc(_In_ LPCWSTR pszUrl, _Outptr_ LPWSTR* pszPath, DWORD dwReserved);

BOOL WINAPI PathFileExistsA(_In_ LPCSTR pszPath);
BOOL WINAPI PathFileExistsW(_In_ LPCWSTR pszPath);
#define PathFileExists WINELIB_NAME_AW(PathFileExists)

BOOL WINAPI PathFileExistsAndAttributesA(LPCSTR lpszPath,DWORD* dwAttr);
BOOL WINAPI PathFileExistsAndAttributesW(LPCWSTR lpszPath,DWORD* dwAttr);
#define PathFileExistsAndAttributes WINELIB_NAME_AW(PathFileExistsAndAttributes)

LPSTR  WINAPI PathFindExtensionA(_In_ LPCSTR pszPath);
LPWSTR WINAPI PathFindExtensionW(_In_ LPCWSTR pszPath);
#define PathFindExtension WINELIB_NAME_AW(PathFindExtension)

LPSTR  WINAPI PathFindFileNameA(_In_ LPCSTR pszPath);
LPWSTR WINAPI PathFindFileNameW(_In_ LPCWSTR pszPath);
#define PathFindFileName WINELIB_NAME_AW(PathFindFileName)

LPSTR  WINAPI PathFindNextComponentA(_In_ LPCSTR);
LPWSTR WINAPI PathFindNextComponentW(_In_ LPCWSTR);
#define PathFindNextComponent WINELIB_NAME_AW(PathFindNextComponent)

BOOL WINAPI PathFindOnPathA(_Inout_updates_(MAX_PATH) LPSTR, _In_opt_ LPCSTR*);
BOOL WINAPI PathFindOnPathW(_Inout_updates_(MAX_PATH) LPWSTR, _In_opt_ LPCWSTR*);
#define PathFindOnPath WINELIB_NAME_AW(PathFindOnPath)

LPSTR  WINAPI PathGetArgsA(_In_ LPCSTR pszPath);
LPWSTR WINAPI PathGetArgsW(_In_ LPCWSTR pszPath);
#define PathGetArgs WINELIB_NAME_AW(PathGetArgs)

UINT WINAPI PathGetCharTypeA(_In_ UCHAR ch);
UINT WINAPI PathGetCharTypeW(_In_ WCHAR ch);
#define PathGetCharType WINELIB_NAME_AW(PathGetCharType)

int WINAPI PathGetDriveNumberA(_In_ LPCSTR);
int WINAPI PathGetDriveNumberW(_In_ LPCWSTR);
#define PathGetDriveNumber WINELIB_NAME_AW(PathGetDriveNumber)

BOOL WINAPI PathIsDirectoryA(_In_ LPCSTR);
BOOL WINAPI PathIsDirectoryW(_In_ LPCWSTR);
#define PathIsDirectory WINELIB_NAME_AW(PathIsDirectory)

BOOL WINAPI PathIsDirectoryEmptyA(_In_ LPCSTR);
BOOL WINAPI PathIsDirectoryEmptyW(_In_ LPCWSTR);
#define PathIsDirectoryEmpty WINELIB_NAME_AW(PathIsDirectoryEmpty)

BOOL WINAPI PathIsFileSpecA(_In_ LPCSTR);
BOOL WINAPI PathIsFileSpecW(_In_ LPCWSTR);
#define PathIsFileSpec WINELIB_NAME_AW(PathIsFileSpec);

BOOL WINAPI PathIsPrefixA(_In_ LPCSTR, _In_ LPCSTR);
BOOL WINAPI PathIsPrefixW(_In_ LPCWSTR, _In_ LPCWSTR);
#define PathIsPrefix WINELIB_NAME_AW(PathIsPrefix)

BOOL WINAPI PathIsRelativeA(_In_ LPCSTR);
BOOL WINAPI PathIsRelativeW(_In_ LPCWSTR);
#define PathIsRelative WINELIB_NAME_AW(PathIsRelative)

BOOL WINAPI PathIsRootA(_In_ LPCSTR);
BOOL WINAPI PathIsRootW(_In_ LPCWSTR);
#define PathIsRoot WINELIB_NAME_AW(PathIsRoot)

BOOL WINAPI PathIsSameRootA(_In_ LPCSTR, _In_ LPCSTR);
BOOL WINAPI PathIsSameRootW(_In_ LPCWSTR, _In_ LPCWSTR);
#define PathIsSameRoot WINELIB_NAME_AW(PathIsSameRoot)

BOOL WINAPI PathIsUNCA(_In_ LPCSTR);
BOOL WINAPI PathIsUNCW(_In_ LPCWSTR);
#define PathIsUNC WINELIB_NAME_AW(PathIsUNC)

BOOL WINAPI PathIsUNCServerA(_In_ LPCSTR);
BOOL WINAPI PathIsUNCServerW(_In_ LPCWSTR);
#define PathIsUNCServer WINELIB_NAME_AW(PathIsUNCServer)

BOOL WINAPI PathIsUNCServerShareA(_In_ LPCSTR);
BOOL WINAPI PathIsUNCServerShareW(_In_ LPCWSTR);
#define PathIsUNCServerShare WINELIB_NAME_AW(PathIsUNCServerShare)

BOOL WINAPI PathIsContentTypeA(_In_ LPCSTR, _In_ LPCSTR);
BOOL WINAPI PathIsContentTypeW(_In_ LPCWSTR, _In_ LPCWSTR);
#define PathIsContentType WINELIB_NAME_AW(PathIsContentType)

BOOL WINAPI PathIsURLA(_In_ LPCSTR);
BOOL WINAPI PathIsURLW(_In_ LPCWSTR);
#define PathIsURL WINELIB_NAME_AW(PathIsURL)

BOOL WINAPI PathMakePrettyA(_Inout_ LPSTR);
BOOL WINAPI PathMakePrettyW(_Inout_ LPWSTR);
#define PathMakePretty WINELIB_NAME_AW(PathMakePretty)

BOOL WINAPI PathMatchSpecA(_In_ LPCSTR, _In_ LPCSTR);
BOOL WINAPI PathMatchSpecW(_In_ LPCWSTR, _In_ LPCWSTR);
#define PathMatchSpec WINELIB_NAME_AW(PathMatchSpec)

int WINAPI PathParseIconLocationA(_Inout_ LPSTR);
int WINAPI PathParseIconLocationW(_Inout_ LPWSTR);
#define PathParseIconLocation WINELIB_NAME_AW(PathParseIconLocation)

VOID WINAPI PathQuoteSpacesA(_Inout_updates_(MAX_PATH) LPSTR);
VOID WINAPI PathQuoteSpacesW(_Inout_updates_(MAX_PATH) LPWSTR);
#define PathQuoteSpaces WINELIB_NAME_AW(PathQuoteSpaces)

BOOL
WINAPI
PathRelativePathToA(
  _Out_writes_(MAX_PATH) LPSTR,
  _In_ LPCSTR,
  _In_ DWORD,
  _In_ LPCSTR,
  _In_ DWORD);

BOOL
WINAPI
PathRelativePathToW(
  _Out_writes_(MAX_PATH) LPWSTR,
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_ LPCWSTR,
  _In_ DWORD);

#define PathRelativePathTo WINELIB_NAME_AW(PathRelativePathTo)

VOID WINAPI PathRemoveArgsA(_Inout_ LPSTR);
VOID WINAPI PathRemoveArgsW(_Inout_ LPWSTR);
#define PathRemoveArgs WINELIB_NAME_AW(PathRemoveArgs)

LPSTR  WINAPI PathRemoveBackslashA(_Inout_ LPSTR);
LPWSTR WINAPI PathRemoveBackslashW(_Inout_ LPWSTR);
#define PathRemoveBackslash WINELIB_NAME_AW(PathRemoveBackslash)

VOID WINAPI PathRemoveBlanksA(_Inout_ LPSTR);
VOID WINAPI PathRemoveBlanksW(_Inout_ LPWSTR);
#define PathRemoveBlanks WINELIB_NAME_AW(PathRemoveBlanks)

VOID WINAPI PathRemoveExtensionA(_Inout_ LPSTR);
VOID WINAPI PathRemoveExtensionW(_Inout_ LPWSTR);
#define PathRemoveExtension WINELIB_NAME_AW(PathRemoveExtension)

BOOL WINAPI PathRemoveFileSpecA(_Inout_ LPSTR);
BOOL WINAPI PathRemoveFileSpecW(_Inout_ LPWSTR);
#define PathRemoveFileSpec WINELIB_NAME_AW(PathRemoveFileSpec)

BOOL
WINAPI
PathRenameExtensionA(
  _Inout_updates_(MAX_PATH) LPSTR,
  _In_ LPCSTR);

BOOL
WINAPI
PathRenameExtensionW(
  _Inout_updates_(MAX_PATH) LPWSTR,
  _In_ LPCWSTR);

#define PathRenameExtension WINELIB_NAME_AW(PathRenameExtension)

BOOL
WINAPI
PathSearchAndQualifyA(
  _In_ LPCSTR,
  _Out_writes_(cchBuf) LPSTR,
  _In_ UINT cchBuf);

BOOL
WINAPI
PathSearchAndQualifyW(
  _In_ LPCWSTR,
  _Out_writes_(cchBuf) LPWSTR,
  _In_ UINT cchBuf);

#define PathSearchAndQualify WINELIB_NAME_AW(PathSearchAndQualify)

VOID WINAPI PathSetDlgItemPathA(_In_ HWND, int, LPCSTR);
VOID WINAPI PathSetDlgItemPathW(_In_ HWND, int, LPCWSTR);
#define PathSetDlgItemPath WINELIB_NAME_AW(PathSetDlgItemPath)

LPSTR  WINAPI PathSkipRootA(_In_ LPCSTR);
LPWSTR WINAPI PathSkipRootW(_In_ LPCWSTR);
#define PathSkipRoot WINELIB_NAME_AW(PathSkipRoot)

VOID WINAPI PathStripPathA(_Inout_ LPSTR);
VOID WINAPI PathStripPathW(_Inout_ LPWSTR);
#define PathStripPath WINELIB_NAME_AW(PathStripPath)

BOOL WINAPI PathStripToRootA(_Inout_ LPSTR);
BOOL WINAPI PathStripToRootW(_Inout_ LPWSTR);
#define PathStripToRoot WINELIB_NAME_AW(PathStripToRoot)

VOID WINAPI PathUnquoteSpacesA(_Inout_ LPSTR);
VOID WINAPI PathUnquoteSpacesW(_Inout_ LPWSTR);
#define PathUnquoteSpaces WINELIB_NAME_AW(PathUnquoteSpaces)

BOOL WINAPI PathMakeSystemFolderA(_In_ LPCSTR);
BOOL WINAPI PathMakeSystemFolderW(_In_ LPCWSTR);
#define PathMakeSystemFolder WINELIB_NAME_AW(PathMakeSystemFolder)

BOOL WINAPI PathUnmakeSystemFolderA(_In_ LPCSTR);
BOOL WINAPI PathUnmakeSystemFolderW(_In_ LPCWSTR);
#define PathUnmakeSystemFolder WINELIB_NAME_AW(PathUnmakeSystemFolder)

BOOL WINAPI PathIsSystemFolderA(_In_opt_ LPCSTR, _In_ DWORD);
BOOL WINAPI PathIsSystemFolderW(_In_opt_ LPCWSTR, _In_ DWORD);
#define PathIsSystemFolder WINELIB_NAME_AW(PathIsSystemFolder)

BOOL WINAPI PathIsNetworkPathA(_In_ LPCSTR);
BOOL WINAPI PathIsNetworkPathW(_In_ LPCWSTR);
#define PathIsNetworkPath WINELIB_NAME_AW(PathIsNetworkPath)

BOOL WINAPI PathIsLFNFileSpecA(_In_ LPCSTR);
BOOL WINAPI PathIsLFNFileSpecW(_In_ LPCWSTR);
#define PathIsLFNFileSpec WINELIB_NAME_AW(PathIsLFNFileSpec)

LPCSTR
WINAPI
PathFindSuffixArrayA(
  _In_ LPCSTR,
  _In_reads_(iArraySize) LPCSTR *,
  int iArraySize);

LPCWSTR
WINAPI
PathFindSuffixArrayW(
  _In_ LPCWSTR,
  _In_reads_(iArraySize) LPCWSTR *,
  int iArraySize);

#define PathFindSuffixArray WINELIB_NAME_AW(PathFindSuffixArray)

VOID WINAPI PathUndecorateA(_Inout_ LPSTR);
VOID WINAPI PathUndecorateW(_Inout_ LPWSTR);
#define PathUndecorate WINELIB_NAME_AW(PathUndecorate)

BOOL
WINAPI
PathUnExpandEnvStringsA(
  _In_ LPCSTR,
  _Out_writes_(cchBuf) LPSTR,
  _In_ UINT cchBuf);

BOOL
WINAPI
PathUnExpandEnvStringsW(
  _In_ LPCWSTR,
  _Out_writes_(cchBuf) LPWSTR,
  _In_ UINT cchBuf);

#define PathUnExpandEnvStrings WINELIB_NAME_AW(PathUnExpandEnvStrings)

/* Url functions */
typedef enum {
    URL_SCHEME_INVALID     = -1,
    URL_SCHEME_UNKNOWN     =  0,
    URL_SCHEME_FTP,
    URL_SCHEME_HTTP,
    URL_SCHEME_GOPHER,
    URL_SCHEME_MAILTO,
    URL_SCHEME_NEWS,
    URL_SCHEME_NNTP,
    URL_SCHEME_TELNET,
    URL_SCHEME_WAIS,
    URL_SCHEME_FILE,
    URL_SCHEME_MK,
    URL_SCHEME_HTTPS,
    URL_SCHEME_SHELL,
    URL_SCHEME_SNEWS,
    URL_SCHEME_LOCAL,
    URL_SCHEME_JAVASCRIPT,
    URL_SCHEME_VBSCRIPT,
    URL_SCHEME_ABOUT,
    URL_SCHEME_RES,
    URL_SCHEME_MSSHELLROOTED,
    URL_SCHEME_MSSHELLIDLIST,
    URL_SCHEME_MSHELP,
    URL_SCHEME_MSSHELLDEVICE,
    URL_SCHEME_WILDCARD,
    URL_SCHEME_SEARCH_MS,
    URL_SCHEME_SEARCH,
    URL_SCHEME_KNOWNFOLDER,
    URL_SCHEME_MAXVALUE
} URL_SCHEME;

/* These are used by UrlGetPart routine */
typedef enum {
    URL_PART_NONE    = 0,
    URL_PART_SCHEME  = 1,
    URL_PART_HOSTNAME,
    URL_PART_USERNAME,
    URL_PART_PASSWORD,
    URL_PART_PORT,
    URL_PART_QUERY
} URL_PART;

#define URL_PARTFLAG_KEEPSCHEME  0x00000001

/* These are used by the UrlIs... routines */
typedef enum {
    URLIS_URL,
    URLIS_OPAQUE,
    URLIS_NOHISTORY,
    URLIS_FILEURL,
    URLIS_APPLIABLE,
    URLIS_DIRECTORY,
    URLIS_HASQUERY
} URLIS;

/* This is used by the UrlApplyScheme... routines */
#define URL_APPLY_FORCEAPPLY         0x00000008
#define URL_APPLY_GUESSFILE          0x00000004
#define URL_APPLY_GUESSSCHEME        0x00000002
#define URL_APPLY_DEFAULT            0x00000001

/* The following are used by UrlEscape..., UrlUnEscape...,
 * UrlCanonicalize..., and UrlCombine... routines
 */
#define URL_WININET_COMPATIBILITY    0x80000000
#define URL_PLUGGABLE_PROTOCOL       0x40000000
#define URL_ESCAPE_UNSAFE            0x20000000
#define URL_UNESCAPE                 0x10000000

#define URL_DONT_SIMPLIFY            0x08000000
#define URL_NO_META                  URL_DONT_SIMPLIFY
#define URL_ESCAPE_SPACES_ONLY       0x04000000
#define URL_DONT_ESCAPE_EXTRA_INFO   0x02000000
#define URL_DONT_UNESCAPE_EXTRA_INFO URL_DONT_ESCAPE_EXTRA_INFO
#define URL_BROWSER_MODE             URL_DONT_ESCAPE_EXTRA_INFO

#define URL_INTERNAL_PATH            0x00800000  /* Will escape #'s in paths */
#define URL_UNESCAPE_HIGH_ANSI_ONLY  0x00400000
#define URL_CONVERT_IF_DOSPATH       0x00200000
#define URL_UNESCAPE_INPLACE         0x00100000

#define URL_FILE_USE_PATHURL         0x00010000
#define URL_ESCAPE_AS_UTF8           0x00040000

#define URL_ESCAPE_SEGMENT_ONLY      0x00002000
#define URL_ESCAPE_PERCENT           0x00001000

HRESULT
WINAPI
UrlApplySchemeA(
  _In_ LPCSTR,
  _Out_writes_(*pcchOut) LPSTR,
  _Inout_ LPDWORD pcchOut,
  DWORD);

HRESULT
WINAPI
UrlApplySchemeW(
  _In_ LPCWSTR,
  _Out_writes_(*pcchOut) LPWSTR,
  _Inout_ LPDWORD pcchOut,
  DWORD);

#define UrlApplyScheme WINELIB_NAME_AW(UrlApplyScheme)

HRESULT
WINAPI
UrlCanonicalizeA(
  _In_ LPCSTR,
  _Out_writes_to_(*pcchCanonicalized, *pcchCanonicalized) LPSTR,
  _Inout_ LPDWORD pcchCanonicalized,
  DWORD);

HRESULT
WINAPI
UrlCanonicalizeW(
  _In_ LPCWSTR,
  _Out_writes_to_(*pcchCanonicalized, *pcchCanonicalized) LPWSTR,
  _Inout_ LPDWORD pcchCanonicalized,
  DWORD);

#define UrlCanonicalize WINELIB_NAME_AW(UrlCanonicalize)

HRESULT
WINAPI
UrlCombineA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _Out_writes_to_opt_(*pcchCombined, *pcchCombined) LPSTR,
  _Inout_ LPDWORD pcchCombined,
  DWORD);

HRESULT
WINAPI
UrlCombineW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _Out_writes_to_opt_(*pcchCombined, *pcchCombined) LPWSTR,
  _Inout_ LPDWORD pcchCombined,
  DWORD);

#define UrlCombine WINELIB_NAME_AW(UrlCombine)

INT WINAPI UrlCompareA(_In_ LPCSTR, _In_ LPCSTR, BOOL);
INT WINAPI UrlCompareW(_In_ LPCWSTR, _In_ LPCWSTR, BOOL);
#define UrlCompare WINELIB_NAME_AW(UrlCompare)

HRESULT
WINAPI
UrlEscapeA(
  _In_ LPCSTR,
  _Out_writes_to_(*pcchEscaped, *pcchEscaped) LPSTR,
  _Inout_ LPDWORD pcchEscaped,
  DWORD);

HRESULT
WINAPI
UrlEscapeW(
  _In_ LPCWSTR,
  _Out_writes_to_(*pcchEscaped, *pcchEscaped) LPWSTR,
  _Inout_ LPDWORD pcchEscaped,
  DWORD);

#define UrlEscape WINELIB_NAME_AW(UrlEscape)

#define UrlEscapeSpacesA(x,y,z) UrlCanonicalizeA(x, y, z, \
                         URL_DONT_ESCAPE_EXTRA_INFO|URL_ESCAPE_SPACES_ONLY)
#define UrlEscapeSpacesW(x,y,z) UrlCanonicalizeW(x, y, z, \
                         URL_DONT_ESCAPE_EXTRA_INFO|URL_ESCAPE_SPACES_ONLY)
#define UrlEscapeSpaces WINELIB_NAME_AW(UrlEscapeSpaces)

LPCSTR  WINAPI UrlGetLocationA(_In_ LPCSTR);
LPCWSTR WINAPI UrlGetLocationW(_In_ LPCWSTR);
#define UrlGetLocation WINELIB_NAME_AW(UrlGetLocation)

HRESULT
WINAPI
UrlGetPartA(
  _In_ LPCSTR,
  _Out_writes_(*pcchOut) LPSTR,
  _Inout_ LPDWORD pcchOut,
  DWORD,
  DWORD);

HRESULT
WINAPI
UrlGetPartW(
  _In_ LPCWSTR,
  _Out_writes_(*pcchOut) LPWSTR,
  _Inout_ LPDWORD pcchOut,
  DWORD,
  DWORD);

#define UrlGetPart WINELIB_NAME_AW(UrlGetPart)

HRESULT
WINAPI
HashData(
  _In_reads_bytes_(cbData) const unsigned char *,
  DWORD cbData,
  _Out_writes_bytes_(cbHash) unsigned char *lpDest,
  DWORD cbHash);

HRESULT
WINAPI
UrlHashA(
  _In_ LPCSTR,
  _Out_writes_bytes_(cbHash) unsigned char *,
  DWORD cbHash);

HRESULT
WINAPI
UrlHashW(
  _In_ LPCWSTR,
  _Out_writes_bytes_(cbHash) unsigned char *,
  DWORD cbHash);

#define UrlHash WINELIB_NAME_AW(UrlHash)

BOOL    WINAPI UrlIsA(_In_ LPCSTR, URLIS);
BOOL    WINAPI UrlIsW(_In_ LPCWSTR, URLIS);
#define UrlIs WINELIB_NAME_AW(UrlIs)

BOOL    WINAPI UrlIsNoHistoryA(_In_ LPCSTR);
BOOL    WINAPI UrlIsNoHistoryW(_In_ LPCWSTR);
#define UrlIsNoHistory WINELIB_NAME_AW(UrlIsNoHistory)

BOOL    WINAPI UrlIsOpaqueA(_In_ LPCSTR);
BOOL    WINAPI UrlIsOpaqueW(_In_ LPCWSTR);
#define UrlIsOpaque WINELIB_NAME_AW(UrlIsOpaque)

#define UrlIsFileUrlA(x) UrlIsA(x, URLIS_FILEURL)
#define UrlIsFileUrlW(x) UrlIsW(x, URLIS_FILEURL)
#define UrlIsFileUrl WINELIB_NAME_AW(UrlIsFileUrl)

HRESULT
WINAPI
UrlUnescapeA(
  _Inout_ LPSTR,
  _Out_writes_to_opt_(*pcchUnescaped, *pcchUnescaped) LPSTR,
  _Inout_opt_ LPDWORD pcchUnescaped,
  DWORD);

HRESULT
WINAPI
UrlUnescapeW(
  _Inout_ LPWSTR,
  _Out_writes_to_opt_(*pcchUnescaped, *pcchUnescaped) LPWSTR,
  _Inout_opt_ LPDWORD pcchUnescaped,
  DWORD);

#define UrlUnescape WINELIB_NAME_AW(UrlUnescape)

#define UrlUnescapeInPlaceA(x,y) UrlUnescapeA(x, NULL, NULL, \
                                              y | URL_UNESCAPE_INPLACE)
#define UrlUnescapeInPlaceW(x,y) UrlUnescapeW(x, NULL, NULL, \
                                              y | URL_UNESCAPE_INPLACE)
#define UrlUnescapeInPlace WINELIB_NAME_AW(UrlUnescapeInPlace)

HRESULT
WINAPI
UrlCreateFromPathA(
  _In_ LPCSTR,
  _Out_writes_to_(*pcchUrl, *pcchUrl) LPSTR,
  _Inout_ LPDWORD pcchUrl,
  DWORD);

HRESULT
WINAPI
UrlCreateFromPathW(
  _In_ LPCWSTR,
  _Out_writes_to_(*pcchUrl, *pcchUrl) LPWSTR,
  _Inout_ LPDWORD pcchUrl,
  DWORD);

#define UrlCreateFromPath WINELIB_NAME_AW(UrlCreateFromPath)

typedef struct tagPARSEDURLA {
    DWORD cbSize;
    LPCSTR pszProtocol;
    UINT cchProtocol;
    LPCSTR pszSuffix;
    UINT cchSuffix;
    UINT nScheme;
} PARSEDURLA, *PPARSEDURLA;

typedef struct tagPARSEDURLW {
    DWORD cbSize;
    LPCWSTR pszProtocol;
    UINT cchProtocol;
    LPCWSTR pszSuffix;
    UINT cchSuffix;
    UINT nScheme;
} PARSEDURLW, *PPARSEDURLW;

HRESULT WINAPI ParseURLA(_In_ LPCSTR pszUrl, _Inout_ PARSEDURLA *ppu);
HRESULT WINAPI ParseURLW(_In_ LPCWSTR pszUrl, _Inout_ PARSEDURLW *ppu);
#define ParseURL WINELIB_NAME_AW(ParseUrl)

#endif /* NO_SHLWAPI_PATH */


/* String functions */
#ifndef NO_SHLWAPI_STRFCNS

/* StrToIntEx flags */
#define STIF_DEFAULT     0x0L
#define STIF_SUPPORT_HEX 0x1L

BOOL WINAPI ChrCmpIA (WORD,WORD);
BOOL WINAPI ChrCmpIW (WCHAR,WCHAR);
#define ChrCmpI WINELIB_NAME_AW(ChrCmpI)

INT WINAPI StrCSpnA(_In_ LPCSTR, _In_ LPCSTR);
INT WINAPI StrCSpnW(_In_ LPCWSTR, _In_ LPCWSTR);
#define StrCSpn WINELIB_NAME_AW(StrCSpn)

INT WINAPI StrCSpnIA(_In_ LPCSTR, _In_ LPCSTR);
INT WINAPI StrCSpnIW(_In_ LPCWSTR, _In_ LPCWSTR);
#define StrCSpnI WINELIB_NAME_AW(StrCSpnI)

#define StrCatA lstrcatA
LPWSTR WINAPI StrCatW(_Inout_ LPWSTR, _In_ LPCWSTR);
#define StrCat WINELIB_NAME_AW(StrCat)

LPSTR
WINAPI
StrCatBuffA(
  _Inout_updates_(cchDestBuffSize) LPSTR,
  _In_ LPCSTR,
  INT cchDestBuffSize);

LPWSTR
WINAPI
StrCatBuffW(
  _Inout_updates_(cchDestBuffSize) LPWSTR,
  _In_ LPCWSTR,
  INT cchDestBuffSize);

#define StrCatBuff WINELIB_NAME_AW(StrCatBuff)

DWORD
WINAPI
StrCatChainW(
  _Out_writes_(cchDst) LPWSTR,
  DWORD cchDst,
  DWORD,
  _In_ LPCWSTR);

LPSTR WINAPI StrChrA(_In_ LPCSTR, WORD);
LPWSTR WINAPI StrChrW(_In_ LPCWSTR, WCHAR);
#define StrChr WINELIB_NAME_AW(StrChr)

LPSTR WINAPI StrChrIA(_In_ LPCSTR, WORD);
LPWSTR WINAPI StrChrIW(_In_ LPCWSTR, WCHAR);
#define StrChrI WINELIB_NAME_AW(StrChrI)

#define StrCmpA lstrcmpA
int WINAPI StrCmpW(_In_ LPCWSTR, _In_ LPCWSTR);
#define StrCmp WINELIB_NAME_AW(StrCmp)

#define StrCmpIA lstrcmpiA
int WINAPI StrCmpIW(_In_ LPCWSTR, _In_ LPCWSTR);
#define StrCmpI WINELIB_NAME_AW(StrCmpI)

#define StrCpyA lstrcpyA
LPWSTR WINAPI StrCpyW(_Out_ LPWSTR, _In_ LPCWSTR);
#define StrCpy WINELIB_NAME_AW(StrCpy)

#define StrCpyNA lstrcpynA
LPWSTR WINAPI StrCpyNW(_Out_writes_(cchMax) LPWSTR, _In_ LPCWSTR, int cchMax);
#define StrCpyN WINELIB_NAME_AW(StrCpyN)
#define StrNCpy WINELIB_NAME_AW(StrCpyN)

INT WINAPI StrCmpLogicalW(_In_ LPCWSTR, _In_ LPCWSTR);

INT WINAPI StrCmpNA(_In_ LPCSTR, _In_ LPCSTR, INT);
INT WINAPI StrCmpNW(_In_ LPCWSTR, _In_ LPCWSTR, INT);
#define StrCmpN WINELIB_NAME_AW(StrCmpN)
#define StrNCmp WINELIB_NAME_AW(StrCmpN)

INT WINAPI StrCmpNIA(_In_ LPCSTR, _In_ LPCSTR, INT);
INT WINAPI StrCmpNIW(_In_ LPCWSTR, _In_ LPCWSTR, INT);
#define StrCmpNI WINELIB_NAME_AW(StrCmpNI)
#define StrNCmpI WINELIB_NAME_AW(StrCmpNI)

LPSTR WINAPI StrDupA(_In_ LPCSTR);
LPWSTR WINAPI StrDupW(_In_ LPCWSTR);
#define StrDup WINELIB_NAME_AW(StrDup)

HRESULT WINAPI SHStrDupA(_In_ LPCSTR, _Outptr_ WCHAR**);
HRESULT WINAPI SHStrDupW(_In_ LPCWSTR, _Outptr_ WCHAR**);
#define SHStrDup WINELIB_NAME_AW(SHStrDup)

LPSTR
WINAPI
StrFormatByteSizeA(
  DWORD,
  _Out_writes_(cchBuf) LPSTR,
  UINT cchBuf);

/* A/W Pairing is broken for this function */

LPSTR
WINAPI
StrFormatByteSize64A(
  LONGLONG,
  _Out_writes_(cchBuf) LPSTR,
  UINT cchBuf);

LPWSTR
WINAPI
StrFormatByteSizeW(
  LONGLONG,
  _Out_writes_(cchBuf) LPWSTR,
  UINT cchBuf);

#ifndef WINE_NO_UNICODE_MACROS
#ifdef UNICODE
#define StrFormatByteSize StrFormatByteSizeW
#else
#define StrFormatByteSize StrFormatByteSize64A
#endif
#endif

LPSTR
WINAPI
StrFormatKBSizeA(
  LONGLONG,
  _Out_writes_(cchBuf) LPSTR,
  UINT cchBuf);

LPWSTR
WINAPI
StrFormatKBSizeW(
  LONGLONG,
  _Out_writes_(cchBuf) LPWSTR,
  UINT cchBuf);

#define StrFormatKBSize WINELIB_NAME_AW(StrFormatKBSize)

int
WINAPI
StrFromTimeIntervalA(
  _Out_writes_(cchMax) LPSTR,
  UINT cchMax,
  DWORD,
  int);

int
WINAPI
StrFromTimeIntervalW(
  _Out_writes_(cchMax) LPWSTR,
  UINT cchMax,
  DWORD,
  int);

#define StrFromTimeInterval WINELIB_NAME_AW(StrFromTimeInterval)

BOOL WINAPI StrIsIntlEqualA(BOOL, _In_ LPCSTR, _In_ LPCSTR, int);
BOOL WINAPI StrIsIntlEqualW(BOOL, _In_ LPCWSTR, _In_ LPCWSTR, int);
#define StrIsIntlEqual WINELIB_NAME_AW(StrIsIntlEqual)

#define StrIntlEqNA(a,b,c) StrIsIntlEqualA(TRUE,a,b,c)
#define StrIntlEqNW(a,b,c) StrIsIntlEqualW(TRUE,a,b,c)

#define StrIntlEqNIA(a,b,c) StrIsIntlEqualA(FALSE,a,b,c)
#define StrIntlEqNIW(a,b,c) StrIsIntlEqualW(FALSE,a,b,c)

LPSTR  WINAPI StrNCatA(_Inout_updates_(cchMax) LPSTR, LPCSTR, int cchMax);
LPWSTR WINAPI StrNCatW(_Inout_updates_(cchMax) LPWSTR, LPCWSTR, int cchMax);
#define StrNCat WINELIB_NAME_AW(StrNCat)
#define StrCatN WINELIB_NAME_AW(StrNCat)

LPSTR  WINAPI StrPBrkA(_In_ LPCSTR, _In_ LPCSTR);
LPWSTR WINAPI StrPBrkW(_In_ LPCWSTR, _In_ LPCWSTR);
#define StrPBrk WINELIB_NAME_AW(StrPBrk)

LPSTR  WINAPI StrRChrA(_In_ LPCSTR, _In_opt_ LPCSTR, WORD);
LPWSTR WINAPI StrRChrW(_In_ LPCWSTR, _In_opt_ LPCWSTR, WCHAR);
#define StrRChr WINELIB_NAME_AW(StrRChr)

LPSTR  WINAPI StrRChrIA(_In_ LPCSTR, _In_opt_ LPCSTR, WORD);
LPWSTR WINAPI StrRChrIW(_In_ LPCWSTR, _In_opt_ LPCWSTR, WCHAR);
#define StrRChrI WINELIB_NAME_AW(StrRChrI)

LPSTR  WINAPI StrRStrIA(_In_ LPCSTR, _In_opt_ LPCSTR, _In_ LPCSTR);
LPWSTR WINAPI StrRStrIW(_In_ LPCWSTR, _In_opt_ LPCWSTR, _In_ LPCWSTR);
#define StrRStrI WINELIB_NAME_AW(StrRStrI)

int WINAPI StrSpnA(_In_ LPCSTR psz, _In_ LPCSTR pszSet);
int WINAPI StrSpnW(_In_ LPCWSTR psz, _In_ LPCWSTR pszSet);
#define StrSpn WINELIB_NAME_AW(StrSpn)

LPSTR  WINAPI StrStrA(_In_ LPCSTR pszFirst, _In_ LPCSTR pszSrch);
LPWSTR WINAPI StrStrW(_In_ LPCWSTR pszFirst, _In_ LPCWSTR pszSrch);
#define StrStr WINELIB_NAME_AW(StrStr)

LPSTR  WINAPI StrStrIA(_In_ LPCSTR pszFirst, _In_ LPCSTR pszSrch);
LPWSTR WINAPI StrStrIW(_In_ LPCWSTR pszFirst, _In_ LPCWSTR pszSrch);
#define StrStrI WINELIB_NAME_AW(StrStrI)

LPWSTR WINAPI StrStrNW(_In_ LPCWSTR, _In_ LPCWSTR, UINT);
LPWSTR WINAPI StrStrNIW(_In_ LPCWSTR, _In_ LPCWSTR, UINT);

int WINAPI StrToIntA(_In_ LPCSTR);
int WINAPI StrToIntW(_In_ LPCWSTR);
#define StrToInt WINELIB_NAME_AW(StrToInt)
#define StrToLong WINELIB_NAME_AW(StrToInt)

BOOL WINAPI StrToIntExA(_In_ LPCSTR, DWORD, _Out_ int*);
BOOL WINAPI StrToIntExW(_In_ LPCWSTR, DWORD, _Out_ int*);
#define StrToIntEx WINELIB_NAME_AW(StrToIntEx)

BOOL WINAPI StrToInt64ExA(_In_ LPCSTR, DWORD, _Out_ LONGLONG*);
BOOL WINAPI StrToInt64ExW(_In_ LPCWSTR, DWORD, _Out_ LONGLONG*);
#define StrToIntEx64 WINELIB_NAME_AW(StrToIntEx64)

BOOL WINAPI StrTrimA(_Inout_ LPSTR, _In_ LPCSTR);
BOOL WINAPI StrTrimW(_Inout_ LPWSTR, _In_ LPCWSTR);
#define StrTrim WINELIB_NAME_AW(StrTrim)

INT
WINAPI
wvnsprintfA(
  _Out_writes_(cchDest) LPSTR,
  _In_ INT cchDest,
  _In_ _Printf_format_string_ LPCSTR,
  _In_ __ms_va_list);

INT
WINAPI
wvnsprintfW(
  _Out_writes_(cchDest) LPWSTR,
  _In_ INT cchDest,
  _In_ _Printf_format_string_ LPCWSTR,
  _In_ __ms_va_list);

#define wvnsprintf WINELIB_NAME_AW(wvnsprintf)

INT
WINAPIV
wnsprintfA(
  _Out_writes_(cchDest) LPSTR,
  _In_ INT cchDest,
  _In_ _Printf_format_string_ LPCSTR,
  ...);

INT
WINAPIV
wnsprintfW(
  _Out_writes_(cchDest) LPWSTR,
  _In_ INT cchDest,
  _In_ _Printf_format_string_ LPCWSTR,
  ...);

#define wnsprintf WINELIB_NAME_AW(wnsprintf)

HRESULT
WINAPI
SHLoadIndirectString(
  _In_ LPCWSTR,
  _Out_writes_(cchOutBuf) LPWSTR,
  _In_ UINT cchOutBuf,
  _Reserved_ PVOID*);

BOOL
WINAPI
IntlStrEqWorkerA(
  BOOL,
  _In_reads_(nChar) LPCSTR,
  _In_reads_(nChar) LPCSTR,
  int nChar);

BOOL
WINAPI
IntlStrEqWorkerW(
  BOOL,
  _In_reads_(nChar) LPCWSTR,
  _In_reads_(nChar) LPCWSTR,
  int nChar);

#define IntlStrEqWorker WINELIB_NAME_AW(IntlStrEqWorker)

#define IntlStrEqNA(s1,s2,n) IntlStrEqWorkerA(TRUE,s1,s2,n)
#define IntlStrEqNW(s1,s2,n) IntlStrEqWorkerW(TRUE,s1,s2,n)
#define IntlStrEqN WINELIB_NAME_AW(IntlStrEqN)

#define IntlStrEqNIA(s1,s2,n) IntlStrEqWorkerA(FALSE,s1,s2,n)
#define IntlStrEqNIW(s1,s2,n) IntlStrEqWorkerW(FALSE,s1,s2,n)
#define IntlStrEqNI WINELIB_NAME_AW(IntlStrEqNI)

HRESULT
WINAPI
StrRetToStrA(
  _Inout_ STRRET*,
  _In_opt_ PCUITEMID_CHILD,
  _Outptr_ LPSTR*);

HRESULT
WINAPI
StrRetToStrW(
  _Inout_ STRRET*,
  _In_opt_ PCUITEMID_CHILD,
  _Outptr_ LPWSTR*);

#define StrRetToStr WINELIB_NAME_AW(StrRetToStr)

HRESULT
WINAPI
StrRetToBufA(
  _Inout_ STRRET*,
  _In_opt_ PCUITEMID_CHILD,
  _Out_writes_(cchBuf) LPSTR,
  UINT cchBuf);

HRESULT
WINAPI
StrRetToBufW(
  _Inout_ STRRET*,
  _In_opt_ PCUITEMID_CHILD,
  _Out_writes_(cchBuf) LPWSTR,
  UINT cchBuf);

#define StrRetToBuf WINELIB_NAME_AW(StrRetToBuf)

HRESULT
WINAPI
StrRetToBSTR(
  _Inout_ STRRET*,
  _In_opt_ PCUITEMID_CHILD,
  _Outptr_ BSTR*);

BOOL WINAPI IsCharSpaceA(CHAR);
BOOL WINAPI IsCharSpaceW(WCHAR);
#define IsCharSpace WINELIB_NAME_AW(IsCharSpace)

#endif /* NO_SHLWAPI_STRFCNS */


/* GDI functions */
#ifndef NO_SHLWAPI_GDI

HPALETTE WINAPI SHCreateShellPalette(_In_opt_ HDC);

COLORREF WINAPI ColorHLSToRGB(WORD,WORD,WORD);

COLORREF WINAPI ColorAdjustLuma(COLORREF,int,BOOL);

VOID WINAPI ColorRGBToHLS(COLORREF, _Out_ LPWORD, _Out_ LPWORD, _Out_ LPWORD);

#endif /* NO_SHLWAPI_GDI */

/* Security functions */
BOOL WINAPI IsInternetESCEnabled(void);

/* Stream functions */
#ifndef NO_SHLWAPI_STREAM

struct IStream *
WINAPI
SHOpenRegStreamA(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD);

struct IStream *
WINAPI
SHOpenRegStreamW(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD);

#define SHOpenRegStream WINELIB_NAME_AW(SHOpenRegStream2) /* Uses version 2 */

struct IStream *
WINAPI
SHOpenRegStream2A(
  _In_ HKEY,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD);

struct IStream *
WINAPI
SHOpenRegStream2W(
  _In_ HKEY,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD);

#define SHOpenRegStream2 WINELIB_NAME_AW(SHOpenRegStream2)

HRESULT
WINAPI
SHCreateStreamOnFileA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Outptr_ struct IStream**);

HRESULT
WINAPI
SHCreateStreamOnFileW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Outptr_ struct IStream**);

#define SHCreateStreamOnFile WINELIB_NAME_AW(SHCreateStreamOnFile)

struct IStream*
WINAPI
SHCreateMemStream(
  _In_reads_bytes_opt_(cbInit) const BYTE *pInit,
  _In_ UINT cbInit);

HRESULT
WINAPI
SHCreateStreamOnFileEx(
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_ DWORD,
  _In_ BOOL,
  _In_opt_ struct IStream*,
  _Outptr_ struct IStream**);

HRESULT WINAPI SHCreateStreamWrapper(LPBYTE,DWORD,DWORD,struct IStream**);

#endif /* NO_SHLWAPI_STREAM */

#ifndef NO_SHLWAPI_SHARED

// These functions are only included in this file starting with the Windows 7 platform SDK

HANDLE
WINAPI
SHAllocShared(
    _In_opt_  const void *pvData,
    _In_      DWORD dwSize,
    _In_      DWORD dwDestinationProcessId
    );

PVOID
WINAPI
SHLockShared(
    _In_  HANDLE hData,
    _In_  DWORD dwProcessId
    );

BOOL
WINAPI
SHUnlockShared(
    _In_  void *pvData
    );

BOOL
WINAPI
SHFreeShared(
    _In_  HANDLE hData,
    _In_  DWORD dwProcessId
    );

#endif /* NO_SHLWAPI_SHARED */

INT WINAPI GetMenuPosFromID(_In_ HMENU hMenu, _In_ UINT uID);

/* SHAutoComplete flags */
#define SHACF_DEFAULT               0x00000000
#define SHACF_FILESYSTEM            0x00000001
#define SHACF_URLHISTORY            0x00000002
#define SHACF_URLMRU                0x00000004
#define SHACF_URLALL                (SHACF_URLHISTORY|SHACF_URLMRU)
#define SHACF_USETAB                0x00000008
#define SHACF_FILESYS_ONLY          0x00000010
#define SHACF_FILESYS_DIRS          0x00000020
#define SHACF_AUTOSUGGEST_FORCE_ON  0x10000000
#define SHACF_AUTOSUGGEST_FORCE_OFF 0x20000000
#define SHACF_AUTOAPPEND_FORCE_ON   0x40000000
#define SHACF_AUTOAPPEND_FORCE_OFF  0x80000000

HRESULT WINAPI SHAutoComplete(_In_ HWND, DWORD);

/* Threads */
HRESULT WINAPI SHCreateThreadRef(_Inout_ LONG*, _Outptr_ IUnknown**);
HRESULT WINAPI SHGetThreadRef(_Outptr_ IUnknown**);
HRESULT WINAPI SHSetThreadRef(_In_opt_ IUnknown*);
HRESULT WINAPI SHReleaseThreadRef(void);

/* SHCreateThread flags */
#define CTF_INSIST          0x01 /* Always call */
#define CTF_THREAD_REF      0x02 /* Hold thread ref */
#define CTF_PROCESS_REF     0x04 /* Hold process ref */
#define CTF_COINIT          0x08 /* Startup COM first */
#define CTF_FREELIBANDEXIT  0x10 /* Hold DLL ref */
#define CTF_REF_COUNTED     0x20 /* Thread is ref counted */
#define CTF_WAIT_ALLOWCOM   0x40 /* Allow marshalling */

BOOL
WINAPI
SHCreateThread(
  _In_ LPTHREAD_START_ROUTINE pfnThreadProc,
  _In_opt_ void* pData,
  _In_ DWORD flags,
  _In_opt_ LPTHREAD_START_ROUTINE pfnCallback);

BOOL WINAPI SHSkipJunction(_In_opt_ struct IBindCtx*, _In_ const CLSID*);

/* Version Information */

typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

#define DLLVER_PLATFORM_WINDOWS 0x01 /* Win9x */
#define DLLVER_PLATFORM_NT      0x02 /* WinNT */

typedef HRESULT (CALLBACK *DLLGETVERSIONPROC)(DLLVERSIONINFO *);

#ifdef __WINESRC__
/* shouldn't be here, but is nice for type checking */
HRESULT WINAPI DllGetVersion(DLLVERSIONINFO *) DECLSPEC_HIDDEN;
#endif

typedef struct _DLLVERSIONINFO2 {
    DLLVERSIONINFO info1;
    DWORD          dwFlags;    /* Reserved */
    ULONGLONG DECLSPEC_ALIGN(8) ullVersion; /* 16 bits each for Major, Minor, Build, QFE */
} DLLVERSIONINFO2;

#define DLLVER_MAJOR_MASK 0xFFFF000000000000
#define DLLVER_MINOR_MASK 0x0000FFFF00000000
#define DLLVER_BUILD_MASK 0x00000000FFFF0000
#define DLLVER_QFE_MASK   0x000000000000FFFF

#define MAKEDLLVERULL(mjr, mnr, bld, qfe) (((ULONGLONG)(mjr)<< 48)| \
  ((ULONGLONG)(mnr)<< 32) | ((ULONGLONG)(bld)<< 16) | (ULONGLONG)(qfe))

HRESULT WINAPI DllInstall(BOOL, _In_opt_ LPCWSTR) DECLSPEC_HIDDEN;


#if (_WIN32_IE >= 0x0600)
#define SHGVSPB_PERUSER        0x00000001
#define SHGVSPB_ALLUSERS       0x00000002
#define SHGVSPB_PERFOLDER      0x00000004
#define SHGVSPB_ALLFOLDERS     0x00000008
#define SHGVSPB_INHERIT        0x00000010
#define SHGVSPB_ROAM           0x00000020
#define SHGVSPB_NOAUTODEFAULTS 0x80000000

#define SHGVSPB_FOLDER           (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER)
#define SHGVSPB_FOLDERNODEFAULTS (SHGVSPB_PERUSER | SHGVSPB_PERFOLDER | SHGVSPB_NOAUTODEFAULTS)
#define SHGVSPB_USERDEFAULTS     (SHGVSPB_PERUSER | SHGVSPB_ALLFOLDERS)
#define SHGVSPB_GLOBALDEAFAULTS  (SHGVSPB_ALLUSERS | SHGVSPB_ALLFOLDERS)

HRESULT
WINAPI
SHGetViewStatePropertyBag(
  _In_opt_ PCIDLIST_ABSOLUTE pidl,
  _In_opt_ LPCWSTR bag_name,
  _In_ DWORD flags,
  _In_ REFIID riid,
  _Outptr_ void **ppv);

#endif  /* (_WIN32_IE >= 0x0600) */


/* IsOS definitions */

#define OS_WIN32SORGREATER        0x00
#define OS_NT                     0x01
#define OS_WIN95ORGREATER         0x02
#define OS_NT4ORGREATER           0x03
#define OS_WIN2000ORGREATER_ALT   0x04
#define OS_WIN98ORGREATER         0x05
#define OS_WIN98_GOLD             0x06
#define OS_WIN2000ORGREATER       0x07
#define OS_WIN2000PRO             0x08
#define OS_WIN2000SERVER          0x09
#define OS_WIN2000ADVSERVER       0x0A
#define OS_WIN2000DATACENTER      0x0B
#define OS_WIN2000TERMINAL        0x0C
#define OS_EMBEDDED               0x0D
#define OS_TERMINALCLIENT         0x0E
#define OS_TERMINALREMOTEADMIN    0x0F
#define OS_WIN95_GOLD             0x10
#define OS_MEORGREATER            0x11
#define OS_XPORGREATER            0x12
#define OS_HOME                   0x13
#define OS_PROFESSIONAL           0x14
#define OS_DATACENTER             0x15
#define OS_ADVSERVER              0x16
#define OS_SERVER                 0x17
#define OS_TERMINALSERVER         0x18
#define OS_PERSONALTERMINALSERVER 0x19
#define OS_FASTUSERSWITCHING      0x1A
#define OS_WELCOMELOGONUI         0x1B
#define OS_DOMAINMEMBER           0x1C
#define OS_ANYSERVER              0x1D
#define OS_WOW6432                0x1E
#define OS_WEBSERVER              0x1F
#define OS_SMALLBUSINESSSERVER    0x20
#define OS_TABLETPC               0x21
#define OS_SERVERADMINUI          0x22
#define OS_MEDIACENTER            0x23
#define OS_APPLIANCE              0x24

BOOL WINAPI IsOS(DWORD);

/* SHSetTimerQueueTimer definitions */
#define TPS_EXECUTEIO    0x00000001
#define TPS_LONGEXECTIME 0x00000008

/* SHFormatDateTimeA/SHFormatDateTimeW flags */
#define FDTF_SHORTTIME          0x00000001
#define FDTF_SHORTDATE          0x00000002
#define FDTF_DEFAULT            (FDTF_SHORTDATE | FDTF_SHORTTIME)
#define FDTF_LONGDATE           0x00000004
#define FDTF_LONGTIME           0x00000008
#define FDTF_RELATIVE           0x00000010
#define FDTF_LTRDATE            0x00000100
#define FDTF_RTLDATE            0x00000200
#define FDTF_NOAUTOREADINGORDER 0x00000400


typedef struct
{
    const IID *piid;
#if defined(__REACTOS__) || (WINVER >= _WIN32_WINNT_WIN10)
    DWORD dwOffset;
#else
    int dwOffset;
#endif
} QITAB, *LPQITAB;

HRESULT
WINAPI
QISearch(
  _Inout_ void* base,
  _In_ const QITAB *pqit,
  _In_ REFIID riid,
  _Outptr_ void **ppv);

#define OFFSETOFCLASS(base, derived) \
    ((DWORD)(DWORD_PTR)(static_cast<base*>((derived*)8))-8)

#define QITABENTMULTI(Cthis, Ifoo, Iimpl) { &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }
#define QITABENT(Cthis, Ifoo) QITABENTMULTI(Cthis, Ifoo, Ifoo)

#include <poppack.h>

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLWAPI_H */
