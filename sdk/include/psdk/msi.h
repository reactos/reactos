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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_MSI_H
#define __WINE_MSI_H

#ifndef _MSI_NO_CRYPTO
#include <wincrypt.h>
#endif

#define MAX_GUID_CHARS 38

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG MSIHANDLE;

typedef enum tagINSTALLSTATE
{
    INSTALLSTATE_NOTUSED = -7,
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

typedef enum tagMSIPATCHSTATE
{
    MSIPATCHSTATE_INVALID = 0,
    MSIPATCHSTATE_APPLIED = 1,
    MSIPATCHSTATE_SUPERSEDED = 2,
    MSIPATCHSTATE_OBSOLETED = 4,
    MSIPATCHSTATE_REGISTERED = 8,
    MSIPATCHSTATE_ALL = (MSIPATCHSTATE_APPLIED | MSIPATCHSTATE_SUPERSEDED |
                         MSIPATCHSTATE_OBSOLETED | MSIPATCHSTATE_REGISTERED)
} MSIPATCHSTATE;

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

typedef enum tagUSERINFOSTATE
{
    USERINFOSTATE_MOREDATA = -3,
    USERINFOSTATE_INVALIDARG = -2,
    USERINFOSTATE_UNKNOWN = -1,
    USERINFOSTATE_ABSENT = 0,
    USERINFOSTATE_PRESENT = 1,
} USERINFOSTATE;

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
    INSTALLMESSAGE_SHOWDIALOG = 0x0e000000,
    INSTALLMESSAGE_RMFILESINUSE = 0x19000000,
    INSTALLMESSAGE_INSTALLSTART = 0x1A000000,
    INSTALLMESSAGE_INSTALLEND = 0x1B000000,
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
    INSTALLLOGMODE_FILESINUSE     = (1 << (INSTALLMESSAGE_FILESINUSE >> 24)),
    INSTALLLOGMODE_RESOLVESOURCE  = (1 << (INSTALLMESSAGE_RESOLVESOURCE >> 24)),
    INSTALLLOGMODE_OUTOFDISKSPACE = (1 << (INSTALLMESSAGE_OUTOFDISKSPACE >> 24)),
    INSTALLLOGMODE_ACTIONSTART    = (1 << (INSTALLMESSAGE_ACTIONSTART >> 24)),
    INSTALLLOGMODE_ACTIONDATA     = (1 << (INSTALLMESSAGE_ACTIONDATA >> 24)),
    INSTALLLOGMODE_PROGRESS       = (1 << (INSTALLMESSAGE_PROGRESS >> 24)),
    INSTALLLOGMODE_PROPERTYDUMP   = (1 << (INSTALLMESSAGE_PROGRESS >> 24)),
    INSTALLLOGMODE_COMMONDATA     = (1 << (INSTALLMESSAGE_COMMONDATA >> 24)),
    INSTALLLOGMODE_INITIALIZE     = (1 << (INSTALLMESSAGE_INITIALIZE >> 24)),
    INSTALLLOGMODE_VERBOSE        = (1 << (INSTALLMESSAGE_INITIALIZE >> 24)),
    INSTALLLOGMODE_TERMINATE      = (1 << (INSTALLMESSAGE_TERMINATE >> 24)),
    INSTALLLOGMODE_EXTRADEBUG     = (1 << (INSTALLMESSAGE_TERMINATE >> 24)),
    INSTALLLOGMODE_SHOWDIALOG     = (1 << (INSTALLMESSAGE_SHOWDIALOG >> 24)),
    INSTALLLOGMODE_RMFILESINUSE   = (1 << (INSTALLMESSAGE_RMFILESINUSE >> 24)),
    INSTALLLOGMODE_INSTALLSTART   = (1 << (INSTALLMESSAGE_INSTALLSTART >> 24)),
    INSTALLLOGMODE_INSTALLEND     = (1 << (INSTALLMESSAGE_INSTALLEND >> 24)),
} INSTALLLOGMODE;

typedef enum tagINSTALLLOGATTRIBUTES
{
    INSTALLLOGATTRIBUTES_APPEND = 0x00000001,
    INSTALLLOGATTRIBUTES_FLUSHEACHLINE = 0x00000002
} INSTALLLOGATTRIBUTES;

typedef enum tagINSTALLMODE
{
    INSTALLMODE_NODETECTION_ANY     = -4,
    INSTALLMODE_NOSOURCERESOLUTION  = -3,
    INSTALLMODE_NODETECTION         = -2,
    INSTALLMODE_EXISTING            = -1,
    INSTALLMODE_DEFAULT             = 0
} INSTALLMODE;

typedef enum tagADVERTISEFLAGS
{
    ADVERTISEFLAGS_MACHINEASSIGN = 0,
    ADVERTISEFLAGS_USERASSIGN = 1
} ADVERTISEFLAGS;

typedef enum tagSCRIPTFLAGS
{
    SCRIPTFLAGS_CACHEINFO = 1,
    SCRIPTFLAGS_SHORTCUTS = 4,
    SCRIPTFLAGS_MACHINEASSIGN = 8,
    SCRIPTFLAGS_REGDATA_APPINFO = 0x10,
    SCRIPTFLAGS_REGDATA_CNFGINFO = 0x20,
    SCRIPTFLAGS_VALIDATE_TRANSFORMS_LIST = 0x40,
    SCRIPTFLAGS_REGDATA_CLASSINFO = 0x80,
    SCRIPTFLAGS_REGDATA_EXTENSIONINFO = 0x100,
} SCRIPTFLAGS;

typedef enum tagINSTALLTYPE
{
    INSTALLTYPE_DEFAULT = 0,
    INSTALLTYPE_NETWORK_IMAGE = 1,
    INSTALLTYPE_SINGLE_INSTANCE = 2,
} INSTALLTYPE;

typedef enum tagMSIINSTALLCONTEXT
{
    MSIINSTALLCONTEXT_FIRSTVISIBLE  =   0,
    MSIINSTALLCONTEXT_NONE          =   0,
    MSIINSTALLCONTEXT_USERMANAGED   =   1,
    MSIINSTALLCONTEXT_USERUNMANAGED =   2,
    MSIINSTALLCONTEXT_MACHINE       =   4,
    MSIINSTALLCONTEXT_ALL           = (MSIINSTALLCONTEXT_USERMANAGED | MSIINSTALLCONTEXT_USERUNMANAGED | MSIINSTALLCONTEXT_MACHINE),
    MSIINSTALLCONTEXT_ALLUSERMANAGED=   8,
} MSIINSTALLCONTEXT;

typedef enum tagMSISOURCETYPE
{
    MSISOURCETYPE_UNKNOWN = 0x00000000L,
    MSISOURCETYPE_NETWORK = 0x00000001L,
    MSISOURCETYPE_URL     = 0x00000002L,
    MSISOURCETYPE_MEDIA   = 0x00000004L
} MSISOURCETYPE;

