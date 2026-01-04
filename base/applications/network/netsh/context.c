/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell context management functions
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _CONTEXT_STACK_ENTRY
{
    struct _CONTEXT_STACK_ENTRY *pPrev;
    struct _CONTEXT_STACK_ENTRY *pNext;

    PCONTEXT_ENTRY pContext;
} CONTEXT_STACK_ENTRY, *PCONTEXT_STACK_ENTRY;


PCONTEXT_ENTRY pRootContext = NULL;
PCONTEXT_ENTRY pCurrentContext = NULL;

PCONTEXT_STACK_ENTRY pContextStackHead = NULL;
PCONTEXT_STACK_ENTRY pContextStackTail = NULL;

PWSTR pszMachine = NULL;

static BOOL bOnline = TRUE; 

/* FUNCTIONS ******************************************************************/

PCONTEXT_ENTRY
AddContext(
    PCONTEXT_ENTRY pParentContext,
    PWSTR pszName,
    GUID *pGuid)
{
    PCONTEXT_ENTRY pEntry;

    DPRINT("AddContext(%S)\n", pszName);
    if (pParentContext)
    {
        DPRINT("ParentContext %S\n", pParentContext->pszContextName);
    }

    if (pParentContext != NULL && pszName == NULL)
        return NULL;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CONTEXT_ENTRY));
    if (pEntry == NULL)
        return NULL;

    /* Allocate the name buffer */
    if (pszName != NULL)
    {
        pEntry->pszContextName = HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           (wcslen(pszName) + 1) * sizeof(WCHAR));
        if (pEntry->pszContextName == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pEntry);
            return NULL;
        }

        /* Fill the entry */
        wcscpy(pEntry->pszContextName, pszName);
    }

    pEntry->pParentContext = pParentContext;
    if (pGuid != NULL)
        CopyMemory(&pEntry->Guid, pGuid, sizeof(pEntry->Guid));

    /* Insert it */
    if (pParentContext != NULL)
    {
        if ((pParentContext->pSubContextHead == NULL) && (pParentContext->pSubContextTail == NULL))
        {
            pParentContext->pSubContextHead = pEntry;
            pParentContext->pSubContextTail = pEntry;
        }
        else
        {
            pEntry->pPrev = pParentContext->pSubContextTail;
            pParentContext->pSubContextTail->pNext = pEntry;
            pParentContext->pSubContextTail = pEntry;
        }
    }

    return pEntry;
}


PCOMMAND_ENTRY
AddContextCommand(
    PCONTEXT_ENTRY pContext,
    LPCWSTR pwszCmdToken,
    PFN_HANDLE_CMD pfnCmdHandler,
    DWORD dwShortCmdHelpToken,
    DWORD dwCmdHlpToken,
    DWORD dwFlags)
{
    PCOMMAND_ENTRY pEntry;

    if (pfnCmdHandler == NULL)
        return NULL;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMMAND_ENTRY));
    if (pEntry == NULL)
        return NULL;

    pEntry->pwszCmdToken = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (wcslen(pwszCmdToken) + 1) * sizeof(WCHAR));
    if (pEntry->pwszCmdToken == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pEntry);
        return NULL;
    }

    wcscpy((LPWSTR)pEntry->pwszCmdToken, pwszCmdToken);

    pEntry->pfnCmdHandler = pfnCmdHandler;
    pEntry->dwShortCmdHelpToken = dwShortCmdHelpToken;
    pEntry->dwCmdHlpToken = dwCmdHlpToken;
    pEntry->dwFlags = dwFlags;

    if (pContext->pCommandListHead == NULL && pContext->pCommandListTail == NULL)
    {
        pContext->pCommandListHead = pEntry;
        pContext->pCommandListTail = pEntry;
    }
    else
    {
        pEntry->pPrev = pContext->pCommandListTail;
        pContext->pCommandListTail->pNext = pEntry;
        pContext->pCommandListTail = pEntry;
    }

    return pEntry;
}


