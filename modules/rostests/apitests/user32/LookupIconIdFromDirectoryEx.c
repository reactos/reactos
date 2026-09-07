
#include "precomp.h"

START_TEST(LookupIconIdFromDirectoryEx)
{
    HRSRC hResource;
    HGLOBAL hMem;
    HMODULE hMod;
    int wResId;
    DEVMODEW dm;
    DWORD dwOrigBpp;
    UINT i;
    BYTE* lpResource;

    /* This tests assumes that default icon size is 32x32 */

    struct
    {
        DWORD bpp;
        int wResId;
        int cxDesired;
        int cyDesired;
        UINT flags;
    }
    TestData[] =
    {
        {8,  1,  0,  0,  0},
        {8,  1,  48, 48, 0},
        {8,  2,  32, 32, 0},
        {8,  3,  24, 24, 0},
        {8,  4,  16, 16, 0},
        {8,  1,  0,  0,  LR_MONOCHROME},
        {8,  1,  48, 48, LR_MONOCHROME},
        {8,  2,  32, 32, LR_MONOCHROME},
        {8,  3,  24, 24, LR_MONOCHROME},
        {8,  4,  16, 16, LR_MONOCHROME},
        {8,  2,  0,  0,  LR_DEFAULTSIZE},
        {8,  1,  48, 48, LR_DEFAULTSIZE},
        /* Non exact sizes */
        {8,  1,  41, 41, 0},
        {8,  1,  40, 40, 0},
        /* Non square sizes */
        {8,  1,  16, 48, 0},
        {8,  1,  48, 16, 0},
        {16, 5,  0,  0,  0},
        {16, 5,  48, 48, 0},
        {16, 6,  32, 32, 0},
        {16, 7,  24, 24, 0},
        {16, 1,  0,  0,  LR_MONOCHROME},
        {16, 1,  48, 48, LR_MONOCHROME},
        {16, 2,  32, 32, LR_MONOCHROME},
        {16, 3,  24, 24, LR_MONOCHROME},
        {16, 4,  16, 16, LR_MONOCHROME},
        {16, 6,  0,  0,  LR_DEFAULTSIZE},
        {16, 5,  48, 48, LR_DEFAULTSIZE},
        {24, 5,  0,  0,  0},
        {24, 5,  48, 48, 0},
        {24, 6,  32, 32, 0},
        {24, 7,  24, 24, 0},
        {24, 8,  16, 16, 0},
        {16, 8,  16, 16, 0},
        {24, 1,  0,  0,  LR_MONOCHROME},
        {24, 1,  48, 48, LR_MONOCHROME},
        {24, 2,  32, 32, LR_MONOCHROME},
        {24, 3,  24, 24, LR_MONOCHROME},
        {24, 4,  16, 16, LR_MONOCHROME},
        {24, 6,  0,  0,  LR_DEFAULTSIZE},
        {24, 5,  48, 48, LR_DEFAULTSIZE},
        {32, 9,  0,  0,  0},
        {32, 9,  48, 48, 0},
        {32, 10, 32, 32, 0},
        {32, 11, 24, 24, 0},
        {32, 12, 16, 16, 0},
        {32, 1,  0,  0,  LR_MONOCHROME},
        {32, 1,  48, 48, LR_MONOCHROME},
        {32, 2,  32, 32, LR_MONOCHROME},
        {32, 3,  24, 24, LR_MONOCHROME},
        {32, 4,  16, 16, LR_MONOCHROME},
        {32, 10, 0,  0,  LR_DEFAULTSIZE},
        {32, 9,  48, 48, LR_DEFAULTSIZE},
    };

    hMod = GetModuleHandle(NULL);
    ok(hMod != NULL, "\n");
    /* Find our cursor directory resource */
    hResource = FindResourceA(hMod,
                            MAKEINTRESOURCE(IDI_TEST),
                            RT_GROUP_ICON);
    ok(hResource != NULL, "\n");

    hMem = LoadResource(hMod, hResource);
    ok(hMem != NULL, "\n");

    lpResource = LockResource(hMem);
    ok(lpResource != NULL, "\n");

    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;

    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");

    dwOrigBpp = dm.dmBitsPerPel;

    for (i = 0; i < sizeof(TestData)/sizeof(TestData[0]); i++)
    {
        dm.dmBitsPerPel = TestData[i].bpp;
        if (ChangeDisplaySettingsExW(NULL, &dm, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            skip("Unable to change bpp to %lu.\n", dm.dmBitsPerPel);
            continue;
        }
        wResId = LookupIconIdFromDirectoryEx(lpResource, TRUE, TestData[i].cxDesired, TestData[i].cyDesired, TestData[i].flags);
        ok(wResId == TestData[i].wResId, "Got %d, expected %d for %dx%dx%lu, flags %x.\n",
            wResId,
            TestData[i].wResId,
            TestData[i].cxDesired,
            TestData[i].cyDesired,
            TestData[i].bpp,
            TestData[i].flags);
    }

    /* Restore */
    dm.dmBitsPerPel = dwOrigBpp;
    ok(ChangeDisplaySettingsExW(NULL, &dm, NULL, 0, NULL) == DISP_CHANGE_SUCCESSFUL, "\n");

    FreeResource(hMem);
}