typedef enum tagMSICODE
{
    MSICODE_PRODUCT = 0x00000000L,
    MSICODE_PATCH   = 0x40000000L
} MSICODE;

typedef enum tagINSTALLFEATUREATTRIBUTE
{
    INSTALLFEATUREATTRIBUTE_FAVORLOCAL             = 1 << 0,
    INSTALLFEATUREATTRIBUTE_FAVORSOURCE            = 1 << 1,
    INSTALLFEATUREATTRIBUTE_FOLLOWPARENT           = 1 << 2,
    INSTALLFEATUREATTRIBUTE_FAVORADVERTISE         = 1 << 3,
    INSTALLFEATUREATTRIBUTE_DISALLOWADVERTISE      = 1 << 4,
    INSTALLFEATUREATTRIBUTE_NOUNSUPPORTEDADVERTISE = 1 << 5
} INSTALLFEATUREATTRIBUTE;

typedef struct _MSIFILEHASHINFO {
    ULONG dwFileHashInfoSize;
    ULONG dwData[4];
} MSIFILEHASHINFO, *PMSIFILEHASHINFO;

typedef enum tagMSIPATCHDATATYPE
{
    MSIPATCH_DATATYPE_PATCHFILE = 0,
    MSIPATCH_DATATYPE_XMLPATH = 1,
    MSIPATCH_DATATYPE_XMLBLOB = 2,
} MSIPATCHDATATYPE, *PMSIPATCHDATATYPE;

typedef struct tagMSIPATCHSEQUENCEINFOA
{
    LPCSTR szPatchData;
    MSIPATCHDATATYPE ePatchDataType;
    DWORD dwOrder;
    UINT uStatus;
} MSIPATCHSEQUENCEINFOA, *PMSIPATCHSEQUENCEINFOA;

typedef struct tagMSIPATCHSEQUENCEINFOW
{
    LPCWSTR szPatchData;
    MSIPATCHDATATYPE ePatchDataType;
    DWORD dwOrder;
    UINT uStatus;
} MSIPATCHSEQUENCEINFOW, *PMSIPATCHSEQUENCEINFOW;

#define MAX_FEATURE_CHARS 38

#define ERROR_PATCH_TARGET_NOT_FOUND        1642

/* Strings defined in msi.h */
/* Advertised Information */

#define INSTALLPROPERTY_PACKAGENAMEA "PackageName"
static const WCHAR INSTALLPROPERTY_PACKAGENAMEW[] = {'P','a','c','k','a','g','e','N','a','m','e',0};
#define INSTALLPROPERTY_PACKAGENAME WINELIB_NAME_AW(INSTALLPROPERTY_PACKAGENAME)

#define INSTALLPROPERTY_TRANSFORMSA "Transforms"
static const WCHAR INSTALLPROPERTY_TRANSFORMSW[] = {'T','r','a','n','s','f','o','r','m','s',0};
#define INSTALLPROPERTY_TRANSFORMS WINELIB_NAME_AW(INSTALLPROPERTY_TRANSFORMS)

#define INSTALLPROPERTY_LANGUAGEA "Language"
static const WCHAR INSTALLPROPERTY_LANGUAGEW[] = {'L','a','n','g','u','a','g','e',0};
#define INSTALLPROPERTY_LANGUAGE WINELIB_NAME_AW(INSTALLPROPERTY_LANGUAGE)

#define INSTALLPROPERTY_PRODUCTNAMEA "ProductName"
static const WCHAR INSTALLPROPERTY_PRODUCTNAMEW[] = {'P','r','o','d','u','c','t','N','a','m','e',0};
#define INSTALLPROPERTY_PRODUCTNAME WINELIB_NAME_AW(INSTALLPROPERTY_PRODUCTNAME)

#define INSTALLPROPERTY_ASSIGNMENTTYPEA "AssignmentType"
static const WCHAR INSTALLPROPERTY_ASSIGNMENTTYPEW[] = {'A','s','s','i','g','n','m','e','n','t','T','y','p','e',0};
#define INSTALLPROPERTY_ASSIGNMENTTYPE WINELIB_NAME_AW(INSTALLPROPERTY_ASSIGNMENTTYPE)

#define INSTALLPROPERTY_PACKAGECODEA "PackageCode"
static const WCHAR INSTALLPROPERTY_PACKAGECODEW[] = {'P','a','c','k','a','g','e','C','o','d','e',0};
#define INSTALLPROPERTY_PACKAGECODE WINELIB_NAME_AW(INSTALLPROPERTY_PACKAGECODE)

#define INSTALLPROPERTY_VERSIONA "Version"
static const WCHAR INSTALLPROPERTY_VERSIONW[]= {'V','e','r','s','i','o','n',0};
#define INSTALLPROPERTY_VERSION WINELIB_NAME_AW(INSTALLPROPERTY_VERSION)

/* MSI version 1.1 and above */

#define INSTALLPROPERTY_PRODUCTICONA "ProductIcon"
static const WCHAR INSTALLPROPERTY_PRODUCTICONW[]  = {'P','r','o','d','u','c','t','I','c','o','n',0};
#define INSTALLPROPERTY_PRODUCTICON WINELIB_NAME_AW(INSTALLPROPERTY_PRODUCTICON)

/* MSI version 1.5 and above */
#define INSTALLPROPERTY_INSTANCETYPEA "InstanceType"
static const WCHAR INSTALLPROPERTY_INSTANCETYPEW[] = {'I','n','s','t','a','n','c','e','T','y','p','e',0};
#define INSTALLPROPERTY_INSTANCETYPE WINELIB_NAME_AW(INSTALLPROPERTY_INSTANCETYPE)

/* MSI version 3 and above */
#define INSTALLPROPERTY_AUTHORIZED_LUA_APPA "AuthorizedLUAApp"
static const WCHAR INSTALLPROPERTY_AUTHORIZED_LUA_APPW[] = {'A','u','t','h','o','r','i','z','e','d','L','U','A','A','p','p',0};
#define INSTALLPROPERTY_AUTHORIZED_LUA_APP WINELIB_NAME_AW(INSTALLPROPERTY_AUTHORIZED_LUA_APP)


/* Installed Information */
#define INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA "InstalledProductName"
static const WCHAR INSTALLPROPERTY_INSTALLEDPRODUCTNAMEW[] = {'I','n','s','t','a','l','l','e','d','P','r','o','d','u','c','t','N','a','m','e',0};
#define INSTALLPROPERTY_INSTALLEDPRODUCTNAME WINELIB_NAME_AW(INSTALLPROPERTY_INSTALLEDPRODUCTNAME)

#define INSTALLPROPERTY_VERSIONSTRINGA "VersionString"
static const WCHAR INSTALLPROPERTY_VERSIONSTRINGW[] = {'V','e','r','s','i','o','n','S','t','r','i','n','g',0};
#define INSTALLPROPERTY_VERSIONSTRING WINELIB_NAME_AW(INSTALLPROPERTY_VERSIONSTRING)

