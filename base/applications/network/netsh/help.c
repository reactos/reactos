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
PrintCurrentContextHeader(
    _In_ PCONTEXT_ENTRY pContext)
{
    WCHAR szBuffer[80];

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
}


static
VOID
PrintShortCommands(
    _In_ PCONTEXT_ENTRY pContext)
{
    PCOMMAND_ENTRY pCommand;
    WCHAR szBuffer[80];

    pCommand = pContext->pCommandListHead;
    while (pCommand != NULL)
    {
        if (LoadStringW(pContext->hModule, pCommand->dwShortCmdHelpToken, szBuffer, 80) == 0)
            szBuffer[0] = UNICODE_NULL;
        ConPrintf(StdOut, L"%-15s - %s\n", pCommand->pwszCmdToken, szBuffer);
        pCommand = pCommand->pNext;
    }
}


static
VOID
PrintShortGroups(
    _In_ PCONTEXT_ENTRY pContext)
{
    PCOMMAND_GROUP pGroup;
    WCHAR szBuffer[80];

    pGroup = pContext->pGroupListHead;
    while (pGroup != NULL)
    {
        if (LoadStringW(pContext->hModule, pGroup->dwShortCmdHelpToken, szBuffer, 80) == 0)
            szBuffer[0] = UNICODE_NULL;
        ConPrintf(StdOut, L"%-15s - %s\n", pGroup->pwszCmdGroupToken, szBuffer);
        pGroup = pGroup->pNext;
    }
}


static
VOID
PrintShortSubContexts(
    _In_ PCONTEXT_ENTRY pContext)
{
    PCONTEXT_ENTRY pSubContext;
    WCHAR szBuffer[80];

    pSubContext = pContext->pSubContextHead;
    while (pSubContext != NULL)
    {
        GetContextFullName(pSubContext, szBuffer, 80);
        ConPrintf(StdOut, L"%-15s - Changes to the \"%s\" context.\n", pSubContext->pszContextName, szBuffer);
        pSubContext = pSubContext->pNext;
    }
}

static
VOID
PrintShortGroupCommands(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ PCOMMAND_GROUP pGroup)
{
    PCOMMAND_ENTRY pCommand;
    WCHAR szBuffer1[64];
    WCHAR szBuffer2[80];

    pCommand = pGroup->pCommandListHead;
    while (pCommand != NULL)
    {
        swprintf(szBuffer1, L"%s %s", pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken);
        LoadStringW(pContext->hModule, pCommand->dwShortCmdHelpToken, szBuffer2, 80);

        ConPrintf(StdOut, L"%-15s - %s\n", szBuffer1, szBuffer2);
        pCommand = pCommand->pNext;
    }
}


static
VOID
PrintLongCommand(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ PCOMMAND_ENTRY pCommand)
{
    WCHAR szBuffer[80];

    LoadStringW(pContext->hModule, pCommand->dwCmdHlpToken, szBuffer, 80);
    ConPrintf(StdOut, szBuffer);
}


static
VOID
PrintContext(
    _In_ PCONTEXT_ENTRY pContext)
{
    DPRINT1("PrintContext()\n");

    if (pContext != pRootContext)
        PrintContext(pContext->pParentContext);

    PrintCurrentContextHeader(pContext);

    PrintShortCommands(pContext);

    PrintShortGroups(pContext);

    PrintShortSubContexts(pContext);
}


static
VOID
PrintGroup(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ LPWSTR pszGroupName,
    _In_ BOOL bRecurse)
{
    PCOMMAND_GROUP pGroup;

    if (bRecurse)
    {
        if (pContext != pRootContext)
            PrintGroup(pContext->pParentContext, pszGroupName, bRecurse);
    }

    pGroup = pContext->pGroupListHead;
    while (pGroup != NULL)
    {
        if (_wcsicmp(pszGroupName, pGroup->pwszCmdGroupToken) == 0)
        {
            PrintCurrentContextHeader(pContext);
            PrintShortGroupCommands(pContext, pGroup);
        }
        pGroup = pGroup->pNext;
    }
}


static
VOID
PrintSubcontexts(
    _In_ PCONTEXT_ENTRY pContext)
{
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
}


BOOL
ProcessHelp(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ DWORD dwArgCount,
    _In_ LPWSTR *argv,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwHelpLevel)
{
    PCONTEXT_ENTRY pSubContext;
    PCOMMAND_ENTRY pCommand;
    PCOMMAND_GROUP pGroup;

    DPRINT("ProcessHelp(dwCurrentIndex %lu  dwArgCount %lu  dwHelpLevel %lu)\n", dwCurrentIndex, dwArgCount, dwHelpLevel);

    if (dwHelpLevel == dwCurrentIndex)
    {
        ConResPrintf(StdOut, IDS_HELP_HEADER);
        PrintContext(pContext);
        PrintSubcontexts(pContext);
        ConResPrintf(StdOut, IDS_HELP_FOOTER);
        return TRUE;
    }

    pCommand = pContext->pCommandListHead;
    while (pCommand != NULL)
    {
        if (_wcsicmp(argv[dwCurrentIndex], pCommand->pwszCmdToken) == 0) 
        {
            if (dwHelpLevel == dwCurrentIndex + 1)
            {
                PrintLongCommand(pContext, pCommand);
                return TRUE;
            }
        }

        pCommand = pCommand->pNext;
    }

    pGroup = pContext->pGroupListHead;
    while (pGroup != NULL)
    {
        if (_wcsicmp(argv[dwCurrentIndex], pGroup->pwszCmdGroupToken) == 0)
        {
            if (dwHelpLevel == dwCurrentIndex + 1)
            {
                ConResPrintf(StdOut, IDS_HELP_HEADER);
                PrintGroup(pContext, argv[dwCurrentIndex], (dwHelpLevel == 1));
                return TRUE;
            }

            pCommand = pGroup->pCommandListHead;
            while (pCommand != NULL)
            {
                if ((dwArgCount > dwCurrentIndex + 1) && (_wcsicmp(argv[dwCurrentIndex + 1], pCommand->pwszCmdToken) == 0))
                {
                    if (dwHelpLevel == dwCurrentIndex + 2)
                    {
                        PrintLongCommand(pContext, pCommand);
                        return TRUE;
                    }
                }

                pCommand = pCommand->pNext;
            }

//            ConResPrintf(StdOut, IDS_HELP_HEADER);
//            PrintGroup(pContext, pGroup);
            return FALSE;
        }

        pGroup = pGroup->pNext;
    }

    if (pContext == pCurrentContext)
    {
        pSubContext = pContext->pSubContextHead;
        while (pSubContext != NULL)
        {
            if (_wcsicmp(argv[dwCurrentIndex], pSubContext->pszContextName) == 0) 
            {
                return ProcessHelp(pSubContext,
                                   dwArgCount,
                                   argv,
                                   dwCurrentIndex + 1,
                                   dwHelpLevel);
            }

            pSubContext = pSubContext->pNext;
        }
    }

    return FALSE;
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
    return ERROR_SUCCESS;
}
