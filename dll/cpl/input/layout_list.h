#pragma once

#include "input.h"

typedef struct _LAYOUT_LIST_NODE
{
    WCHAR *pszName;

    DWORD dwId;
    DWORD dwSpecialId;

    struct _LAYOUT_LIST_NODE *pPrev;
    struct _LAYOUT_LIST_NODE *pNext;
} LAYOUT_LIST_NODE;

VOID
LayoutList_Create(VOID);

VOID
LayoutList_Destroy(VOID);

LAYOUT_LIST_NODE*
LayoutList_GetByHkl(HKL hkl);

LAYOUT_LIST_NODE*
LayoutList_GetFirst(VOID);