#define INSTALLPROPERTY_HELPLINKA "HelpLink"
static const WCHAR INSTALLPROPERTY_HELPLINKW[] = {'H','e','l','p','L','i','n','k',0};
#define INSTALLPROPERTY_HELPLINK WINELIB_NAME_AW(INSTALLPROPERTY_HELPLINK)

#define INSTALLPROPERTY_HELPTELEPHONEA "HelpTelephone"
static const WCHAR INSTALLPROPERTY_HELPTELEPHONEW[] = {'H','e','l','p','T','e','l','e','p','h','o','n','e',0};
#define INSTALLPROPERTY_HELPTELEPHONE WINELIB_NAME_AW(INSTALLPROPERTY_HELPTELEPHONE)

#define INSTALLPROPERTY_INSTALLLOCATIONA "InstallLocation"
static const WCHAR INSTALLPROPERTY_INSTALLLOCATIONW[] = {'I','n','s','t','a','l','l','L','o','c','a','t','i','o','n',0};
#define INSTALLPROPERTY_INSTALLLOCATION WINELIB_NAME_AW(INSTALLPROPERTY_INSTALLLOCATION)

#define INSTALLPROPERTY_INSTALLSOURCEA "InstallSource"
static const WCHAR INSTALLPROPERTY_INSTALLSOURCEW[] = {'I','n','s','t','a','l','l','S','o','u','r','c','e',0};
#define INSTALLPROPERTY_INSTALLSOURCE WINELIB_NAME_AW(INSTALLPROPERTY_INSTALLSOURCE)

#define INSTALLPROPERTY_INSTALLDATEA "InstallDate"
static const WCHAR INSTALLPROPERTY_INSTALLDATEW[] = {'I','n','s','t','a','l','l','D','a','t','e',0};
#define INSTALLPROPERTY_INSTALLDATE WINELIB_NAME_AW(INSTALLPROPERTY_INSTALLDATE)

#define INSTALLPROPERTY_PUBLISHERA "Publisher"
static const WCHAR INSTALLPROPERTY_PUBLISHERW[] ={'P','u','b','l','i','s','h','e','r',0};
#define INSTALLPROPERTY_PUBLISHER WINELIB_NAME_AW(INSTALLPROPERTY_PUBLISHER)

#define INSTALLPROPERTY_LOCALPACKAGEA "LocalPackage"
static const WCHAR INSTALLPROPERTY_LOCALPACKAGEW[] = {'L','o','c','a','l','P','a','c','k','a','g','e',0};
#define INSTALLPROPERTY_LOCALPACKAGE WINELIB_NAME_AW(INSTALLPROPERTY_LOCALPACKAGE)

#define INSTALLPROPERTY_URLINFOABOUTA "URLInfoAbout"
static const WCHAR INSTALLPROPERTY_URLINFOABOUTW[] = {'U','R','L','I','n','f','o','A','b','o','u','t',0};
#define INSTALLPROPERTY_URLINFOABOUT WINELIB_NAME_AW(INSTALLPROPERTY_URLINFOABOUT)

#define INSTALLPROPERTY_URLUPDATEINFOA "URLUpdateInfo"
static const WCHAR INSTALLPROPERTY_URLUPDATEINFOW[] = {'U','R','L','U','p','d','a','t','e','I','n','f','o',0};
#define INSTALLPROPERTY_URLUPDATEINFO WINELIB_NAME_AW(INSTALLPROPERTY_URLUPDATEINFO)

#define INSTALLPROPERTY_VERSIONMINORA "VersionMinor"
static const WCHAR INSTALLPROPERTY_VERSIONMINORW[] = {'V','e','r','s','i','o','n','M','i','n','o','r',0};
#define INSTALLPROPERTY_VERSIONMINOR WINELIB_NAME_AW(INSTALLPROPERTY_VERSIONMINOR)

#define INSTALLPROPERTY_VERSIONMAJORA "VersionMajor"
static const WCHAR INSTALLPROPERTY_VERSIONMAJORW[] = {'V','e','r','s','i','o','n','M','a','j','o','r',0};
#define INSTALLPROPERTY_VERSIONMAJOR WINELIB_NAME_AW(INSTALLPROPERTY_VERSIONMAJOR)

#define INSTALLPROPERTY_PRODUCTIDA "ProductID"
static const WCHAR INSTALLPROPERTY_PRODUCTIDW[] = {'P','r','o','d','u','c','t','I','D',0};
#define INSTALLPROPERTY_PRODUCTID WINELIB_NAME_AW(INSTALLPROPERTY_PRODUCTID)

#define INSTALLPROPERTY_REGCOMPANYA "RegCompany"
static const WCHAR INSTALLPROPERTY_REGCOMPANYW[] = {'R','e','g','C','o','m','p','a','n','y',0};
#define INSTALLPROPERTY_REGCOMPANY WINELIB_NAME_AW(INSTALLPROPERTY_REGCOMPANY)

#define INSTALLPROPERTY_REGOWNERA "RegOwner"
static const WCHAR INSTALLPROPERTY_REGOWNERW[] = {'R','e','g','O','w','n','e','r',0};
#define INSTALLPROPERTY_REGOWNER WINELIB_NAME_AW(INSTALLPROPERTY_REGOWNER)

/* MSI Version 3.0 and greater */
#define INSTALLPROPERTY_UNINSTALLABLEA "Uninstallable"
static const WCHAR INSTALLPROPERTY_UNINSTALLABLEW[] = {'U','n','i','n','s','t','a','l','l','a','b','l','e',0};
#define INSTALLPROPERTY_UNINSTALLABLE WINELIB_NAME_AW(INSTALLPROPERTY_UNINSTALLABLE)

#define INSTALLPROPERTY_PRODUCTSTATEA "State"
static const WCHAR INSTALLPROPERTY_PRODUCTSTATEW[] = {'S','t','a','t','e',0};
#define INSTALLPROPERTY_PRODUCTSTATE WINELIB_NAME_AW(INSTALLPROPERTY_PRODUCTSTATE)

#define INSTALLPROPERTY_PATCHSTATEA "State"
static const WCHAR INSTALLPROPERTY_PATCHSTATEW[] ={'S','t','a','t','e',0};
#define INSTALLPROPERTY_PATCHSTATE WINELIB_NAME_AW(INSTALLPROPERTY_PATCHSTATE)

#define INSTALLPROPERTY_PATCHTYPEA "PatchType"
static const WCHAR INSTALLPROPERTY_PATCHTYPEW[] = {'P','a','t','c','h','T','y','p','e',0};
#define INSTALLPROPERTY_PATCHTYPE WINELIB_NAME_AW(INSTALLPROPERTY_PATCHTYPE)

