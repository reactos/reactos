/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - List handling
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "compat.h"
#include "list.h"
#include "util.h"
#include "options.h"

PLIST_MEMBER
entry_lookup(PLIST list, char *name)
{
    PLIST_MEMBER pprev = NULL;
    PLIST_MEMBER pnext;

    if (!name || !name[0])
        return NULL;

    pnext = list->phead;
    while (pnext != NULL)
    {
        if (PATHCMP(name, pnext->name) == 0)
        {
            if (pprev)
            {   // move to head for faster lookup next time
                pprev->pnext = pnext->pnext;
                pnext->pnext = list->phead;
                list->phead = pnext;
            }
            return pnext;
        }
        pprev = pnext;
        pnext = pnext->pnext;
    }
    return NULL;
}

PLIST_MEMBER
entry_delete(PLIST_MEMBER pentry)
{
    if (!pentry)
        return NULL;
    if (pentry->buf)
        free(pentry->buf);
    free(pentry);
    return NULL;
}

PLIST_MEMBER
entry_insert(PLIST list, PLIST_MEMBER pentry)
{
    if (!pentry)
        return NULL;

    pentry->pnext = list->phead;
    list->phead = pentry;
    if (!list->ptail)
        list->ptail = pentry;
    return pentry;
}

void list_clear(PLIST list)
{
    PLIST_MEMBER pentry = list->phead;
    PLIST_MEMBER pnext;
    while (pentry)
    {
        pnext = pentry->pnext;
        entry_delete(pentry);
        pentry = pnext;
    }
    list->phead = list->ptail = NULL;
}

#if 0
LIST_MEMBER *
entry_remove(LIST *list, LIST_MEMBER *pentry)
{
    LIST_MEMBER *pprev = NULL, *p = NULL;

    if (!pentry)
        return NULL;

    if (pentry == list->phead)
    {
        list->phead = pentry->pnext;
        p = pentry;
    }
    else
    {
        pprev = list->phead;
        while (pprev->pnext)
        {
            if (pprev->pnext == pentry)
            {
                pprev->pnext = pentry->pnext;
                p = pentry;
                break;
            }
            pprev = pprev->pnext;
        }
    }
    if (pentry == list->ptail)
        list->ptail = pprev;

    return p;
}
#endif

PLIST_MEMBER
cache_entry_create(char *Line)
{
    PLIST_MEMBER pentry;
    char *s = NULL;
    int l;

    if (!Line)
        return NULL;

    pentry = malloc(sizeof(LIST_MEMBER));
    if (!pentry)
        return NULL;

    l = strlen(Line);
    pentry->buf = s = malloc(l + 1);
    if (!s)
    {
        l2l_dbg(1, "Alloc entry failed\n");
        return entry_delete(pentry);
    }

    strcpy(s, Line);
    if (s[l] == '\n')
        s[l] = '\0';

    pentry->name = s;
    s = strchr(s, '|');
    if (!s)
    {
        l2l_dbg(1, "Name field missing\n");
        return entry_delete(pentry);
    }
    *s++ = '\0';

    pentry->path = s;
    s = strchr(s, '|');
    if (!s)
    {
        l2l_dbg(1, "Path field missing\n");
        return entry_delete(pentry);
    }
    *s++ = '\0';
    if (1 != sscanf(s, "%x", (unsigned int *)(&pentry->ImageBase)))
    {
        l2l_dbg(1, "ImageBase field missing\n");
        return entry_delete(pentry);
    }
    pentry->RelBase = INVALID_BASE;
    pentry->Size = 0;
    return pentry;
}


PLIST_MEMBER
sources_entry_create(PLIST list, char *path, char *prefix)
{
    PLIST_MEMBER pentry;
    char *s = NULL;
    int l;

    if (!path)
        return NULL;
    if (!prefix)
        prefix = "";

    pentry = malloc(sizeof(LIST_MEMBER));
    if (!pentry)
        return NULL;

    l = strlen(path) + strlen(prefix);
    pentry->buf = s = malloc(l + 1);
    if (!s)
    {
        l2l_dbg(1, "Alloc entry failed\n");
        return entry_delete(pentry);
    }

    strcpy(s, prefix);
    strcat(s, path);
    if (s[l] == '\n')
        s[l] = '\0';

    pentry->name = s;
    if (list)
    {
        if (entry_lookup(list, pentry->name))
        {
            l2l_dbg(1, "Entry %s exists\n", pentry->name);
            pentry = entry_delete(pentry);
        }
        else
        {
            l2l_dbg(1, "Inserting entry %s\n", pentry->name);
            entry_insert(list, pentry);
        }
    }

    return pentry;
}

/* EOF */
