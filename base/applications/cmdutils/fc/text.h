/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing text files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "fc.h"

#define IS_SPACE(ch) ((ch) == TEXT(' ') || (ch) == TEXT('\t'))

#ifdef UNICODE
    #define NODE NODE_W
    #define PrintLine PrintLineW
    #define TextCompare TextCompareW
#else
    #define NODE NODE_A
    #define PrintLine PrintLineA
    #define TextCompare TextCompareA
#endif

static LPTSTR AllocLine(LPCTSTR pch, DWORD cch)
{
    LPTSTR pszNew = malloc((cch + 1) * sizeof(TCHAR));
    if (!pszNew)
        return NULL;
    memcpy(pszNew, pch, cch * sizeof(TCHAR));
    pszNew[cch] = 0;
    return pszNew;
}

static NODE *AllocNode(LPTSTR psz, DWORD lineno)
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
    node->lineno = lineno;
    return node;
}

static __inline VOID DeleteNode(NODE *node)
{
    if (node)
    {
        free(node->pszLine);
        free(node->pszComp);
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

static __inline LPCTSTR FindLastNonSpace(LPCTSTR pch)
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

static VOID DeleteDuplicateSpaces(LPTSTR psz)
{
    LPTSTR pch0, pch1;
    for (pch0 = pch1 = psz; *pch0; ++pch0)
    {
        *pch1++ = *pch0;
        if (IS_SPACE(*pch0))
        {
            do
            {
                ++pch0;
            } while (IS_SPACE(*pch0));
            --pch0;
        }
    }
    *pch1 = 0;
}

static LPTSTR CompressSpace(LPCTSTR line)
{
    LPTSTR pszNew;
    LPCTSTR pchLast;

    line = SkipSpace(line);
    pchLast = FindLastNonSpace(line);
    if (pchLast == NULL)
        return AllocLine(NULL, 0);

    pszNew = AllocLine(line, (DWORD)(pchLast - line) + 1);
    if (!pszNew)
        return NULL;

    DeleteDuplicateSpaces(pszNew);
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
    INT spaces, cch = ExpandTabLength(line), ich;
    LPTSTR pszNew = malloc((cch + 1) * sizeof(TCHAR));
    LPCTSTR pch;
    if (!pszNew)
        return NULL;
    ich = 0;
    for (pch = line; *pch; ++pch)
    {
        if (*pch == TEXT('\t'))
        {
            spaces = TAB_WIDTH - (ich % TAB_WIDTH);
            while (spaces-- > 0)
            {
                pszNew[ich++] = TEXT(' ');
            }
        }
        else
        {
            pszNew[ich++] = *pch;
        }
    }
    pszNew[ich] = 0;
    return pszNew;
}

#define HASH_EOF 0xFFFFFFFF
#define HASH_MASK 0x7FFFFFFF

static DWORD GetHash(LPCTSTR psz, BOOL bIgnoreCase)
{
    DWORD ret = 0xDEADFACE;
    while (*psz)
    {
        ret += (bIgnoreCase ? towupper(*psz) : *psz);
        ret <<= 2;
        ++psz;
    }
    return (ret & HASH_MASK);
}

static NODE *AllocEOFNode(DWORD lineno)
{
    NODE *node = AllocNode(AllocLine(NULL, 0), 0);
    if (node == NULL)
        return NULL;
    node->pszComp = AllocLine(NULL, 0);
    if (node->pszComp == NULL)
    {
        DeleteNode(node);
        return NULL;
    }
    node->lineno = lineno;
    node->hash = HASH_EOF;
    return node;
}

static __inline BOOL IsEOFNode(NODE *node)
{
    return !node || node->hash == HASH_EOF;
}

static BOOL ConvertNode(const FILECOMPARE *pFC, NODE *node)
{
    if (!(pFC->dwFlags & FLAG_T))
    {
        LPTSTR tmp = ExpandTab(node->pszLine);
        if (!tmp)
            return FALSE;
        free(node->pszLine);
        node->pszLine = tmp;
        if (!(pFC->dwFlags & FLAG_W))
            node->hash = GetHash(node->pszLine, !!(pFC->dwFlags & FLAG_C));
    }
    if (pFC->dwFlags & FLAG_W)
    {
        node->pszComp = CompressSpace(node->pszLine);
        if (!node->pszComp)
            return FALSE;
        node->hash = GetHash(node->pszComp, !!(pFC->dwFlags & FLAG_C));
    }
    return TRUE;
}

static FCRET CompareNode(const FILECOMPARE *pFC, const NODE *node0, const NODE *node1)
{
    DWORD dwCmpFlags;
    LPTSTR psz0, psz1;
    INT ret;
    if (node0->hash != node1->hash)
        return FCRET_DIFFERENT;

    psz0 = (pFC->dwFlags & FLAG_W) ? node0->pszComp : node0->pszLine;
    psz1 = (pFC->dwFlags & FLAG_W) ? node1->pszComp : node1->pszLine;
    dwCmpFlags = ((pFC->dwFlags & FLAG_C) ? NORM_IGNORECASE : 0);
    ret = CompareString(LOCALE_USER_DEFAULT, dwCmpFlags, psz0, -1, psz1, -1);
    return (ret == CSTR_EQUAL) ? FCRET_IDENTICAL : FCRET_DIFFERENT;
}

static BOOL FindNextLine(LPCTSTR pch, DWORD ich, DWORD cch, LPDWORD pich)
{
    while (ich < cch)
    {
        if (pch[ich] == TEXT('\n') || pch[ich] == TEXT('\0'))
        {
            *pich = ich;
            return TRUE;
        }
        ++ich;
    }
    *pich = cch;
    return FALSE;
}

static FCRET
ParseLines(const FILECOMPARE *pFC, HANDLE *phMapping,
           LARGE_INTEGER *pib, const LARGE_INTEGER *pcb, struct list *list)
{
    DWORD lineno = 1, ich, cch, ichNext, cbView, cchNode;
    LPTSTR psz, pszLine;
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
    while (ich < cch &&
           (FindNextLine(psz, ich, cch, &ichNext) ||
            (ichNext == cch && (fLast || ich == 0))))
    {
        bCR = (ichNext > 0) && (psz[ichNext - 1] == TEXT('\r'));
        cchNode = ichNext - ich - bCR;
        pszLine = AllocLine(&psz[ich], cchNode);
        node = AllocNode(pszLine, lineno++);
        if (!node || !ConvertNode(pFC, node))
        {
            DeleteNode(node);
            UnmapViewOfFile(psz);
            return OutOfMemory();
        }
        list_add_tail(list, &node->entry);
        ich = ichNext + 1;
    }

    UnmapViewOfFile(psz);

    pib->QuadPart += ichNext * sizeof(WCHAR);

    if (pib->QuadPart < pcb->QuadPart)
        return FCRET_IDENTICAL;

    // append EOF node
    node = AllocEOFNode(lineno);
    if (!node)
        return OutOfMemory();
    list_add_tail(list, &node->entry);

    return FCRET_NO_MORE_DATA;
}

static VOID
ShowDiff(FILECOMPARE *pFC, INT i, struct list *begin, struct list *end)
{
    NODE* node;
    struct list *list = &pFC->list[i];
    struct list *first = NULL, *last = NULL;
    PrintCaption(pFC->file[i]);
    if (begin && end && list_prev(list, begin))
        begin = list_prev(list, begin);
    while (begin != end)
    {
        node = LIST_ENTRY(begin, NODE, entry);
        if (IsEOFNode(node))
            break;
        if (!first)
            first = begin;
        last = begin;
        if (!(pFC->dwFlags & FLAG_A))
            PrintLine(pFC, node->lineno, node->pszLine);
        begin = list_next(list, begin);
    }
    if ((pFC->dwFlags & FLAG_A) && first)
    {
        node = LIST_ENTRY(first, NODE, entry);
        PrintLine(pFC, node->lineno, node->pszLine);
        first = list_next(list, first);
        if (first != last)
        {
            if (list_next(list, first) == last)
            {
                node = LIST_ENTRY(first, NODE, entry);
                PrintLine(pFC, node->lineno, node->pszLine);
            }
            else
            {
                PrintDots();
            }
        }
        node = LIST_ENTRY(last, NODE, entry);
        PrintLine(pFC, node->lineno, node->pszLine);
    }
}

static VOID
SkipIdentical(FILECOMPARE *pFC, struct list **pptr0, struct list **pptr1)
{
    struct list *ptr0 = *pptr0, *ptr1 = *pptr1;
    while (ptr0 && ptr1)
    {
        NODE *node0 = LIST_ENTRY(ptr0, NODE, entry);
        NODE *node1 = LIST_ENTRY(ptr1, NODE, entry);
        if (CompareNode(pFC, node0, node1) != FCRET_IDENTICAL)
            break;
        ptr0 = list_next(&pFC->list[0], ptr0);
        ptr1 = list_next(&pFC->list[1], ptr1);
    }
    *pptr0 = ptr0;
    *pptr1 = ptr1;
}

static DWORD
SkipIdenticalN(FILECOMPARE *pFC, struct list **pptr0, struct list **pptr1,
               DWORD nnnn, DWORD lineno0, DWORD lineno1)
{
    struct list *ptr0 = *pptr0, *ptr1 = *pptr1;
    DWORD count = 0;
    while (ptr0 && ptr1)
    {
        NODE *node0 = LIST_ENTRY(ptr0, NODE, entry);
        NODE *node1 = LIST_ENTRY(ptr1, NODE, entry);
        if (node0->lineno >= lineno0)
            break;
        if (node1->lineno >= lineno1)
            break;
        if (CompareNode(pFC, node0, node1) != FCRET_IDENTICAL)
            break;
        ptr0 = list_next(&pFC->list[0], ptr0);
        ptr1 = list_next(&pFC->list[1], ptr1);
        ++count;
        if (count >= nnnn)
            break;
    }
    *pptr0 = ptr0;
    *pptr1 = ptr1;
    return count;
}

static FCRET
ScanDiff(FILECOMPARE *pFC, struct list **pptr0, struct list **pptr1,
         DWORD lineno0, DWORD lineno1)
{
    struct list *ptr0 = *pptr0, *ptr1 = *pptr1, *tmp0, *tmp1;
    NODE *node0, *node1;
    INT count;
    while (ptr0 && ptr1)
    {
        node0 = LIST_ENTRY(ptr0, NODE, entry);
        node1 = LIST_ENTRY(ptr1, NODE, entry);
        if (node0->lineno >= lineno0)
            return FCRET_DIFFERENT;
        if (node1->lineno >= lineno1)
            return FCRET_DIFFERENT;
        tmp0 = ptr0;
        tmp1 = ptr1;
        count = SkipIdenticalN(pFC, &tmp0, &tmp1, pFC->nnnn, lineno0, lineno1);
        if (count >= pFC->nnnn)
            break;
        if (count > 0)
        {
            ptr0 = tmp0;
            ptr1 = tmp1;
        }
        else
        {
            ptr0 = list_next(&pFC->list[0], ptr0);
            ptr1 = list_next(&pFC->list[1], ptr1);
        }
    }
    *pptr0 = ptr0;
    *pptr1 = ptr1;
    return FCRET_IDENTICAL;
}

static FCRET
Resync(FILECOMPARE *pFC, struct list **pptr0, struct list **pptr1)
{
    FCRET ret;
    struct list *ptr0, *ptr1, *save0 = NULL, *save1 = NULL;
    NODE *node0, *node1;
    struct list *list0 = &pFC->list[0], *list1 = &pFC->list[1];
    DWORD lineno0, lineno1;
    INT penalty, i0, i1, min_penalty = MAXLONG;

    node0 = LIST_ENTRY(*pptr0, NODE, entry);
    node1 = LIST_ENTRY(*pptr1, NODE, entry);
    lineno0 = node0->lineno + pFC->n;
    lineno1 = node1->lineno + pFC->n;

    // ``If the files that you are comparing have more than pFC->n consecutive
    //   differing lines, FC cancels the comparison,,
    // ``If the number of matching lines in the files is less than pFC->nnnn,
    //   FC displays the matching lines as differences,,
    for (ptr1 = list_next(list1, *pptr1), i1 = 0; ptr1; ptr1 = list_next(list1, ptr1), ++i1)
    {
        node1 = LIST_ENTRY(ptr1, NODE, entry);
        if (node1->lineno >= lineno1)
            break;
        for (ptr0 = list_next(list0, *pptr0), i0 = 0; ptr0; ptr0 = list_next(list0, ptr0), ++i0)
        {
            node0 = LIST_ENTRY(ptr0, NODE, entry);
            if (node0->lineno >= lineno0)
                break;
            if (CompareNode(pFC, node0, node1) == FCRET_IDENTICAL)
            {
                penalty = min(i0, i1) + abs(i1 - i0);
                if (min_penalty > penalty)
                {
                    min_penalty = penalty;
                    save0 = ptr0;
                    save1 = ptr1;
                }
            }
        }
    }

    if (save0 && save1)
    {
        *pptr0 = save0;
        *pptr1 = save1;
        ret = ScanDiff(pFC, &save0, &save1, lineno0, lineno1);
        if (save0 && save1)
        {
            *pptr0 = save0;
            *pptr1 = save1;
        }
        return ret;
    }

    for (ptr0 = *pptr0; ptr0; ptr0 = list_next(list0, ptr0))
    {
        node0 = LIST_ENTRY(ptr0, NODE, entry);
        if (node0->lineno == lineno0)
            break;
    }
    for (ptr1 = *pptr1; ptr1; ptr1 = list_next(list1, ptr1))
    {
        node1 = LIST_ENTRY(ptr1, NODE, entry);
        if (node1->lineno == lineno1)
            break;
    }
    *pptr0 = ptr0;
    *pptr1 = ptr1;
    return FCRET_DIFFERENT;
}

static FCRET
Finalize(FILECOMPARE* pFC, struct list *ptr0, struct list* ptr1, BOOL fDifferent)
{
    if (!ptr0 && !ptr1)
    {
        if (fDifferent)
            return Different(pFC->file[0], pFC->file[1]);
        return NoDifference();
    }
    else
    {
        ShowDiff(pFC, 0, ptr0, NULL);
        ShowDiff(pFC, 1, ptr1, NULL);
        PrintEndOfDiff();
        return FCRET_DIFFERENT;
    }
}

// FIXME: "cmd_apitest fc" has some failures.
FCRET TextCompare(FILECOMPARE *pFC, HANDLE *phMapping0, const LARGE_INTEGER *pcb0,
                                    HANDLE *phMapping1, const LARGE_INTEGER *pcb1)
{
    FCRET ret, ret0, ret1;
    struct list *ptr0, *ptr1, *save0, *save1, *next0, *next1;
    NODE* node0, * node1;
    BOOL fDifferent = FALSE;
    LARGE_INTEGER ib0 = { .QuadPart = 0 }, ib1 = { .QuadPart = 0 };
    struct list *list0 = &pFC->list[0], *list1 = &pFC->list[1];
    list_init(list0);
    list_init(list1);

    do
    {
        ret0 = ParseLines(pFC, phMapping0, &ib0, pcb0, list0);
        if (ret0 == FCRET_INVALID)
        {
            ret = ret0;
            goto cleanup;
        }
        ret1 = ParseLines(pFC, phMapping1, &ib1, pcb1, list1);
        if (ret1 == FCRET_INVALID)
        {
            ret = ret1;
            goto cleanup;
        }

        ptr0 = list_head(list0);
        ptr1 = list_head(list1);
        for (;;)
        {
            if (!ptr0 || !ptr1)
                goto quit;

            // skip identical (sync'ed)
            SkipIdentical(pFC, &ptr0, &ptr1);
            if (ptr0 || ptr1)
                fDifferent = TRUE;
            node0 = LIST_ENTRY(ptr0, NODE, entry);
            node1 = LIST_ENTRY(ptr1, NODE, entry);
            if (IsEOFNode(node0) || IsEOFNode(node1))
                goto quit;

            // try to resync
            save0 = ptr0;
            save1 = ptr1;
            ret = Resync(pFC, &ptr0, &ptr1);
            if (ret == FCRET_INVALID)
                goto cleanup;
            if (ret == FCRET_DIFFERENT)
            {
                // resync failed
                ret = ResyncFailed();
                // show the difference
                ShowDiff(pFC, 0, save0, ptr0);
                ShowDiff(pFC, 1, save1, ptr1);
                PrintEndOfDiff();
                goto cleanup;
            }

            // show the difference
            fDifferent = TRUE;
            next0 = ptr0 ? list_next(list0, ptr0) : ptr0;
            next1 = ptr1 ? list_next(list1, ptr1) : ptr1;
            ShowDiff(pFC, 0, save0, (next0 ? next0 : ptr0));
            ShowDiff(pFC, 1, save1, (next1 ? next1 : ptr1));
            PrintEndOfDiff();

            // now resync'ed
        }
    } while (ret0 != FCRET_NO_MORE_DATA || ret1 != FCRET_NO_MORE_DATA);

quit:
    ret = Finalize(pFC, ptr0, ptr1, fDifferent);
cleanup:
    DeleteList(list0);
    DeleteList(list1);
    return ret;
}
