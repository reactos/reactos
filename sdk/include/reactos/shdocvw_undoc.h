/*
 * PROJECT:     ReactOS Headers
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     shdocvw.dll undocumented APIs
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

BOOL WINAPI
IEILIsEqual(
    _In_ LPCITEMIDLIST pidl1,
    _In_ LPCITEMIDLIST pidl2,
    _In_ BOOL bUnknown);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */
