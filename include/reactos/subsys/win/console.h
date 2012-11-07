/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Client/Server Runtime SubSystem
 * FILE:            include/reactos/subsys/win/console.h
 * PURPOSE:         Public definitions for Console API Clients
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#pragma once

#define ConsoleGetPerProcessData(pcsrprocess)   \
    ((PCONSOLE_PROCESS_DATA)((pcsrprocess)->ServerData[CONSRV_SERVERDLL_INDEX]))


typedef struct _CONSOLE_PROCESS_DATA
{
    LIST_ENTRY ConsoleLink;
    PCSR_PROCESS Process;   // Parent process.
    HANDLE ConsoleEvent;
    /* PCSRSS_CONSOLE */ struct tagCSRSS_CONSOLE* Console;
    /* PCSRSS_CONSOLE */ struct tagCSRSS_CONSOLE* ParentConsole;
    BOOL bInheritHandles;
    RTL_CRITICAL_SECTION HandleTableLock;
    ULONG HandleTableSize;
    /* PCSRSS_HANDLE */ struct _CSRSS_HANDLE* HandleTable; // Is it a length-varying table or length-fixed ??
    LPTHREAD_START_ROUTINE CtrlDispatcher;
} CONSOLE_PROCESS_DATA, *PCONSOLE_PROCESS_DATA;

#endif // _CONSOLE_H

/* EOF */
