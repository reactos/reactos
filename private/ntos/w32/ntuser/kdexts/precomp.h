#ifndef USEREXTS
#define USEREXTS

#include <ntos.h>
#include <w32p.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <excpt.h>
#include <atom.h>
#include <stdio.h>
#include <limits.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <string.h>
#include <ntstatus.h>
#include <windows.h>
#include <ntddvdeo.h>
#include <ntcsrsrv.h>

#define PROCESS_PROCESSLINK     0
#define PROCESS_WIN32PROCESS    1
#define PROCESS_IMAGEFILENAME   2
#define PROCESS_THREADLIST      3
#define PROCESS_PRIORITYCLASS   4
#define PROCESS_PROCESSHEAD     5
#define PROCESS_PROCESSID       6

PVOID GetEProcessData_X86(
    PEPROCESS pEProcess,
    UINT iData,
    PVOID pBuffer);

PVOID GetEProcessData_MIPS(
    PEPROCESS pEProcess,
    UINT iData,
    PVOID pBuffer);

PVOID GetEProcessData_ALPHA(
    PEPROCESS pEProcess,
    UINT iData,
    PVOID pBuffer);

PVOID GetEProcessData_PPC(
    PEPROCESS pEProcess,
    UINT iData,
    PVOID pBuffer);

PVOID GetEProcessData_IA64(
    PEPROCESS pEProcess,
    UINT iData,
    PVOID pBuffer);

typedef PVOID (*PGETEPROCESSDATAFUNC)(PEPROCESS, UINT, PVOID);

#endif /* USEREXTS */
