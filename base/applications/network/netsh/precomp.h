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

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>

#include <errno.h>

#include <conutils.h>
#include <netsh.h>

#include "resource.h"


/* DEFINES *******************************************************************/

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

    PDLL_LIST_ENTRY pDllEntry;
    BOOL bStarted;

    struct _HELPER_ENTRY *pSubHelperHead;
    struct _HELPER_ENTRY *pSubHelperTail;

} HELPER_ENTRY, *PHELPER_ENTRY;



typedef struct _COMMAND_ENTRY
{
    struct _COMMAND_ENTRY *pPrev;
    struct _COMMAND_ENTRY *pNext;

    LPCWSTR pwszCmdToken;
    PFN_HANDLE_CMD pfnCmdHandler;
    DWORD dwShortCmdHelpToken;
    DWORD dwCmdHlpToken;
    DWORD dwFlags;
} COMMAND_ENTRY, *PCOMMAND_ENTRY;

typedef struct _COMMAND_GROUP
{
    struct _COMMAND_GROUP *pPrev;
    struct _COMMAND_GROUP *pNext;

    LPCWSTR pwszCmdGroupToken;
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

    PWSTR pszContextName;
    GUID Guid;
    HMODULE hModule;

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


/* PROTOTYPES *****************************************************************/

/* context.c */

BOOL
CreateRootContext(VOID);


/* help.c */
DWORD
WINAPI
HelpCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone);

VOID
HelpGroup(
    PCOMMAND_GROUP pGroup);


/* helper.c */
VOID
LoadHelpers(VOID);

VOID
UnloadHelpers(VOID);


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
BOOL
InterpretScript(
    LPWSTR pszFileName);

BOOL
InterpretCommand(
    LPWSTR *argv,
    DWORD dwArgCount);

VOID
InterpretInteractive(VOID);

#endif /* PRECOMP_H */
