LONG WINAPI GdiGetCharDimensions(HDC, LPTEXTMETRICW, LONG *);

INT
Test_GdiGetCharDimensions(PTESTINFO pti)
{
	LOGFONT logfont = {-11, 0, 0, 0, 400,
	                    0, 0, 0, 0, 0, 0, 0, 0,
	                    "MS Shell Dlg 2"};
	HFONT hFont, hOldFont;
	HDC hDC;
	LONG x,y;
	TEXTMETRICW tm;

	hFont = CreateFontIndirect(&logfont);
	hDC = CreateCompatibleDC(NULL);
	hOldFont = SelectObject(hDC, hFont);

	x = GdiGetCharDimensions(hDC, &tm, &y);

	RTEST(y == tm.tmHeight);

	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);
	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
