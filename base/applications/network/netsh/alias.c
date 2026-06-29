/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell alias management functions
 * COPYRIGHT:  Copyright 2025 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _ALIAS_ENTRY
{
    struct _ALIAS_ENTRY *pPrev;
    struct _ALIAS_ENTRY *pNext;

    PWSTR pszAliasName;
    PWSTR pszAlias;
} ALIAS_ENTRY, *PALIAS_ENTRY;

PALIAS_ENTRY AliasListHead = NULL;
PALIAS_ENTRY AliasListTail = NULL;


/* FUNCTIONS ******************************************************************/

static
VOID
ShowAliases(VOID)
{
    PALIAS_ENTRY pAliasEntry = NULL;

    DPRINT1("ShowAliases()\n");

    pAliasEntry = AliasListHead;
    while (pAliasEntry)
    {
        ConPrintf(StdOut, L"%s :  %s\n", pAliasEntry->pszAliasName, pAliasEntry->pszAlias);
        pAliasEntry = pAliasEntry->pNext;
    }
}


static
VOID
ShowAlias(
    PWSTR pszAliasName)
{
    PALIAS_ENTRY pAliasEntry = NULL;

    DPRINT1("ShowAlias(%S)\n", pszAliasName);

    pAliasEntry = AliasListHead;
    while (pAliasEntry)
    {
        if (wcscmp(pAliasEntry->pszAliasName, pszAliasName) == 0)
        {
            ConPrintf(StdOut, L"%s\n", pAliasEntry->pszAlias);
            return;
        }

        pAliasEntry = pAliasEntry->pNext;
    }

    ConResPrintf(StdOut, IDS_ALIAS_NOT_FOUND, pszAliasName);
}


static
PALIAS_ENTRY
GetAliasEntry(
    PWSTR pszAliasName)
{
    PALIAS_ENTRY pAliasEntry = NULL;

    pAliasEntry = AliasListHead;
    while (pAliasEntry)
    {
        if (wcscmp(pAliasEntry->pszAliasName, pszAliasName) == 0)
            return pAliasEntry;

        pAliasEntry = pAliasEntry->pNext;
    }

    return NULL;
}


VOID
InitAliases(VOID)
{
    AliasListHead = NULL;
    AliasListTail = NULL;
}


VOID
DestroyAliases(VOID)
{
    PALIAS_ENTRY pAliasEntry;

    while (AliasListHead != NULL)
    {
        pAliasEntry = AliasListHead;
        AliasListHead = AliasListHead->pNext;

        HeapFree(GetProcessHeap(), 0, pAliasEntry->pszAliasName);
        HeapFree(GetProcessHeap(), 0, pAliasEntry->pszAlias);
        HeapFree(GetProcessHeap(), 0, pAliasEntry);
    }

    AliasListTail = NULL;
}


DWORD
WINAPI
AliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PALIAS_ENTRY pAliasEntry = NULL;
    PWSTR pszAlias;

    DPRINT("AliasCommand(dwCurrentIndex %lu dwArgCount %lu)\n",
           dwCurrentIndex, dwArgCount);

    /* Show aliases */
    if (dwArgCount - dwCurrentIndex == 0)
    {
        ShowAliases();
        return ERROR_SUCCESS;
    }

    if (dwArgCount - dwCurrentIndex == 1)
    {
        ShowAlias(argv[dwCurrentIndex]);
        return ERROR_SUCCESS;
    }


    /* TODO: Check builtin commands */


    pAliasEntry = GetAliasEntry(argv[dwCurrentIndex]);
    if (pAliasEntry)
    {
        pszAlias = MergeStrings(&argv[dwCurrentIndex + 1], dwArgCount - dwCurrentIndex - 1);
        DPRINT("Alias: %S\n", pszAlias);
        if (pszAlias == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        if (pAliasEntry->pszAlias)
            HeapFree(GetProcessHeap(), 0, pAliasEntry->pszAlias);

        pAliasEntry->pszAlias = pszAlias;
    }
    else
    {
        pAliasEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ALIAS_ENTRY));
        if (pAliasEntry == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        pAliasEntry->pszAliasName = HeapAlloc(GetProcessHeap(), 0,
                                              (wcslen(argv[dwCurrentIndex]) + 1) * sizeof(WCHAR));
        if (pAliasEntry->pszAliasName == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pAliasEntry);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pAliasEntry->pszAliasName, argv[dwCurrentIndex]);
        DPRINT("AliasName: %S\n", pAliasEntry->pszAliasName);

        pAliasEntry->pszAlias = MergeStrings(&argv[dwCurrentIndex + 1], dwArgCount - dwCurrentIndex - 1);
        DPRINT("Alias: %S\n", pAliasEntry->pszAlias);
        if (pAliasEntry->pszAlias == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pAliasEntry->pszAliasName);
            HeapFree(GetProcessHeap(), 0, pAliasEntry);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (AliasListHead == NULL)
        {
            AliasListHead = pAliasEntry;
            AliasListTail = pAliasEntry;
        }
        else
        {
            pAliasEntry->pPrev = AliasListTail;
            AliasListTail->pNext = pAliasEntry;
            AliasListTail = pAliasEntry;
        }
    }

    DPRINT("Done\n");

    return ERROR_OKAY;
}


DWORD
WINAPI
ShowAliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    DPRINT("ShowAliasCommand()\n");

    ShowAliases();

    return ERROR_SUCCESS;
}


DWORD
WINAPI
UnaliasCommand(
    LPCWSTR pwszMachine,
    LPWSTR *argv,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PALIAS_ENTRY pAliasEntry = NULL;

    DPRINT("UnaliasCommand()\n");

    if (dwArgCount - dwCurrentIndex != 1)
        return ERROR_INVALID_SYNTAX;

    DPRINT("Alias %S\n", argv[dwCurrentIndex]);

    pAliasEntry = AliasListHead;
    while (pAliasEntry)
    {
        if (wcscmp(pAliasEntry->pszAliasName, argv[dwCurrentIndex]) == 0)
        {
            DPRINT("Alias found %S\n", argv[dwCurrentIndex]);

            if (pAliasEntry->pNext != NULL)
                pAliasEntry->pNext->pPrev = pAliasEntry->pPrev;
            else
                AliasListTail = pAliasEntry->pPrev;

            if (pAliasEntry->pPrev != NULL)
                pAliasEntry->pPrev->pNext = pAliasEntry->pNext;
            else
                AliasListHead = pAliasEntry->pNext;

            HeapFree(GetProcessHeap(), 0, pAliasEntry->pszAliasName);
            HeapFree(GetProcessHeap(), 0, pAliasEntry->pszAlias);
            HeapFree(GetProcessHeap(), 0, pAliasEntry);

            return ERROR_SUCCESS;
        }

        pAliasEntry = pAliasEntry->pNext;
    }

    ConResPrintf(StdOut, IDS_ALIAS_NOT_FOUND, argv[dwCurrentIndex]);

    return ERROR_SUCCESS;
}