#define INSTALLPROPERTY_LUAENABLEDA "LUAEnabled"
static const WCHAR INSTALLPROPERTY_LUAENABLEDW[] = {'L','U','A','E','n','a','b','l','e','d',0};
#define INSTALLPROPERTY_LUAENABLED WINELIB_NAME_AW(INSTALLPROPERTY_LUAENABLED)

#define INSTALLPROPERTY_DISPLAYNAMEA "DisplayName"
static const WCHAR INSTALLPROPERTY_DISPLAYNAMEW[] = {'D','i','s','p','l','a','y','N','a','m','e',0};
#define INSTALLPROPERTY_DISPLAYNAME WINELIB_NAME_AW(INSTALLPROPERTY_DISPLAYNAME)

#define INSTALLPROPERTY_MOREINFOURLA "MoreInfoURL"
static const WCHAR INSTALLPROPERTY_MOREINFOURLW[] = {'M','o','r','e','I','n','f','o','U','R','L',0};
#define INSTALLPROPERTY_MOREINFOURL WINELIB_NAME_AW(INSTALLPROPERTY_MOREINFOURL)

/* Source List Info */
#define INSTALLPROPERTY_LASTUSEDSOURCEA "LastUsedSource"
static const WCHAR INSTALLPROPERTY_LASTUSEDSOURCEW[] = {'L','a','s','t','U','s','e','d','S','o','u','r','c','e',0};
#define INSTALLPROPERTY_LASTUSEDSOURCE WINELIB_NAME_AW(INSTALLPROPERTY_LASTUSEDSOURCE)

#define INSTALLPROPERTY_LASTUSEDTYPEA "LastUsedType"
static const WCHAR INSTALLPROPERTY_LASTUSEDTYPEW[] = {'L','a','s','t','U','s','e','d','T','y','p','e',0};
#define INSTALLPROPERTY_LASTUSEDTYPE WINELIB_NAME_AW(INSTALLPROPERTY_LASTUSEDTYPE)

#define INSTALLPROPERTY_MEDIAPACKAGEPATHA "MediaPackagePath"
static const WCHAR INSTALLPROPERTY_MEDIAPACKAGEPATHW[] = {'M','e','d','i','a','P','a','c','k','a','g','e','P','a','t','h',0};
#define INSTALLPROPERTY_MEDIAPACKAGEPATH WINELIB_NAME_AW(INSTALLPROPERTY_MEDIAPACKAGEPATH)

#define INSTALLPROPERTY_DISKPROMPTA "DiskPrompt"
static const WCHAR INSTALLPROPERTY_DISKPROMPTW[] = {'D','i','s','k','P','r','o','m','p','t',0};
#define INSTALLPROPERTY_DISKPROMPT WINELIB_NAME_AW(INSTALLPROPERTY_DISKPROMPT)

typedef INT (CALLBACK *INSTALLUI_HANDLERA)(LPVOID, UINT, LPCSTR);
typedef INT (CALLBACK *INSTALLUI_HANDLERW)(LPVOID, UINT, LPCWSTR);
typedef INT (CALLBACK *INSTALLUI_HANDLER_RECORD)(LPVOID, UINT, MSIHANDLE);
typedef INSTALLUI_HANDLER_RECORD* PINSTALLUI_HANDLER_RECORD;

UINT
WINAPI
MsiAdvertiseProductA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ LANGID);

UINT
WINAPI
MsiAdvertiseProductW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ LANGID);

#define     MsiAdvertiseProduct WINELIB_NAME_AW(MsiAdvertiseProduct)

UINT
WINAPI
MsiAdvertiseProductExA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ LANGID,
  _In_ DWORD,
  _In_ DWORD);

UINT
WINAPI
MsiAdvertiseProductExW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ LANGID,
  _In_ DWORD,
  _In_ DWORD);

#define     MsiAdvertiseProductEx WINELIB_NAME_AW(MsiAdvertiseProductEx)

UINT WINAPI MsiInstallProductA(_In_ LPCSTR, _In_opt_ LPCSTR);
UINT WINAPI MsiInstallProductW(_In_ LPCWSTR, _In_opt_ LPCWSTR);
#define     MsiInstallProduct WINELIB_NAME_AW(MsiInstallProduct)

UINT WINAPI MsiReinstallProductA(_In_ LPCSTR, _In_ DWORD);
UINT WINAPI MsiReinstallProductW(_In_ LPCWSTR, _In_ DWORD);
#define     MsiReinstallProduct WINELIB_NAME_AW(MsiReinstallProduct)

UINT
WINAPI
MsiApplyPatchA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ INSTALLTYPE,
  _In_opt_ LPCSTR);

UINT
WINAPI
MsiApplyPatchW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ INSTALLTYPE,
  _In_opt_ LPCWSTR);

#define     MsiApplyPatch WINELIB_NAME_AW(MsiApplyPatch)

UINT WINAPI MsiEnumComponentCostsA(MSIHANDLE, LPCSTR, DWORD, INSTALLSTATE, LPSTR, LPDWORD, LPINT, LPINT);
UINT WINAPI MsiEnumComponentCostsW(MSIHANDLE, LPCWSTR, DWORD, INSTALLSTATE, LPWSTR, LPDWORD, LPINT, LPINT);
#define     MsiEnumComponentCosts WINELIB_NAME_AW(MsiEnumComponentCosts)

UINT
WINAPI
MsiEnumProductsA(
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPSTR);

UINT
WINAPI
MsiEnumProductsW(
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPWSTR);

#define     MsiEnumProducts WINELIB_NAME_AW(MsiEnumProducts)

UINT
WINAPI
MsiEnumProductsExA(
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) CHAR[39],
  _Out_opt_ MSIINSTALLCONTEXT*,
  _Out_writes_opt_(*pcchSid) LPSTR,
  _Inout_opt_ LPDWORD pcchSid);

UINT
WINAPI
MsiEnumProductsExW(
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) WCHAR[39],
  _Out_opt_ MSIINSTALLCONTEXT*,
  _Out_writes_opt_(*pcchSid) LPWSTR,
  _Inout_opt_ LPDWORD pcchSid);

#define     MsiEnumProductsEx WINELIB_NAME_AW(MsiEnumProductsEx)

UINT
WINAPI
MsiEnumFeaturesA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Out_writes_(MAX_FEATURE_CHARS + 1) LPSTR,
  _Out_writes_opt_(MAX_FEATURE_CHARS + 1) LPSTR);

UINT
WINAPI
MsiEnumFeaturesW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Out_writes_(MAX_FEATURE_CHARS + 1) LPWSTR,
  _Out_writes_opt_(MAX_FEATURE_CHARS + 1) LPWSTR);

#define     MsiEnumFeatures WINELIB_NAME_AW(MsiEnumFeatures)

UINT
WINAPI
MsiEnumComponentsA(
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPSTR);

