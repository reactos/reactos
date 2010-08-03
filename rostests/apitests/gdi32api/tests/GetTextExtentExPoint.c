#define NUM_SYSCOLORS 31

INT
Test_GetTextExtentExPoint(PTESTINFO pti)
{
    INT nFit;
    SIZE size;
    BOOL result;

    SetLastError(0);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, 1000, &nFit, NULL, &size);
    TEST(result == 1);
    TEST(nFit == 4);
    TEST(GetLastError() == 0);
    printf("nFit = %d\n", nFit);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, 1, &nFit, NULL, &size);
    TEST(result == 1);
    TEST(nFit == 0);
    TEST(GetLastError() == 0);
    printf("nFit = %d\n", nFit);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, 0, &nFit, NULL, &size);
    TEST(result == 1);
    TEST(nFit == 0);
    TEST(GetLastError() == 0);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, -1, &nFit, NULL, &size);
    TEST(result == 1);
    TEST(nFit == 4);
    TEST(GetLastError() == 0);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, -2, &nFit, NULL, &size);
    TEST(result == 0);
    TEST(GetLastError() == 87);

	return APISTATUS_NORMAL;
}
