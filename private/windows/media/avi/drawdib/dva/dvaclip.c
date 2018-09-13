#define TestWF(hwnf, f) (GetWindowLong(hwnd, GWL_STYLE) & (f))

BOOL DVAIsClipped(HDC hdc, RECT& rc)
{
    RECT rcClip;

    if (GetClipBox(hdc, &rcClip) != SIMPLEREGION)
        return TRUE;

    if (rc.left   < rcClip.left  ||
        rc.top    < rcClip.top   ||
        rc.right  > rcClip.right ||
        rc.bottom > rcClip.bottom)

        return TRUE;

    return FALSE;
}

int DVAGetClipList(HWND hwnd, LPRECT prc, LPRECT RectList, int RectCount)
{
    RECT rc;
    HWND hwndP;
    HWND hwndT;

    if (RectCount == 0 || !IsVisible(hwnd))
        return 0;

    //
    // get the client area of the window
    //
    GetClientRect(hwnd, RectList);

    if (prcTest)
        IntersectRect(RectList, RectList, prc);

    ClientToScreen(hwnd, (LPPOINT)RectList);
    ClientToScreen(hwnd, (LPPOINT)RectList + 1);

    RectCount = 1;
    RectList[1] = RectList[0];

    //
    // walk all children of hwnd and remove them if needed
    //
    if (TestWF(hwnd WS_CLIPCHILDREN))
    {
        RectCount = ExcludeWindowRects(RectList, RectCount,
            GetWindow(hwnd, GW_CHILD), NULL);

        if (RectCount == 0)
            return 0;
    }

    //
    // walk all the siblings of hwnd and exclude them from the list.
    //
    for (; (hwndP = GetWindow(hwnd, GW_PARENT)) != NULL; hwnd = hwndP)
    {
        GetWindowRect(hwndP, &rc);
        RectCount = IntersectRectList(RectList, RectCount, &rc;

        if (RectCount == 0)
            return 0;

        if (TestWF(hwnd, WS_CLIPSIBLINGS))
        {
            RectCount = ExcludeWindowRects(RectList, RectCount,
                GetWindow(hwndP, GW_CHILD), hwnd);

            if (RectCount == 0)
                return 0;
        }
    }

    return RectCount;
}

int ExcludeWindowRects(LPRECT RectList, int RectCount, HWND hwndA, HWND hwndB)
{
    RECT rc;

    for (hwnd = hwndA; hwnd != NULL && hwnd != hwndB; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        if (!IsWindowVisible(hwnd))
	    continue;

	// Don't subtract off transparent windows...
	//
	if (TestWF(hwnd, WEFTRANSPARENT))
            continue;

        GetWindowRect(hwnd, &rc);

        RectCount = ExcludeRectList(RectList, RectCount, &rc);

        if (RectCount == 0)
            return 0;
    }

    return RectCount;
}


int ExcludeRectList(LPRECT RectList, int RectCount, LPRECT prc)
{
    int i;
    int n;
    RECT rc;

    if (RectCount == 0)
        return 0;

    SubtractRect(RectList, prc);

    for (i=1; i <= RectCount; i++)
    {
        if (!IntersectRect(&rc, &RectList[i], prc))
            continue;

        //
        //
        //

    }
}
