
/* 
HBITMAP
APIENTRY
NtGdiCreateDIBSection(
    IN HDC hDC,
    IN OPTIONAL HANDLE hSection,
    IN DWORD dwOffset,
    IN LPBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN UINT cjHeader,
    IN FLONG fl,
    IN ULONG_PTR dwColorSpace,
    OUT PVOID *ppvBits)
*/

ULONG
GetBitmapSize(BITMAPINFOHEADER *pbih)
{
    ULONG WidthBits, WidthBytes;

    WidthBits = pbih->biWidth * pbih->biBitCount * pbih->biPlanes;
    WidthBytes = ((WidthBits + 31) & ~ 31) >> 3;

    return pbih->biHeight * WidthBytes;     
}


INT
Test_NtGdiCreateDIBSection(PTESTINFO pti)
{
    HBITMAP hbmp;
    HDC hDC;
    ULONG cjHeader;
    PVOID pvBits = NULL;
    ULONG cEntries;

    struct
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD          bmiColors[100]; 
    } bmi;
    PBITMAPINFO pbmi = (PBITMAPINFO)&bmi;
    PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER)&bmi.bmiHeader;
    PBITMAPV4HEADER pbV4h = (PBITMAPV4HEADER)&bmi.bmiHeader;
    PBITMAPV5HEADER pbV5h = (PBITMAPV5HEADER)&bmi.bmiHeader;

    hDC = GetDC(0);
    pbih->biSize = sizeof(BITMAPINFOHEADER);
    pbih->biWidth = 2;
    pbih->biHeight = 2;
    pbih->biPlanes = 1;
    pbih->biBitCount = 1;
    pbih->biCompression = BI_RGB;
    pbih->biSizeImage = 0;
    pbih->biXPelsPerMeter = 100;
    pbih->biYPelsPerMeter = 100;
    pbih->biClrUsed = 2;
    pbih->biClrImportant = 2;

    cEntries = 0;

/** iUsage = 0 (DIB_RGB_COLORS) ***********************************************/

    cjHeader = bmi.bmiHeader.biSize + cEntries * 4 + 8;

    /* Test something simple */
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
//    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);


    /* Test a 0 HDC */
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(0, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
    TEST(GetLastError() == ERROR_NOT_ENOUGH_MEMORY);
    if (hbmp) DeleteObject(hbmp);

    /* Test a wrong HDC */
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection((HDC)0xdeadbeef, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != 0);
    TEST(hbmp != 0);
    TEST(GetLastError() == 8);
    if (hbmp) DeleteObject(hbmp);

    /* Test pbmi = NULL */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, NULL, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* Test invalid pbmi */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, (PVOID)0x80001234, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);
    if (hbmp) DeleteObject(hbmp);

    /* Test invalid pbmi */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, (PVOID)1, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);
    if (hbmp) DeleteObject(hbmp);

    /* Test ppvBits = NULL */
    SetLastError(0);
    hbmp = NtGdiCreateDIBSection(0, NULL, 0, pbmi, 0, cjHeader, 0, 0, NULL);
    TEST(hbmp == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);
    if (hbmp) DeleteObject(hbmp);

    /* Test ppvBits = NULL and pbmi == 0*/
    SetLastError(0);
    hbmp = NtGdiCreateDIBSection(0, NULL, 0, NULL, 0, cjHeader, 0, 0, NULL);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* Test ppvBits = NULL and wrong cjHeader */
    SetLastError(0);
    hbmp = NtGdiCreateDIBSection(0, NULL, 0, pbmi, 0, cjHeader+4, 0, 0, NULL);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* Test ppvBits = NULL and cjHeader = 0 */
    SetLastError(0);
    hbmp = NtGdiCreateDIBSection(0, NULL, 0, pbmi, 0, 0, 0, 0, NULL);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* Test wrong cjHeader */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader+4, 0, 0, &pvBits);
    pvBits = (PVOID)-1;
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);


/** iUsage = 1 (DIB_PAL_COLORS) ***********************************************/

    pbmi->bmiHeader.biClrUsed = 2;
    pbmi->bmiHeader.biClrImportant = 2;

    cEntries = 2;
    cjHeader = bmi.bmiHeader.biSize + cEntries * 4 + 8;

    /* Test iUsage = 1 */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 1, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);



/** iUsage = 2 (???) **********************************************************/

    /* Test iUsage = 2 */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 2, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);


