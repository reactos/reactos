#pragma once

#include "input.h"

typedef struct _LAYOUT_LIST_NODE
{
    DWORD dwKLID;           /* The physical KLID */
    WORD wSpecialId;        /* The special ID */
    LPWSTR pszText;         /* The layout text */
    LPWSTR pszFile;         /* The layout file */
    LPWSTR pszImeFile;      /* The IME file */

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
