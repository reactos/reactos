/*
 * Copyright (C) 2002,2003 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_MSI_H
#define __WINE_MSI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long MSIHANDLE;

typedef enum tagINSTALLSTATE
{
    INSTALLSTATE_BADCONFIG = -6,
    INSTALLSTATE_INCOMPLETE = -5,
    INSTALLSTATE_SOURCEABSENT = -4,
    INSTALLSTATE_MOREDATA = -3,
    INSTALLSTATE_INVALIDARG = -2,
    INSTALLSTATE_UNKNOWN = -1,
    INSTALLSTATE_BROKEN = 0,
    INSTALLSTATE_ADVERTISED = 1,
    INSTALLSTATE_ABSENT = 2,
    INSTALLSTATE_LOCAL = 3,
    INSTALLSTATE_SOURCE = 4,
    INSTALLSTATE_DEFAULT = 5
} INSTALLSTATE;

typedef enum tagINSTALLUILEVEL
{
    INSTALLUILEVEL_NOCHANGE = 0,
    INSTALLUILEVEL_DEFAULT = 1,
    INSTALLUILEVEL_NONE = 2,
    INSTALLUILEVEL_BASIC = 3,
    INSTALLUILEVEL_REDUCED = 4,
    INSTALLUILEVEL_FULL = 5,
    INSTALLUILEVEL_HIDECANCEL = 0x20,
    INSTALLUILEVEL_PROGRESSONLY = 0x40,
    INSTALLUILEVEL_ENDDIALOG = 0x80,
    INSTALLUILEVEL_SOURCERESONLY = 0x100
} INSTALLUILEVEL;

typedef enum tagINSTALLLEVEL
{
    INSTALLLEVEL_DEFAULT = 0,
    INSTALLLEVEL_MINIMUM = 1,
    INSTALLLEVEL_MAXIMUM = 0xFFFF
} INSTALLLEVEL;

typedef enum tagINSTALLMESSAGE
{
    INSTALLMESSAGE_FATALEXIT = 0,
    INSTALLMESSAGE_ERROR = 0x01000000,
    INSTALLMESSAGE_WARNING = 0x02000000,
    INSTALLMESSAGE_USER = 0x03000000,
    INSTALLMESSAGE_INFO = 0x04000000,
    INSTALLMESSAGE_FILESINUSE = 0x05000000,
    INSTALLMESSAGE_RESOLVESOURCE = 0x06000000,
    INSTALLMESSAGE_OUTOFDISKSPACE = 0x07000000,
    INSTALLMESSAGE_ACTIONSTART = 0x08000000,
    INSTALLMESSAGE_ACTIONDATA = 0x09000000,
    INSTALLMESSAGE_PROGRESS = 0x0a000000,
    INSTALLMESSAGE_COMMONDATA = 0x0b000000,
    INSTALLMESSAGE_INITIALIZE = 0x0c000000,
    INSTALLMESSAGE_TERMINATE = 0x0d000000,
    INSTALLMESSAGE_SHOWDIALOG = 0x0e000000
} INSTALLMESSAGE;

typedef enum tagREINSTALLMODE
{
    REINSTALLMODE_REPAIR = 0x00000001,
    REINSTALLMODE_FILEMISSING = 0x00000002,
    REINSTALLMODE_FILEOLDERVERSION = 0x00000004,
    REINSTALLMODE_FILEEQUALVERSION = 0x00000008,
    REINSTALLMODE_FILEEXACT = 0x00000010,
    REINSTALLMODE_FILEVERIFY = 0x00000020,
    REINSTALLMODE_FILEREPLACE = 0x00000040,
    REINSTALLMODE_MACHINEDATA = 0x00000080,
    REINSTALLMODE_USERDATA = 0x00000100,
    REINSTALLMODE_SHORTCUT = 0x00000200,
    REINSTALLMODE_PACKAGE = 0x00000400
} REINSTALLMODE;

typedef enum tagINSTALLLOGMODE
{
    INSTALLLOGMODE_FATALEXIT      = (1 << (INSTALLMESSAGE_FATALEXIT >> 24)),
    INSTALLLOGMODE_ERROR          = (1 << (INSTALLMESSAGE_ERROR >> 24)),
    INSTALLLOGMODE_WARNING        = (1 << (INSTALLMESSAGE_WARNING >> 24)),
    INSTALLLOGMODE_USER           = (1 << (INSTALLMESSAGE_USER >> 24)),
    INSTALLLOGMODE_INFO           = (1 << (INSTALLMESSAGE_INFO >> 24)),
    INSTALLLOGMODE_RESOLVESOURCE  = (1 << (INSTALLMESSAGE_RESOLVESOURCE >> 24)),
    INSTALLLOGMODE_OUTOFDISKSPACE = (1 << (INSTALLMESSAGE_OUTOFDISKSPACE >> 24)),
    INSTALLLOGMODE_ACTIONSTART    = (1 << (INSTALLMESSAGE_ACTIONSTART >> 24)),
    INSTALLLOGMODE_ACTIONDATA     = (1 << (INSTALLMESSAGE_ACTIONDATA >> 24)),
    INSTALLLOGMODE_COMMONDATA     = (1 << (INSTALLMESSAGE_COMMONDATA >> 24)),
    INSTALLLOGMODE_PROPERTYDUMP   = (1 << (INSTALLMESSAGE_PROGRESS >> 24)),
    INSTALLLOGMODE_VERBOSE        = (1 << (INSTALLMESSAGE_INITIALIZE >> 24)),
    INSTALLLOGMODE_EXTRADEBUG     = (1 << (INSTALLMESSAGE_TERMINATE >> 24)),
    INSTALLLOGMODE_PROGRESS       = (1 << (INSTALLMESSAGE_PROGRESS >> 24)),
    INSTALLLOGMODE_INITIALIZE     = (1 << (INSTALLMESSAGE_INITIALIZE >> 24)),
    INSTALLLOGMODE_TERMINATE      = (1 << (INSTALLMESSAGE_TERMINATE >> 24)),
    INSTALLLOGMODE_SHOWDIALOG     = (1 << (INSTALLMESSAGE_SHOWDIALOG >> 24))
} INSTALLLOGMODE;

typedef enum tagINSTALLLOGATTRIBUTES
{
    INSTALLLOGATTRIBUTES_APPEND = 0x00000001,
    INSTALLLOGATTRIBUTES_FLUSHEACHLINE = 0x00000002
} INSTALLLOGATTRIBUTES;

typedef enum tagADVERTISEFLAGS
{
    ADVERTISEFLAGS_MACHINEASSIGN = 0,
    ADVERTISEFLAGS_USERASSIGN = 1
} ADVERTISEFLAGS;

typedef enum tagINSTALLTYPE
{
    INSTALLTYPE_DEFAULT = 0,
    INSTALLTYPE_NETWORK_IMAGE = 1
} INSTALLTYPE;

#define MAX_FEATURE_CHARS 38

typedef INT (CALLBACK *INSTALLUI_HANDLERA)(LPVOID pvContext, UINT iMessageType,
                                       LPCSTR szMessage);
typedef INT (CALLBACK *INSTALLUI_HANDLERW)(LPVOID pvContext, UINT iMessageType,
                                       LPCWSTR szMessage);

UINT WINAPI MsiAdvertiseProductA(LPCSTR, LPCSTR, LPCSTR, LANGID);
UINT WINAPI MsiAdvertiseProductW(LPCWSTR, LPCWSTR, LPCWSTR, LANGID);
#define     MsiAdvertiseProduct WINELIB_NAME_AW(MsiAdvertiseProduct)

UINT WINAPI MsiInstallProductA(LPCSTR, LPCSTR);
UINT WINAPI MsiInstallProductW(LPCWSTR, LPCWSTR);
#define     MsiInstallProduct WINELIB_NAME_AW(MsiInstallProduct)

UINT WINAPI MsiReinstallProductA(LPCSTR, DWORD);
UINT WINAPI MsiReinstallProductW(LPCWSTR, DWORD);
#define     MsiReinstallProduct WINELIB_NAME_AW(MsiReinstallProduct)

UINT WINAPI MsiApplyPatchA(LPCSTR, LPCSTR, INSTALLTYPE, LPCSTR);
UINT WINAPI MsiApplyPatchW(LPCWSTR, LPCWSTR, INSTALLTYPE, LPCWSTR);
#define     MsiApplyPatch WINELIB_NAME_AW(MsiApplyPatch)

UINT WINAPI MsiEnumProductsA(DWORD index, LPSTR lpguid);
UINT WINAPI MsiEnumProductsW(DWORD index, LPWSTR lpguid);
#define     MsiEnumProducts WINELIB_NAME_AW(MsiEnumProducts)

UINT WINAPI MsiEnumFeaturesA(LPCSTR, DWORD, LPSTR, LPSTR);
UINT WINAPI MsiEnumFeaturesW(LPCWSTR, DWORD, LPWSTR, LPWSTR);
#define     MsiEnumFeatures WINELIB_NAME_AW(MsiEnumFeatures)

UINT WINAPI MsiEnumComponentsA(DWORD, LPSTR);
UINT WINAPI MsiEnumComponentsW(DWORD, LPWSTR);
#define     MsiEnumComponents WINELIB_NAME_AW(MsiEnumComponents)

UINT WINAPI MsiEnumClientsA(LPCSTR, DWORD, LPSTR);
UINT WINAPI MsiEnumClientsW(LPCWSTR, DWORD, LPWSTR);
#define     MsiEnumClients WINELIB_NAME_AW(MsiEnumClients)

UINT WINAPI MsiOpenDatabaseA(LPCSTR, LPCSTR, MSIHANDLE *);
UINT WINAPI MsiOpenDatabaseW(LPCWSTR, LPCWSTR, MSIHANDLE *);
#define     MsiOpenDatabase WINELIB_NAME_AW(MsiOpenDatabase)

UINT WINAPI MsiOpenPackageA(LPCSTR, MSIHANDLE*);
UINT WINAPI MsiOpenPackageW(LPCWSTR, MSIHANDLE*);
#define     MsiOpenPackage WINELIB_NAME_AW(MsiOpenPackage)

UINT WINAPI MsiOpenPackageExA(LPCSTR, DWORD, MSIHANDLE*);
UINT WINAPI MsiOpenPackageExW(LPCWSTR, DWORD, MSIHANDLE*);
#define     MsiOpenPackageEx WINELIB_NAME_AW(MsiOpenPackageEx)

UINT WINAPI MsiOpenProductA(LPCSTR, MSIHANDLE*);
UINT WINAPI MsiOpenProductW(LPCWSTR, MSIHANDLE*);
#define     MsiOpenProduct WINELIB_NAME_AW(MsiOpenProduct)

UINT WINAPI MsiGetSummaryInformationA(MSIHANDLE, LPCSTR, UINT, MSIHANDLE *);
UINT WINAPI MsiGetSummaryInformationW(MSIHANDLE, LPCWSTR, UINT, MSIHANDLE *);
#define     MsiGetSummaryInformation WINELIB_NAME_AW(MsiGetSummaryInformation)

UINT WINAPI MsiSummaryInfoGetPropertyA(MSIHANDLE,UINT,UINT*,INT*,FILETIME*,LPSTR,DWORD*);
UINT WINAPI MsiSummaryInfoGetPropertyW(MSIHANDLE,UINT,UINT*,INT*,FILETIME*,LPWSTR,DWORD*);
#define     MsiSummaryInfoGetProperty WINELIB_NAME_AW(MsiSummaryInfoGetProperty)

UINT WINAPI MsiProvideComponentFromDescriptorA(LPCSTR,LPSTR,DWORD*,DWORD*);
UINT WINAPI MsiProvideComponentFromDescriptorW(LPCWSTR,LPWSTR,DWORD*,DWORD*);
#define     MsiProvideComponentFromDescriptor WINELIB_NAME_AW(MsiProvideComponentFromDescriptor)

UINT WINAPI MsiGetProductPropertyA(MSIHANDLE,LPCSTR,LPSTR,DWORD*);
UINT WINAPI MsiGetProductPropertyW(MSIHANDLE,LPCWSTR,LPWSTR,DWORD*);
#define     MsiGetProductProperty WINELIB_NAME_AW(MsiGetProductProperty)

UINT WINAPI MsiGetPropertyA(MSIHANDLE, LPCSTR, LPSTR, DWORD*);
UINT WINAPI MsiGetPropertyW(MSIHANDLE, LPCWSTR, LPWSTR, DWORD*);
#define     MsiGetProperty WINELIB_NAME_AW(MsiGetProperty)

UINT WINAPI MsiVerifyPackageA(LPCSTR);
UINT WINAPI MsiVerifyPackageW(LPCWSTR);
#define     MsiVerifyPackage WINELIB_NAME_AW(MsiVerifyPackage)

INSTALLSTATE WINAPI MsiQueryProductStateA(LPCSTR);
INSTALLSTATE WINAPI MsiQueryProductStateW(LPCWSTR);
#define      MsiQueryProductState WINELIB_NAME_AW(MsiQueryProductState)

UINT WINAPI MsiConfigureProductA(LPCSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState);
UINT WINAPI MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState);
#define     MsiConfigureProduct WINELIB_NAME_AW(MsiConfigureProduct);

UINT WINAPI MsiGetProductCodeA(LPCSTR szComponent, LPSTR szBuffer);
UINT WINAPI MsiGetProductCodeW(LPCWSTR szComponent, LPWSTR szBuffer);
#define     MsiGetProductCode WINELIB_NAME_AW(MsiGetProductCode)

UINT WINAPI MsiGetProductInfoA(LPCSTR szProduct, LPCSTR szAttribute, LPSTR szBuffer, DWORD *pcchValueBuf);
UINT WINAPI MsiGetProductInfoW(LPCWSTR szProduct, LPCWSTR szAttribute, LPWSTR szBuffer, DWORD *pcchValueBuf);
#define     MsiGetProductInfo WINELIB_NAME_AW(MsiGetProductInfo)

UINT WINAPI MsiEnableLogA(DWORD dwLogMode, LPCSTR szLogFile, DWORD attributes);
UINT WINAPI MsiEnableLogW(DWORD dwLogMode, LPCWSTR szLogFile, DWORD attributes);
#define     MsiEnableLog WINELIB_NAME_AW(MsiEnableLog)

INSTALLUI_HANDLERA WINAPI MsiSetExternalUIA(INSTALLUI_HANDLERA, DWORD, LPVOID);
INSTALLUI_HANDLERW WINAPI MsiSetExternalUIW(INSTALLUI_HANDLERW, DWORD, LPVOID);
#define MsiSetExternalUI WINELIB_NAME_AW(MsiSetExternalUI)

INSTALLSTATE WINAPI MsiGetComponentPathA(LPCSTR, LPCSTR, LPSTR, DWORD*);
INSTALLSTATE WINAPI MsiGetComponentPathW(LPCWSTR, LPCWSTR, LPWSTR, DWORD*);
#define MsiGetComponentPath WINELIB_NAME_AW(MsiGetComponentPath)

INSTALLSTATE WINAPI MsiQueryFeatureStateA(LPCSTR szProduct, LPCSTR szFeature);
INSTALLSTATE WINAPI MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature);
#define MsiQueryFeatureState WINELIB_NAME_AW(MsiQueryFeatureState)

/**
 * Non Unicode
 */
UINT WINAPI MsiCloseHandle(MSIHANDLE);
UINT WINAPI MsiCloseAllHandles();
INSTALLUILEVEL WINAPI MsiSetInternalUI(INSTALLUILEVEL, HWND*);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MSI_H */
