INT
Test_CreatePen(PTESTINFO pti)
{
	HPEN hPen;
	LOGPEN logpen;

	SetLastError(ERROR_SUCCESS);
	hPen = CreatePen(PS_DASHDOT, 5, RGB(1,2,3));
	RTEST(hPen);

	/* Test if we have a PEN */
	RTEST(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_PEN);

	GetObject(hPen, sizeof(logpen), &logpen);
	RTEST(logpen.lopnStyle == PS_DASHDOT);
	RTEST(logpen.lopnWidth.x == 5);
	RTEST(logpen.lopnColor == RGB(1,2,3));
	DeleteObject(hPen);

	/* PS_GEOMETRIC | PS_DASHDOT = 0x00001011 will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_GEOMETRIC | PS_DASHDOT, 5, RGB(1,2,3));
	RTEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	RTEST(logpen.lopnStyle == PS_SOLID);
	DeleteObject(hPen);

	/* PS_USERSTYLE will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_USERSTYLE, 5, RGB(1,2,3));
	RTEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	RTEST(logpen.lopnStyle == PS_SOLID);
	DeleteObject(hPen);

	/* PS_ALTERNATE will become PS_SOLID */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_ALTERNATE, 5, RGB(1,2,3));
	RTEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	RTEST(logpen.lopnStyle == PS_SOLID);
	DeleteObject(hPen);

	/* PS_INSIDEFRAME is ok */
	logpen.lopnStyle = 22;
	hPen = CreatePen(PS_INSIDEFRAME, 5, RGB(1,2,3));
	RTEST(hPen);
	GetObject(hPen, sizeof(logpen), &logpen);
	RTEST(logpen.lopnStyle == PS_INSIDEFRAME);
	DeleteObject(hPen);

	RTEST(GetLastError() == ERROR_SUCCESS);

	return APISTATUS_NORMAL;
}

