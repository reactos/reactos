/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserGetClassInfo
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtUserGetClassInfo)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
	WNDCLASSEXW wclex, wclex2 = {0};
	UNICODE_STRING us;
	PWSTR pwstr = NULL;

	us.Length = 8;
	us.MaximumLength = 8;
	us.Buffer = L"test";

	wclex.cbSize = sizeof(WNDCLASSEXW);
	wclex.style = 0;
	wclex.lpfnWndProc = NULL;
	wclex.cbClsExtra = 2;
	wclex.cbWndExtra = 4;
	wclex.hInstance = hinst;
	wclex.hIcon = NULL;
	wclex.hCursor = NULL;
	wclex.hbrBackground = CreateSolidBrush(RGB(4,7,5));
	wclex.lpszMenuName = L"MyMenu";
	wclex.lpszClassName = us.Buffer;
	wclex.hIconSm = NULL;

	ASSERT(RegisterClassExW(&wclex) != 0);

	TEST(GetClassInfoExW(hinst, us.Buffer, &wclex) != 0);
	wclex2.cbSize = sizeof(WNDCLASSEXW);
	TEST(NtUserGetClassInfo(hinst, &us, &wclex2, &pwstr, 0) != 0);

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

}
