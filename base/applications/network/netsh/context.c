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


DWORD
WINAPI
UpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    if (pCurrentContext != pRootContext)
        pCurrentContext = pCurrentContext->pParentContext;

    return ERROR_SUCCESS;
}


DWORD
WINAPI
ExitCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    *pbDone = TRUE;
    return ERROR_SUCCESS;
}


DWORD
WINAPI
RemCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    return ERROR_SUCCESS;
}


DWORD
WINAPI
PopdCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PCONTEXT_STACK_ENTRY pEntry;

    DPRINT("PopdCommand()\n");

    if (pContextStackHead == NULL)
        return 0;

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
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    _Out_ BOOL *pbDone)
{
    PCONTEXT_STACK_ENTRY pEntry;

    DPRINT("PushdCommand()\n");

    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CONTEXT_STACK_ENTRY));
    if (pEntry == NULL)
        return 1;

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


BOOL
CreateRootContext(VOID)
{
    PCOMMAND_GROUP pGroup;

    pRootContext = AddContext(NULL, L"netsh", NULL);
    DPRINT("pRootContext: %p\n", pRootContext);
    if (pRootContext == NULL)
        return FALSE;

    pRootContext->hModule = GetModuleHandle(NULL);

    AddContextCommand(pRootContext, L"..",    UpCommand,    IDS_HLP_UP,    IDS_HLP_UP_EX, 0);
    AddContextCommand(pRootContext, L"?",     NULL,         IDS_HLP_HELP, IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"bye",   ExitCommand,  IDS_HLP_EXIT,  IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"exit",  ExitCommand,  IDS_HLP_EXIT,  IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"help",  NULL,         IDS_HLP_HELP, IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"popd",  PopdCommand,  IDS_HLP_POPD,  IDS_HLP_POPD_EX, 0);
    AddContextCommand(pRootContext, L"pushd", PushdCommand, IDS_HLP_PUSHD, IDS_HLP_PUSHD_EX, 0);
    AddContextCommand(pRootContext, L"quit",  ExitCommand,  IDS_HLP_EXIT,  IDS_HLP_EXIT_EX, 0);

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

    pGroup = AddCommandGroup(pRootContext, L"show", IDS_HLP_GROUP_SHOW, 0);
    if (pGroup)
    {
        AddGroupCommand(pGroup, L"helper", ShowHelperCommand, IDS_HLP_SHOW_HELPER, IDS_HLP_SHOW_HELPER_EX, 0);
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
