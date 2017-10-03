/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetTextFace
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>

#define TEST(x) ok(x, #x"\n")
#define RTEST(x) ok(x, #x"\n")

void Test_GetTextFace()
{
    HDC hDC;
    INT ret;
    INT ret2;
    WCHAR Buffer[20];

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hDC) return;

	/* Whether asking for the string size (NULL buffer) ignores the size argument */
	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 0, NULL);
    TEST(ret != 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());
	ret2 = ret;

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, -1, NULL);
    TEST(ret != 0);
    TEST(ret == ret2);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());
	ret2 = ret;

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 10000, NULL);
    TEST(ret != 0);
    TEST(ret == ret2);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());
	ret2 = ret;

	/* Whether the buffer is correctly filled */
	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 20, Buffer);
    TEST(ret != 0);
    TEST(ret <= 20);
    TEST(Buffer[ret - 1] == 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 1, Buffer);
    TEST(ret == 1);
    TEST(Buffer[ret - 1] == 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 2, Buffer);
    TEST(ret == 2);
    TEST(Buffer[ret - 1] == 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());

	/* Whether invalid buffer sizes are correctly ignored */
	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 0, Buffer);
    TEST(ret == 0);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() == %ld\n", GetLastError());

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, -1, Buffer);
    TEST(ret == 0);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() == %ld\n", GetLastError());

	DeleteDC(hDC);
}

START_TEST(GetTextFace)
{
    Test_GetTextFace();
}

