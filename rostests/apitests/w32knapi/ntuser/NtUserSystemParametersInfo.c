
static const WCHAR* KEY_MOUSE = L"Control Panel\\Mouse";
//static const WCHAR* VAL_MOUSE1 = L"MouseThreshold1";
//static const WCHAR* VAL_MOUSE2 = L"MouseThreshold2";
//static const WCHAR* VAL_MOUSE3 = L"MouseSpeed";
//static const WCHAR* VAL_DBLCLKWIDTH = L"DoubleClickWidth";
//static const WCHAR* VAL_DBLCLKHEIGHT = L"DoubleClickHeight";
//static const WCHAR* VAL_DBLCLKTIME = L"DoubleClickSpeed";
static const WCHAR* VAL_SWAP = L"SwapMouseButtons";

static const WCHAR* KEY_DESKTOP = L"Control Panel\\Desktop";
//static const WCHAR* VAL_SCRTO = L"ScreenSaveTimeOut";
//static const WCHAR* VAL_SCRACT = L"ScreenSaveActive";
//static const WCHAR* VAL_GRID = L"GridGranularity";
//static const WCHAR* VAL_DRAG = L"DragFullWindows";
//static const WCHAR* VAL_DRAGHEIGHT = L"DragHeight";
//static const WCHAR* VAL_DRAGWIDTH = L"DragWidth";
//static const WCHAR* VAL_FNTSMOOTH = L"FontSmoothing";
static const WCHAR* VAL_PREFMASK = L"UserPreferencesMask";

enum
{
    UPM_ACTIVEWINDOWTRACKING = 0x01,
    UPM_MENUANIMATION = 0x02,
    UPM_COMBOBOXANIMATION = 0x04,
    UPM_LISTBOXSMOOTHSCROLLING = 0x08,
    UPM_GRADIENTCAPTIONS = 0x10,
    UPM_KEYBOARDCUES = 0x20,
    UPM_ACTIVEWNDTRKZORDER = 0x40,
    UPM_HOTTRACKING = 0x80,
    UPM_RESERVED = 0x100,
    UPM_MENUFADE = 0x200,
    UPM_SELECTIONFADE = 0x400,
    UPM_TOOLTIPANIMATION = 0x800,
    UPM_TOOLTIPFADE = 0x1000,
    UPM_CURSORSHADOW = 0x2000,
    UPM_CLICKLOCK = 0x8000,
    // room for more
    UPM_UIEFFECTS = 0x80000000,
    UPM_DEFAULT = 0x80003E9E
} USERPREFMASKS;


//static const WCHAR* KEY_MDALIGN = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
//static const WCHAR* VAL_MDALIGN = L"MenuDropAlignment";

static const WCHAR* KEY_METRIC = L"Control Panel\\Desktop\\WindowMetrics";
//static const WCHAR* VAL_BORDER = L"BorderWidth";
//static const WCHAR* VAL_ICONSPC = L"IconSpacing";
//static const WCHAR* VAL_ICONVSPC = L"IconVerticalspacing";
//static const WCHAR* VAL_ITWRAP = L"IconTitleWrap";

static const WCHAR* KEY_SOUND = L"Control Panel\\Sound";
static const WCHAR* VAL_BEEP = L"Beep";

//static const WCHAR* KEY_KBD = L"Control Panel\\Keyboard";
//static const WCHAR* VAL_KBDSPD = L"KeyboardSpeed";
//static const WCHAR* VAL_KBDDELAY = L"KeyboardDelay";

//static const WCHAR* KEY_SHOWSNDS = L"Control Panel\\Accessibility\\ShowSounds";
//static const WCHAR* KEY_KDBPREF = L"Control Panel\\Accessibility\\Keyboard Preference";
//static const WCHAR* KEY_SCRREAD = L"Control Panel\\Accessibility\\Blind Access";
//static const WCHAR* VAL_ON = L"On";

LONG
QueryUserRegValueW(PCWSTR pszSubKey, PCWSTR pszValueName, PVOID pData, LPDWORD cbSize, LPDWORD pType)
{
	HKEY hKey;
	LONG ret;

	RegOpenKeyExW(HKEY_CURRENT_USER, pszSubKey, 0, KEY_QUERY_VALUE, &hKey);
	ret = RegQueryValueExW(hKey, pszValueName, NULL, pType, (LPBYTE)pData, cbSize);
	RegCloseKey(hKey);
	return ret;
}


HWND
CreateTestWindow()
{
	WNDCLASSA wc;

	wc.style = 0;
	wc.lpfnWndProc = DefWindowProcA;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "testclass";

	RegisterClassA(&wc);
	return CreateWindowA("testclass",
	                     "testwnd",
	                     WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     30,
	                     NULL,
	                     NULL,
	                     g_hInstance,
	                     0);
}

