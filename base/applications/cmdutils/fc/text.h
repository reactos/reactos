/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing text files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "fc.h"

#define IS_SPACEW(ch) ((ch) == L' ' || (ch) == L'\t')
#define IS_SPACEA(ch) ((ch) == ' ' || (ch) == '\t')

#ifdef UNICODE
    #define NODE NODE_W
    #define IS_SPACE IS_SPACEW
    #define PrintLine PrintLineW
    #define PrintLine2 PrintLine2W
    #define TextCompare TextCompareW
    #define last_match last_matchW
#else
    #define NODE NODE_A
    #define IS_SPACE IS_SPACEA
    #define PrintLine PrintLineA
    #define PrintLine2 PrintLine2A
    #define TextCompare TextCompareA
    #define last_match last_matchA
#endif

static BOOL FindNextLine(LPCTSTR pch, DWORD ich, DWORD cch, LPDWORD pich)
{
    while (ich < cch)
    {
        if (pch[ich] == TEXT('\n'))
        {
            *pich = ich;
            return TRUE;
        }
        ++ich;
    }
    *pich = cch;
    return FALSE;
}

static LPTSTR AllocLine(LPCTSTR pch, DWORD cch)
{
    LPTSTR pszNew = malloc((cch + 1) * sizeof(TCHAR));
    if (!pszNew)
        return NULL;
    memcpy(pszNew, pch, cch * sizeof(TCHAR));
    pszNew[cch] = 0;
    return pszNew;
}

static NODE *AllocNode(LPTSTR psz, DWORD cch, DWORD lineno)
{
    NODE *node;
    if (!psz)
        return NULL;
    node = calloc(1, sizeof(NODE));
    if (!node)
    {
        free(psz);
        return NULL;
    }
    node->pszLine = psz;
    node->cch = cch;
    node->lineno = lineno;
    return node;
}

static __inline VOID DeleteNode(NODE *node)
{
    if (node)
    {
        free(node->pszLine);
        free(node);
    }
}

static VOID DeleteList(struct list *list)
{
    struct list *ptr;
    NODE *node;
    while ((ptr = list_head(list)) != NULL)
    {
        list_remove(ptr);
        node = LIST_ENTRY(ptr, NODE, entry);
        DeleteNode(node);
    }
}

static __inline LPCTSTR SkipSpace(LPCTSTR pch)
{
    while (IS_SPACE(*pch))
        ++pch;
    return pch;
}

static __inline LPCTSTR LastNonSpace(LPCTSTR pch)
{
    LPCTSTR pchLast = NULL;
    while (*pch)
    {
        if (!IS_SPACE(*pch))
            pchLast = pch;
        ++pch;
    }
    return pchLast;
}

static LPTSTR CompressSpace(LPCTSTR line)
{
    LPTSTR pszNew, pch1, pch2;
    LPCTSTR pchLast;

    line = SkipSpace(line);
    pchLast = LastNonSpace(line);
    if (pchLast == NULL)
        return AllocLine(TEXT(""), 0);

    pszNew = AllocLine(line, (DWORD)(pchLast - line) + 1);
    if (!pszNew)
        return NULL;

    for (pch1 = pch2 = pszNew; *pch1; ++pch1)
    {
        *pch2++ = *pch1;
        if (IS_SPACE(*pch1))
        {
            // skip duplicated spaces
            do
            {
                ++pch1;
            } while (IS_SPACE(*pch1));
            --pch1;
        }
    }
    *pch2 = 0;
    return pszNew;
}

#define TAB_WIDTH 8

static INT ExpandTabLength(LPCTSTR line)
{
    LPCTSTR pch;
    INT cch = 0;
    for (pch = line; *pch; ++pch)
    {
        if (*pch == TEXT('\t'))
            cch += TAB_WIDTH - (cch % TAB_WIDTH);
        else
            ++cch;
    }
    return cch;
}

static LPTSTR ExpandTab(LPCTSTR line)
{
    INT spaces, cch = ExpandTabLength(line);
    LPTSTR pszNew = malloc((cch + 1) * sizeof(TCHAR));
    LPCTSTR pch;
    if (!pszNew)
        return NULL;
    cch = 0;
    for (pch = line; *pch; ++pch)
    {
        if (*pch == TEXT('\t'))
        {
            spaces = TAB_WIDTH - (cch % TAB_WIDTH);
            while (spaces-- > 0)
            {
                pszNew[cch++] = TEXT(' ');
            }
        }
        else
        {
            pszNew[cch++] = *pch;
        }
    }
    pszNew[cch] = 0;
    return pszNew;
}

static BOOL ExpandNode(const FILECOMPARE *pFC, NODE *node)
{
    if (!(pFC->dwFlags & FLAG_T))
    {
        LPTSTR tmp = ExpandTab(node->pszLine);
        if (!tmp)
            return FALSE;
        free(node->pszLine);
        node->pszLine = tmp;
        node->cch = lstrlen(tmp);
    }
    return TRUE;
}

static FCRET CompareNode(const FILECOMPARE *pFC, const NODE *node1, const NODE *node2)
{
    DWORD dwCmpFlags = ((pFC->dwFlags & FLAG_C) ? NORM_IGNORECASE : 0);
    LPTSTR line1 = node1->pszLine, line2 = node2->pszLine;
    DWORD cch1 = node1->cch, cch2 = node2->cch;
    INT ret;
    if (pFC->dwFlags & FLAG_W) // compress space
    {
        LPTSTR psz1 = CompressSpace(line1);
        LPTSTR psz2 = CompressSpace(line2);
        if (psz1 && psz2)
        {
            ret = CompareString(LOCALE_USER_DEFAULT, dwCmpFlags, psz1, -1, psz2, -1);
            ret = ((ret == CSTR_EQUAL) ? FCRET_IDENTICAL : FCRET_DIFFERENT);
        }
        else
        {
            ret = OutOfMemory();
        }
        free(psz1);
        free(psz2);
    }
    else
    {
        ret = CompareString(LOCALE_USER_DEFAULT, dwCmpFlags, line1, cch1, line2, cch2);
        ret = (ret == CSTR_EQUAL) ? FCRET_IDENTICAL : FCRET_DIFFERENT;
    }
    return ret;
}

static FCRET
ParseLines(const FILECOMPARE *pFC, HANDLE *phMapping,
           LARGE_INTEGER *pib, const LARGE_INTEGER *pcb, struct list *list)
{
    DWORD lineno = 1, ich, cch, ichNext, cbView, cchNode;
    LPTSTR psz;
    BOOL fLast, bCR;
    NODE *node;

    if (*phMapping == NULL)
        return FCRET_NO_MORE_DATA;

    if (pib->QuadPart >= pcb->QuadPart)
    {
        CloseHandle(*phMapping);
        *phMapping = NULL;
        return FCRET_NO_MORE_DATA;
    }

    cbView = (DWORD)min(pcb->QuadPart - pib->QuadPart, MAX_VIEW_SIZE);
    psz = MapViewOfFile(*phMapping, FILE_MAP_READ, pib->HighPart, pib->LowPart, cbView);
    if (!psz)
    {
        return OutOfMemory();
    }

    ich = 0;
    cch = cbView / sizeof(TCHAR);
    fLast = (pib->QuadPart + cbView >= pcb->QuadPart);
    while (FindNextLine(psz, ich, cch, &ichNext) ||
           (ichNext == cch && (fLast || ich == 0)))
    {
        bCR = (ichNext > 0) && (psz[ichNext - 1] == TEXT('\r'));
        cchNode = ichNext - ich - bCR;
        node = AllocNode(AllocLine(&psz[ich], cchNode), cchNode, lineno++);
        if (!node || !ExpandNode(pFC, node))
        {
            DeleteNode(node);
            UnmapViewOfFile(psz);
            return OutOfMemory();
        }
        list_add_tail(list, &node->entry);
        ich = ichNext + 1;
    }

    UnmapViewOfFile(psz);
    psz = NULL;

    pib->QuadPart += ichNext * sizeof(WCHAR);

    if (pib->QuadPart < pcb->QuadPart)
        return FCRET_IDENTICAL;
    return FCRET_NO_MORE_DATA;
}