PCOMMAND_GROUP
AddCommandGroup(
    PCONTEXT_ENTRY pContext,
    LPCWSTR pwszCmdGroupToken,
    DWORD dwShortCmdHelpToken,
    DWORD dwFlags)
{
    PCOMMAND_GROUP pEntry;

    DPRINT("AddCommandGroup(%S %lu)\n", pwszCmdGroupToken, dwShortCmdHelpToken);

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMMAND_GROUP));
    if (pEntry == NULL)
        return NULL;

    pEntry->pwszCmdGroupToken = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          (wcslen(pwszCmdGroupToken) + 1) * sizeof(WCHAR));
    if (pEntry->pwszCmdGroupToken == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pEntry);
        return NULL;
    }

    wcscpy((LPWSTR)pEntry->pwszCmdGroupToken, pwszCmdGroupToken);
    pEntry->dwShortCmdHelpToken = dwShortCmdHelpToken;
    pEntry->dwFlags = dwFlags;

    if (pContext->pGroupListHead == NULL && pContext->pGroupListTail == NULL)
    {
        pContext->pGroupListHead = pEntry;
        pContext->pGroupListTail = pEntry;
    }
    else
    {
        pEntry->pPrev = pContext->pGroupListTail;
        pContext->pGroupListTail->pNext = pEntry;
        pContext->pGroupListTail = pEntry;
    }

    return pEntry;
}


PCOMMAND_ENTRY
AddGroupCommand(
    PCOMMAND_GROUP pGroup,
    LPCWSTR pwszCmdToken,
    PFN_HANDLE_CMD pfnCmdHandler,
    DWORD dwShortCmdHelpToken,
    DWORD dwCmdHlpToken,
    DWORD dwFlags)
{
    PCOMMAND_ENTRY pEntry;

    if (pfnCmdHandler == NULL)
        return NULL;

    /* Allocate the entry */
    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(COMMAND_ENTRY));
    if (pEntry == NULL)
        return NULL;

    pEntry->pwszCmdToken = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (wcslen(pwszCmdToken) + 1) * sizeof(WCHAR));
    if (pEntry->pwszCmdToken == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pEntry);
        return NULL;
    }

    wcscpy((LPWSTR)pEntry->pwszCmdToken, pwszCmdToken);

    pEntry->pfnCmdHandler = pfnCmdHandler;
    pEntry->dwShortCmdHelpToken = dwShortCmdHelpToken;
    pEntry->dwCmdHlpToken = dwCmdHlpToken;
    pEntry->dwFlags = dwFlags;

    if (pGroup->pCommandListHead == NULL && pGroup->pCommandListTail == NULL)
    {
        pGroup->pCommandListHead = pEntry;
        pGroup->pCommandListTail = pEntry;
    }
    else
    {
        pEntry->pPrev = pGroup->pCommandListTail;
        pGroup->pCommandListTail->pNext = pEntry;
        pGroup->pCommandListTail = pEntry;
    }

    return pEntry;
}


VOID
RemoveContextFromStack(
    _In_ PCONTEXT_ENTRY pContextEntry)
{
    PCONTEXT_STACK_ENTRY pStackEntry, pNextEntry;

    if (pContextStackHead == NULL)
        return;

    pStackEntry = pContextStackHead;
    while (1)
    {
        if (pStackEntry->pContext == pContextEntry)
        {
            if (pStackEntry == pContextStackHead && pStackEntry == pContextStackHead)
            {
                pContextStackHead = NULL;
                pContextStackTail = NULL;
                HeapFree(GetProcessHeap(), 0, pStackEntry);
                return;
            }
            else if (pStackEntry == pContextStackHead)
            {
                pStackEntry->pNext->pPrev = NULL;
                pContextStackHead = pStackEntry->pNext;
                HeapFree(GetProcessHeap(), 0, pStackEntry);
                pStackEntry = pContextStackHead;
            }
            else if (pStackEntry == pContextStackTail)
            {
                pStackEntry->pPrev->pNext = NULL;
                pContextStackTail = pStackEntry->pPrev;
                HeapFree(GetProcessHeap(), 0, pStackEntry);
                return;
            }
            else
            {
                pNextEntry = pStackEntry->pNext;
                pStackEntry->pPrev->pNext = pStackEntry->pNext;
                pStackEntry->pNext->pPrev = pStackEntry->pPrev;
                HeapFree(GetProcessHeap(), 0, pStackEntry);
                pStackEntry = pNextEntry;
            }
        }
        else
        {
            if (pStackEntry == pContextStackTail)
                return;

            pStackEntry = pStackEntry->pNext;
        }
    }
}


VOID
DeleteContext(
    PWSTR pszName)
{
    /* Remove the context from the stack */
    /* RemoveContextFromStack(); */

    /* Delete all commands */
    /* Delete the context */
}


