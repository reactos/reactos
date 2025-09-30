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

typedef enum
{
    Command,
    Group,
    SubContext
} HELP_TYPE, *PHELP_TYPE;

typedef struct
{
    HELP_TYPE Type;
    PWSTR pszCommand;
    DWORD dwHelpId;
    union
    {
        PCOMMAND_ENTRY pCommand;
        PCOMMAND_GROUP pGroup;
        PCONTEXT_ENTRY pSubContext;
    } Pointer;
} HELP_ENTRY, *PHELP_ENTRY;


/* FUNCTIONS ******************************************************************/

static
VOID
GetContextFullName(
    _In_ PCONTEXT_ENTRY pContext,
    _Inout_ LPWSTR pszBuffer,
    _In_ DWORD cchLength)
{
    if (pContext != pRootContext)
    {
        GetContextFullName(pContext->pParentContext, pszBuffer, cchLength);
        wcscat(pszBuffer, L" ");
        wcscat(pszBuffer, pContext->pszContextName);
    }
    else
    {
        wcscpy(pszBuffer, pContext->pszContextName);
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
        ConResPrintf(StdOut, IDS_THIS_COMMANDS);
    }
    else
    {
        GetContextFullName(pContext, szBuffer, 80);
        ConResPrintf(StdOut, IDS_CONTEXT_COMMANDS, szBuffer);
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

        ConPrintf(StdOut, L"%-15s - %s", szBuffer1, szBuffer2);
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
int
HelpCompare(
    _In_ const void *p1,
    _In_ const void *p2)
{
    return _wcsicmp(((PHELP_ENTRY)p1)->pszCommand, ((PHELP_ENTRY)p2)->pszCommand);
}


static
VOID
PrintContext(
    _In_ PCONTEXT_ENTRY pContext)
{
    PCOMMAND_ENTRY pCommand;
    PCOMMAND_GROUP pGroup;
    PCONTEXT_ENTRY pSubContext;
    PHELP_ENTRY pHelpArray = NULL;
    DWORD dwCount = 0, dwIndex;
    WCHAR szBuffer[80];

    DPRINT("PrintContext()\n");

    if (pContext != pRootContext)
        PrintContext(pContext->pParentContext);

    PrintCurrentContextHeader(pContext);

    /* Count short commands */
    pCommand = pContext->pCommandListHead;
    while (pCommand != NULL)
    {
        dwCount++;
        pCommand = pCommand->pNext;
    }

    /* Count short groups */
    pGroup = pContext->pGroupListHead;
    while (pGroup != NULL)
    {
        dwCount++;
        pGroup = pGroup->pNext;
    }

    /* Count short subcontexts */
    pSubContext = pContext->pSubContextHead;
    while (pSubContext != NULL)
    {
        dwCount++;
        pSubContext = pSubContext->pNext;
    }

    pHelpArray = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCount * sizeof(HELP_ENTRY));
    if (pHelpArray == NULL)
        return;

    dwIndex = 0;

    /* Add short commands */
    pCommand = pContext->pCommandListHead;
    while (pCommand != NULL)
    {
        pHelpArray[dwIndex].Type = Command;
        pHelpArray[dwIndex].pszCommand = pCommand->pwszCmdToken;
        pHelpArray[dwIndex].dwHelpId = pCommand->dwShortCmdHelpToken;
//        pHelpArray[dwIndex].Pointer.pCommand = pCommand;
        dwIndex++;
        pCommand = pCommand->pNext;
    }

    /* Add short groups */
    pGroup = pContext->pGroupListHead;
    while (pGroup != NULL)
    {
        pHelpArray[dwIndex].Type = Group;
        pHelpArray[dwIndex].pszCommand = pGroup->pwszCmdGroupToken;
        pHelpArray[dwIndex].dwHelpId = pGroup->dwShortCmdHelpToken;
//        pHelpArray[dwIndex].Pointer.pGroup = pGroup;
        dwIndex++;
        pGroup = pGroup->pNext;
    }

    /* Count short subcontexts */
    pSubContext = pContext->pSubContextHead;
    while (pSubContext != NULL)
    {
        pHelpArray[dwIndex].Type = SubContext;
        pHelpArray[dwIndex].pszCommand = pSubContext->pszContextName;
        pHelpArray[dwIndex].Pointer.pSubContext = pSubContext;
        dwIndex++;
        pSubContext = pSubContext->pNext;
    }

    qsort(pHelpArray, dwCount, sizeof(HELP_ENTRY), HelpCompare);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        switch (pHelpArray[dwIndex].Type)
        {
            case Command:
            case Group:
                if (LoadStringW(pContext->hModule, pHelpArray[dwIndex].dwHelpId, szBuffer, 80) == 0)
                    szBuffer[0] = UNICODE_NULL;
                ConPrintf(StdOut, L"%-15s - %s", pHelpArray[dwIndex].pszCommand, szBuffer);
                break;

            case SubContext:
                GetContextFullName(pHelpArray[dwIndex].Pointer.pSubContext, szBuffer, 80);
                ConPrintf(StdOut, L"%-15s - Changes to the \"%s\" context.\n", pHelpArray[dwIndex].pszCommand, szBuffer);
                break;
        }
    }

    if (pHelpArray)
        HeapFree(GetProcessHeap(), 0, pHelpArray);
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