static VOID ShowDiffAndClean(FILECOMPARE *pFC, struct list *ptr1, struct list *ptr2, INT i1, INT i2)
{
    struct list *next, *ptr[2] = { ptr1, ptr2 };
    INT ai[2] = { i1, i2 };
    BOOL fFirst;
    NODE *node;
    BOOL fAbbrev = (pFC->dwFlags & FLAG_A);

    for (INT i = 0; i < 2; ++i)
    {
        ShowCaption(pFC->file[i]);
        if (!ptr[i])
            continue;

        fFirst = TRUE;

        if (pFC->last_match[i])
        {
            PrintLine2(pFC, pFC->last_lineno[i], pFC->last_match[i]);
            fFirst = FALSE;
        }

        while (ptr[i] && ai[i]-- > 0)
        {
            node = LIST_ENTRY(ptr[i], NODE, entry);
            if (!fAbbrev || fFirst)
            {
                PrintLine(pFC, node);
                fFirst = FALSE;
            }
            next = list_next(&pFC->list[i], ptr[i]);

            // clean up
            DeleteNode(node);
            list_remove(ptr[i]);

            ptr[i] = next;
        }

        if (fAbbrev && !fFirst && ptr[i])
        {
            node = LIST_ENTRY(ptr[i], NODE, entry);
            PrintDots();
            PrintLine(pFC, node);
        }
    }
}

static FCRET
Finalize(FILECOMPARE *pFC, struct list *ptr1, struct list *ptr2, BOOL fDifferent)
{
    if (!ptr1 && !ptr2)
    {
        if (fDifferent)
        {
            ShowCaption(L"");
            return Different(pFC->file[0], pFC->file[1]);
        }
        return NoDifference();
    }

    ShowDiffAndClean(pFC, ptr1, ptr2, pFC->n, pFC->n);
    ShowCaption(L"");
    return FCRET_DIFFERENT;
}

static VOID
SaveLastMatch(FILECOMPARE *pFC, const NODE *node1, const NODE *node2)
{
    free(pFC->last_match[0]);
    free(pFC->last_match[1]);
    pFC->last_match[0] = AllocLine(node1->pszLine, lstrlen(node1->pszLine));
    pFC->last_match[1] = AllocLine(node2->pszLine, lstrlen(node2->pszLine));
    pFC->last_lineno[0] = node1->lineno;
    pFC->last_lineno[1] = node2->lineno;
}

static FCRET
SkipIdentical(FILECOMPARE *pFC, struct list **pptr1, struct list **pptr2)
{
    struct list *ptr1 = *pptr1, *ptr2 = *pptr2;
    while (ptr1 && ptr2)
    {
        NODE *node1 = LIST_ENTRY(ptr1, NODE, entry);
        NODE *node2 = LIST_ENTRY(ptr2, NODE, entry);
        FCRET ret = CompareNode(pFC, node1, node2);
        if (ret != FCRET_IDENTICAL)
            return ret;

        // save last match
        SaveLastMatch(pFC, node1, node2);

        ptr1 = list_next(&pFC->list[0], ptr1);
        ptr2 = list_next(&pFC->list[1], ptr2);
    }
    *pptr1 = ptr1;
    *pptr2 = ptr2;
    return FCRET_IDENTICAL;
}

