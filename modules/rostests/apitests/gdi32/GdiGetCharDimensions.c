/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GdiGetCharDimensions
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_GdiGetCharDimensions()
{
	LOGFONT logfont = {-11, 0, 0, 0, 400,
	                    0, 0, 0, 0, 0, 0, 0, 0,
	                    "MS Shell Dlg 2"};
	HFONT hFont, hOldFont;
	HDC hdc;
	LONG x, y, x2;
	TEXTMETRICW tm;
	SIZE size;
    static const WCHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

	hFont = CreateFontIndirect(&logfont);
	hdc = CreateCompatibleDC(NULL);
	hOldFont = SelectObject(hdc, hFont);

	x = GdiGetCharDimensions(hdc, &tm, &y);
    GetTextExtentPointW(hdc, alphabet, 52, &size);
    x2 = (size.cx / 26 + 1) / 2;

    ok(x == x2, "x=%ld, x2=%ld\n", x, x2);
	ok(y == tm.tmHeight, "y = %ld, tm.tmHeight = %ld\n", y, tm.tmHeight);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
	DeleteDC(hdc);
}

START_TEST(GdiGetCharDimensions)
{
    Test_GdiGetCharDimensions();
}

