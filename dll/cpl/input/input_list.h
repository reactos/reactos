#pragma once

#include "input.h"
#include "locale_list.h"
#include "layout_list.h"


#define INPUT_LIST_NODE_FLAG_EDITED    0x0001
#define INPUT_LIST_NODE_FLAG_ADDED     0x0002
#define INPUT_LIST_NODE_FLAG_DELETED   0x0004
#define INPUT_LIST_NODE_FLAG_DEFAULT   0x0008


typedef struct _INPUT_LIST_NODE
{
    WORD wFlags;

    LOCALE_LIST_NODE *pLocale;
    LAYOUT_LIST_NODE *pLayout;

    HKL hkl; /* Only for loaded input methods */

    WCHAR *pszIndicator;

    struct _INPUT_LIST_NODE *pPrev;
    struct _INPUT_LIST_NODE *pNext;
} INPUT_LIST_NODE;


VOID
InputList_Create(VOID);

BOOL
InputList_Process(VOID);

BOOL
InputList_Add(LOCALE_LIST_NODE *pLocale, LAYOUT_LIST_NODE *pLayout);

VOID
InputList_SetDefault(INPUT_LIST_NODE *pNode);

VOID
InputList_Remove(INPUT_LIST_NODE *pNode);

VOID
InputList_Destroy(VOID);

INPUT_LIST_NODE*
InputList_GetFirst(VOID);
