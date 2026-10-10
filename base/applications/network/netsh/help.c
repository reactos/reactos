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

#define HUGE_HELP_BUFFER_SIZE  2048
#define SMALL_HELP_BUFFER_SIZE  160
#define TINY_HELP_BUFFER_SIZE    80

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
    if (pContext == pCurrentContext)
    {
        ConResPrintf(StdOut, IDS_THIS_COMMANDS);
    }
    else
    {
        WCHAR szBuffer[SMALL_HELP_BUFFER_SIZE];
        GetContextFullName(pContext, szBuffer, _countof(szBuffer));
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
    WCHAR szBuffer1[TINY_HELP_BUFFER_SIZE];
    WCHAR szBuffer2[SMALL_HELP_BUFFER_SIZE];

    for (pCommand = pGroup->pCommandListHead; pCommand != NULL; pCommand = pCommand->pNext)
    {
        DPRINT("CheckVersion (Command) %S %S", pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken);
        if (!CheckOsVersion(pCommand->pfnOsVersionCheck))
            continue;

        if (((pCommand->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pCommand->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pCommand->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        _swprintf(szBuffer1, L"%s %s", pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken);
        LoadStringW(pContext->hModule, pCommand->dwShortCmdHelpToken, szBuffer2, _countof(szBuffer2));

        ConPrintf(StdOut, L"%-15s - %s", szBuffer1, szBuffer2);
    }
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
    WCHAR szBuffer[SMALL_HELP_BUFFER_SIZE];

    DPRINT("PrintContext()\n");

    if (pContext != pRootContext)
        PrintContext(pContext->pParentContext);

    PrintCurrentContextHeader(pContext);

    /* Count short commands */
    for (pCommand = pContext->pCommandListHead; pCommand != NULL; pCommand = pCommand->pNext)
    {
        if (!CheckOsVersion(pCommand->pfnOsVersionCheck))
            continue;

        if (((pCommand->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pCommand->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pCommand->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        dwCount++;
    }

    /* Count short groups */
    for (pGroup = pContext->pGroupListHead; pGroup != NULL; pGroup = pGroup->pNext)
    {
        if (!CheckOsVersion(pGroup->pfnOsVersionCheck))
            continue;

        if (((pGroup->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pGroup->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pGroup->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        dwCount++;
    }

    /* Count short subcontexts */
    for (pSubContext = pContext->pSubContextHead; pSubContext != NULL; pSubContext = pSubContext->pNext)
    {
        if (!CheckOsVersion(pSubContext->pfnOsVersionCheck))
            continue;

        if (((pSubContext->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pSubContext->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pSubContext->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        dwCount++;
    }

    pHelpArray = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCount * sizeof(HELP_ENTRY));
    if (pHelpArray == NULL)
        return;

    dwIndex = 0;

    /* Add short commands */
    for (pCommand = pContext->pCommandListHead; pCommand != NULL; pCommand = pCommand->pNext)
    {
        if (!CheckOsVersion(pCommand->pfnOsVersionCheck))
            continue;

        if (((pCommand->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pCommand->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pCommand->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        pHelpArray[dwIndex].Type = Command;
        pHelpArray[dwIndex].pszCommand = pCommand->pwszCmdToken;
        pHelpArray[dwIndex].dwHelpId = pCommand->dwShortCmdHelpToken;
        pHelpArray[dwIndex].Pointer.pCommand = pCommand;
        dwIndex++;
    }

    /* Add short groups */
    for (pGroup = pContext->pGroupListHead; pGroup != NULL; pGroup = pGroup->pNext)
    {
        if (!CheckOsVersion(pGroup->pfnOsVersionCheck))
            continue;

        if (((pGroup->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pGroup->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pGroup->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        pHelpArray[dwIndex].Type = Group;
        pHelpArray[dwIndex].pszCommand = pGroup->pwszCmdGroupToken;
        pHelpArray[dwIndex].dwHelpId = pGroup->dwShortCmdHelpToken;
        pHelpArray[dwIndex].Pointer.pGroup = pGroup;
        dwIndex++;
    }

    /* Count short subcontexts */
    for (pSubContext = pContext->pSubContextHead; pSubContext != NULL; pSubContext = pSubContext->pNext)
    {
        if (!CheckOsVersion(pSubContext->pfnOsVersionCheck))
            continue;

        if (((pSubContext->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pSubContext->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pSubContext->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        pHelpArray[dwIndex].Type = SubContext;
        pHelpArray[dwIndex].pszCommand = pSubContext->pszContextName;
        pHelpArray[dwIndex].Pointer.pSubContext = pSubContext;
        dwIndex++;
    }

    qsort(pHelpArray, dwCount, sizeof(HELP_ENTRY), HelpCompare);

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        switch (pHelpArray[dwIndex].Type)
        {
            case Command:
                if (LoadStringW(pContext->hModule, pHelpArray[dwIndex].dwHelpId, szBuffer, _countof(szBuffer)) == 0)
                    szBuffer[0] = UNICODE_NULL;
                ConPrintf(StdOut, L"%-15s - %s", pHelpArray[dwIndex].pszCommand, szBuffer);
                break;

            case Group:
                if (LoadStringW(pContext->hModule, pHelpArray[dwIndex].dwHelpId, szBuffer, _countof(szBuffer)) == 0)
                    szBuffer[0] = UNICODE_NULL;
                ConPrintf(StdOut, L"%-15s - %s", pHelpArray[dwIndex].pszCommand, szBuffer);
                break;

            case SubContext:
                GetContextFullName(pHelpArray[dwIndex].Pointer.pSubContext, szBuffer, _countof(szBuffer));
                ConPrintf(StdOut, L"%-15s - Changes to the \"%s\" context.\n", pHelpArray[dwIndex].pszCommand, szBuffer);
                break;
        }
    }

    if (pHelpArray)
        HeapFree(GetProcessHeap(), 0, pHelpArray);
}


static
int
SubContextCompare(
    _In_ const void *p1,
    _In_ const void *p2)
{
    return _wcsicmp((*((PCONTEXT_ENTRY*)p1))->pszContextName, (*((PCONTEXT_ENTRY*)p2))->pszContextName);
}


static
VOID
PrintSubcontexts(
    _In_ PCONTEXT_ENTRY pContext)
{
    PCONTEXT_ENTRY pSubContext, *pSubContextArray = NULL;
    DWORD dwCount, dwIndex;

    if (pContext->pSubContextHead == NULL)
        return;

    dwCount = 0;
    for (pSubContext = pContext->pSubContextHead; pSubContext != NULL; pSubContext = pSubContext->pNext)
    {
        if (!CheckOsVersion(pSubContext->pfnOsVersionCheck))
            continue;

        if (((pSubContext->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pSubContext->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pSubContext->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        dwCount++;
    }

    pSubContextArray = HeapAlloc(GetProcessHeap(), 0, dwCount * sizeof(PCONTEXT_ENTRY));
    if (pSubContextArray == NULL)
        return;

    dwIndex = 0;
    for (pSubContext = pContext->pSubContextHead; pSubContext != NULL; pSubContext = pSubContext->pNext)
    {
        if (!CheckOsVersion(pSubContext->pfnOsVersionCheck))
            continue;

        if (((pSubContext->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
            ((pSubContext->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
            (pSubContext->dwFlags & CMD_FLAG_HIDDEN))
            continue;

        pSubContextArray[dwIndex] = pSubContext;
        dwIndex++;
    }
  
    qsort(pSubContextArray, dwCount, sizeof(PCONTEXT_ENTRY), SubContextCompare);

    ConResPrintf(StdOut, IDS_SUBCONTEXT_HEADER);
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        ConPrintf(StdOut, L" %s", pSubContextArray[dwIndex]->pszContextName);
    }
    ConPuts(StdOut, L"\n");

    HeapFree(GetProcessHeap(), 0, pSubContextArray);
}


VOID
PrintCommandHelp(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ PCOMMAND_GROUP pGroup,
    _In_ PCOMMAND_ENTRY pCommand)
{
    LPWSTR pszCommandBuffer;
    DWORD_PTR Args[2];
    DWORD dwLength = 1;

    DPRINT("PrintCommandHelp(%p %p %p)\n", pContext, pGroup, pCommand);

    if (!CheckOsVersion(pCommand->pfnOsVersionCheck))
        return;

    if (((pCommand->dwFlags & CMD_FLAG_LOCAL) && (g_pszMachine != NULL)) ||
        ((pCommand->dwFlags & CMD_FLAG_ONLINE) && (g_bOnline == FALSE)) ||
        (pCommand->dwFlags & CMD_FLAG_HIDDEN))
        return;

    dwLength += wcslen(pCommand->pwszCmdToken);
    if (pGroup)
        dwLength += (wcslen(pGroup->pwszCmdGroupToken) + 1);

    pszCommandBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength * sizeof(WCHAR));
    if (pszCommandBuffer == NULL)
        return;

    _swprintf(pszCommandBuffer, L"%s%s%s",
              (pGroup) ? pGroup->pwszCmdGroupToken : L"",
              (pGroup) ? L" " : L"",
              pCommand->pwszCmdToken);

    Args[0] = (DWORD_PTR)pszCommandBuffer;
    Args[1] = (DWORD_PTR)NULL;

    ConResMsgPrintfExV(StdOut,
                       pContext->hModule,
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       pCommand->dwCmdHlpToken,
                       LANG_USER_DEFAULT,
                       (va_list *)&Args);
    ConPuts(StdOut, L"\n");

    HeapFree(GetProcessHeap(), 0, pszCommandBuffer);
}


VOID
PrintGroupHelp(
    _In_ PCONTEXT_ENTRY pContext,
    _In_ LPWSTR pszGroupName,
    _In_ BOOL bRecurse)
{
    PCOMMAND_GROUP pGroup;

    if (bRecurse)
    {
        if (pContext != pRootContext)
            PrintGroupHelp(pContext->pParentContext, pszGroupName, bRecurse);
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


VOID
PrintContextHelp(
    _In_ PCONTEXT_ENTRY pContext)
{
    ConResPrintf(StdOut, IDS_HELP_HEADER);
    PrintContext(pContext);
    PrintSubcontexts(pContext);
    ConResPrintf(StdOut, IDS_HELP_FOOTER);
}
