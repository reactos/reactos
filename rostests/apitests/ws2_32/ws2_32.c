/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/ws2_32.c
 * PURPOSE:     Program entry point
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "ws2_32.h"

HANDLE g_hHeap;

BOOL
IsFunctionPresent(LPWSTR lpszFunction)
{
    return TRUE;
}

int wmain()
{
    g_hHeap = GetProcessHeap();

    return TestMain(L"ws2_32_apitests", L"ws2_32.dll");
}
