
INT
Test_GetTextFace(PTESTINFO pti)
{
    HDC hDC;
    INT ret;
    CHAR Buffer[20];

    hDC = GetDC(NULL);
    ASSERT(hDC);

    ret = GetTextFaceA(hDC, 0, NULL);
    TEST(ret != 0);

    ret = GetTextFaceA(hDC, 20, Buffer);
    TEST(ret != 0);

    ret = GetTextFaceA(hDC, 0, Buffer);
    TEST(ret == 0);

    ret = GetTextFaceA(hDC, 20, NULL);
    TEST(ret != 0);

    return APISTATUS_NORMAL;
}
