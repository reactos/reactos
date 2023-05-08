/*
 * PROJECT:     ReactOS header
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     The special virtual keys for Japanese
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

/*
 * The special virtual keys for Japanese: Used for key states.
 * https://www.kthree.co.jp/kihelp/index.html?page=app/vkey&type=html
 */
#define VK_DBE_ALPHANUMERIC 0xF0
#define VK_DBE_KATAKANA 0xF1
#define VK_DBE_HIRAGANA 0xF2
#define VK_DBE_SBCSCHAR 0xF3
#define VK_DBE_DBCSCHAR 0xF4
#define VK_DBE_ROMAN 0xF5
#define VK_DBE_NOROMAN 0xF6
#define VK_DBE_ENTERWORDREGISTERMODE 0xF7
#define VK_DBE_ENTERCONFIGMODE 0xF8
#define VK_DBE_FLUSHSTRING 0xF9
#define VK_DBE_CODEINPUT 0xFA
#define VK_DBE_NOCODEINPUT 0xFB
#define VK_DBE_DETERINESTRING 0xFC
#define VK_DBE_ENTERDLGCONVERSIONMODE 0xFD
