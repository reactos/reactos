/*
 * services.h
 */

#ifndef _SERVICES_H
#define _SERVICES_H

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <winreg.h>
#include <winuser.h>
#include <netevent.h>
#define NTOS_MODE_USER
#include <ndk/setypes.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <services/services.h>
#include <svcctl_s.h>

#include "resource.h"

typedef struct _SERVICE_GROUP
{
    LIST_ENTRY GroupListEntry;
    LPWSTR lpGroupName;

    DWORD dwRefCount;
    BOOLEAN ServicesRunning;
    ULONG TagCount;
    PULONG TagArray;

    WCHAR szGroupName[1];
} SERVICE_GROUP, *PSERVICE_GROUP;


typedef struct _SERVICE_IMAGE
{
    LIST_ENTRY ImageListEntry;
    LPWSTR pszImagePath;
    LPWSTR pszAccountName;
    DWORD dwImageRunCount;

    HANDLE hControlPipe;
    HANDLE hProcess;
    DWORD dwProcessId;
    HANDLE hToken;
    HANDLE hProfile;
} SERVICE_IMAGE, *PSERVICE_IMAGE;


typedef struct _SERVICE
{
    LIST_ENTRY ServiceListEntry;
    LPWSTR lpServiceName;
    LPWSTR lpDisplayName;
    PSERVICE_GROUP lpGroup;
    PSERVICE_IMAGE lpImage;
    BOOL bDeleted;
    DWORD dwResumeCount;
    DWORD dwRefCount;

    SERVICE_STATUS Status;
    DWORD dwStartType;
    DWORD dwErrorControl;
    DWORD dwTag;

    DWORD dwServiceBits;

    ULONG Flags;

    PSECURITY_DESCRIPTOR pSecurityDescriptor;

    BOOLEAN ServiceVisited;

    WCHAR szServiceName[1];
} SERVICE, *PSERVICE;


#define LOCK_TAG 0x4C697041 /* 'ApiL' */

typedef struct _START_LOCK
{
    DWORD Tag;             /* Must be LOCK_TAG */
    DWORD TimeWhenLocked;  /* Number of seconds since 1970 */
    PSID LockOwnerSid;     /* It is NULL if the SCM acquired the lock */
} START_LOCK, *PSTART_LOCK;


/* VARIABLES ***************************************************************/

extern LIST_ENTRY ServiceListHead;
extern LIST_ENTRY GroupListHead;
extern LIST_ENTRY ImageListHead;
extern BOOL ScmInitialize;
extern BOOL ScmShutdown;
extern BOOL ScmLiveSetup;
extern BOOL ScmSetupInProgress;
extern PSECURITY_DESCRIPTOR pPipeSD;


/* FUNCTIONS ***************************************************************/

/* config.c */

DWORD ScmOpenServiceKey(LPWSTR lpServiceName,
                        REGSAM samDesired,
                        PHKEY phKey);

DWORD ScmCreateServiceKey(LPCWSTR lpServiceName,
                          REGSAM samDesired,
                          PHKEY phKey);

DWORD ScmWriteDependencies(HKEY hServiceKey,
                           LPCWSTR lpDependencies,
                           DWORD dwDependenciesLength);

DWORD ScmMarkServiceForDelete(PSERVICE pService);
BOOL ScmIsDeleteFlagSet(HKEY hServiceKey);

DWORD ScmReadString(HKEY hServiceKey,
                    LPCWSTR lpValueName,
                    LPWSTR *lpValue);

DWORD
ScmReadDependencies(HKEY hServiceKey,
                    LPWSTR *lpDependencies,
                    DWORD *lpdwDependenciesLength);

DWORD
ScmSetServicePassword(
    IN PCWSTR pszServiceName,
    IN PCWSTR pszPassword);

DWORD
ScmWriteSecurityDescriptor(
    _In_ HKEY hServiceKey,
    _In_ PSECURITY_DESCRIPTOR pSecurityDescriptor);

DWORD
ScmReadSecurityDescriptor(
    _In_ HKEY hServiceKey,
    _Out_ PSECURITY_DESCRIPTOR *ppSecurityDescriptor);

DWORD
ScmDeleteRegKey(
    _In_ HKEY hKey,
    _In_ PCWSTR pszSubKey);

DWORD
ScmDecryptPassword(
    _In_ PVOID ContextHandle,
    _In_ PBYTE pPassword,
    _In_ DWORD dwPasswordSize,
    _Out_ PWSTR *pDecryptedPassword);


/* controlset.c */

DWORD
ScmCreateLastKnownGoodControlSet(VOID);

DWORD
ScmAcceptBoot(VOID);

DWORD
ScmRunLastKnownGood(VOID);


/* database.c */

DWORD ScmCreateServiceDatabase(VOID);
VOID ScmShutdownServiceDatabase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);
VOID ScmAutoShutdownServices(VOID);
DWORD ScmStartService(PSERVICE Service,
                      DWORD argc,
                      LPWSTR *argv);

VOID ScmRemoveServiceImage(PSERVICE_IMAGE pServiceImage);
PSERVICE ScmGetServiceEntryByName(LPCWSTR lpServiceName);
PSERVICE ScmGetServiceEntryByDisplayName(LPCWSTR lpDisplayName);
PSERVICE ScmGetServiceEntryByResumeCount(DWORD dwResumeCount);
DWORD ScmCreateNewServiceRecord(LPCWSTR lpServiceName,
                                PSERVICE *lpServiceRecord,
                                DWORD dwServiceType,
                                DWORD dwStartType);
VOID ScmDeleteServiceRecord(PSERVICE lpService);
DWORD ScmMarkServiceForDelete(PSERVICE pService);

DWORD ScmControlService(HANDLE hControlPipe,
                        PWSTR pServiceName,
                        SERVICE_STATUS_HANDLE hServiceStatus,
                        DWORD dwControl);

BOOL ScmLockDatabaseExclusive(VOID);
BOOL ScmLockDatabaseShared(VOID);
VOID ScmUnlockDatabase(VOID);

VOID ScmInitNamedPipeCriticalSection(VOID);
VOID ScmDeleteNamedPipeCriticalSection(VOID);

DWORD ScmGetServiceNameFromTag(PTAG_INFO_NAME_FROM_TAG_IN_PARAMS InParams,
                               PTAG_INFO_NAME_FROM_TAG_OUT_PARAMS *OutParams);

DWORD ScmGenerateServiceTag(PSERVICE lpServiceRecord);

/* driver.c */

DWORD ScmStartDriver(PSERVICE lpService);
DWORD ScmControlDriver(PSERVICE lpService,
                       DWORD dwControl,
                       LPSERVICE_STATUS lpServiceStatus);


/* groupdb.c */

PSERVICE_GROUP
ScmGetServiceGroupByName(
    _In_ LPCWSTR lpGroupName);

DWORD ScmCreateGroupList(VOID);
DWORD ScmSetServiceGroup(PSERVICE lpService,
                         LPCWSTR lpGroupName);


/* lock.c */

DWORD ScmAcquireServiceStartLock(IN BOOL IsServiceController,
                                 OUT LPSC_RPC_LOCK lpLock);
DWORD ScmReleaseServiceStartLock(IN OUT LPSC_RPC_LOCK lpLock);
VOID ScmQueryServiceLockStatusW(OUT LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus);
VOID ScmQueryServiceLockStatusA(OUT LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus);


/* rpcserver.c */

VOID ScmStartRpcServer(VOID);


/* security.c */

DWORD ScmInitializeSecurity(VOID);
VOID ScmShutdownSecurity(VOID);

DWORD
ScmCreateDefaultServiceSD(
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor);


/* services.c */

VOID PrintString(LPCSTR fmt, ...);
DWORD SetSecurityServicesEvent(VOID);
VOID ScmLogEvent(DWORD dwEventId,
                 WORD wType,
                 WORD wStrings,
                 LPCWSTR *lpStrings);
VOID ScmWaitForLsa(VOID);

#endif /* _SERVICES_H */
