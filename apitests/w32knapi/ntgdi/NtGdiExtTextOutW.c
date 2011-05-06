
/* 
BOOL
APIENTRY
NtGdiExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL LPRECT UnsafeRect,
    IN LPWSTR UnsafeString,
    IN INT Count,
    IN OPTIONAL LPINT UnsafeDx,
    IN DWORD dwCodePage)
*/

INT
Test_NtGdiExtTextOutW(PTESTINFO pti)
{
    HWND hWnd;
    HDC hDC;
    RECT rect;
    LPWSTR lpstr;
    BOOL ret;
    ULONG len;
    INT Dx[10] = {10, -5, 10, 5, 10, -10, 10, 5, 10, 5};

	/* Create a window */
	hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
	hDC = GetDC(hWnd);

    lpstr = L"Hallo";
    len = wcslen(lpstr);

    ret = NtGdiExtTextOutW(hDC, 0, 0, 0, &rect, lpstr, len, Dx, 0);
    TEST(ret == 1);

    ret = NtGdiExtTextOutW(hDC, 0, 0, ETO_PDY, &rect, lpstr, len, Dx, 0);
    TEST(ret == 1);

    /* Test invalid lpDx */
    ret = NtGdiExtTextOutW(hDC, 0, 0, 0, 0, lpstr, len, (INT*)((ULONG_PTR)-1), 0);
    TEST(ret == 0);

    /* Test alignment requirement for lpDx */
    ret = NtGdiExtTextOutW(hDC, 0, 0, 0, 0, lpstr, len, (INT*)((ULONG_PTR)Dx + 1), 0);
    TEST(ret == 1);

    return APISTATUS_NORMAL;
}

