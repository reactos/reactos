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

#include <ndk/rtlfuncs.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>
#include <iphlpapi_undoc.h>

#include <errno.h>

#include <conutils.h>
#include <netsh.h>
#include <netsh_undoc.h>

#include "resource.h"


/* DEFINES *******************************************************************/

#define HUGE_BUFFER_SIZE  2048

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
    ULONG ulPriority;
    PNS_CONTEXT_COMMIT_FN pfnCommitFn;
    PNS_CONTEXT_DUMP_FN pfnDumpFn;
    PNS_CONTEXT_CONNECT_FN pfnConnectFn;

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

extern HMODULE hModule;
extern PWSTR pszMachine;

/* PROTOTYPES *****************************************************************/

/* alias.c */

VOID
InitAliases(VOID);

VOID
DestroyAliases(VOID);

DWORD
WINAPI
AliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

DWORD
WINAPI
ShowAliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

DWORD
WINAPI
UnaliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

/* context.c */

BOOL
CreateRootContext(VOID);

VOID
CleanupContext(VOID);

PCONTEXT_ENTRY
FindContextByGuid(
    _In_ const GUID *pGuid);

/* help.c */

VOID
PrintCommandHelp(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ PCOMMAND_GROUP pGroup,
    _In_ PCOMMAND_ENTRY pCommand);

VOID
PrintGroupHelp(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ LPWSTR pszGroupName,
    _In_ BOOL bRecurse);

VOID
PrintContextHelp(
    _In_ PCONTEXT_ENTRY pContext);

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

DWORD
InterpretLine(
    _In_ LPWSTR pszFileName);

VOID
InterpretInteractive(VOID);

/* netsh.c */

DWORD
RunScript(
    _In_ LPCWSTR filename);

LPWSTR
MergeStrings(
    _In_ LPWSTR pszStringArray[],
    _In_ INT nCount);

#endif /* PRECOMP_H */
