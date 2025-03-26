
#include "precomp.h"

START_TEST(DestroyCursorIcon)
{
    HICON hicon;
    HCURSOR hcursor;
    ICONINFO iconinfo;

    ZeroMemory(&iconinfo, sizeof(iconinfo));

    iconinfo.hbmMask = CreateBitmap(8, 16, 1, 1, NULL);
    ok(iconinfo.hbmMask != NULL, "\n");

    /*
     * Test if DestroyCursor can destroy an icon, and vice-versa .
     * It can.
     */
    iconinfo.fIcon = TRUE;
    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon != 0, "should not fail\n");
    ok(DestroyCursor(hicon), "\n");
    ok(!DestroyIcon(hicon), "\n");

    iconinfo.fIcon = FALSE;
    hcursor = CreateIconIndirect(&iconinfo);
    ok(hcursor != 0, "should not fail\n");
    ok(DestroyIcon(hcursor), "\n");
    ok(!DestroyCursor(hcursor), "\n");

    /* Clean up */
    DeleteObject(iconinfo.hbmMask);

    /* Now check its behaviour regarding Shared icons/cursors */
    hcursor = LoadCursor(GetModuleHandle(NULL), "TESTCURSOR");
    ok(hcursor != 0, "\n");

    /* MSDN says we shouldn't do that, but it still succeeds */
    ok(DestroyCursor(hcursor), "\n");

    /* In fact, it's still there */
    ZeroMemory(&iconinfo, sizeof(iconinfo));
    ok(GetIconInfo(hcursor, &iconinfo), "\n");
    ok(iconinfo.hbmMask != NULL, "\n");
    ok(iconinfo.hbmColor != NULL, "\n");
    ok(!iconinfo.fIcon, "\n");

    /* clean up */
    DeleteObject(iconinfo.hbmMask);
    DeleteObject(iconinfo.hbmColor);
}
