/*
 * PROJECT:     ReactOS Settings
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Settings search engine (portable, unit-testable on any host)
 */

#pragma once

#include <stddef.h>

/* The engine only needs 16-bit characters. On non-Windows hosts the unit test
 * is compiled with -fshort-wchar so that L"..." literals match WCHAR. */
typedef wchar_t SCHAR;

typedef struct _SEARCH_ITEM
{
    const SCHAR *Title;         /* "Background" */
    const SCHAR *Keywords;      /* comma separated synonyms: "wallpaper, desktop picture" */
    const SCHAR *Description;   /* "Choose a picture or color for your desktop" */
    const SCHAR *Category;      /* "Personalization" */
} SEARCH_ITEM;

/*
 * Ranks Items[] against Query. Writes up to MaxResults indexes into Results,
 * best match first, and returns how many were written.
 *
 * Every query word is matched against the words of each field: exact match,
 * then word prefix (as-you-type), then substring, then small typos. Title
 * matches weigh most, then keywords, category and description. Items matching
 * more query words always rank above items matching fewer, and only items
 * matching the highest number of query words are returned.
 */
int
SearchItems(
    const SEARCH_ITEM *Items,
    int ItemCount,
    const SCHAR *Query,
    int *Results,
    int MaxResults);