static
int
ContextCompare(
    _In_ const void *p1,
    _In_ const void *p2)
{
    return ((PCONTEXT_ENTRY)p1)->ulPriority - ((PCONTEXT_ENTRY)p2)->ulPriority;
}


static
DWORD
DumpContext(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwArgCount,
    _In_ LPCVOID pvData)
{
    PCONTEXT_ENTRY pSubContext, *pSortArray = NULL;
    DWORD dwCount, dwIndex;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("DumpContext()\n");

    if (pContext->pfnDumpFn)
    {
        dwError = pContext->pfnDumpFn(pwszMachine,
                                      ppwcArguments,
                                      dwArgCount,
                                      pvData);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("Dump function failed (Error %lu)\n", dwError);
            return dwError;
        }
    }

    if (pContext->pSubContextHead == NULL)
        return dwError;

    /* Count the sub-contexts */
    dwCount = 0;
    pSubContext = pContext->pSubContextHead;
    while (pSubContext)
    {
        dwCount++;
        pSubContext = pSubContext->pNext;
    }

    /* Allocate the sort array */
    pSortArray = HeapAlloc(GetProcessHeap(), 0, dwCount * sizeof(PCONTEXT_ENTRY));
    if (pSortArray == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill the sort array */
    dwIndex = 0;
    pSubContext = pContext->pSubContextHead;
    while (pSubContext)
    {
        pSortArray[dwIndex] = pSubContext;
        dwIndex++;
        pSubContext = pSubContext->pNext;
    }

    /* Sort the array */
    qsort(pSortArray, dwCount, sizeof(PCONTEXT_ENTRY), ContextCompare);

    /* Dump the sub-contexts */
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        dwError = DumpContext(pSortArray[dwIndex],
                              pwszMachine,
                              ppwcArguments,
                              dwArgCount,
                              pvData);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("Dump function failed (Error %lu)\n", dwError);
            break;
        }
    }

    /* Free the sort array */
    HeapFree(GetProcessHeap(), 0, pSortArray);

    return dwError;
}

static
DWORD
CommitContext(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ DWORD dwAction)
{
    PCONTEXT_ENTRY pSubContext, *pSortArray = NULL;
    DWORD dwCount, dwIndex;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT1("CommitContext(%p %lu)\n", pContext, dwAction);

    if (pContext->pfnCommitFn)
    {
        dwError = pContext->pfnCommitFn(dwAction);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("Commit function failed (Error %lu)\n", dwError);
            return dwError;
        }
    }

    if (pContext->pSubContextHead == NULL)
        return dwError;

    /* Count the sub-contexts */
    dwCount = 0;
    pSubContext = pContext->pSubContextHead;
    while (pSubContext)
    {
        dwCount++;
        pSubContext = pSubContext->pNext;
    }

    /* Allocate the sort array */
    pSortArray = HeapAlloc(GetProcessHeap(), 0, dwCount * sizeof(PCONTEXT_ENTRY));
    if (pSortArray == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill the sort array */
    dwIndex = 0;
    pSubContext = pContext->pSubContextHead;
    while (pSubContext)
    {
        pSortArray[dwIndex] = pSubContext;
        dwIndex++;
        pSubContext = pSubContext->pNext;
    }

    /* Sort the array */
    qsort(pSortArray, dwCount, sizeof(PCONTEXT_ENTRY), ContextCompare);

    /* Commit the sub-contexts */
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        dwError = CommitContext(pSortArray[dwIndex],
                                dwAction);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("Commit function failed (Error %lu)\n", dwError);
            break;
        }
    }

    /* Free the sort array */
    HeapFree(GetProcessHeap(), 0, pSortArray);

    return dwError;
}


DWORD
WINAPI
UpCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    if (pCurrentContext != pRootContext)
        pCurrentContext = pCurrentContext->pParentContext;

    return ERROR_SUCCESS;
}


DWORD
WINAPI
AbortCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("AbortCommand()\n");
    CommitContext(pRootContext, NETSH_FLUSH);
    return ERROR_SUCCESS;
}


DWORD
WINAPI
CommitCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("CommitCommand()\n");
    CommitContext(pRootContext, NETSH_SAVE);
    return ERROR_SUCCESS;
}


DWORD
WINAPI
DumpCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("DumpCommand()\n");

    return DumpContext(pCurrentContext,
                       pwszMachine,
                       ppwcArguments,
                       dwArgCount,
                       pvData);
}


