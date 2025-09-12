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

PCONTEXT_ENTRY pRootContext = NULL;
PCONTEXT_ENTRY pCurrentContext = NULL;

/* FUNCTIONS ******************************************************************/

PCONTEXT_ENTRY
AddContext(
    PCONTEXT_ENTRY pParentContext,
    PWSTR pszName,
    GUID *pGuid)
{
    PCONTEXT_ENTRY pEntry;

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
        if (pParentContext->pSubContextHead == NULL && pParentContext->pSubContextTail == NULL)
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
DeleteContext(
    PWSTR pszName)
{
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

    return 0;
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
    return 0;
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
    return 0;
}


BOOL
CreateRootContext(VOID)
{
    PCOMMAND_GROUP pGroup;

    pRootContext = AddContext(NULL, NULL, NULL);
    DPRINT1("pRootContext: %p\n", pRootContext);
    if (pRootContext == NULL)
        return FALSE;

    pRootContext->hModule = GetModuleHandle(NULL);

    AddContextCommand(pRootContext, L"..",   UpCommand, IDS_HLP_UP, IDS_HLP_UP_EX, 0);
    AddContextCommand(pRootContext, L"?",    HelpCommand, IDS_HLP_HELP, IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"bye",  ExitCommand, IDS_HLP_EXIT, IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"exit", ExitCommand, IDS_HLP_EXIT, IDS_HLP_EXIT_EX, 0);
    AddContextCommand(pRootContext, L"help", HelpCommand, IDS_HLP_HELP, IDS_HLP_HELP_EX, 0);
    AddContextCommand(pRootContext, L"quit", ExitCommand, IDS_HLP_EXIT, IDS_HLP_EXIT_EX, 0);

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


DWORD
WINAPI
RegisterContext(
    _In_ const NS_CONTEXT_ATTRIBUTES *pChildContext)
{
    PCONTEXT_ENTRY pContext;
    DWORD i;

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

    DPRINT1("Name: %S\n", pChildContext->pwszContext);

    pContext = AddContext(pRootContext, pChildContext->pwszContext, (GUID*)&pChildContext->guidHelper);
    if (pContext != NULL)
    {
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
            AddCommandGroup(pContext,
                pChildContext->pCmdGroups[i].pwszCmdGroupToken,
                pChildContext->pCmdGroups[i].dwShortCmdHelpToken,
                pChildContext->pCmdGroups[i].dwFlags);
        }
    }

    return ERROR_SUCCESS;
}
