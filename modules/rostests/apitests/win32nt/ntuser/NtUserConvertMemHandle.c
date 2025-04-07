/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for NtUserConvertMemHandle
 * COPYRIGHT:   Copyright 2025 Max Korostil (mrmks04@yandex.ru)
 */

#include "../win32nt.h"


START_TEST(NtUserConvertMemHandle)
{
    HANDLE hMem;
 
    hMem = NtUserConvertMemHandle((PVOID)0xDEADBEEF, 0xFFFF);
    TEST(hMem == NULL);
}