UINT
WINAPI
MsiEnumComponentsW(
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPWSTR);

#define     MsiEnumComponents WINELIB_NAME_AW(MsiEnumComponents)

UINT
WINAPI
MsiEnumComponentsExA(
  _In_opt_ LPCSTR,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) CHAR[39],
  _Out_opt_ MSIINSTALLCONTEXT *,
  _Out_writes_opt_(*pcchSid) LPSTR,
  _Inout_opt_ LPDWORD pcchSid);

UINT
WINAPI
MsiEnumComponentsExW(
  _In_opt_ LPCWSTR,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) WCHAR[39],
  _Out_opt_ MSIINSTALLCONTEXT *,
  _Out_writes_opt_(*pcchSid) LPWSTR,
  _Inout_opt_ LPDWORD pcchSid);

#define     MsiEnumComponentsEx WINELIB_NAME_AW(MsiEnumComponentsEx)

UINT
WINAPI
MsiEnumClientsA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPSTR);

UINT
WINAPI
MsiEnumClientsW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPWSTR);

#define     MsiEnumClients WINELIB_NAME_AW(MsiEnumClients)

UINT WINAPI MsiEnumClientsExA(_In_ LPCSTR szComponent, _In_opt_ LPCSTR szUserSid, _In_ DWORD dwContext, _In_ DWORD dwProductIndex, _Out_opt_ CHAR szProductBuf[39], _Out_opt_ MSIINSTALLCONTEXT* pdwInstalledContext, _Out_opt_ LPSTR szSid, _Inout_opt_ LPDWORD pcchSid);
UINT WINAPI MsiEnumClientsExW(_In_ LPCWSTR szComponent, _In_opt_ LPCWSTR szUserSid, _In_ DWORD dwContext, _In_ DWORD dwProductIndex, _Out_opt_ WCHAR szProductBuf[39], _Out_opt_ MSIINSTALLCONTEXT* pdwInstalledContext, _Out_opt_ LPWSTR szSid, _Inout_opt_ LPDWORD pcchSid);
#define     MsiEnumClientsEx WINELIB_NAME_AW(MsiEnumClientsEx)

UINT WINAPI MsiOpenPackageA(_In_ LPCSTR, _Out_ MSIHANDLE*);
UINT WINAPI MsiOpenPackageW(_In_ LPCWSTR, _Out_ MSIHANDLE*);
#define     MsiOpenPackage WINELIB_NAME_AW(MsiOpenPackage)

UINT WINAPI MsiOpenPackageExA(_In_ LPCSTR, _In_ DWORD, _Out_ MSIHANDLE*);
UINT WINAPI MsiOpenPackageExW(_In_ LPCWSTR, _In_ DWORD, _Out_ MSIHANDLE*);
#define     MsiOpenPackageEx WINELIB_NAME_AW(MsiOpenPackageEx)

UINT WINAPI MsiOpenProductA(_In_ LPCSTR, _Out_ MSIHANDLE*);
UINT WINAPI MsiOpenProductW(_In_ LPCWSTR, _Out_ MSIHANDLE*);
#define     MsiOpenProduct WINELIB_NAME_AW(MsiOpenProduct)

UINT
WINAPI
MsiGetProductPropertyA(
  _In_ MSIHANDLE,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchValueBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchValueBuf);

UINT
WINAPI
MsiGetProductPropertyW(
  _In_ MSIHANDLE,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchValueBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchValueBuf);

#define     MsiGetProductProperty WINELIB_NAME_AW(MsiGetProductProperty)

UINT WINAPI MsiVerifyPackageA(_In_ LPCSTR);
UINT WINAPI MsiVerifyPackageW(_In_ LPCWSTR);
#define     MsiVerifyPackage WINELIB_NAME_AW(MsiVerifyPackage)

UINT
WINAPI
MsiQueryComponentStateA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCSTR,
  _Out_opt_ INSTALLSTATE*);

UINT
WINAPI
MsiQueryComponentStateW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCWSTR,
  _Out_opt_ INSTALLSTATE*);

#define     MsiQueryComponentState WINELIB_NAME_AW(MsiQueryComponentState)

INSTALLSTATE WINAPI MsiQueryProductStateA(_In_ LPCSTR);
INSTALLSTATE WINAPI MsiQueryProductStateW(_In_ LPCWSTR);
#define      MsiQueryProductState WINELIB_NAME_AW(MsiQueryProductState)

UINT WINAPI MsiConfigureProductA(_In_ LPCSTR, _In_ int, _In_ INSTALLSTATE);
UINT WINAPI MsiConfigureProductW(_In_ LPCWSTR, _In_ int, _In_ INSTALLSTATE);
#define     MsiConfigureProduct WINELIB_NAME_AW(MsiConfigureProduct)

UINT
WINAPI
MsiConfigureProductExA(
  _In_ LPCSTR,
  _In_ int,
  _In_ INSTALLSTATE,
  _In_opt_ LPCSTR);

UINT
WINAPI
MsiConfigureProductExW(
  _In_ LPCWSTR,
  _In_ int,
  _In_ INSTALLSTATE,
  _In_opt_ LPCWSTR);

#define     MsiConfigureProductEx WINELIB_NAME_AW(MsiConfigureProductEx)

UINT
WINAPI
MsiConfigureFeatureA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ INSTALLSTATE);

UINT
WINAPI
MsiConfigureFeatureW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ INSTALLSTATE);

#define     MsiConfigureFeature WINELIB_NAME_AW(MsiConfigureFeature)

UINT
WINAPI
MsiGetProductCodeA(
  _In_ LPCSTR,
  _Out_writes_(MAX_GUID_CHARS + 1) LPSTR);

UINT
WINAPI
MsiGetProductCodeW(
  _In_ LPCWSTR,
  _Out_writes_(MAX_GUID_CHARS + 1) LPWSTR);

#define     MsiGetProductCode WINELIB_NAME_AW(MsiGetProductCode)

UINT
WINAPI
MsiGetProductInfoA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchValueBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchValueBuf);

UINT
WINAPI
MsiGetProductInfoW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchValueBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchValueBuf);

#define     MsiGetProductInfo WINELIB_NAME_AW(MsiGetProductInfo)

UINT
WINAPI
MsiGetProductInfoExA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchValue) LPSTR,
  _Inout_opt_ LPDWORD pcchValue);

UINT
WINAPI
MsiGetProductInfoExW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchValue) LPWSTR,
  _Inout_opt_ LPDWORD pcchValue);

#define     MsiGetProductInfoEx WINELIB_NAME_AW(MsiGetProductInfoEx)

UINT
WINAPI
MsiGetPatchInfoExA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchValue) LPSTR,
  _Inout_opt_ LPDWORD pcchValue);

UINT
WINAPI
MsiGetPatchInfoExW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchValue) LPWSTR,
  _Inout_opt_ LPDWORD pcchValue);

