#pragma once

#include "input.h"
#include "locale_list.h"
#include "layout_list.h"

/*
 * INPUT_LIST_NODE_FLAG_EDITED
 * --- The modification flag. Since previous time, this entry is modified.
 */
#define INPUT_LIST_NODE_FLAG_EDITED    0x0001

/*
 * INPUT_LIST_NODE_FLAG_ADDED
 * --- The addition flag. Since previous time, this entry is newly added.
 */
#define INPUT_LIST_NODE_FLAG_ADDED     0x0002

/*
 * INPUT_LIST_NODE_FLAG_DELETED
 * --- The deletion flag.
 *     The application should ignore the entry with this flag if necessary.
 */
#define INPUT_LIST_NODE_FLAG_DELETED   0x0004

/*
 * INPUT_LIST_NODE_FLAG_DEFAULT
 * --- The default flag. The entry with this flag should be single in the list.
 */
#define INPUT_LIST_NODE_FLAG_DEFAULT   0x0008

typedef struct _INPUT_LIST_NODE
{
    WORD wFlags;

    LOCALE_LIST_NODE *pLocale;
    LAYOUT_LIST_NODE *pLayout;

    HKL hkl; /* Only for loaded input methods */

    LPWSTR pszIndicator;

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
InputList_RemoveByLang(LANGID wLangId);

VOID
InputList_Sort(VOID);

VOID
InputList_Destroy(VOID);

INPUT_LIST_NODE*
InputList_GetFirst(VOID);
