#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

START_TEST(EnumDisplaySettings)
{
    DEVMODEW dm;
    HDC icDisplay;

    /* TODO: test with a printer driver */

    icDisplay = CreateICW(L"DISPLAY", NULL, NULL, NULL);
    ok(icDisplay != NULL, "\n");

    dm.dmDriverExtra = 0x7777;

    /* Try ridiculous size */
    dm.dmSize = 0x8888;
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    ok(dm.dmBitsPerPel == GetDeviceCaps(icDisplay, BITSPIXEL), "%lu, should be %d.\n", dm.dmBitsPerPel, GetDeviceCaps(icDisplay, BITSPIXEL));
    ok(dm.dmSize == SIZEOF_DEVMODEW_300, "%d\n", dm.dmSize);
    ok(dm.dmDriverExtra == 0, "%d\n", dm.dmDriverExtra);

    /* Try a tiny size */
    dm.dmSize = 4;
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    ok(dm.dmBitsPerPel == GetDeviceCaps(icDisplay, BITSPIXEL), "%lu, should be %d.\n", dm.dmBitsPerPel, GetDeviceCaps(icDisplay, BITSPIXEL));
    ok(dm.dmSize == SIZEOF_DEVMODEW_300, "%d\n", dm.dmSize);

    /* Something in between */
    dm.dmSize = (SIZEOF_DEVMODEW_300 + SIZEOF_DEVMODEW_400) / 2 ;
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    ok(dm.dmBitsPerPel == GetDeviceCaps(icDisplay, BITSPIXEL), "%lu, should be %d.\n", dm.dmBitsPerPel, GetDeviceCaps(icDisplay, BITSPIXEL));
    ok(dm.dmSize == SIZEOF_DEVMODEW_300, "%d\n", dm.dmSize);

    /* WINVER < 0x0400 */
    dm.dmSize = SIZEOF_DEVMODEW_300;
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    ok(dm.dmBitsPerPel == GetDeviceCaps(icDisplay, BITSPIXEL), "%lu, should be %d.\n", dm.dmBitsPerPel, GetDeviceCaps(icDisplay, BITSPIXEL));
    ok(dm.dmSize == SIZEOF_DEVMODEW_300, "%d\n", dm.dmSize);

    /* WINVER < 0x0500 */
    dm.dmSize = SIZEOF_DEVMODEW_400;
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    ok(dm.dmBitsPerPel == GetDeviceCaps(icDisplay, BITSPIXEL), "%lu, should be %d.\n", dm.dmBitsPerPel, GetDeviceCaps(icDisplay, BITSPIXEL));
    ok(dm.dmSize == SIZEOF_DEVMODEW_300, "%d\n", dm.dmSize);

    /* "Modern" windows */
    dm.dmSize = SIZEOF_DEVMODEW_500;
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    ok(dm.dmBitsPerPel == GetDeviceCaps(icDisplay, BITSPIXEL), "%lu, should be %d.\n", dm.dmBitsPerPel, GetDeviceCaps(icDisplay, BITSPIXEL));
    ok(dm.dmSize == SIZEOF_DEVMODEW_300, "%d\n", dm.dmSize);

    DeleteDC(icDisplay);
}