#define     MsiGetPatchInfoEx WINELIB_NAME_AW(MsiGetPatchInfoEx)

UINT
WINAPI
MsiGetPatchInfoA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchValueBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchValueBuf);

UINT
WINAPI
MsiGetPatchInfoW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchValueBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchValueBuf);

#define     MsiGetPatchInfo WINELIB_NAME_AW(MsiGetPatchInfo)

UINT WINAPI MsiEnableLogA(_In_ DWORD, _In_opt_ LPCSTR, _In_ DWORD);
UINT WINAPI MsiEnableLogW(_In_ DWORD, _In_opt_ LPCWSTR, _In_ DWORD);
#define     MsiEnableLog WINELIB_NAME_AW(MsiEnableLog)

INSTALLUI_HANDLERA
WINAPI
MsiSetExternalUIA(
  _In_opt_ INSTALLUI_HANDLERA,
  _In_ DWORD,
  _In_opt_ LPVOID);

INSTALLUI_HANDLERW
WINAPI
MsiSetExternalUIW(
  _In_opt_ INSTALLUI_HANDLERW,
  _In_ DWORD,
  _In_opt_ LPVOID);

#define MsiSetExternalUI WINELIB_NAME_AW(MsiSetExternalUI)

INSTALLSTATE
WINAPI
MsiGetComponentPathA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchBuf);

INSTALLSTATE
WINAPI
MsiGetComponentPathW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchBuf);

#define MsiGetComponentPath WINELIB_NAME_AW(MsiGetComponentPath)

INSTALLSTATE WINAPI MsiGetComponentPathExA(LPCSTR, LPCSTR, LPCSTR, MSIINSTALLCONTEXT, LPSTR, LPDWORD);
INSTALLSTATE WINAPI MsiGetComponentPathExW(LPCWSTR, LPCWSTR, LPCWSTR, MSIINSTALLCONTEXT, LPWSTR, LPDWORD);
#define MsiGetComponentPathEx WINELIB_NAME_AW(MsiGetComponentPathEx)

INSTALLSTATE WINAPI MsiQueryFeatureStateA(_In_ LPCSTR, _In_ LPCSTR);
INSTALLSTATE WINAPI MsiQueryFeatureStateW(_In_ LPCWSTR, _In_ LPCWSTR);
#define MsiQueryFeatureState WINELIB_NAME_AW(MsiQueryFeatureState)

UINT
WINAPI
MsiQueryFeatureStateExA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCSTR,
  _Out_opt_ INSTALLSTATE*);

UINT
WINAPI
MsiQueryFeatureStateExW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ LPCWSTR,
  _Out_opt_ INSTALLSTATE*);

#define     MsiQueryFeatureStateEx WINELIB_NAME_AW(MsiQueryFeatureStateEx)

UINT
WINAPI
MsiGetFeatureInfoA(
  _In_ MSIHANDLE,
  _In_ LPCSTR,
  _Out_opt_ LPDWORD,
  _Out_writes_opt_(*pcchTitleBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchTitleBuf,
  _Out_writes_opt_(*pcchHelpBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchHelpBuf);

UINT
WINAPI
MsiGetFeatureInfoW(
  _In_ MSIHANDLE,
  _In_ LPCWSTR,
  _Out_opt_ LPDWORD,
  _Out_writes_opt_(*pcchTitleBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchTitleBuf,
  _Out_writes_opt_(*pcchHelpBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchHelpBuf);

#define MsiGetFeatureInfo WINELIB_NAME_AW(MsiGetFeatureInfo)

UINT
WINAPI
MsiGetFeatureUsageA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPWORD);

UINT
WINAPI
MsiGetFeatureUsageW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _Out_opt_ LPDWORD,
  _Out_opt_ LPWORD);

#define MsiGetFeatureUsage WINELIB_NAME_AW(MsiGetFeatureUsage)

UINT
WINAPI
MsiEnumRelatedProductsA(
  _In_ LPCSTR,
  _Reserved_ DWORD,
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPSTR);

UINT
WINAPI
MsiEnumRelatedProductsW(
  _In_ LPCWSTR,
  _Reserved_ DWORD,
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPWSTR);

#define MsiEnumRelatedProducts WINELIB_NAME_AW(MsiEnumRelatedProducts)

UINT
WINAPI
MsiProvideAssemblyA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(*pcchPathBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchPathBuf);

UINT
WINAPI
MsiProvideAssemblyW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(*pcchPathBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchPathBuf);

#define MsiProvideAssembly WINELIB_NAME_AW(MsiProvideAssembly)

UINT
WINAPI
MsiEnumComponentQualifiersA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Out_writes_(*pcchQualifierBuf) LPSTR,
  _Inout_ LPDWORD pcchQualifierBuf,
  _Out_writes_opt_(*pcchApplicationDataBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchApplicationDataBuf);

UINT
WINAPI
MsiEnumComponentQualifiersW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Out_writes_(*pcchQualifierBuf) LPWSTR,
  _Inout_ LPDWORD pcchQualifierBuf,
  _Out_writes_opt_(*pcchApplicationDataBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchApplicationDataBuf);

#define MsiEnumComponentQualifiers WINELIB_NAME_AW(MsiEnumComponentQualifiers)

UINT
WINAPI
MsiGetFileVersionA(
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchVersionBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchVersionBuf,
  _Out_writes_opt_(*pcchLangBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchLangBuf);

UINT
WINAPI
MsiGetFileVersionW(
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchVersionBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchVersionBuf,
  _Out_writes_opt_(*pcchLangBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchLangBuf);

#define MsiGetFileVersion WINELIB_NAME_AW(MsiGetFileVersion)

UINT WINAPI MsiMessageBoxA(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);
UINT WINAPI MsiMessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT, WORD, DWORD);
#define MsiMessageBox WINELIB_NAME_AW(MsiMessageBox)

UINT
WINAPI
MsiProvideQualifiedComponentExA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ DWORD,
  _In_opt_ LPCSTR,
  _Reserved_ DWORD,
  _Reserved_ DWORD,
  _Out_writes_opt_(*pcchPathBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchPathBuf);

UINT
WINAPI
MsiProvideQualifiedComponentExW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_opt_ LPCWSTR,
  _Reserved_ DWORD,
  _Reserved_ DWORD,
  _Out_writes_opt_(*pcchPathBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchPathBuf);

#define MsiProvideQualifiedComponentEx WINELIB_NAME_AW(MsiProvideQualifiedComponentEx)

UINT
WINAPI
MsiProvideQualifiedComponentA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ DWORD,
  _Out_writes_opt_(*pcchPathBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchPathBuf);

UINT
WINAPI
MsiProvideQualifiedComponentW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ DWORD,
  _Out_writes_opt_(*pcchPathBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchPathBuf);

#define MsiProvideQualifiedComponent WINELIB_NAME_AW(MsiProvideQualifiedComponent)

USERINFOSTATE
WINAPI
MsiGetUserInfoA(
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchUserNameBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchUserNameBuf,
  _Out_writes_opt_(*pcchOrgNameBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchOrgNameBuf,
  _Out_writes_opt_(*pcchSerialBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchSerialBuf);

USERINFOSTATE
WINAPI
MsiGetUserInfoW(
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchUserNameBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchUserNameBuf,
  _Out_writes_opt_(*pcchOrgNameBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchOrgNameBuf,
  _Out_writes_opt_(*pcchSerialBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchSerialBuf);

#define MsiGetUserInfo WINELIB_NAME_AW(MsiGetUserInfo)

UINT WINAPI MsiProvideComponentA(LPCSTR, LPCSTR, LPCSTR, DWORD, LPSTR, LPDWORD);
UINT WINAPI MsiProvideComponentW(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPWSTR, LPDWORD);
#define MsiProvideComponent WINELIB_NAME_AW(MsiProvideComponent)

UINT WINAPI MsiCollectUserInfoA(_In_ LPCSTR);
UINT WINAPI MsiCollectUserInfoW(_In_ LPCWSTR);
#define MsiCollectUserInfo WINELIB_NAME_AW(MsiCollectUserInfo)

UINT WINAPI MsiReinstallFeatureA(_In_ LPCSTR, _In_ LPCSTR, _In_ DWORD);
UINT WINAPI MsiReinstallFeatureW(_In_ LPCWSTR, _In_ LPCWSTR, _In_ DWORD);
#define MsiReinstallFeature WINELIB_NAME_AW(MsiReinstallFeature)

UINT
WINAPI
MsiGetShortcutTargetA(
  _In_ LPCSTR,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPSTR,
  _Out_writes_opt_(MAX_FEATURE_CHARS + 1) LPSTR,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPSTR);

UINT
WINAPI
MsiGetShortcutTargetW(
  _In_ LPCWSTR,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPWSTR,
  _Out_writes_opt_(MAX_FEATURE_CHARS + 1) LPWSTR,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPWSTR);

#define MsiGetShortcutTarget WINELIB_NAME_AW(MsiGetShortcutTarget)

INSTALLSTATE WINAPI MsiUseFeatureW(_In_ LPCWSTR, _In_ LPCWSTR);
INSTALLSTATE WINAPI MsiUseFeatureA(_In_ LPCSTR, _In_ LPCSTR);
#define MsiUseFeature WINELIB_NAME_AW(MsiUseFeature)

INSTALLSTATE
WINAPI
MsiUseFeatureExW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ DWORD,
  _Reserved_ DWORD);

INSTALLSTATE
WINAPI
MsiUseFeatureExA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ DWORD,
  _Reserved_ DWORD);

#define MsiUseFeatureEx WINELIB_NAME_AW(MsiUseFeatureEx)

HRESULT
WINAPI
MsiGetFileSignatureInformationA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Outptr_ PCCERT_CONTEXT*,
  _Out_writes_bytes_opt_(*pcbHashData) LPBYTE,
  _Inout_opt_ LPDWORD pcbHashData);

HRESULT
WINAPI
MsiGetFileSignatureInformationW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Outptr_ PCCERT_CONTEXT*,
  _Out_writes_bytes_opt_(*pcbHashData) LPBYTE,
  _Inout_opt_ LPDWORD pcbHashData);

#define MsiGetFileSignatureInformation WINELIB_NAME_AW(MsiGetFileSignatureInformation)

INSTALLSTATE
WINAPI
MsiLocateComponentA(
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchBuf) LPSTR,
  _Inout_opt_ LPDWORD pcchBuf);

INSTALLSTATE
WINAPI
MsiLocateComponentW(
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchBuf) LPWSTR,
  _Inout_opt_ LPDWORD pcchBuf);

#define  MsiLocateComponent WINELIB_NAME_AW(MsiLocateComponent)

UINT
WINAPI
MsiSourceListAddSourceA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _Reserved_ DWORD,
  _In_ LPCSTR);

