/*
 * PROJECT:     ReactOS Headers
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Group Policy and shell
 * COPYRIGHT:   Copyright 2005 Mike McCormack
 *              Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifndef _APPMGMT_H
#define _APPMGMT_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef enum _INSTALLSPECTYPE
{
    APPNAME,
    FILEEXT,
    PROGID,
    COMCLASS
} INSTALLSPECTYPE;

typedef union _INSTALLSPEC
{
    struct {
        WCHAR *Name;
        GUID  GPOId;
    } AppName;
    WCHAR *FileExt;
    WCHAR *ProgId;
    struct {
        GUID  Clsid;
        DWORD ClsCtx;
    } COMClass;
} INSTALLSPEC;

typedef struct _INSTALLDATA
{
    INSTALLSPECTYPE Type;
    INSTALLSPEC Spec;
} INSTALLDATA, *PINSTALLDATA;

typedef struct _APPCATEGORYINFO
{
    LCID Locale;
    LPWSTR pszDescription;
    GUID AppCategoryId;
} APPCATEGORYINFO;

typedef struct _APPCATEGORYINFOLIST
{
    DWORD cCategory;
    APPCATEGORYINFO *pCategoryInfo;
} APPCATEGORYINFOLIST;

typedef struct _LOCALMANAGEDAPPLICATION
{
    LPWSTR pszDeploymentName;
    LPWSTR pszPolicyName;
    LPWSTR pszProductId;
    DWORD dwState;
} LOCALMANAGEDAPPLICATION, *PLOCALMANAGEDAPPLICATION;

/* LOCALMANAGEDAPPLICATION.dwState flags */
#define LOCALSTATE_ASSIGNED                 0x1
#define LOCALSTATE_PUBLISHED                0x2
#define LOCALSTATE_UNINSTALL_UNMANAGED      0x4
#define LOCALSTATE_POLICYREMOVE_ORPHAN      0x8
#define LOCALSTATE_POLICYREMOVE_UNINSTALL   0x10
#define LOCALSTATE_ORPHANED                 0x20
#define LOCALSTATE_UNINSTALLED              0x40

typedef struct _MANAGEDAPPLICATION
{
    LPWSTR pszPackageName;
    LPWSTR pszPublisher;
    DWORD dwVersionHi;
    DWORD dwVersionLo;
    DWORD dwRevision;
    GUID GpoId;
    LPWSTR pszPolicyName;
    GUID ProductId;
    LANGID Language;
    LPWSTR pszOwner;
    LPWSTR pszCompany;
    LPWSTR pszComments;
    LPWSTR pszContact;
    LPWSTR pszSupportUrl;
    DWORD dwPathType;
    BOOL bInstalled;
} MANAGEDAPPLICATION, *PMANAGEDAPPLICATION;

/* MANAGEDAPPLICATION.dwPathType values */
#define MANAGED_APPTYPE_WINDOWSINSTALLER    0x1
#define MANAGED_APPTYPE_SETUPEXE            0x2
#define MANAGED_APPTYPE_UNSUPPORTED         0x3

DWORD WINAPI
GetLocalManagedApplications(
    _In_ BOOL bUserApps,
    _Out_ LPDWORD pdwApps,
    _Out_ PLOCALMANAGEDAPPLICATION *prgLocalApps);

DWORD WINAPI
GetManagedApplicationCategories(
    _Out_ DWORD dwReserved,
    _Out_ APPCATEGORYINFOLIST *pAppCategory);

DWORD WINAPI
GetManagedApplications(
    _In_ GUID *pCategory,
    _In_ DWORD dwQueryFlags,
    _In_ DWORD dwInfoLevel,
    _Out_ LPDWORD pdwApps,
    _Out_ PMANAGEDAPPLICATION *prgManagedApps);

/* GetManagedApplications dwQueryFlags flags */
#define MANAGED_APPS_USERAPPLICATIONS   0x1
#define MANAGED_APPS_FROMCATEGORY       0x2
#define MANAGED_APPS_INFOLEVEL_DEFAULT  0x10000

/* FIXME: SAL2 annotation */
DWORD WINAPI
CommandLineFromMsiDescriptor(
    WCHAR *szDescriptor,
    WCHAR *szCommandLine,
    DWORD *pcchCommandLine);

DWORD WINAPI
InstallApplication(
    _In_ PINSTALLDATA pInstallInfo);

DWORD WINAPI
UninstallApplication(
    _In_ LPWSTR ProductCode,
    _In_ DWORD dwStatus);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* _APPMGMT_H */
