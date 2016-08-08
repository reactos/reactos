#pragma once

#include "input.h"
#include "locale_list.h"
#include "layout_list.h"


#define INPUT_LIST_NODE_FLAG_EDITED    0x00000001
#define INPUT_LIST_NODE_FLAG_ADDED     0x00000002
#define INPUT_LIST_NODE_FLAG_DELETED   0x00000004
#define INPUT_LIST_NODE_FLAG_DEFAULT   0x00000008


typedef struct _INPUT_LIST_NODE
{
    DWORD dwFlags;

    LOCALE_LIST_NODE *pLocale;
    LAYOUT_LIST_NODE *pLayout;

    HKL hkl;

    struct _INPUT_LIST_NODE *pPrev;
    struct _INPUT_LIST_NODE *pNext;
} INPUT_LIST_NODE;


VOID
InputList_Create(VOID);

VOID
InputList_Process(VOID);

VOID
InputList_Add(LOCALE_LIST_NODE *pLocale, LAYOUT_LIST_NODE *pLayout);

VOID
InputList_Remove(INPUT_LIST_NODE *pNode);

VOID
InputList_Destroy(VOID);

INPUT_LIST_NODE*
InputList_Get(VOID);
