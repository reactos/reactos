

INT
Test_BeginPath(PTESTINFO pti)
{
    HDC hDC;
    BOOL ret;

    SetLastError(0);
    ret = BeginPath(0);
    TEST(ret == 0);
    TEST(GetLastError() == ERROR_INVALID_HANDLE);

    hDC = CreateCompatibleDC(NULL);

    SetLastError(0);
    ret = BeginPath(hDC);
    TEST(ret == 1);
    TEST(GetLastError() == 0);

    DeleteDC(hDC);

    return APISTATUS_NORMAL;
}
