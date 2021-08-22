/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS Extended RunOnce processing with UI.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#pragma once

typedef VOID(CALLBACK *RUNONCEEX_CALLBACK)(UINT CompleteCnt, UINT TotalCnt, DWORD Reserved);
