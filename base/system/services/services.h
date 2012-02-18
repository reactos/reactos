/*
 * services.h
 */

#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <netevent.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <services/services.h>
#include "svcctl_s.h"


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
    DWORD dwImageRunCount;

    HANDLE hControlPipe;
    HANDLE hProcess;
    DWORD dwProcessId;

    WCHAR szImagePath[1];
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

    ULONG Flags;

    PSECURITY_DESCRIPTOR lpSecurityDescriptor;

    BOOLEAN ServiceVisited;

    WCHAR szServiceName[1];
} SERVICE, *PSERVICE;


/* VARIABLES ***************************************************************/

extern LIST_ENTRY ServiceListHead;
extern LIST_ENTRY GroupListHead;
extern LIST_ENTRY ImageListHead;
extern BOOL ScmShutdown;


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
                    LPWSTR lpValueName,
                    LPWSTR *lpValue);

DWORD
ScmReadDependencies(HKEY hServiceKey,
                    LPWSTR *lpDependencies,
                    DWORD *lpdwDependenciesLength);


/* database.c */

DWORD ScmCreateServiceDatabase(VOID);
VOID ScmShutdownServiceDatabase(VOID);
VOID ScmGetBootAndSystemDriverState(VOID);
VOID ScmAutoStartServices(VOID);
VOID ScmAutoShutdownServices(VOID);
DWORD ScmStartService(PSERVICE Service,
                      DWORD argc,
                      LPWSTR *argv);

PSERVICE ScmGetServiceEntryByName(LPCWSTR lpServiceName);
PSERVICE ScmGetServiceEntryByDisplayName(LPCWSTR lpDisplayName);
PSERVICE ScmGetServiceEntryByResumeCount(DWORD dwResumeCount);
DWORD ScmCreateNewServiceRecord(LPCWSTR lpServiceName,
                                PSERVICE *lpServiceRecord);
VOID ScmDeleteServiceRecord(PSERVICE lpService);
DWORD ScmMarkServiceForDelete(PSERVICE pService);

DWORD ScmControlService(PSERVICE Service,
                        DWORD dwControl);

BOOL ScmLockDatabaseExclusive(VOID);
BOOL ScmLockDatabaseShared(VOID);
VOID ScmUnlockDatabase(VOID);

VOID ScmInitNamedPipeCriticalSection(VOID);
VOID ScmDeleteNamedPipeCriticalSection(VOID);


/* driver.c */

DWORD ScmLoadDriver(PSERVICE lpService);
DWORD ScmUnloadDriver(PSERVICE lpService);
DWORD ScmControlDriver(PSERVICE lpService,
                       DWORD dwControl,
                       LPSERVICE_STATUS lpServiceStatus);


/* groupdb.c */

DWORD ScmCreateGroupList(VOID);
DWORD ScmSetServiceGroup(PSERVICE lpService,
                         LPCWSTR lpGroupName);


/* rpcserver.c */

VOID ScmStartRpcServer(VOID);


/* services.c */

VOID PrintString(LPCSTR fmt, ...);
VOID ScmLogError(DWORD dwEventId,
                 WORD wStrings,
                 LPCWSTR *lpStrings);

/* EOF */

