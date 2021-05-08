/*
 * PROJECT:     ReactOS WHERE command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Providing string list
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

static LPWSTR str_clone(LPCWSTR psz)
{
    size_t cb;
    LPWSTR pszNew;
    if (!psz)
        return NULL;
    cb = (wcslen(psz) + 1) * sizeof(WCHAR);
    pszNew = (LPWSTR)malloc(cb);
    if (!pszNew)
        return NULL;
    memcpy(pszNew, psz, cb);
    return pszNew;
}

typedef struct strlist_t
{
    LPWSTR *ppsz;
    unsigned int count;
} strlist_t;
#define strlist_default { NULL, 0 }

static inline void strlist_init(strlist_t *plist)
{
    plist->ppsz = NULL;
    plist->count = 0;
}

static inline LPWSTR strlist_get_at(strlist_t *plist, unsigned int i)
{
    return plist->ppsz[i];
}

static int strlist_add(strlist_t *plist, LPCWSTR psz)
{
    LPWSTR *ppsz, clone = str_clone(psz);
    if (!clone)
        return 0;
    ppsz = (LPWSTR *)realloc(plist->ppsz, (plist->count + 1) * sizeof(LPWSTR));
    if (!ppsz)
        return 0;
    plist->ppsz = ppsz;
    plist->ppsz[plist->count] = clone;
    ++(plist->count);
    return 1;
}

static void strlist_destroy(strlist_t *plist)
{
    unsigned int i;
    for (i = 0; i < plist->count; ++i)
        free(plist->ppsz[i]);
    plist->count = 0;
    free(plist->ppsz);
    plist->ppsz = NULL;
}

static inline int strlist_find_i(strlist_t *plist, LPCWSTR psz)
{
    unsigned int i;
    for (i = 0; i < plist->count; ++i)
    {
        if (_wcsicmp(plist->ppsz[i], psz) == 0)
            return i;
    }
    return -1;
}