void
Test_NtUserSystemParametersInfo_Params(PTESTINFO pti)
{
//	UINT uint;
	DWORD data[1000];
	UINT i, uint;
	ACCESSTIMEOUT ato;
	/* Test normal */
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, &uint, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid SPI code */
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(7, 0, &data, 0) == FALSE);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test wrong cbSize member */
	ato.cbSize = 1;
	SetLastError(0xdeadbeef);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &ato, 0) == FALSE);
	TEST(GetLastError() == 0xdeadbeef);
#if 0
	/* Test undocumented, but valid SPI codes */
	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(0x1010, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] <= 1);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(0x1011, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);
	data[0] = 0;
	NtUserSystemParametersInfo(0x1010, 0, &data, 0);
	TEST(data[0] == 1);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(0x1028, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(0x1029, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(0x102A, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4139, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4140, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4141, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4142, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4143, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4144, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4145, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4146, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4147, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4148, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4149, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4150, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4151, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4152, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4153, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4154, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4155, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4156, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4157, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);
#endif
	/* Test invalid pointer */
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, (PVOID)0x80000000, 0) == FALSE);
	TEST(GetLastError() == ERROR_NOACCESS);
	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;

	/* test wrong bools */
}

void
Test_NtUserSystemParametersInfo_Winsta(PTESTINFO pti)
{
    HWINSTA hwinsta, hwinstaOld;
    INT ai[20];
    BOOL bRet;
//    INT i;

    hwinstaOld = GetProcessWindowStation();
    hwinsta = CreateWindowStation(NULL, 0, READ_CONTROL, NULL);
    SetProcessWindowStation(hwinsta);
    printf("hwinstaOld=%p, hwinsta=%p\n", hwinstaOld, hwinsta);

#if 1 // currently Winsta stuff is broken in ros
    TEST(SystemParametersInfoA(SPI_GETBEEP, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_SETBEEP, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    TEST(SystemParametersInfoA(SPI_GETMOUSE, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_SETMOUSE, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    TEST(SystemParametersInfoA(SPI_GETBORDER, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_SETBORDER, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    TEST(SystemParametersInfoA(SPI_GETKEYBOARDSPEED, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_SETKEYBOARDSPEED, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    TEST(SystemParametersInfoA(SPI_ICONHORIZONTALSPACING, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_ICONHORIZONTALSPACING, 32, 0, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    TEST(SystemParametersInfoA(SPI_GETSCREENSAVETIMEOUT, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_SETSCREENSAVETIMEOUT, 0, ai, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
    TEST(SystemParametersInfoA(SPI_GETKEYBOARDCUES, 0, &bRet, 0) == 0);
    TEST(GetLastError() == ERROR_ACCESS_DENIED);
    TEST(SystemParametersInfoA(SPI_SETKEYBOARDCUES, 0, (PVOID)1, 0) == 0);
    TEST(GetLastError() == ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
#endif

    SetProcessWindowStation(hwinstaOld);
}

void
Test_NtUserSystemParametersInfo_fWinIni(PTESTINFO pti)
{
	ACCESSTIMEOUT ato;
//	UINT uFocusBorderHeight;
	WCHAR Buffer[6];
	DWORD cbSize;

	ato.cbSize = sizeof(ato);
	NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &ato, 0);
	ato.iTimeOutMSec++;
	NtUserSystemParametersInfo(SPI_SETACCESSTIMEOUT, 0, &ato, 0);
	ato.iTimeOutMSec--;

//	NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, &uFocusBorderHeight, 0);
//	NtUserSystemParametersInfo(SPI_SETFOCUSBORDERHEIGHT, 0, &uFocusBorderHeight, SPIF_UPDATEINIFILE);

	NtUserSystemParametersInfo(SPI_SETACCESSTIMEOUT, 0, &ato, 0);

	cbSize = 6;
	QueryUserRegValueW(L"Control Panel\\Accessibility\\TimeOut", L"TimeToWait", &Buffer, &cbSize, NULL);

}

void
Test_MetricKey(PTESTINFO pti, PCWSTR pwszVal, INT iVal)
{
	WCHAR szReg[10];
	DWORD cbSize;

	cbSize = sizeof(szReg);
	TESTX(QueryUserRegValueW(KEY_METRIC, pwszVal, &szReg, &cbSize, NULL) == ERROR_SUCCESS, "Value=%S\n", pwszVal);
	TESTX(_wcsicmp(szReg, L"1") == 0, "Value=%S\n", pwszVal);

}

INT
Test_UserPref(PTESTINFO pti, UINT uiGet, UINT uiSet, DWORD dwPrefMask)
{
	BOOL bOrig, bTemp = 0;
	DWORD dwUserPref, dwUserPrefOrg;
	DWORD cbSize;

	/* Get original values */
	NtUserSystemParametersInfo(uiGet, 0, &bOrig, 0);
	cbSize = sizeof(dwUserPrefOrg);
	QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPrefOrg, &cbSize, NULL);

	/* Value 0 */
	NtUserSystemParametersInfo(uiSet, 0, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(uiGet, 0, &bTemp, 0);
	TEST(bTemp == 0);
	cbSize = sizeof(dwUserPref);
	TEST(QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPref, &cbSize, NULL) == ERROR_SUCCESS);
	TEST((dwUserPref & dwPrefMask) == 0);
	TEST((dwUserPref & (~dwPrefMask)) == (dwUserPrefOrg & (~dwPrefMask)));

	/* Value 1 without Registry */
	NtUserSystemParametersInfo(uiSet, 0, (PVOID)1, 0);
	NtUserSystemParametersInfo(uiGet, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(dwUserPref);
	TEST(QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPref, &cbSize, NULL) == ERROR_SUCCESS);
	TEST((dwUserPref & dwPrefMask) == 0);
	TEST((dwUserPref & (~dwPrefMask)) == (dwUserPrefOrg & (~dwPrefMask)));

	/* Value 2 without Registry */
	NtUserSystemParametersInfo(uiSet, 0, (PVOID)2, 0);
	NtUserSystemParametersInfo(uiGet, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(dwUserPref);
	TEST(QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPref, &cbSize, NULL) == ERROR_SUCCESS);
	TEST((dwUserPref & dwPrefMask) == 0);
	TEST((dwUserPref & (~dwPrefMask)) == (dwUserPrefOrg & (~dwPrefMask)));

	/* Value 1 with Registry */
	NtUserSystemParametersInfo(uiSet, 0, (PVOID)1, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(uiGet, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(dwUserPref);
	TEST(QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPref, &cbSize, NULL) == ERROR_SUCCESS);
	TEST((dwUserPref & dwPrefMask) == dwPrefMask);
	TEST((dwUserPref & (~dwPrefMask)) == (dwUserPrefOrg & (~dwPrefMask)));

	/* Restore original value */
	NtUserSystemParametersInfo(uiSet, 0, (PVOID)bOrig, SPIF_UPDATEINIFILE);

    return APISTATUS_NORMAL;
}


/******************************************************************************/

void
Test_SPI_SETBEEP(PTESTINFO pti)
{
	BOOL bOrig, bTemp = 0;
	WCHAR szReg[10];
	DWORD cbSize;

	/* Get original value */
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bOrig, 0);

	/* Value 0 */
	NtUserSystemParametersInfo(SPI_SETBEEP, 0, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bTemp, 0);
	TEST(bTemp == 0);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_SOUND, VAL_BEEP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"No") == 0);

	/* Value 1 */
	NtUserSystemParametersInfo(SPI_SETBEEP, 1, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_SOUND, VAL_BEEP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"Yes") == 0);

	/* Value 2 */
	NtUserSystemParametersInfo(SPI_SETBEEP, 2, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_SOUND, VAL_BEEP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"Yes") == 0);

	/* Restore original value */
	NtUserSystemParametersInfo(SPI_SETBEEP, 0, &bOrig, SPIF_UPDATEINIFILE);
}

void
Test_SPI_SETMOUSE(PTESTINFO pti)
{
	INT aiOrig[3], aiTemp[3];

	/* Get original value */
	NtUserSystemParametersInfo(SPI_GETMOUSE, 0, aiOrig, 0);

	/* Test uiParam value */
	TEST(NtUserSystemParametersInfo(SPI_GETMOUSE, 0, aiTemp, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETMOUSE, 1, aiTemp, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETMOUSE, -1, aiTemp, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETMOUSE, 0xdeadbeef, aiTemp, 0) == 1);

    /* Set modified values */
    aiTemp[0] = aiOrig[0] + 1;
    aiTemp[1] = aiOrig[1] - 1;
    aiTemp[2] = aiOrig[2] + 2;
	NtUserSystemParametersInfo(SPI_SETMOUSE, 2, aiTemp, SPIF_UPDATEINIFILE);
    aiTemp[0] = aiTemp[1] = aiTemp[2] = 0;

    /* Get new values */
	NtUserSystemParametersInfo(SPI_GETMOUSE, 0, aiTemp, 0);

    /* Test modified values */
    TEST(aiTemp[0] == aiOrig[0] + 1);
    TEST(aiTemp[1] == aiOrig[1] - 1);
    TEST(aiTemp[2] == aiOrig[2] + 2);

    // FIXME: Test registry values

	/* Restore original value */
	NtUserSystemParametersInfo(SPI_SETMOUSE, 0, aiOrig, SPIF_UPDATEINIFILE);
}

void
Test_SPI_SETBORDER(PTESTINFO pti)
{
	INT iOrig, iTemp = 0;

	/* Get original value */
	NtUserSystemParametersInfo(SPI_GETBORDER, 0, &iOrig, 0);

	/* Value 0 */
	NtUserSystemParametersInfo(SPI_SETBORDER, 0, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBORDER, 0, &iTemp, 0);
	TEST(iTemp == 1);
	TEST(GetSystemMetrics(SM_CXBORDER) == 1);

	/* Value 1 */
	NtUserSystemParametersInfo(SPI_SETBORDER, 1, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBORDER, 0, &iTemp, 0);
	TEST(iTemp == 1);
//	Test_MetricKey(pti, VAL_BORDER, 1);

	/* Value 2 */
	NtUserSystemParametersInfo(SPI_SETBORDER, 2, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBORDER, 0, &iTemp, 0);
	TEST(iTemp == 2);
	TEST(GetSystemMetrics(SM_CXBORDER) == 1);

	/* Restore original value */
	NtUserSystemParametersInfo(SPI_SETBORDER, iOrig, NULL, SPIF_UPDATEINIFILE);

}

//	Test_SPI_SETKEYBOARDSPEED(pti);
//	Test_SPI_LANGDRIVER(pti);
//	Test_SPI_ICONHORIZONTALSPACING(pti);
//	Test_SPI_SETSCREENSAVETIMEOUT(pti);
//	Test_SPI_SETSCREENSAVEACTIVE(pti);
//	Test_SPI_SETGRIDGRANULARITY(pti);

void
Test_SPI_SETDESKWALLPAPER(PTESTINFO pti)
{
    UNICODE_STRING ustrOld, ustrNew;
    WCHAR szOld[MAX_PATH];
    WCHAR szNew[MAX_PATH];

	/* Get old Wallpaper */
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, szOld, 0) == 1);
    RtlInitUnicodeString(&ustrOld, szOld);

	/* Set no Wallpaper */
	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, L"", 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, szNew, 0) == 1);
	TEST(szNew[0] == 0);

    /* Set no Wallpaper 2 */
    RtlInitUnicodeString(&ustrNew, L"");
	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &ustrNew, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, szNew, 0) == 1);
	TEST(szNew[0] == 0);

    /* Reset Wallpaper */
	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, szNew, 0) == 1);
	TEST(wcscmp(szNew, szOld) == 0);

    /* Set new Wallpaper */
    RtlInitUnicodeString(&ustrNew, L"test.bmp");
	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &ustrNew, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, szNew, 0) == 1);
	TEST(wcscmp(szNew, L"test.bmp") == 0);

	/* Get Wallpaper, too small buffer  */
	szNew[0] = 0; szNew[1] = 0; szNew[2] = 0;
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, 3, szNew, 0) == 1);
    TEST(szNew[0] != 0);
    TEST(szNew[1] != 0);
    TEST(szNew[2] != 0);

    /* Set invalid Wallpaper */
    SetLastError(0xdeadbeef);
    RtlInitUnicodeString(&ustrNew, L"*#!!-&");
	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &ustrNew, 0) == 0);
	TEST(GetLastError() == ERROR_FILE_NOT_FOUND);
	TEST(NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, szNew, 0) == 1);
	TEST(wcscmp(szNew, L"*#!!-&") == 0);

	/* Restore old Wallpaper */
	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, MAX_PATH, &ustrOld, SPIF_UPDATEINIFILE) == 1);

}

