
INT
Test_AddFontResourceA(PTESTINFO pti)
{
    CHAR szFileNameA[MAX_PATH];
    CHAR szFileNameFont1A[MAX_PATH];
    CHAR szFileNameFont2A[MAX_PATH];

    GetCurrentDirectoryA(MAX_PATH,szFileNameA);

    memcpy(szFileNameFont1A,szFileNameA,MAX_PATH );
    strcat(szFileNameFont1A, "\\testdata\\test.ttf");

    memcpy(szFileNameFont2A,szFileNameA,MAX_PATH );
    strcat(szFileNameFont2A, "\\testdata\\test.otf");

    RtlZeroMemory(szFileNameA,MAX_PATH);

    /* 
     * Start testing Ansi version
     *
     */

    /* Testing NULL pointer */
    SetLastError(ERROR_SUCCESS);
    TEST(AddFontResourceA(NULL) == 0);
    TEST(GetLastError() == ERROR_SUCCESS);

    /* Testing -1 pointer */
    SetLastError(ERROR_SUCCESS);
    TEST(AddFontResourceA((CHAR*)-1) == 0);
    TEST(GetLastError() == ERROR_SUCCESS);

    /* Testing address 1 pointer */
    SetLastError(ERROR_SUCCESS);
    TEST(AddFontResourceA((CHAR*)1) == 0);
    TEST(GetLastError() == ERROR_SUCCESS);

    /* Testing address empty string */
    SetLastError(ERROR_SUCCESS);
    TEST(AddFontResourceA("") == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    /* Testing one ttf font */
    SetLastError(ERROR_SUCCESS);
    TEST(AddFontResourceA(szFileNameFont1A) == 1);
    TEST(GetLastError() == ERROR_SUCCESS);

    /* Testing one otf font */
    SetLastError(ERROR_SUCCESS);
    TEST(AddFontResourceA(szFileNameFont2A) == 1);
    TEST(GetLastError() == ERROR_SUCCESS);

    /* Testing two font */
    SetLastError(ERROR_SUCCESS);
    sprintf(szFileNameA,"%s | %s",szFileNameFont1A, szFileNameFont2A);
    TEST(AddFontResourceA(szFileNameA) == 0);
    printf("%x\n",(INT)GetLastError());
    TEST(GetLastError() == ERROR_FILE_NOT_FOUND);

    return APISTATUS_NORMAL;
}