static FCRET
Resynch(FILECOMPARE *pFC, struct list *head1, struct list *head2, INT *pi1, INT *pi2)
{
    FCRET ret;
    INT i1, i2, m, p, k;
    struct list *ptr1, *ptr2, *ptr3, *ptr4;
    NODE *node1, *node2;
    struct list *list1 = &pFC->list[0], *list2 = &pFC->list[1];
    INT n = min(pFC->n, max(list_count(list1), list_count(list2)));

    // ``If the files that you are comparing have more than pFC->n consecutive
    //   differing lines, FC cancels the comparison,,
    // ``If the number of matching lines in the files is less than pFC->nnnn,
    //   FC displays the matching lines as differences,,
    for (m = 0; m < n; ++m) // m in [0, pFC->n)
    {
        for (p = 0; p <= m; ++p) // p in [0, m]
        {
            // resume pointers
            ptr1 = head1;
            ptr2 = head2;
            // skip ptr1 (m - p) times if possible. i1 in [0, m - p)
            for (i1 = 0; ptr1 && i1 < m - p; ++i1)
                ptr1 = list_next(list1, ptr1);
            if (!ptr1)
                continue; // beyond the list
            // skip ptr2 p times if possible. i2 in [0, p)
            for (i2 = 0; ptr2 && i2 < p; ++i2)
                ptr2 = list_next(list2, ptr2);
            if (!ptr2)
                continue; // beyond the list

            // compare
            node1 = LIST_ENTRY(ptr1, NODE, entry);
            node2 = LIST_ENTRY(ptr2, NODE, entry);
            ret = CompareNode(pFC, node1, node2);
            if (ret == FCRET_INVALID)
                return ret;
            if (ret == FCRET_DIFFERENT)
                continue;

            ptr3 = list_next(list1, ptr1);
            ptr4 = list_next(list2, ptr2);
            k = 1;
            while (ptr3 && ptr4 && i1 < pFC->n && i2 < pFC->n && k < pFC->nnnn)
            {
                node1 = LIST_ENTRY(ptr3, NODE, entry);
                node2 = LIST_ENTRY(ptr4, NODE, entry);
                ret = CompareNode(pFC, node1, node2);
                if (ret == FCRET_INVALID)
                    return ret;
                if (ret == FCRET_DIFFERENT)
                    break;
                ++i1, ++i2;
                ++k;
                ptr3 = list_next(list1, ptr3);
                ptr4 = list_next(list2, ptr4);
            }

            if (k >= pFC->nnnn)
            {
                *pi1 = i1;
                *pi2 = i2;
                return FCRET_IDENTICAL;
            }
        }
    }

    *pi1 = i1;
    *pi2 = i2;
    return FCRET_DIFFERENT;
}

FCRET TextCompare(FILECOMPARE *pFC, HANDLE *phMapping1, const LARGE_INTEGER *pcb1,
                                    HANDLE *phMapping2, const LARGE_INTEGER *pcb2)
{
    FCRET ret, ret1, ret2;
    struct list *list1, *list2, *ptr1, *ptr2, *head1, *head2, *next;
    NODE *node1, *node2;
    BOOL fDifferent = FALSE;
    INT i1, i2;
    LARGE_INTEGER ib1 = { .QuadPart = 0 }, ib2 = { .QuadPart = 0 };
    list1 = &pFC->list[0];
    list2 = &pFC->list[1];
    list_init(list1);
    list_init(list2);

    do
    {
        ret1 = ParseLines(pFC, phMapping1, &ib1, pcb1, list1);
        if (ret1 == FCRET_INVALID)
            return ret1;
        ret2 = ParseLines(pFC, phMapping2, &ib2, pcb2, list2);
        if (ret2 == FCRET_INVALID)
            return ret2;

        head1 = ptr1 = list_head(list1);
        head2 = ptr2 = list_head(list2);
        if (!ptr1 || !ptr2)
            goto quit;

        // skip identical (synch'ed)
        ret = SkipIdentical(pFC, &ptr1, &ptr2);
        if (ret == FCRET_INVALID)
            goto hell;
        if (ret == FCRET_DIFFERENT)
            fDifferent = TRUE;
        if (!ptr1 || !ptr2)
            goto quit;

        // free useless data
        while (head1 && head1 != ptr1)
        {
            node1 = LIST_ENTRY(ptr1, NODE, entry);
            next = list_next(list1, head1);
            DeleteNode(node1);
            head1 = next;
        }
        while (head2 && head2 != ptr2)
        {
            node2 = LIST_ENTRY(ptr2, NODE, entry);
            next = list_next(list2, head2);
            DeleteNode(node2);
            head2 = next;
        }

        // try to resynch
        ret = Resynch(pFC, head1, head2, &i1, &i2);
        if (ret == FCRET_INVALID)
            goto hell;
        if (ret == FCRET_DIFFERENT)
        {
            // resynch failed
            ret = ResynchFailed();
            goto hell;
        }
        // now, show the difference (with clean-up)
        fDifferent = TRUE;
        ShowDiffAndClean(pFC, head1, head2, i1, i2);

        // now resynch'ed
    } while (ret1 != FCRET_NO_MORE_DATA && ret2 != FCRET_NO_MORE_DATA);

quit:
    // finalizing...
    ret = Finalize(pFC, ptr1, ptr2, fDifferent);
hell:
    DeleteList(list1);
    DeleteList(list2);
    return ret;
}
