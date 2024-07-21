/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     shdocvw.dll objects
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once

HRESULT SHELL_GetIDListFromObject(IUnknown *punk, PIDLIST_ABSOLUTE *ppidl);
BOOL SHELL_IsEqualAbsoluteID(PCIDLIST_ABSOLUTE a, PCIDLIST_ABSOLUTE b);
BOOL SHELL_IsVerb(IContextMenu *pcm, UINT_PTR idCmd, LPCWSTR Verb);