DWORD
WINAPI
ExecCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("ExecCommand()\n");

    if (dwArgCount - dwCurrentIndex != 1)
        return ERROR_SHOW_USAGE;

    return RunScript(argv[dwCurrentIndex]);
}


DWORD
WINAPI
ExitCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    if (bOnline == FALSE)
        CommitContext(pRootContext, NETSH_FLUSH);

    *pbDone = TRUE;
    return ERROR_SUCCESS;
}


DWORD
WINAPI
RemCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    return ERROR_SUCCESS;
}


DWORD
WINAPI
OfflineCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("OfflineCommand()\n");
    CommitContext(pRootContext, NETSH_UNCOMMIT);
    bOnline = FALSE;
    return ERROR_SUCCESS;
}


DWORD
WINAPI
OnlineCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("OnlineCommand()\n");
    CommitContext(pRootContext, NETSH_COMMIT);
    bOnline = TRUE;
    return ERROR_SUCCESS;
}


DWORD
WINAPI
PopdCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    PCONTEXT_STACK_ENTRY pEntry;

    DPRINT("PopdCommand()\n");

    if (pContextStackHead == NULL)
        return ERROR_SUCCESS;

    pEntry = pContextStackHead;

    pCurrentContext = pEntry->pContext;

    if (pContextStackTail == pEntry)
    {
        pContextStackHead = NULL;
        pContextStackTail = NULL;
    }
    else
    {
        pContextStackHead = pEntry->pNext;
        pContextStackHead->pPrev = NULL;
    }

    HeapFree(GetProcessHeap(), 0, pEntry);

    return ERROR_SUCCESS;
}


DWORD
WINAPI
PushdCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    PCONTEXT_STACK_ENTRY pEntry;

    DPRINT("PushdCommand()\n");

    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CONTEXT_STACK_ENTRY));
    if (pEntry == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    pEntry->pContext = pCurrentContext;
    if (pContextStackHead == NULL)
    {
        pContextStackHead = pEntry;
        pContextStackTail = pEntry;
    }
    else
    {
        pEntry->pNext = pContextStackHead;
        pContextStackHead->pPrev = pEntry;
        pContextStackHead = pEntry;
    }

    return ERROR_SUCCESS;
}


DWORD
WINAPI
SetMachineCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("SetMachineCommand(pwszMachine %S  dwCurrentIndex %lu  dwArgCount %lu)\n",
           pwszMachine, dwCurrentIndex, dwArgCount);

    if ((dwArgCount - dwCurrentIndex) > 1)
        return ERROR_SHOW_USAGE;

    if (pszMachine != NULL)
    {
        HeapFree(GetProcessHeap(), 0, pszMachine);
        pszMachine = NULL;
    }

    if ((dwArgCount - dwCurrentIndex) == 1)
    {
        pszMachine = HeapAlloc(GetProcessHeap(), 0, (sizeof(argv[dwCurrentIndex]) + 1) * sizeof(WCHAR));
        if (pszMachine == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        wcscpy(pszMachine, argv[dwCurrentIndex]);
    }

    return dwError;
}


DWORD
WINAPI
SetModeCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("SetModeCommand(pwszMachine %S  dwCurrentIndex %lu  dwArgCount %lu)\n",
           pwszMachine, dwCurrentIndex, dwArgCount);

    if ((dwArgCount - dwCurrentIndex) != 1)
        return ERROR_SHOW_USAGE;

    if (!_wcsicmp(argv[dwCurrentIndex], L"offline"))
    {
        CommitContext(pRootContext, NETSH_UNCOMMIT);
        bOnline = FALSE;
    }
    else if (!_wcsicmp(argv[dwCurrentIndex], L"online"))
    {
        CommitContext(pRootContext, NETSH_COMMIT);
        bOnline = TRUE;
    }
    else
    {
        dwError = ERROR_INVALID_SYNTAX;
    }

    return dwError;
}


DWORD
WINAPI
ShowModeCommand(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    DPRINT("ShowModeCommand()\n");
    ConPuts(StdOut, bOnline ? L"online\n\n" : L"offline\n\n");
    return ERROR_SUCCESS;
}


BOOL
CreateRootContext(VOID)
{
    PCOMMAND_GROUP pGroup;

    pRootContext = AddContext(NULL, L"netsh", NULL);
    DPRINT("pRootContext: %p\n", pRootContext);
    if (pRootContext == NULL)
        return FALSE;

    pRootContext->hModule = hModule;

    AddContextCommand(pRootContext, L"..",      UpCommand,      IDS_HLP_UP,      IDS_HLP_UP_EX, 0);
    AddContextCommand(pRootContext, L"?",       NULL,           IDS_HLP_HELP,    IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"abort",   AbortCommand,   IDS_HLP_ABORT,   IDS_HLP_ABORT_EX, 0);
    AddContextCommand(pRootContext, L"alias",   AliasCommand,   IDS_HLP_ALIAS,   IDS_HLP_ALIAS_EX, 0);
    AddContextCommand(pRootContext, L"bye",     ExitCommand,    IDS_HLP_EXIT,    IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"commit",  CommitCommand,  IDS_HLP_COMMIT,  IDS_HLP_COMMIT_EX, 0);
    AddContextCommand(pRootContext, L"dump",    DumpCommand,    IDS_HLP_DUMP,    IDS_HLP_DUMP_EX, 0);
    AddContextCommand(pRootContext, L"exec",    ExecCommand,    IDS_HLP_EXEC,    IDS_HLP_EXEC_EX, 0);
    AddContextCommand(pRootContext, L"exit",    ExitCommand,    IDS_HLP_EXIT,    IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"help",    NULL,           IDS_HLP_HELP,    IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"offline", OfflineCommand, IDS_HLP_OFFLINE, IDS_HLP_OFFLINE_EX, 0);
    AddContextCommand(pRootContext, L"online",  OnlineCommand,  IDS_HLP_ONLINE,  IDS_HLP_ONLINE_EX, 0);
    AddContextCommand(pRootContext, L"popd",    PopdCommand,    IDS_HLP_POPD,    IDS_HLP_POPD_EX, 0);
    AddContextCommand(pRootContext, L"pushd",   PushdCommand,   IDS_HLP_PUSHD,   IDS_HLP_PUSHD_EX, 0);
    AddContextCommand(pRootContext, L"quit",    ExitCommand,    IDS_HLP_EXIT,    IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"unalias", UnaliasCommand, IDS_HLP_UNALIAS, IDS_HLP_UNALIAS_EX, 0);

    pGroup = AddCommandGroup(pRootContext, L"add", IDS_HLP_GROUP_ADD, 0);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"helper", AddHelperCommand, IDS_HLP_ADD_HELPER, IDS_HLP_ADD_HELPER_EX, 0);
    }

    pGroup = AddCommandGroup(pRootContext, L"delete", IDS_HLP_GROUP_DELETE, 0);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"helper", DeleteHelperCommand, IDS_HLP_DEL_HELPER, IDS_HLP_DEL_HELPER_EX, 0);
    }

    pGroup = AddCommandGroup(pRootContext, L"set", IDS_HLP_GROUP_SET, 0);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"machine", SetMachineCommand, IDS_HLP_SET_MACHINE, IDS_HLP_SET_MACHINE_EX, 0);
        AddGroupCommand(pGroup, L"mode",    SetModeCommand,    IDS_HLP_SET_MODE,    IDS_HLP_SET_MODE_EX, 0);
    }

    pGroup = AddCommandGroup(pRootContext, L"show", IDS_HLP_GROUP_SHOW, 0);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"alias",  ShowAliasCommand,  IDS_HLP_SHOW_ALIAS,  IDS_HLP_SHOW_ALIAS_EX, 0);
        AddGroupCommand(pGroup, L"helper", ShowHelperCommand, IDS_HLP_SHOW_HELPER, IDS_HLP_SHOW_HELPER_EX, 0);
        AddGroupCommand(pGroup, L"mode",   ShowModeCommand,   IDS_HLP_SHOW_MODE,   IDS_HLP_SHOW_MODE_EX, 0);
    }

    pCurrentContext = pRootContext;

    return TRUE;
}


static
PCONTEXT_ENTRY
FindSubContextByGuid(
    PCONTEXT_ENTRY pContext,
    const GUID *pGuid)
{
    PCONTEXT_ENTRY pResultContext, pSubContext;

    DPRINT("FindSubContextByGuid(%p)\n", pContext);
    DPRINT("%lx <--> %lx\n", pContext->Guid.Data1, pGuid->Data1);

    if (IsEqualGUID(&pContext->Guid, pGuid))
    {
        DPRINT("Found!\n");
        return pContext;
    }

    pSubContext = pContext->pSubContextHead;
    while (pSubContext)
    {
        pResultContext = FindSubContextByGuid(pSubContext, pGuid);
        if (pResultContext)
            return pResultContext;

        pSubContext = pSubContext->pNext;
    }

    return NULL;
}