UINT
WINAPI
MsiSourceListAddSourceW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Reserved_ DWORD,
  _In_ LPCWSTR);

#define     MsiSourceListAddSource WINELIB_NAME_AW(MsiSourceListAddSource)

UINT
WINAPI
MsiSourceListEnumMediaDisksA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ DWORD,
  _Out_opt_ LPDWORD,
  _Out_writes_opt_(*pcchVolumeLabel) LPSTR,
  _Inout_opt_ LPDWORD pcchVolumeLabel,
  _Out_writes_opt_(*pcchDiskPrompt) LPSTR,
  _Inout_opt_ LPDWORD pcchDiskPrompt);

UINT
WINAPI
MsiSourceListEnumMediaDisksW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ DWORD,
  _Out_opt_ LPDWORD,
  _Out_writes_opt_(*pcchVolumeLabel) LPWSTR,
  _Inout_opt_ LPDWORD pcchVolumeLabel,
  _Out_writes_opt_(*pcchDiskPrompt) LPWSTR,
  _Inout_opt_ LPDWORD pcchDiskPrompt);

#define     MsiSourceListEnumMediaDisks WINELIB_NAME_AW(MsiSourceListEnumMediaDisks)

UINT
WINAPI
MsiSourceListEnumSourcesA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(*pcchSource) LPSTR,
  _Inout_opt_ LPDWORD pcchSource);

UINT
WINAPI
MsiSourceListEnumSourcesW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(*pcchSource) LPWSTR,
  _Inout_opt_ LPDWORD pcchSource);

#define     MsiSourceListEnumSources WINELIB_NAME_AW(MsiSourceListEnumSources)

UINT
WINAPI
MsiSourceListClearSourceA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCSTR);

UINT
WINAPI
MsiSourceListClearSourceW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCWSTR);

#define     MsiSourceListClearSource WINELIB_NAME_AW(MsiSourceListClearSource)

UINT
WINAPI
MsiSourceListClearAllA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _Reserved_ DWORD);

UINT
WINAPI
MsiSourceListClearAllW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _Reserved_ DWORD);

#define     MsiSourceListClearAll WINELIB_NAME_AW(MsiSourceListClearAll)

UINT
WINAPI
MsiSourceListGetInfoA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCSTR,
  _Out_writes_opt_(*pcchValue) LPSTR,
  _Inout_opt_ LPDWORD pcchValue);

UINT
WINAPI
MsiSourceListGetInfoW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCWSTR,
  _Out_writes_opt_(*pcchValue) LPWSTR,
  _Inout_opt_ LPDWORD pcchValue);

#define     MsiSourceListGetInfo WINELIB_NAME_AW(MsiSourceListGetInfo)

UINT
WINAPI
MsiSourceListSetInfoA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCSTR,
  _In_ LPCSTR);

