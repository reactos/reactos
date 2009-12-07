BOOL
NTAPI
NtUserGetClassInfo2(
	HINSTANCE hInstance,
	PUNICODE_STRING ClassName,
	LPWNDCLASSEXW wcex,
	LPWSTR *ppwstr,
	BOOL Ansi)
{
	return (BOOL)Syscall(L"NtUserGetClassInfo", 5, &hInstance);
}

INT
Test_NtUserGetClassInfo(PTESTINFO pti)
{
	WNDCLASSEXW wclex, wclex2 = {0};
	UNICODE_STRING us;
	PWSTR pwstr;

	us.Length = 8;
	us.MaximumLength = 8;
	us.Buffer = L"test";

	wclex.cbSize = sizeof(WNDCLASSEXW);
	wclex.style = 0;
	wclex.lpfnWndProc = NULL;
	wclex.cbClsExtra = 2;
	wclex.cbWndExtra = 4;
	wclex.hInstance = g_hInstance;
	wclex.hIcon = NULL;
	wclex.hCursor = NULL;
	wclex.hbrBackground = CreateSolidBrush(RGB(4,7,5));
	wclex.lpszMenuName = L"MyMenu";
	wclex.lpszClassName = us.Buffer;
	wclex.hIconSm = NULL;

	ASSERT(RegisterClassExW(&wclex) != 0);

	TEST(GetClassInfoExW(g_hInstance, us.Buffer, &wclex) != 0);
	wclex2.cbSize = sizeof(WNDCLASSEXW);
	TEST(NtUserGetClassInfo2(g_hInstance, &us, &wclex2, &pwstr, 0) != 0);

    TEST(pwstr == wclex.lpszMenuName);
    TEST(wclex2.cbSize == wclex.cbSize);
    TEST(wclex2.style == wclex.style);
    TEST(wclex2.lpfnWndProc == wclex.lpfnWndProc);
    TEST(wclex2.cbClsExtra == wclex.cbClsExtra);
    TEST(wclex2.cbWndExtra == wclex.cbWndExtra);
    TEST(wclex2.hInstance == wclex.hInstance);
    TEST(wclex2.hIcon == wclex.hIcon);
    TEST(wclex2.hCursor == wclex.hCursor);
    TEST(wclex2.hbrBackground == wclex.hbrBackground);
    TEST(wclex2.lpszMenuName == 0);
    TEST(wclex2.lpszClassName == 0);
    TEST(wclex2.hIconSm == wclex.hIconSm);

	return APISTATUS_NORMAL;
}
