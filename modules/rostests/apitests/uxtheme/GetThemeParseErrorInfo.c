/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for GetThemeParseErrorInfo
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <stdlib.h>
#include <windows.h>
#include <uxtheme.h>
#include <uxundoc.h>

START_TEST(GetThemeParseErrorInfo)
{
    HRESULT hr;
    PARSE_ERROR_INFO Info;

    hr = GetThemeParseErrorInfo(NULL);
    ok_hex(hr, E_POINTER);

    ZeroMemory(&Info, sizeof(Info));
    hr = GetThemeParseErrorInfo(&Info);
    ok_hex(hr, E_INVALIDARG);

    Info.cbSize = sizeof(Info);
    hr = GetThemeParseErrorInfo(&Info);
    ok_hex(hr, HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND));
}