//	Test_SPI_SETDESKPATTERN(pti);
//	Test_SPI_SETKEYBOARDDELAY(pti);
//	Test_SPI_ICONVERTICALSPACING(pti);
//	Test_SPI_SETICONTITLEWRAP(pti);
//	Test_SPI_SETMENUDROPALIGNMENT(pti);
//	Test_SPI_SETDOUBLECLKWIDTH(pti);
//	Test_SPI_SETDOUBLECLKHEIGHT(pti);
//	Test_SPI_SETDOUBLECLICKTIME(pti);

INT
Test_SPI_SETMOUSEBUTTONSWAP(PTESTINFO pti)
{
	BOOL bOrig, bTemp = 0;
	WCHAR szReg[10];
	DWORD cbSize;

	/* Get original value */
	bOrig = GetSystemMetrics(SM_SWAPBUTTON);

	/* Value 0 */
	NtUserSystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, 0, NULL, SPIF_UPDATEINIFILE);
	bTemp = GetSystemMetrics(SM_SWAPBUTTON);
	TEST(bTemp == 0);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_MOUSE, VAL_SWAP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"0") == 0);

	/* Value 1 */
	NtUserSystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, 1, NULL, SPIF_UPDATEINIFILE);
	bTemp = GetSystemMetrics(SM_SWAPBUTTON);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_MOUSE, VAL_SWAP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"1") == 0);

	/* Value 2 */
	NtUserSystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, 2, NULL, SPIF_UPDATEINIFILE);
	bTemp = GetSystemMetrics(SM_SWAPBUTTON);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_MOUSE, VAL_SWAP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"2") == 0);

	/* Value -1 */
	NtUserSystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, -1, NULL, SPIF_UPDATEINIFILE);
	bTemp = GetSystemMetrics(SM_SWAPBUTTON);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(KEY_MOUSE, VAL_SWAP, &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"-1") == 0);

	/* Restore original value */
	NtUserSystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, bOrig, 0, SPIF_UPDATEINIFILE);

    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETICONTITLELOGFONT(PTESTINFO pti)
{
    LOGFONTW lfOrig;
    struct
    {
        LOGFONTW lf;
        DWORD dwRedzone;
    } buf;

	/* Get original value */
	ASSERT(NtUserSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lfOrig), &lfOrig, 0));

    /* Test uiParam == 0 */
    memset(&buf, 0, sizeof(buf));
    buf.lf.lfFaceName[LF_FACESIZE-1] = 33;
    buf.dwRedzone = 0xdeadbeef;
	TEST(NtUserSystemParametersInfo(SPI_GETICONTITLELOGFONT, 0, &buf.lf, 0) == 1);
    TEST(buf.lf.lfHeight != 0);
    TEST(buf.lf.lfWeight != 0);
    TEST(buf.lf.lfFaceName[0] != 0);
    TEST(buf.lf.lfFaceName[LF_FACESIZE-1] == 0);
    TEST(buf.dwRedzone == 0xdeadbeef);

    /* Test uiParam < sizeof(LOGFONTW) */
    memset(&buf, 0, sizeof(buf));
    buf.lf.lfFaceName[LF_FACESIZE-1] = 33;
    buf.dwRedzone = 0xdeadbeef;
	TEST(NtUserSystemParametersInfo(SPI_GETICONTITLELOGFONT, 8, &buf.lf, 0) == 1);
    TEST(buf.lf.lfHeight != 0);
    TEST(buf.lf.lfWeight != 0);
    TEST(buf.lf.lfFaceName[0] != 0);
    TEST(buf.lf.lfFaceName[LF_FACESIZE-1] == 0);
    TEST(buf.dwRedzone == 0xdeadbeef);

    /* Test uiParam < 0 */
	TEST(NtUserSystemParametersInfo(SPI_GETICONTITLELOGFONT, -1, &buf.lf, 0) == 1);

    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETFASTTASKSWITCH(PTESTINFO pti)
{
    char buf[10];
	TEST(NtUserSystemParametersInfo(SPI_SETFASTTASKSWITCH, 0, 0, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_SETFASTTASKSWITCH, 0, buf, 0) == 1);

    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETDRAGFULLWINDOWS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETNONCLIENTMETRICS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETMINIMIZEDMETRICS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETICONMETRICS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETWORKAREA(PTESTINFO pti)
{
    RECT rcOrig, rc;

    /* Get the original value */
	ASSERT(NtUserSystemParametersInfo(SPI_GETWORKAREA, 0, &rcOrig, 0) == 1);

    /* Change value */
    rc = rcOrig;
    rc.left += 1;
    rc.top += 2;
    rc.right -= 3;
    rc.bottom -= 2;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 1, &rc, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, sizeof(RECT), &rc, 0) == 1);

	TEST(NtUserSystemParametersInfo(SPI_GETWORKAREA, 1, &rc, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETWORKAREA, -1, &rc, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETWORKAREA, 0xdeadbeef, &rc, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)0xdeadbeef, 0) == 0);

    /* Test values */
    rc = rcOrig; rc.left = -1;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.top = -1;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.top = 10; rc.bottom = 11;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 1);
    rc = rcOrig; rc.top = 10; rc.bottom = 10;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.top = 10; rc.bottom = 9;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.left = 10; rc.right = 11;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 1);
    rc = rcOrig; rc.left = 10; rc.right = 10;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.left = 10; rc.right = 9;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 1);
    rc = rcOrig; rc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN)+1;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);
    rc = rcOrig; rc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 1);
    rc = rcOrig; rc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN)+1;
	TEST(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rc, 0) == 0);

    /* Restore original value */
	ASSERT(NtUserSystemParametersInfo(SPI_SETWORKAREA, 0, &rcOrig, 0) == 1);

    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETPENWINDOWS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETFILTERKEYS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETTOGGLEKEYS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETMOUSEKEYS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETSHOWSOUNDS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETSTICKYKEYS(PTESTINFO pti)
{
	STICKYKEYS skOrig, sk;

    /* Get original values */
	skOrig.cbSize = sizeof(STICKYKEYS);
	ASSERT(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 0, &skOrig, 0) == 1);

	sk.cbSize = sizeof(STICKYKEYS)+1;
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 0, &sk, 0) == 0);

	sk.cbSize = sizeof(STICKYKEYS)-1;
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 0, &sk, 0) == 0);

	sk.cbSize = sizeof(STICKYKEYS);
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 1, &sk, 0) == 0);
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, -1, &sk, 0) == 0);
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, sk.cbSize, &sk, 0) == 1);
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, sk.cbSize-1, &sk, 0) == 0);
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, sk.cbSize+1, &sk, 0) == 0);

    sk = skOrig;
    sk.dwFlags = (skOrig.dwFlags ^ 1);
	TEST(NtUserSystemParametersInfo(SPI_SETSTICKYKEYS, sk.cbSize+1, &sk, 0) == 0);
	TEST(NtUserSystemParametersInfo(SPI_SETSTICKYKEYS, sk.cbSize-1, &sk, 0) == 0);
	TEST(NtUserSystemParametersInfo(SPI_SETSTICKYKEYS, sk.cbSize, &sk, 0) == 1);

    sk = skOrig;
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 0, &sk, 0) == 1);
    TEST(sk.dwFlags == (skOrig.dwFlags ^ 1));

    /* Restore original values */
	skOrig.cbSize = sizeof(STICKYKEYS);
	ASSERT(NtUserSystemParametersInfo(SPI_SETSTICKYKEYS, 0, &skOrig, 0) == 1);

	return APISTATUS_NORMAL;
}

