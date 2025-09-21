/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell main header file
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

#ifndef PRECOMP_H
#define PRECOMP_H

/* INCLUDES ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>

#include <errno.h>

#include <conutils.h>
#include <netsh.h>

#include "resource.h"


/* DEFINES *******************************************************************/

#define MAX_STRING_SIZE 1024
#define MAX_ARGS_COUNT 256

#define REG_NETSH_PATH L"Software\\Microsoft\\NetSh"


/* TYPEDEFS ******************************************************************/

typedef struct _DLL_LIST_ENTRY
{
    struct _DLL_LIST_ENTRY *pPrev;
    struct _DLL_LIST_ENTRY *pNext;

    PWSTR pszDllName;
    PWSTR pszShortName;
    PWSTR pszValueName;

    HMODULE hModule;

} DLL_LIST_ENTRY, *PDLL_LIST_ENTRY;

typedef struct _HELPER_ENTRY
{
    struct _HELPER_ENTRY *pPrev;
    struct _HELPER_ENTRY *pNext;

    NS_HELPER_ATTRIBUTES Attributes;
    GUID ParentHelperGuid;

    PDLL_LIST_ENTRY pDllEntry;
    BOOL bStarted;

    struct _HELPER_ENTRY *pSubHelperHead;
    struct _HELPER_ENTRY *pSubHelperTail;

} HELPER_ENTRY, *PHELPER_ENTRY;



typedef struct _COMMAND_ENTRY
{
    struct _COMMAND_ENTRY *pPrev;
    struct _COMMAND_ENTRY *pNext;

    PWSTR pwszCmdToken;
    PFN_HANDLE_CMD pfnCmdHandler;
    DWORD dwShortCmdHelpToken;
    DWORD dwCmdHlpToken;
    DWORD dwFlags;
} COMMAND_ENTRY, *PCOMMAND_ENTRY;

typedef struct _COMMAND_GROUP
{
    struct _COMMAND_GROUP *pPrev;
    struct _COMMAND_GROUP *pNext;

    PWSTR pwszCmdGroupToken;
    DWORD dwShortCmdHelpToken;
    DWORD dwFlags;

    PCOMMAND_ENTRY pCommandListHead;
    PCOMMAND_ENTRY pCommandListTail;
} COMMAND_GROUP, *PCOMMAND_GROUP;

typedef struct _CONTEXT_ENTRY
{
    struct _CONTEXT_ENTRY *pPrev;
    struct _CONTEXT_ENTRY *pNext;

    struct _CONTEXT_ENTRY *pParentContext;
//    PHELPER_ENTRY pHelper;

    PWSTR pszContextName;
    GUID Guid;
    HMODULE hModule;

    PCOMMAND_ENTRY pCommandListHead;
    PCOMMAND_ENTRY pCommandListTail;

    PCOMMAND_GROUP pGroupListHead;
    PCOMMAND_GROUP pGroupListTail;

    struct _CONTEXT_ENTRY *pSubContextHead;
    struct _CONTEXT_ENTRY *pSubContextTail;
} CONTEXT_ENTRY, *PCONTEXT_ENTRY;


/* GLOBAL VARIABLES ***********************************************************/

extern PCONTEXT_ENTRY pRootContext;
extern PCONTEXT_ENTRY pCurrentContext;

extern PHELPER_ENTRY pHelperListHead;

/* PROTOTYPES *****************************************************************/

/* context.c */

BOOL
CreateRootContext(VOID);

VOID
CleanupContext(VOID);

PCONTEXT_ENTRY
FindContextByGuid(
    _In_ const GUID *pGuid);

/* help.c */

BOOL
ProcessHelp(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ DWORD dwCurrentIndex,
    _In_ LPWSTR *argv,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwHelpLevel);


DWORD
WINAPI
HelpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

#if 0
VOID
HelpGroup(
    PCONTEXT_ENTRY pContext,
    LPWSTR pszGroupName,
    BOOL bRecurse);

VOID
HelpContext(
    PCONTEXT_ENTRY pContext);

VOID
HelpSubcontexts(
    PCONTEXT_ENTRY pContext);
#endif

/* helper.c */

DWORD
CreateRootHelper(VOID);

VOID
LoadHelpers(VOID);

VOID
UnloadHelpers(VOID);

PHELPER_ENTRY
FindHelper(
    _In_ const GUID *pguidHelper,
    _In_ PHELPER_ENTRY pHelper);

DWORD
WINAPI
AddHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

DWORD
WINAPI
DeleteHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

DWORD
WINAPI
ShowHelperCommand(
    LPCWSTR pwszMachine,
    PWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);


/* interpreter.c */
BOOL
InterpretScript(
    _In_ LPWSTR pszFileName);

BOOL
InterpretCommand(
    _In_ LPWSTR *argv,
    _In_ DWORD dwArgCount,
    _Inout_ PBOOL bDone);

VOID
InterpretInteractive(VOID);

#endif /* PRECOMP_H */
