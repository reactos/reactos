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
    CHAR data[] = {1, 2, 3, 4};
 
    hMem = NtUserConvertMemHandle((PVOID)(UINT_PTR)0xDEADBEEF, 0xFFFF);
    ok_hdl(hMem, NULL);

    hMem = NtUserConvertMemHandle(data, sizeof(data));
    ok(hMem != NULL, "hMem is NULL\n");
    // FIXME: Clean up hMem.
}
