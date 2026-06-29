/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for LdrFindResource_U
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

static void Test_CORE_20401(void)
{
    HMODULE hmod = GetModuleHandleW(NULL);
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;
    NTSTATUS Status;

    // Use LdrFindResource_U to find a bitmap resource called "NORMAL_FRAMECAPTION_BMP"
    // CORE-20401 resulted in wrong comparison of strings containing underscores.
    // If the test fails, the resource name comparison is probably broken (again).
    info.Type = 2; // RT_BITMAP;
    info.Name = (ULONG_PTR)L"NORMAL_FRAMECAPTION_BMP";
    info.Language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    Status = LdrFindResource_U(hmod, &info, 3, &entry);
    ok_ntstatus(Status, STATUS_SUCCESS);
}

START_TEST(LdrFindResource_U)
{
    Test_CORE_20401();
}
