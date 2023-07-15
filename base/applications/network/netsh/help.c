/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell builtin help command and support functions
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
GetContextFullName(
    _In_ PCONTEXT_ENTRY pContext,
    _Inout_ LPWSTR pszBuffer,
    _In_ DWORD cchLength)
{
    if (pContext->pParentContext != NULL)
    {
        GetContextFullName(pContext->pParentContext, pszBuffer, cchLength);
        wcscat(pszBuffer, L" ");
        wcscat(pszBuffer, pContext->pszContextName);
    }
    else
    {
        wcscpy(pszBuffer, L"netsh");
    }
}


static
VOID
HelpContext(
    PCONTEXT_ENTRY pContext)
{
    PCONTEXT_ENTRY pSubContext;
    PCOMMAND_ENTRY pCommand;
    PCOMMAND_GROUP pGroup;
    WCHAR szBuffer[80];

    if (pContext != pRootContext)
        HelpContext(pContext->pParentContext);

    if (pContext == pCurrentContext)
    {
        ConPrintf(StdOut, L"\nCommands in this context:\n");
    }
    else if (pContext == pRootContext)
    {
        ConPrintf(StdOut, L"\nCommands in the netsh-context:\n");
    }
    else
    {
        GetContextFullName(pContext, szBuffer, 80);
        ConPrintf(StdOut, L"\nCommands in the %s-context:\n", szBuffer);
    }

    pCommand = pContext->pCommandListHead;
    while (pCommand != NULL)
    {
        if (LoadStringW(pContext->hModule, pCommand->dwShortCmdHelpToken, szBuffer, 80) == 0)
            szBuffer[0] = UNICODE_NULL;
        ConPrintf(StdOut, L"%-15s - %s\n", pCommand->pwszCmdToken, szBuffer);
        pCommand = pCommand->pNext;
    }

    pGroup = pContext->pGroupListHead;
    while (pGroup != NULL)
    {
        if (LoadStringW(pContext->hModule, pGroup->dwShortCmdHelpToken, szBuffer, 80) == 0)
            szBuffer[0] = UNICODE_NULL;
        ConPrintf(StdOut, L"%-15s - %s\n", pGroup->pwszCmdGroupToken, szBuffer);
        pGroup = pGroup->pNext;
    }

    pSubContext = pContext->pSubContextHead;
    while (pSubContext != NULL)
    {
        GetContextFullName(pSubContext, szBuffer, 80);
        ConPrintf(StdOut, L"%-15s - Changes to the \"%s\" context.\n", pSubContext->pszContextName, szBuffer);
        pSubContext = pSubContext->pNext;
    }
}


VOID
HelpGroup(
    PCOMMAND_GROUP pGroup)
{
    PCOMMAND_ENTRY pCommand;
    WCHAR szBuffer[64];

    ConResPrintf(StdOut, IDS_HELP_HEADER);

    ConPrintf(StdOut, L"\nCommands in this context:\n");

    pCommand = pGroup->pCommandListHead;
    while (pCommand != NULL)
    {
        swprintf(szBuffer, L"%s %s", pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken);
        ConPrintf(StdOut, L"%-15s - ", szBuffer);
        ConResPuts(StdOut, pCommand->dwShortCmdHelpToken);
        pCommand = pCommand->pNext;
    }
}


DWORD
WINAPI
HelpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PCONTEXT_ENTRY pContext;

    ConResPrintf(StdOut, IDS_HELP_HEADER);

    pContext = pCurrentContext;
    if (pContext == NULL)
    {
        DPRINT1("HelpCommand: invalid context %p\n", pContext);
        return 1;
    }

    HelpContext(pContext);

    if (pCurrentContext->pSubContextHead != NULL)
    {
        ConResPrintf(StdOut, IDS_SUBCONTEXT_HEADER);
        pContext = pCurrentContext->pSubContextHead;
        while (pContext != NULL)
        {
            ConPrintf(StdOut, L" %s", pContext->pszContextName);
            pContext = pContext->pNext;
        }
        ConPuts(StdOut, L"\n");
    }
    ConPuts(StdOut, L"\n");

    return ERROR_SUCCESS;
}
