/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/settings.c
 * PURPOSE:         Device settings support functions
 * PROGRAMMERS:     Eric Kohl
 *                  Colin Finck
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

ULONG DefaultLanguageIndex = 0;

/* FUNCTIONS ****************************************************************/

ULONG
GetDefaultLanguageIndex(VOID)
{
    return DefaultLanguageIndex;
}

typedef struct _LANG_ENTRY_PARAM
{
    ULONG uIndex;
    PWCHAR DefaultLanguage;
} LANG_ENTRY_PARAM, *PLANG_ENTRY_PARAM;

static UCHAR
NTAPI
ProcessLangEntry(
    IN PWCHAR KeyName,
    IN PWCHAR KeyValue,
    IN PCHAR DisplayText,
    IN SIZE_T DisplayTextSize,
    OUT PVOID* UserData,
    OUT PBOOLEAN Current,
    IN PVOID Parameter OPTIONAL)
{
    PLANG_ENTRY_PARAM LangEntryParam = (PLANG_ENTRY_PARAM)Parameter;

    if (!IsLanguageAvailable(KeyName))
    {
        /* The specified language is unavailable, skip the entry */
        return 2;
    }

    *UserData = RtlAllocateHeap(ProcessHeap, 0,
                                (wcslen(KeyName) + 1) * sizeof(WCHAR));
    if (*UserData == NULL)
    {
        /* Failure, stop enumeration */
        DPRINT1("RtlAllocateHeap() failed\n");
        return 0;
    }

    wcscpy((PWCHAR)*UserData, KeyName);
    sprintf(DisplayText, "%S", KeyValue);

    *Current = FALSE;

    if (!_wcsicmp(KeyName, LangEntryParam->DefaultLanguage))
        DefaultLanguageIndex = LangEntryParam->uIndex;

    LangEntryParam->uIndex++;

    /* Add the entry */
    return 1;
}

PGENERIC_LIST
CreateLanguageList(
    HINF InfFile,
    WCHAR *DefaultLanguage)
{
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyValue;

    LANG_ENTRY_PARAM LangEntryParam;

    LangEntryParam.uIndex = 0;
    LangEntryParam.DefaultLanguage = DefaultLanguage;

    /* Get default language id */
    if (!SetupFindFirstLineW(InfFile, L"NLS", L"DefaultLanguage", &Context))
        return NULL;

    if (!INF_GetData(&Context, NULL, &KeyValue))
        return NULL;

    wcscpy(DefaultLanguage, KeyValue);
    SelectedLanguageId = KeyValue;

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    if (AddEntriesFromInfSection(List,
                                 InfFile,
                                 L"Language",
                                 &Context,
                                 ProcessLangEntry,
                                 &LangEntryParam) == -1)
    {
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    /* Only one language available, make it the default one */
    if (LangEntryParam.uIndex == 1)
    {
        DefaultLanguageIndex = 0;
        wcscpy(DefaultLanguage,
               (PWSTR)GetListEntryUserData(GetFirstListEntry(List)));
    }

    return List;
}


PGENERIC_LIST
CreateKeyboardLayoutList(
    HINF InfFile,
    WCHAR *DefaultKBLayout)
{
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyValue;
    const MUI_LAYOUTS * LayoutsList;
    ULONG uIndex = 0;

    /* Get default layout id */
    if (!SetupFindFirstLineW(InfFile, L"NLS", L"DefaultLayout", &Context))
        return NULL;

    if (!INF_GetData(&Context, NULL, &KeyValue))
        return NULL;

    wcscpy(DefaultKBLayout, KeyValue);

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    LayoutsList = MUIGetLayoutsList();

    do
    {
        // NOTE: See https://svn.reactos.org/svn/reactos?view=revision&revision=68354
        if (AddEntriesFromInfSection(List,
                                     InfFile,
                                     L"KeyboardLayout",
                                     &Context,
                                     DefaultProcessEntry,
                                     DefaultKBLayout) == -1)
        {
            DestroyGenericList(List, TRUE);
            return NULL;
        }

        uIndex++;

    } while (LayoutsList[uIndex].LangID != NULL);

    /* Check whether some keyboard layouts have been found */
    /* FIXME: Handle this case */
    if (GetNumberOfListEntries(List) == 0)
    {
        DPRINT1("No keyboard layouts have been found\n");
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    return List;
}


BOOLEAN
ProcessKeyboardLayoutRegistry(
    PGENERIC_LIST List)
{
    PGENERIC_LIST_ENTRY Entry;
    PWCHAR LayoutId;
    const MUI_LAYOUTS * LayoutsList;
    MUI_LAYOUTS NewLayoutsList[20];
    ULONG uIndex;
    ULONG uOldPos = 0;

    Entry = GetCurrentListEntry(List);
    if (Entry == NULL)
        return FALSE;

    LayoutId = (PWCHAR)GetListEntryUserData(Entry);
    if (LayoutId == NULL)
        return FALSE;

    LayoutsList = MUIGetLayoutsList();

    if (_wcsicmp(LayoutsList[0].LayoutID, LayoutId) != 0)
    {
        for (uIndex = 1; LayoutsList[uIndex].LangID != NULL; uIndex++)
        {
            if (_wcsicmp(LayoutsList[uIndex].LayoutID, LayoutId) == 0)
            {
                uOldPos = uIndex;
                continue;
            }

            NewLayoutsList[uIndex].LangID   = LayoutsList[uIndex].LangID;
            NewLayoutsList[uIndex].LayoutID = LayoutsList[uIndex].LayoutID;
        }

        NewLayoutsList[uIndex].LangID    = NULL;
        NewLayoutsList[uIndex].LayoutID  = NULL;
        NewLayoutsList[uOldPos].LangID   = LayoutsList[0].LangID;
        NewLayoutsList[uOldPos].LayoutID = LayoutsList[0].LayoutID;
        NewLayoutsList[0].LangID         = LayoutsList[uOldPos].LangID;
        NewLayoutsList[0].LayoutID       = LayoutsList[uOldPos].LayoutID;

        return AddKbLayoutsToRegistry(NewLayoutsList);
    }

    return TRUE;
}

#if 0
BOOLEAN
ProcessKeyboardLayoutFiles(
    PGENERIC_LIST List)
{
    return TRUE;
}
#endif

/* EOF */