INT
Test_SPI_SETACCESSTIMEOUT(PTESTINFO pti)
{
    ACCESSTIMEOUT atoOrig, atoTmp;

    /* Get original values */
    atoOrig.cbSize = sizeof(ACCESSTIMEOUT);
	ASSERT(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &atoOrig, 0) == 1);

    atoTmp.cbSize = sizeof(ACCESSTIMEOUT) - 1;
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &atoTmp, 0) == 0);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT) + 1;
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &atoTmp, 0) == 0);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &atoTmp, 0) == 1);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 1, &atoTmp, 0) == 0);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, -1, &atoTmp, 0) == 0);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, sizeof(ACCESSTIMEOUT), &atoTmp, 0) == 1);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, sizeof(ACCESSTIMEOUT)-1, &atoTmp, 0) == 0);
    atoTmp.cbSize = sizeof(ACCESSTIMEOUT);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, sizeof(ACCESSTIMEOUT)+1, &atoTmp, 0) == 0);

    /* Restore original values */
	ASSERT(NtUserSystemParametersInfo(SPI_SETACCESSTIMEOUT, sizeof(atoOrig), &atoOrig, 0) == 1);

    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETSERIALKEYS(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETSOUNDSENTRY(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETHIGHCONTRAST(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

INT
Test_SPI_SETKEYBOARDPREF(PTESTINFO pti)
{
    return APISTATUS_NORMAL;
}

//	Test_SPI_SETSCREENREADER(pti);
///	Test_SPI_SETANIMATION(pti);
//	Test_SPI_SETFONTSMOOTHING(pti);
//	Test_SPI_SETDRAGWIDTH(pti);
//	Test_SPI_SETDRAGHEIGHT(pti);
//	Test_SPI_SETHANDHELD(pti);
//	Test_SPI_SETLOWPOWERTIMEOUT(pti);
//	Test_SPI_SETPOWEROFFTIMEOUT(pti);
//	Test_SPI_SETLOWPOWERACTIVE(pti);
//	Test_SPI_SETPOWEROFFACTIVE(pti);
//	Test_SPI_SETCURSORS(pti);
//	Test_SPI_SETICONS(pti);
//	Test_SPI_SETDEFAULTINPUTLANG(pti);
//	Test_SPI_SETLANGTOGGLE(pti);
//	Test_SPI_GETWINDOWSEXTENSION(pti);
//	Test_SPI_SETMOUSETRAILS(pti);
//	Test_SPI_SETSNAPTODEFBUTTON(pti);
//	Test_SPI_GETSCREENSAVERRUNNING(pti);
//	Test_SPI_SETMOUSEHOVERWIDTH(pti);
//	Test_SPI_SETMOUSEHOVERHEIGHT(pti);
//	Test_SPI_SETMOUSEHOVERTIME(pti);
//	Test_SPI_SETWHEELSCROLLLINES(pti);
//	Test_SPI_SETMENUSHOWDELAY(pti);
//	Test_SPI_SETWHEELSCROLLCHARS(pti);
//	Test_SPI_SETSHOWIMEUI(pti);
//	Test_SPI_SETMOUSESPEED(pti);
//	Test_SPI_GETSCREENSAVERRUNNING(pti);
//	Test_SPI_SETAUDIODESCRIPTION(pti);
//	Test_SPI_SETSCREENSAVESECURE(pti);
//	Test_SPI_SETACTIVEWINDOWTRACKING(pti);

INT
Test_SPI_SETMENUANIMATION(PTESTINFO pti)
{
	BOOL bOrig, bTemp = 0;
	DWORD dwUserPrefMask;
	DWORD cbSize;

	/* Get original values */
	NtUserSystemParametersInfo(SPI_GETMENUANIMATION, 0, &bOrig, 0);

	/* Value 0 */
	NtUserSystemParametersInfo(SPI_SETMENUANIMATION, 0, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETMENUANIMATION, 0, &bTemp, 0);
	TEST(bTemp == 0);
	cbSize = sizeof(dwUserPrefMask);
	TEST(QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPrefMask, &cbSize, NULL) == ERROR_SUCCESS);
	TEST((dwUserPrefMask & UPM_MENUANIMATION) == 0);

	/* Value 1 */
	NtUserSystemParametersInfo(SPI_SETMENUANIMATION, 0, (PVOID)1, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETMENUANIMATION, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(dwUserPrefMask);
	TEST(QueryUserRegValueW(KEY_DESKTOP, VAL_PREFMASK, &dwUserPrefMask, &cbSize, NULL) == ERROR_SUCCESS);
	TEST((dwUserPrefMask & UPM_MENUANIMATION) != 0);


	/* Restore original values */
	NtUserSystemParametersInfo(SPI_SETMENUANIMATION, 0, (PVOID)bOrig, SPIF_UPDATEINIFILE);

    return APISTATUS_NORMAL;
}

//	Test_SPI_SETCOMBOBOXANIMATION(pti);
//	Test_SPI_SETLISTBOXSMOOTHSCROLLING(pti);
//	Test_SPI_SETGRADIENTCAPTIONS(pti);

INT
Test_SPI_SETKEYBOARDCUES(PTESTINFO pti)
{
    return Test_UserPref(pti, SPI_GETKEYBOARDCUES, SPI_SETKEYBOARDCUES, UPM_KEYBOARDCUES);
}

//	Test_SPI_SETACTIVEWNDTRKZORDER(pti);
//	Test_SPI_SETHOTTRACKING(pti);
//	Test_SPI_SETMENUFADE(pti);
//	Test_SPI_SETSELECTIONFADE(pti);
//	Test_SPI_SETTOOLTIPANIMATION(pti);
//	Test_SPI_SETTOOLTIPFADE(pti);
//	Test_SPI_SETCURSORSHADOW(pti);
//	Test_SPI_SETMOUSESONAR(pti);

INT
Test_SPI_SETMOUSECLICKLOCK(PTESTINFO pti)
{
    return Test_UserPref(pti, SPI_GETMOUSECLICKLOCK, SPI_SETMOUSECLICKLOCK, UPM_CLICKLOCK);
}

//	Test_SPI_SETMOUSEVANISH(pti);
//	Test_SPI_SETFLATMENU(pti);
//	Test_SPI_SETDROPSHADOW(pti);
//	Test_SPI_SETBLOCKSENDINPUTRESETS(pti);
//	Test_SPI_GETSETUIEFFECTS(pti);
//	Test_SPI_SETDISABLEOVERLAPPEDCONTENT(pti);
//	Test_SPI_SETCLIENTAREAANIMATION(pti);
//	Test_SPI_SETCLEARTYPE(pti);
//	Test_SPI_SETSPEECHRECOGNITION(pti);
//	Test_SPI_SETFOREGROUNDLOCKTIMEOUT(pti);
//	Test_SPI_SETACTIVEWNDTRKTIMEOUT(pti);
//	Test_SPI_SETFOREGROUNDFLASHCOUNT(pti);
//	Test_SPI_SETCARETWIDTH(pti);
//	Test_SPI_SETMOUSECLICKLOCKTIME(pti);
//	Test_SPI_SETFONTSMOOTHINGTYPE(pti);
//	Test_SPI_SETFONTSMOOTHINGCONTRAST(pti);
//	Test_SPI_SETFOCUSBORDERWIDTH(pti);
//	Test_SPI_SETFOCUSBORDERHEIGHT(pti);
//	Test_SPI_SETFONTSMOOTHINGORIENTATION(pti);


INT
Test_NtUserSystemParametersInfo(PTESTINFO pti)
{
	HWND hWnd;

	hWnd = CreateTestWindow();
	ASSERT(hWnd);

	Test_NtUserSystemParametersInfo_Params(pti);
	Test_NtUserSystemParametersInfo_fWinIni(pti);
    Test_NtUserSystemParametersInfo_Winsta(pti);

	Test_SPI_SETBEEP(pti);
    Test_SPI_SETMOUSE(pti);
    Test_SPI_SETBORDER(pti);
//	Test_SPI_SETKEYBOARDSPEED(pti);
//	Test_SPI_LANGDRIVER(pti);
//	Test_SPI_ICONHORIZONTALSPACING(pti);
//	Test_SPI_SETSCREENSAVETIMEOUT(pti);
//	Test_SPI_SETSCREENSAVEACTIVE(pti);
//	Test_SPI_SETGRIDGRANULARITY(pti);
	Test_SPI_SETDESKWALLPAPER(pti);
//	Test_SPI_SETDESKPATTERN(pti);
//	Test_SPI_SETKEYBOARDDELAY(pti);
//	Test_SPI_ICONVERTICALSPACING(pti);
//	Test_SPI_SETICONTITLEWRAP(pti);
//	Test_SPI_SETMENUDROPALIGNMENT(pti);
//	Test_SPI_SETDOUBLECLKWIDTH(pti);
//	Test_SPI_SETDOUBLECLKHEIGHT(pti);
//	Test_SPI_SETDOUBLECLICKTIME(pti);
	Test_SPI_SETMOUSEBUTTONSWAP(pti);
	Test_SPI_SETICONTITLELOGFONT(pti);
	Test_SPI_SETFASTTASKSWITCH(pti);
	Test_SPI_SETDRAGFULLWINDOWS(pti);
	Test_SPI_SETNONCLIENTMETRICS(pti);
	Test_SPI_SETMINIMIZEDMETRICS(pti);
	Test_SPI_SETICONMETRICS(pti);
	Test_SPI_SETWORKAREA(pti);
	Test_SPI_SETPENWINDOWS(pti);
	Test_SPI_SETFILTERKEYS(pti);
	Test_SPI_SETTOGGLEKEYS(pti);
	Test_SPI_SETMOUSEKEYS(pti);
	Test_SPI_SETSHOWSOUNDS(pti);
	Test_SPI_SETSTICKYKEYS(pti);
	Test_SPI_SETACCESSTIMEOUT(pti);
	Test_SPI_SETSERIALKEYS(pti);
	Test_SPI_SETSOUNDSENTRY(pti);
	Test_SPI_SETHIGHCONTRAST(pti);
	Test_SPI_SETKEYBOARDPREF(pti);
//	Test_SPI_SETSCREENREADER(pti);
///	Test_SPI_SETANIMATION(pti);
//	Test_SPI_SETFONTSMOOTHING(pti);
//	Test_SPI_SETDRAGWIDTH(pti);
//	Test_SPI_SETDRAGHEIGHT(pti);
//	Test_SPI_SETHANDHELD(pti);
//	Test_SPI_SETLOWPOWERTIMEOUT(pti);
//	Test_SPI_SETPOWEROFFTIMEOUT(pti);
//	Test_SPI_SETLOWPOWERACTIVE(pti);
//	Test_SPI_SETPOWEROFFACTIVE(pti);
//	Test_SPI_SETCURSORS(pti);
//	Test_SPI_SETICONS(pti);
//	Test_SPI_SETDEFAULTINPUTLANG(pti);
//	Test_SPI_SETLANGTOGGLE(pti);
//	Test_SPI_GETWINDOWSEXTENSION(pti);
//	Test_SPI_SETMOUSETRAILS(pti);
//	Test_SPI_SETSNAPTODEFBUTTON(pti);
//	Test_SPI_GETSCREENSAVERRUNNING(pti);
//	Test_SPI_SETMOUSEHOVERWIDTH(pti);
//	Test_SPI_SETMOUSEHOVERHEIGHT(pti);
//	Test_SPI_SETMOUSEHOVERTIME(pti);
//	Test_SPI_SETWHEELSCROLLLINES(pti);
//	Test_SPI_SETMENUSHOWDELAY(pti);
//	Test_SPI_SETWHEELSCROLLCHARS(pti);
//	Test_SPI_SETSHOWIMEUI(pti);
//	Test_SPI_SETMOUSESPEED(pti);
//	Test_SPI_GETSCREENSAVERRUNNING(pti);
//	Test_SPI_SETAUDIODESCRIPTION(pti);
//	Test_SPI_SETSCREENSAVESECURE(pti);
//	Test_SPI_SETACTIVEWINDOWTRACKING(pti);
	Test_SPI_SETMENUANIMATION(pti);
//	Test_SPI_SETCOMBOBOXANIMATION(pti);
//	Test_SPI_SETLISTBOXSMOOTHSCROLLING(pti);
//	Test_SPI_SETGRADIENTCAPTIONS(pti);
	Test_SPI_SETKEYBOARDCUES(pti);
//	Test_SPI_SETACTIVEWNDTRKZORDER(pti);
//	Test_SPI_SETHOTTRACKING(pti);
//	Test_SPI_SETMENUFADE(pti);
//	Test_SPI_SETSELECTIONFADE(pti);
//	Test_SPI_SETTOOLTIPANIMATION(pti);
//	Test_SPI_SETTOOLTIPFADE(pti);
//	Test_SPI_SETCURSORSHADOW(pti);
//	Test_SPI_SETMOUSESONAR(pti);
	Test_SPI_SETMOUSECLICKLOCK(pti);
//	Test_SPI_SETMOUSEVANISH(pti);
//	Test_SPI_SETFLATMENU(pti);
//	Test_SPI_SETDROPSHADOW(pti);
//	Test_SPI_SETBLOCKSENDINPUTRESETS(pti);
//	Test_SPI_GETSETUIEFFECTS(pti);
//	Test_SPI_SETDISABLEOVERLAPPEDCONTENT(pti);
//	Test_SPI_SETCLIENTAREAANIMATION(pti);
//	Test_SPI_SETCLEARTYPE(pti);
//	Test_SPI_SETSPEECHRECOGNITION(pti);
//	Test_SPI_SETFOREGROUNDLOCKTIMEOUT(pti);
//	Test_SPI_SETACTIVEWNDTRKTIMEOUT(pti);
//	Test_SPI_SETFOREGROUNDFLASHCOUNT(pti);
//	Test_SPI_SETCARETWIDTH(pti);
//	Test_SPI_SETMOUSECLICKLOCKTIME(pti);
//	Test_SPI_SETFONTSMOOTHINGTYPE(pti);
//	Test_SPI_SETFONTSMOOTHINGCONTRAST(pti);
//	Test_SPI_SETFOCUSBORDERWIDTH(pti);
//	Test_SPI_SETFOCUSBORDERHEIGHT(pti);
//	Test_SPI_SETFONTSMOOTHINGORIENTATION(pti);

	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
