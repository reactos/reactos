#include "screenshot.h"

/*
 * Save a screenshot to file until the clipboard
 * is ready to accept images.
 */


static VOID
GetError(VOID)
{
    LPVOID lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );

    MessageBox(NULL,
               lpMsgBuf,
               _T("Error!"),
               MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
}


static BOOL
DoWriteFile(PSCREENSHOT pScrSht,
            LPTSTR pstrFileName)
{
    BITMAPFILEHEADER bmfh;
    BOOL bSuccess;
    DWORD dwBytesWritten;
    HANDLE hFile;
    //INT PalEntries;

    hFile = CreateFile(pstrFileName,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    /* write the BITMAPFILEHEADER to file */
    bmfh.bfType = *(WORD *)"BM";  // 0x4D 0x42
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bSuccess = WriteFile(hFile,
                         &bmfh,
                         sizeof(bmfh),
                         &dwBytesWritten,
                         NULL);
    if ((!bSuccess) || (dwBytesWritten < sizeof(bmfh)))
        goto fail;

    /* write the BITMAPINFOHEADER to file */
    bSuccess = WriteFile(hFile,
                         &pScrSht->lpbi->bmiHeader,
                         sizeof(BITMAPINFOHEADER),
                         &dwBytesWritten,
                         NULL);
    if ((!bSuccess) || (dwBytesWritten < sizeof(BITMAPINFOHEADER)))
        goto fail;

    /* calculate the size of the pallete * /
    if (pScrSht->lpbi->bmiHeader.biCompression == BI_BITFIELDS)
        PalEntries = 3;
    else
    {
        if (pScrSht->lpbi->bmiHeader.biBitCount <= 8)
            PalEntries = (INT)(1 << pScrSht->lpbi->bmiHeader.biBitCount);
        else
            PalEntries = 0;
    }
    if (pScrSht->lpbi->bmiHeader.biClrUsed)
        PalEntries = pScrSht->lpbi->bmiHeader.biClrUsed;

    / * write pallete to file * /
    if (PalEntries != 0)
    {
        bSuccess = WriteFile(hFile,
                             &pScrSht->lpbi->bmiColors,
                             PalEntries * sizeof(RGBQUAD),
                             &dwBytesWritten,
                             NULL);
        if ((!bSuccess) || (dwBytesWritten < PalEntries * sizeof(RGBQUAD)))
            goto fail;
    }
*/
    /* save the current file position at the bginning of the bitmap bits */
    bmfh.bfOffBits = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

    /* write the bitmap bits to file */
    bSuccess = WriteFile(hFile,
                         pScrSht->lpvBits,
                         pScrSht->lpbi->bmiHeader.biSizeImage,
                         &dwBytesWritten,
                         NULL);
    if ((!bSuccess) || (dwBytesWritten < pScrSht->lpbi->bmiHeader.biSizeImage))
        goto fail;

    /* save the current file position at the final file size */
    bmfh.bfSize = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

    /* rewrite the updated file headers */
    SetFilePointer(hFile, 0, 0, FILE_BEGIN);
    bSuccess = WriteFile(hFile,
                         &bmfh,
                         sizeof(bmfh),
                         &dwBytesWritten,
                         NULL);
    if ((!bSuccess) || (dwBytesWritten < sizeof(bmfh)))
        goto fail;

    return TRUE;

fail:
    GetError();
    if (hFile) CloseHandle(hFile);
    DeleteFile(pstrFileName);
    return FALSE;

}


static BOOL
DoSaveFile(HWND hwnd, LPTSTR szFileName)
{
    OPENFILENAME ofn;

	static TCHAR Filter[] = _T("24 bit Bitmap (*.bmp,*.dib)\0*.bmp\0");

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize   = sizeof(OPENFILENAME);
    ofn.hwndOwner     = hwnd;
    ofn.nMaxFile      = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrDefExt   = _T("bmp");
	ofn.lpstrFilter = Filter;
	ofn.lpstrFile = szFileName;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
        return TRUE;

	if (CommDlgExtendedError() != CDERR_GENERALCODES)
        MessageBox(NULL, _T("Save to file failed"), NULL, 0);

    return FALSE;
}


static BOOL
CaptureScreen(PSCREENSHOT pScrSht)
{
    HDC ScreenDC;
    RECT rect;

    /* get window resolution */
    //pScrSht->Width = GetSystemMetrics(SM_CXSCREEN);
    //pScrSht->Height = GetSystemMetrics(SM_CYSCREEN);

    GetWindowRect(pScrSht->hSelf, &rect);
    pScrSht->Width = rect.right - rect.left;
    pScrSht->Height = rect.bottom - rect.top;

    /* get a DC for the screen */
    if (!(ScreenDC = GetDC(pScrSht->hSelf)))
        return FALSE;

    /* get a bitmap handle for the screen
     * needed to convert to a DIB */
    pScrSht->hBitmap = CreateCompatibleBitmap(ScreenDC,
                                              pScrSht->Width,
                                              pScrSht->Height);
    if (pScrSht->hBitmap == NULL)
    {
        GetError();
        ReleaseDC(pScrSht->hSelf, ScreenDC);
        return FALSE;
    }

    /* get a DC compatable with the screen DC */
    if (!(pScrSht->hDC = CreateCompatibleDC(ScreenDC)))
    {
        GetError();
        ReleaseDC(pScrSht->hSelf, ScreenDC);
        return FALSE;
    }

    /* select the bitmap into the DC */
    SelectObject(pScrSht->hDC,
                 pScrSht->hBitmap);

    /* copy the screen DC to the bitmap */
    BitBlt(pScrSht->hDC,
           0,
           0,
           pScrSht->Width,
           pScrSht->Height,
           ScreenDC,
           0,
           0,
           SRCCOPY);

    /* we're finished with the screen DC */
    ReleaseDC(pScrSht->hSelf, ScreenDC);

    return TRUE;
}


static BOOL
ConvertDDBtoDIB(PSCREENSHOT pScrSht)
{
    INT Ret;
    BITMAP bitmap;
    WORD cClrBits;


/*
    / * can't call GetDIBits with hBitmap selected * /
    //SelectObject(hDC, hOldBitmap);

    / * let GetDIBits fill the lpbi structure by passing NULL pointer * /
    Ret = GetDIBits(hDC,
                    hBitmap,
                    0,
                    Height,
                    NULL,
                    lpbi,
                    DIB_RGB_COLORS);
    if (Ret == 0)
    {
        GetError();
        ReleaseDC(hwnd, hDC);
        HeapFree(GetProcessHeap(), 0, lpbi);
        return -1;
    }
*/

////////////////////////////////////////////////////

	if (!GetObjectW(pScrSht->hBitmap,
                    sizeof(BITMAP),
                    (LPTSTR)&bitmap))
    {
        GetError();
		return FALSE;
	}

	cClrBits = (WORD)(bitmap.bmPlanes * bitmap.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	if (cClrBits != 24)
        pScrSht->lpbi = (PBITMAPINFO) HeapAlloc(GetProcessHeap(),
                                                0,
                                                sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));
    else
        pScrSht->lpbi = (PBITMAPINFO) HeapAlloc(GetProcessHeap(),
                                                0,
                                                sizeof(BITMAPINFOHEADER));

	if (!pScrSht->lpbi)
	{
		GetError();
		return FALSE;
	}

	pScrSht->lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pScrSht->lpbi->bmiHeader.biWidth = bitmap.bmWidth;
	pScrSht->lpbi->bmiHeader.biHeight = bitmap.bmHeight;
	pScrSht->lpbi->bmiHeader.biPlanes = bitmap.bmPlanes;
	pScrSht->lpbi->bmiHeader.biBitCount = bitmap.bmBitsPixel;

	if (cClrBits < 24)
		pScrSht->lpbi->bmiHeader.biClrUsed = (1 << cClrBits);

	pScrSht->lpbi->bmiHeader.biCompression = BI_RGB;
	pScrSht->lpbi->bmiHeader.biSizeImage = ((pScrSht->lpbi->bmiHeader.biWidth * cClrBits +31) & ~31) /8
                                           * pScrSht->lpbi->bmiHeader.biHeight;

	pScrSht->lpbi->bmiHeader.biClrImportant = 0;

//////////////////////////////////////////////////////

    /* reserve memory to hold the screen bitmap */
    pScrSht->lpvBits = HeapAlloc(GetProcessHeap(),
                                 0,
                                 pScrSht->lpbi->bmiHeader.biSizeImage);
    if (pScrSht->lpvBits == NULL)
    {
        GetError();
        return FALSE;
    }

    /* convert the DDB to a DIB */
    Ret = GetDIBits(pScrSht->hDC,
                    pScrSht->hBitmap,
                    0,
                    pScrSht->Height,
                    pScrSht->lpvBits,
                    pScrSht->lpbi,
                    DIB_RGB_COLORS);
    if (Ret == 0)
    {
        GetError();
        return FALSE;
    }

    return TRUE;

}


// INT WINAPI GetScreenshot(BOOL bFullScreen)
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   PSTR szCmdLine,
                   int iCmdShow)
{
    PSCREENSHOT pScrSht;
    TCHAR szFileName[MAX_PATH] = _T("");

    BOOL bFullScreen = TRUE;

    pScrSht = HeapAlloc(GetProcessHeap(),
                        0,
                        sizeof(SCREENSHOT));
    if (pScrSht == NULL)
        return -1;

    if (bFullScreen)
    {
        pScrSht->hSelf = GetDesktopWindow();
    }
    else
    {
        pScrSht->hSelf = GetForegroundWindow();
    }

    if (pScrSht->hSelf == NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 pScrSht);

        return -1;
    }

    if (CaptureScreen(pScrSht))
    {
        /* convert the DDB image to DIB */
        if(ConvertDDBtoDIB(pScrSht))
        {
            /* Get filename from user */
            if(DoSaveFile(pScrSht->hSelf, szFileName))
            {
                /* build the headers and write to file */
                DoWriteFile(pScrSht, szFileName);
            }
        }
    }

    /* cleanup */
    if (pScrSht->hSelf != NULL)
        ReleaseDC(pScrSht->hSelf, pScrSht->hDC);
    if (pScrSht->hBitmap != NULL)
        DeleteObject(pScrSht->hBitmap);
    if (pScrSht->lpbi != NULL)
        HeapFree(GetProcessHeap(),
                 0,
                 pScrSht->lpbi);
    if (pScrSht->lpvBits != NULL)
        HeapFree(GetProcessHeap(),
                 0,
                 pScrSht->lpvBits);
    if (pScrSht != NULL)
        HeapFree(GetProcessHeap(),
                 0,
                 pScrSht);

    return 0;
}
