#ifndef _SCHEDSVC_PCH_
#define _SCHEDSVC_PCH_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <winuser.h>
#include <undocuser.h>

#include <ndk/rtlfuncs.h>

#include <atsvc_s.h>

#include <wine/debug.h>

#define JOB_NAME_LENGTH 9

NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
    PULONG Seed);

typedef struct _JOB
{
    LIST_ENTRY JobEntry;

    FILETIME StartTime;
    WCHAR Name[JOB_NAME_LENGTH];

    DWORD JobId;
    DWORD_PTR JobTime;
    DWORD DaysOfMonth;
    UCHAR DaysOfWeek;
    UCHAR Flags;
    WCHAR Command[1];
} JOB, *PJOB;


extern DWORD dwNextJobId;
extern DWORD dwJobCount;

extern LIST_ENTRY JobListHead;
extern RTL_RESOURCE JobListLock;

extern LIST_ENTRY StartListHead;
extern RTL_RESOURCE StartListLock;

extern HANDLE Events[3];


/* job.c */

VOID
GetNextJobTimeout(
    HANDLE hTimer);

VOID
RunCurrentJobs(VOID);

LONG
SaveJob(
    PJOB pJob);

LONG
DeleteJob(
    PJOB pJob);

LONG
LoadJobs(VOID);

VOID
CalculateNextStartTime(
    _In_ PJOB pJob);

VOID
InsertJobIntoStartList(
    _In_ PLIST_ENTRY StartListHead,
    _In_ PJOB pJob);

VOID
DumpStartList(
    _In_ PLIST_ENTRY StartListHead);


/* rpcserver.c */

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter);

#endif /* _SCHEDSVC_PCH_ */
