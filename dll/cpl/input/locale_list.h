#pragma once

#include "input.h"

typedef struct _LOCALE_LIST_NODE
{
    WCHAR *pszName;

    DWORD dwId;

    struct _LOCALE_LIST_NODE *pPrev;
    struct _LOCALE_LIST_NODE *pNext;
} LOCALE_LIST_NODE;

LOCALE_LIST_NODE*
LocaleList_Create(VOID);

VOID
LocaleList_Destroy(VOID);

LOCALE_LIST_NODE*
LocaleList_GetByHkl(HKL hkl);

LOCALE_LIST_NODE*
LocaleList_GetFirst(VOID);
