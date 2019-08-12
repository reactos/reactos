/*
 * PROJECT:     ReactOS strutil (ntlmssp)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     provides string-functions similar to RtlString-functions
 *              however it provies a variable alloc/free function.
 *              so its useful in sspi where it can be user or lsa mode.
 *              (if useful make a lib or something ...)
 * COPYRIGHT:   Copyright 2019 Andreas Maier <staubim@quantentunnel.de>
 */

#include <stdlib.h>
#include <ntlmssp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

_strutil_alloc_proc _strutil_alloc = malloc;
_strutil_free_proc _strutil_free = free;
BOOL _strutil_is_custom = FALSE;

BOOL
init_strutil(
    IN _strutil_alloc_proc ap,
    IN _strutil_free_proc fp)
{
    /* set memmory manager once and dont change
     * it while running ... */
    if (_strutil_is_custom)
    {
        WARN("init_strutil was called more then once!\n");
        return FALSE;
    }
    _strutil_alloc = ap;
    _strutil_free = fp;
    _strutil_is_custom = TRUE;
    return TRUE;
}