UINT
WINAPI
MsiSourceListSetInfoW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCWSTR,
  _In_ LPCWSTR);

#define     MsiSourceListSetInfo WINELIB_NAME_AW(MsiSourceListSetInfo)

UINT
WINAPI
MsiSourceListAddSourceExA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCSTR,
  _In_ DWORD);

UINT
WINAPI
MsiSourceListAddSourceExW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ LPCWSTR,
  _In_ DWORD);

#define     MsiSourceListAddSourceEx WINELIB_NAME_AW(MsiSourceListAddSourceEx)

UINT
WINAPI
MsiSourceListAddMediaDiskA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ DWORD,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR);

UINT
WINAPI
MsiSourceListAddMediaDiskW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD,
  _In_ DWORD,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR);

#define     MsiSourceListAddMediaDisk WINELIB_NAME_AW(MsiSourceListAddMediaDisk)

UINT WINAPI MsiSourceListForceResolutionA(const CHAR*, const CHAR*, DWORD);
UINT WINAPI MsiSourceListForceResolutionW(const WCHAR*, const WCHAR*, DWORD);
#define     MsiSourceListForceResolution WINELIB_NAME_AW(MsiSourceListForceResolution)

UINT
WINAPI
MsiEnumPatchesA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPSTR,
  _Out_writes_(*pcchTransformsBuf) LPSTR,
  _Inout_ LPDWORD pcchTransformsBuf);

UINT
WINAPI
MsiEnumPatchesW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Out_writes_(MAX_GUID_CHARS + 1) LPWSTR,
  _Out_writes_(*pcchTransformsBuf) LPWSTR,
  _Inout_ LPDWORD pcchTransformsBuf);

#define     MsiEnumPatches WINELIB_NAME_AW(MsiEnumPatches)

UINT
WINAPI
MsiEnumPatchesExA(
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPSTR,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPSTR,
  _Out_opt_ MSIINSTALLCONTEXT*,
  _Out_writes_opt_(*pcchTargetUserSid) LPSTR,
  _Inout_opt_ LPDWORD pcchTargetUserSid);

UINT
WINAPI
MsiEnumPatchesExW(
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPWSTR,
  _Out_writes_opt_(MAX_GUID_CHARS + 1) LPWSTR,
  _Out_opt_ MSIINSTALLCONTEXT*,
  _Out_writes_opt_(*pcchTargetUserSid) LPWSTR,
  _Inout_opt_ LPDWORD pcchTargetUserSid);

#define     MsiEnumPatchesEx WINELIB_NAME_AW(MsiEnumPatchesEx)

UINT
WINAPI
MsiGetFileHashA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Inout_ PMSIFILEHASHINFO);

UINT
WINAPI
MsiGetFileHashW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Inout_ PMSIFILEHASHINFO);

#define     MsiGetFileHash WINELIB_NAME_AW(MsiGetFileHash)

UINT
WINAPI
MsiAdvertiseScriptA(
  _In_ LPCSTR,
  _In_ DWORD,
  _In_opt_ PHKEY,
  _In_ BOOL);

UINT
WINAPI
MsiAdvertiseScriptW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_opt_ PHKEY,
  _In_ BOOL);

#define     MsiAdvertiseScript WINELIB_NAME_AW(MsiAdvertiseScript)

UINT WINAPI MsiIsProductElevatedA(_In_ LPCSTR, _Out_ BOOL *);
UINT WINAPI MsiIsProductElevatedW(_In_ LPCWSTR, _Out_ BOOL *);
#define     MsiIsProductElevated WINELIB_NAME_AW(MsiIsProductElevated)

UINT WINAPI MsiDatabaseMergeA(MSIHANDLE, MSIHANDLE, LPCSTR);
UINT WINAPI MsiDatabaseMergeW(MSIHANDLE, MSIHANDLE, LPCWSTR);
#define     MsiDatabaseMerge WINELIB_NAME_AW(MsiDatabaseMerge)

UINT
WINAPI
MsiInstallMissingComponentA(
  _In_ LPCSTR,
  _In_ LPCSTR,
  _In_ INSTALLSTATE);

UINT
WINAPI
MsiInstallMissingComponentW(
  _In_ LPCWSTR,
  _In_ LPCWSTR,
  _In_ INSTALLSTATE);

#define     MsiInstallMissingComponent WINELIB_NAME_AW(MsiInstallMissingComponent)

UINT
WINAPI
MsiDetermineApplicablePatchesA(
  _In_ LPCSTR,
  _In_ DWORD cPatchInfo,
  _Inout_updates_(cPatchInfo) PMSIPATCHSEQUENCEINFOA);

UINT
WINAPI
MsiDetermineApplicablePatchesW(
  _In_ LPCWSTR,
  _In_ DWORD cPatchInfo,
  _Inout_updates_(cPatchInfo) PMSIPATCHSEQUENCEINFOW);

#define     MsiDetermineApplicablePatches WINELIB_NAME_AW(MsiDetermineApplicablePatches)

UINT
WINAPI
MsiDeterminePatchSequenceA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD cPatchInfo,
  _Inout_updates_(cPatchInfo) PMSIPATCHSEQUENCEINFOA);

UINT
WINAPI
MsiDeterminePatchSequenceW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ MSIINSTALLCONTEXT,
  _In_ DWORD cPatchInfo,
  _Inout_updates_(cPatchInfo) PMSIPATCHSEQUENCEINFOW);

#define     MsiDeterminePatchSequence WINELIB_NAME_AW(MsiDeterminePatchSequence)

UINT
WINAPI
MsiApplyMultiplePatchesA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR);

UINT
WINAPI
MsiApplyMultiplePatchesW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR);

#define     MsiApplyMultiplePatches WINELIB_NAME_AW(MsiApplyMultiplePatches)

UINT
WINAPI
MsiBeginTransactionA(
  _In_ LPCSTR,
  _In_ DWORD,
  _Out_ MSIHANDLE *,
  _Out_ HANDLE *);

UINT
WINAPI
MsiBeginTransactionW(
  _In_ LPCWSTR,
  _In_ DWORD,
  _Out_ MSIHANDLE *,
  _Out_ HANDLE *);

#define     MsiBeginTransaction WINELIB_NAME_AW(MsiBeginTransaction)

UINT WINAPI MsiEndTransaction(_In_ DWORD);

/* Non Unicode */
UINT WINAPI MsiCloseHandle(MSIHANDLE);
UINT WINAPI MsiCloseAllHandles(void);
INSTALLUILEVEL WINAPI MsiSetInternalUI(_In_ INSTALLUILEVEL, _Inout_opt_ HWND*);

UINT
WINAPI
MsiSetExternalUIRecord(
  _In_opt_ INSTALLUI_HANDLER_RECORD,
  _In_ DWORD,
  _In_opt_ LPVOID,
  _Out_opt_ PINSTALLUI_HANDLER_RECORD);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MSI_H */
