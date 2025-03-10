/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Helper functions used thoughout the program
 * COPYRIGHT:  Copyright 2025 Curtis Wilson <LiquidFox1776@gmail.com>
 */
 
 #include "precomp.h"
 #define NDEBUG
 #include <debug.h>
 /*
 GUID *
 GuidFromStringW(LPCWSTR lpwszString)
 {
 
 }
 
 LPCWSTR
 StringFromGuidW(GUID * pGuid)
 {
 
 }
 
 */
 
 PWSTR JoinStringsW(
    PWSTR* ppwszStrings, 
    size_t sizeCount, 
    PCWSTR pcwszSeparator) 
{
    size_t sizeLength = 0;

    if (sizeCount == 0) 
        return NULL;

    if (pcwszSeparator == NULL)
        return NULL;

    for (size_t index = 0; index < sizeCount; index++) {
        sizeLength += wcslen(ppwszStrings[index]);
    }

    sizeLength += wcslen(pcwszSeparator) * (sizeCount - 1);

    HANDLE hHeap = GetProcessHeap();
    if (hHeap == NULL) 
        return NULL;

    PWSTR pwszResult = (PWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (sizeLength + 1) * sizeof(wchar_t));
    if (pwszResult == NULL) 
        return NULL;

    // start the string
    wcscpy(pwszResult, ppwszStrings[0]);
    // join the rest of the strings
    for (size_t index = 1; index < sizeCount; index++) 
    {
        wcscat(pwszResult, pcwszSeparator);
        wcscat(pwszResult, ppwszStrings[index]);
    }

    return pwszResult;
}