PCONTEXT_ENTRY
FindContextByGuid(
    const GUID *pGuid)
{
    if (pRootContext == NULL)
        return NULL;
    return FindSubContextByGuid(pRootContext, pGuid);
}


DWORD
WINAPI
RegisterContext(
    _In_ const NS_CONTEXT_ATTRIBUTES *pChildContext)
{
    PHELPER_ENTRY pHelper;
    PCONTEXT_ENTRY pContext, pParentContext;
    PCOMMAND_GROUP pGroup;
    DWORD i, j;

    DPRINT1("RegisterContext(%p)\n", pChildContext);
    if (pChildContext == NULL)
    {
        DPRINT1("Invalid child context!\n");
        return ERROR_INVALID_PARAMETER;
    }

    if ((pChildContext->pwszContext == NULL) ||
        (wcslen(pChildContext->pwszContext) == 0) ||
        (wcschr(pChildContext->pwszContext, L' ') != 0) ||
        (wcschr(pChildContext->pwszContext, L'=') != 0))
    {
        DPRINT1("Invalid context name!\n");
        return ERROR_INVALID_PARAMETER;
    }

    DPRINT("Name: %S\n", pChildContext->pwszContext);
    DPRINT("Groups: %lu\n", pChildContext->ulNumGroups);
    DPRINT("Top commands: %lu\n", pChildContext->ulNumTopCmds);

    pHelper = FindHelper(&pChildContext->guidHelper, pHelperListHead);
    DPRINT("Helper %p\n", pHelper);
    pParentContext = pRootContext;
    if (pHelper != NULL)
    {
        pParentContext = FindContextByGuid(&pHelper->ParentHelperGuid);
        DPRINT("pParentContext %p\n", pParentContext);
        if (pParentContext == NULL)
            pParentContext = pRootContext;
    }

    pContext = AddContext(pParentContext, pChildContext->pwszContext, (GUID*)&pChildContext->guidHelper);
    if (pContext != NULL)
    {
        pContext->pfnCommitFn = pChildContext->pfnCommitFn;
        pContext->pfnDumpFn = pChildContext->pfnDumpFn;
        pContext->pfnConnectFn = pChildContext->pfnConnectFn;
        pContext->ulPriority = (pChildContext->dwFlags & CMD_FLAG_PRIORITY) ?
                               pChildContext->ulPriority : DEFAULT_CONTEXT_PRIORITY;

        if ((pHelper != NULL) && (pHelper->pDllEntry != NULL))
        {
            pContext->hModule = pHelper->pDllEntry->hModule;
        }

        for (i = 0; i < pChildContext->ulNumTopCmds; i++)
        {
            AddContextCommand(pContext,
                              pChildContext->pTopCmds[i].pwszCmdToken,
                              pChildContext->pTopCmds[i].pfnCmdHandler,
                              pChildContext->pTopCmds[i].dwShortCmdHelpToken,
                              pChildContext->pTopCmds[i].dwCmdHlpToken,
                              pChildContext->pTopCmds[i].dwFlags);
        }

        /* Add command groups */
        for (i = 0; i < pChildContext->ulNumGroups; i++)
        {
            pGroup = AddCommandGroup(pContext,
                                     pChildContext->pCmdGroups[i].pwszCmdGroupToken,
                                     pChildContext->pCmdGroups[i].dwShortCmdHelpToken,
                                     pChildContext->pCmdGroups[i].dwFlags);
            if (pGroup != NULL)
            {
                for (j = 0; j < pChildContext->pCmdGroups[i].ulCmdGroupSize; j++)
                {
                    AddGroupCommand(pGroup,
                                    pChildContext->pCmdGroups[i].pCmdGroup[j].pwszCmdToken,
                                    pChildContext->pCmdGroups[i].pCmdGroup[j].pfnCmdHandler,
                                    pChildContext->pCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken,
                                    pChildContext->pCmdGroups[i].pCmdGroup[j].dwCmdHlpToken,
                                    pChildContext->pCmdGroups[i].pCmdGroup[j].dwFlags);
                }
            }
        }
    }

    return ERROR_SUCCESS;
}


VOID
CleanupContext(VOID)
{
    /* Delete the context stack */

}