/** wrong iUsage **************************************************************/

    cEntries = 0;
    cjHeader = bmi.bmiHeader.biSize + cEntries * 4 + 8;

    /* Test iUsage = 3 */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 3, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);
    if (hbmp) DeleteObject(hbmp);

    /* Test iUsage = 3 */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 3, cjHeader+4, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* Test wrong iUsage */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, -55, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);
    if (hbmp) DeleteObject(hbmp);

    /* Test wrong iUsage and wrong cjHeader */
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, -55, cjHeader+4, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* increased header size */
    pbih->biSize = sizeof(BITMAPINFOHEADER) + 4;
    cjHeader = pbih->biSize + cEntries * 4 + 8;
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
    TEST(GetLastError() == 8);
    if (hbmp) DeleteObject(hbmp);

    /* increased header size */
    pbih->biSize = sizeof(BITMAPINFOHEADER) + 2;
    cjHeader = pbih->biSize + cEntries * 4 + 8;
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);

    /* decreased header size */
    pbih->biSize = sizeof(BITMAPINFOHEADER) - 4;
    cjHeader = pbih->biSize + cEntries * 4 + 8;
    SetLastError(0);
    pvBits = (PVOID)-1;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits == (PVOID)-1);
    TEST(hbmp == 0);
    TEST(GetLastError() == 0);
    if (hbmp) DeleteObject(hbmp);


/** BITMAPV4HEADER ************************************************************/

    pbV4h->bV4Size = sizeof(BITMAPV4HEADER);
    pbV4h->bV4Width = 2;
    pbV4h->bV4Height = 3;
    pbV4h->bV4Planes = 1;
    pbV4h->bV4BitCount = 1;
    pbV4h->bV4V4Compression = BI_RGB;
    pbV4h->bV4SizeImage = 0;
    pbV4h->bV4XPelsPerMeter = 100;
    pbV4h->bV4YPelsPerMeter = 100;
    pbV4h->bV4ClrUsed = 0;
    pbV4h->bV4ClrImportant = 0;
    pbV4h->bV4RedMask = 0;
    pbV4h->bV4GreenMask = 0;
    pbV4h->bV4BlueMask = 0;
    pbV4h->bV4AlphaMask = 0;
    pbV4h->bV4CSType = 0;
    memset(&pbV4h->bV4Endpoints, sizeof(CIEXYZTRIPLE), 0);
    pbV4h->bV4GammaRed = 0;
    pbV4h->bV4GammaGreen = 0;
    pbV4h->bV4GammaBlue = 0;

    cEntries = 0;
    cjHeader = bmi.bmiHeader.biSize + cEntries * 4 + 8;

    /* Test something simple */
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
    TEST(GetLastError() == 8);
    if (hbmp) DeleteObject(hbmp);


/** BITMAPV5HEADER ************************************************************/

    pbV5h->bV5Size = sizeof(BITMAPV5HEADER);
    pbV5h->bV5Width = 2;
    pbV5h->bV5Height = 3;
    pbV5h->bV5Planes = 1;
    pbV5h->bV5BitCount = 1;
    pbV5h->bV5Compression = BI_RGB;
    pbV5h->bV5SizeImage = 0;
    pbV5h->bV5XPelsPerMeter = 100;
    pbV5h->bV5YPelsPerMeter = 100;
    pbV5h->bV5ClrUsed = 2;
    pbV5h->bV5ClrImportant = 2;
    pbV5h->bV5RedMask = 0;
    pbV5h->bV5GreenMask = 0;
    pbV5h->bV5BlueMask = 0;
    pbV5h->bV5AlphaMask = 0;
    pbV5h->bV5CSType = 0;
    memset(&pbV5h->bV5Endpoints, sizeof(CIEXYZTRIPLE), 0);
    pbV5h->bV5GammaRed = 0;
    pbV5h->bV5GammaGreen = 0;
    pbV5h->bV5GammaBlue = 0;
    pbV5h->bV5Intent = 0;
    pbV5h->bV5ProfileData = 0;
    pbV5h->bV5ProfileSize = 0;
    pbV5h->bV5Reserved = 0;

    cEntries = 0;
    cjHeader = pbV5h->bV5Size + cEntries * 4 + 8;

    /* Test something simple */
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
    TEST(GetLastError() == 8);
    if (hbmp) DeleteObject(hbmp);

    /* increased header size */
    pbV5h->bV5Size = sizeof(BITMAPV5HEADER) + 64;
    cjHeader = pbV5h->bV5Size + cEntries * 4 + 8;
    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(hDC, NULL, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
    TEST(GetLastError() == 8);
    if (hbmp) DeleteObject(hbmp);

    printf("GetLastError() == %ld\n", GetLastError());

    /* Test section */
    HANDLE hSection;
    NTSTATUS Status;
    LARGE_INTEGER MaximumSize;
    
    MaximumSize.QuadPart = 4096;
    Status = ZwCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    ASSERT(NT_SUCCESS(Status));
    Status = ZwCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    ASSERT(NT_SUCCESS(Status));

    SetLastError(0);
    pvBits = 0;
    hbmp = NtGdiCreateDIBSection(hDC, hSection, 0, pbmi, 0, cjHeader, 0, 0, &pvBits);
    TEST(pvBits != NULL);
    TEST(hbmp != 0);
//    TEST(GetLastError() == 0);
    printf("hbmp = %p, pvBits = %p, hSection = %p\n", hbmp, pvBits, hSection);
    system("PAUSE");
    if (hbmp) DeleteObject(hbmp);


    return APISTATUS_NORMAL;
}
