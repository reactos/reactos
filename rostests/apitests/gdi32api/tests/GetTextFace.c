
INT
Test_GetTextFace(PTESTINFO pti)
{
    HDC hDC;
    INT ret;
    INT ret2;
    WCHAR Buffer[20];

    hDC = CreateCompatibleDC(NULL);
    ASSERT(hDC);

	/* Whether asking for the string size (NULL buffer) ignores the size argument */
	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 0, NULL);
    TEST(ret != 0);
    TEST(GetLastError() == 0xE000BEEF);
	ret2 = ret;

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, -1, NULL);
    TEST(ret != 0);
    TEST(ret == ret2);
    TEST(GetLastError() == 0xE000BEEF);
	ret2 = ret;

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 10000, NULL);
    TEST(ret != 0);
    TEST(ret == ret2);
    TEST(GetLastError() == 0xE000BEEF);
	ret2 = ret;

	/* Whether the buffer is correctly filled */
	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 20, Buffer);
    TEST(ret != 0);
    TEST(ret <= 20);
    TEST(Buffer[ret - 1] == 0);
    TEST(GetLastError() == 0xE000BEEF);

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 1, Buffer);
    TEST(ret == 1);
    TEST(Buffer[ret - 1] == 0);
    TEST(GetLastError() == 0xE000BEEF);

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 2, Buffer);
    TEST(ret == 2);
    TEST(Buffer[ret - 1] == 0);
    TEST(GetLastError() == 0xE000BEEF);

	/* Whether invalid buffer sizes are correctly ignored */
	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 0, Buffer);
    TEST(ret == 0);
    TEST(GetLastError() == 0xE000BEEF);

	SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, -1, Buffer);
    TEST(ret == 0);
    TEST(GetLastError() == 0xE000BEEF);

	DeleteDC(hDC);

    return APISTATUS_NORMAL;
}
