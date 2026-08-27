/*
 * PROJECT:     ReactOS System Setup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 *              or GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Various utility routines
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

BOOL
DoesPathExist(
    _In_ PCWSTR pszPath,
    _Out_opt_ PDWORD pAttrs)
{
    UINT prevMode;
    DWORD attrs;

    /* Prevent a dialog box if path is on a disk that has been ejected */
    prevMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    attrs = GetFileAttributesW(pszPath);
    SetErrorMode(prevMode);

    if (pAttrs)
        *pAttrs = attrs;
    return (attrs != INVALID_FILE_ATTRIBUTES);
}

BOOL
DoesFileExist(
    _In_ PCWSTR pszPath)
{
    DWORD attrs = 0;
    return (DoesPathExist(pszPath, &attrs) && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL
DoesDirExist(
    _In_ PCWSTR pszPath)
{
    DWORD attrs = 0;
    return (DoesPathExist(pszPath, &attrs) && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}
