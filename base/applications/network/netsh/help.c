/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell builtin help command and support functions
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
               Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
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
    DPRINT("%s()\n", __FUNCTION__);
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


/*
Displays help for a context, by listing the available
commands, groups, and sub-contexts available
*/
VOID
HelpContext(
    PCONTEXT_ENTRY pContext)
{
    DPRINT("%s()\n", __FUNCTION__);
    PCONTEXT_ENTRY pSubContext;
    PCOMMAND_ENTRY pCommand;
    PCOMMAND_GROUP pGroup;
    WCHAR szBuffer[80];
    BOOL bDontSkip = TRUE;
    PCONTEXT_ENTRY pRootContext = GetRootContext();
    STACK *pContextStack = NULL;
    
    pContextStack = CreateStack();
    
    if (pContextStack == NULL)
    {
        DPRINT1("%s Cannot create stack\n", __FUNCTION__);
        return;
    }
    
    // Backtrack to root node
    do
    {
        StackPush(pContextStack, pContext);
        pContext = pContext->pParentContext;
    }while (pContext);
    
    // process the contexts    
    while (!IsStackEmpty(pContextStack))
    {   
        pContext = StackPop(pContextStack);
        if (pContext == NULL)
        {
            StackFree(pContextStack, FALSE);
            return;
        }
                
        if (pContext == GetCurrentContext())
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
            ConPrintf(StdOut, L"\nCommands in the %s context:\n", szBuffer);
        }

        // process commands 
        pCommand = pContext->pCommandListHead;
        while (pCommand != NULL)
        {
            if (pCommand->pOsVersionCheck != NULL)
            {
                DPRINT1("%s CallOsVersionCheck(pCommand->pOsVersionCheck\n", __FUNCTION__);
                bDontSkip  = CallOsVersionCheck(pCommand->pOsVersionCheck);
            }
            else
            {
                bDontSkip = TRUE;
            }
      
            if(bDontSkip)
            {
                if (LoadStringW(pCommand->hModule, pCommand->dwShortCmdHelpToken, szBuffer, 80) == 0)
                    szBuffer[0] = UNICODE_NULL;
                ConPrintf(StdOut, L"%-15s - %s", pCommand->pwszCmdToken, szBuffer);
            }
        
            pCommand = pCommand->pNext;
        }

        // process groups
        pGroup = pContext->pGroupListHead;
        while (pGroup != NULL)
        {
            if (pGroup->pOsVersionCheck != NULL)
            {
                DPRINT1("%s CallOsVersionCheck(pGroup->pOsVersionCheck)\n", __FUNCTION__);
                bDontSkip = CallOsVersionCheck(pGroup->pOsVersionCheck);
            }
            else
            {
                bDontSkip = TRUE;
            }
      
            if (bDontSkip)
            {
                if (LoadStringW(pGroup->hModule, pGroup->dwShortCmdHelpToken, szBuffer, 80) == 0)
                    szBuffer[0] = UNICODE_NULL;
                ConPrintf(StdOut, L"%-15s - %s", pGroup->pwszCmdGroupToken, szBuffer);
            }
        
            pGroup = pGroup->pNext;
    
        }
        DPRINT1("%s pSubContext = pContext->pSubContextHead;\n", __FUNCTION__);
        pSubContext = pContext->pSubContextHead;
        while (pSubContext != NULL)
        {
            DPRINT1("%s In while (pSubContext != NULL) loop\n", __FUNCTION__);
            GetContextFullName(pSubContext, szBuffer, 80);
            ConPrintf(StdOut, L"%-15s - Changes to the \"%s\" context.\n", pSubContext->pszContextName, szBuffer);
            pSubContext = pSubContext->pNext;
        }
    }
    
    StackFree(pContextStack, FALSE);
    
    pSubContext = pContext->pSubContextHead;
    
    if (pSubContext != NULL)
    {
        // TODO localize
        GetContextFullName(pSubContext, szBuffer, 80);
        ConPrintf(StdOut, L"\nThe following sub-contexts are available:\n");
        
        while (pSubContext != NULL)
        {
            ConPrintf(StdOut, L" %s", pSubContext->pszContextName, szBuffer);
            pSubContext = pSubContext->pNext;
        }
        
        ConPrintf(StdOut, L"\n");
    }
    
    // TODO localize
    ConPrintf(StdOut, L"\nTo view help for a command, type the command, followed by a space, and then\n type ?.\n\n");
    
}

// shows help for a command group
VOID
HelpGroup(
    PCOMMAND_GROUP pGroup)
{
    DPRINT("%s()\n", __FUNCTION__);
    PCOMMAND_ENTRY pCommand;
    WCHAR szBuffer[1024]; // todo allocate dynamically

    ConResPrintf(StdOut, IDS_HELP_HEADER);
    ConPrintf(StdOut, L"\nCommands in this context:\n");

    pCommand = pGroup->pCommandListHead;
    
    if(pCommand == NULL)
        ConPuts(StdOut, L"\n");
    
    // Follow the linked list until we run out of nodes    
    while (pCommand != NULL)
    {
        // Create a temporary array that holds a concatenated string containing  pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken
        // for every entry, once thats done sort the array in lexicographical order
        // display the results
        swprintf(szBuffer, L"%s %s", pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken);
        ConPrintf(StdOut, L"%-15s - ", szBuffer);
        PrintMessageFromModule(pGroup->hModule, pCommand->dwShortCmdHelpToken);
        pCommand = pCommand->pNext;
    }
    
}

/*


// shows help for a command group
VOID
HelpGroup(
    PCOMMAND_GROUP pGroup)
{
    DPRINT("%s()\n", __FUNCTION__);
    PCOMMAND_ENTRY pCommand;
    WCHAR szBuffer[64];

    ConResPrintf(StdOut, IDS_HELP_HEADER);

    ConPrintf(StdOut, L"\nCommands in this context:\n");

    pCommand = pGroup->pCommandListHead;
    
    if(pCommand == NULL)
        ConPuts(StdOut, L"\n");
    
    // Follow the linked list until we run out of nodes    
    while (pCommand != NULL)
    {
        swprintf(szBuffer, L"%s %s", pGroup->pwszCmdGroupToken, pCommand->pwszCmdToken);
        ConPrintf(StdOut, L"%-15s - ", szBuffer);
        PrintMessageFromModule(pGroup->hModule, pCommand->dwShortCmdHelpToken);
        pCommand = pCommand->pNext;
    }
    
}






*/
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
    DPRINT("%s()\n", __FUNCTION__);
    PCONTEXT_ENTRY pContext;

    ConResPrintf(StdOut, IDS_HELP_HEADER);

    pContext = GetCurrentContext();
    if (pContext == NULL)
    {
        DPRINT1("HelpCommand: invalid context %p\n", pContext);
        return 1;
    }

    HelpContext(pContext);

    if (GetCurrentContext()->pSubContextHead != NULL)
    {
        ConResPrintf(StdOut, IDS_SUBCONTEXT_HEADER);
        pContext = GetCurrentContext()->pSubContextHead;
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
