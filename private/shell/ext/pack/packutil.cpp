#include "priv.h"
#include "privcpp.h"
#include "shlwapi.h"

extern HINSTANCE g_hinst;

//////////////////////////////////////////////////////////////////////
//
// Icon Helper Functions
//
//////////////////////////////////////////////////////////////////////

void IconDraw(LPIC lpic, HDC hdc, LPRECT lprc)
{
    //
    // draw's the icon and the text to the specfied DC in the given 
    // bounding rect.  
    //    
    DebugMsg(DM_TRACE, "pack - IconDraw() called.");
    DebugMsg(DM_TRACE, "         left==%d,top==%d,right==%d,bottom==%d",
             lprc->left,lprc->top,lprc->right,lprc->bottom);

    // make sure we'll fit in the given rect
    if (((lpic->rc.right-lpic->rc.left) > (lprc->right - lprc->left)) ||
        ((lpic->rc.bottom-lpic->rc.top) > (lprc->bottom - lprc->top)))
        return;
    
    // Draw the icon
    if (lpic->hDlgIcon)
        DrawIcon(hdc, (lprc->left + lprc->right - g_cxIcon) / 2,
            (lprc->top + lprc->bottom - lpic->rc.bottom) / 2, lpic->hDlgIcon);

    if ((lpic->szIconText) && *(lpic->szIconText))
    {    
        HFONT hfont = SelectFont(hdc, g_hfontTitle);
        RECT rcText;

        rcText.left = lprc->left;
        rcText.right = lprc->right;
        rcText.top = (lprc->top + lprc->bottom - lpic->rc.bottom) / 2 + g_cyIcon + 1;
        rcText.bottom = lprc->bottom;
        DrawText(hdc, lpic->szIconText, -1, &rcText,
            DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE | DT_TOP);

        if (hfont)
            SelectObject(hdc, hfont);
    }
}


LPIC IconCreate(void)
{
    // 
    // allocates space for our icon structure which holds icon index,
    // the icon path, the handle to the icon, and the icon text
    // return:  NULL on failure
    //          a valid pointer on success
    //
    
    DebugMsg(DM_TRACE, "pack - IconCreate() called.");

    // Allocate memory for the IC structure
    return (LPIC)GlobalAlloc(GPTR, sizeof(IC));
}

LPIC IconCreateFromFile(LPTSTR lpstrFile)
{
    //
    // initializes an IC structure (defined in pack2.h) from a given
    // filename.
    // return:  NULL on failure
    //          a valid pointer on success
    //
    
    LPIC lpic;

    DebugMsg(DM_TRACE, "pack - IconCreateFromFile() called.");

    if (lpic = IconCreate())
    {
        // Get the icon
        lstrcpy(lpic->szIconPath, lpstrFile);
        lpic->iDlgIcon = 0;

        if (*(lpic->szIconPath))
            GetCurrentIcon(lpic);

        // Get the icon text -- calls ILGetDisplayName
	// 
        GetDisplayName(lpic->szIconText, lpstrFile);
        if (!IconCalcSize(lpic)) {
            if (lpic->hDlgIcon)
                DestroyIcon(lpic->hDlgIcon);
            GlobalFree(lpic);
            lpic = NULL;
        }
    }
    return lpic;
}


BOOL IconCalcSize(LPIC lpic) 
{
    HDC hdcWnd;
    RECT rcText = { 0 };
    SIZE Image;
    HFONT hfont;
    
    DebugMsg(DM_TRACE, "pack - IconCalcSize called.");
    
    // get the window DC, and make a DC compatible to it
    if (!(hdcWnd = GetDC(NULL)))  {
        DebugMsg(DM_TRACE, "         couldn't get DC!!");
        return FALSE;
    }
    ASSERT(lpic);


    if (lpic->szIconText && *(lpic->szIconText))
    {    
        SetRect(&rcText, 0, 0, g_cxArrange, g_cyArrange);
        
        // Set the icon text rectangle, and the icon font
        hfont = SelectFont(hdcWnd, g_hfontTitle);

        // Figure out how large the text region will be
        rcText.bottom = DrawText(hdcWnd, lpic->szIconText, -1, &rcText,
            DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);

        if (hfont)
            SelectObject(hdcWnd, hfont);
    }
    
    // Compute the image size
    rcText.right++;
    Image.cx = (rcText.right > g_cxIcon) ? rcText.right : g_cxIcon;
    Image.cy = g_cyIcon + rcText.bottom + 1;
    
    // grow the image a bit
    Image.cx += Image.cx / 4;
    Image.cy += Image.cy / 8;
    
    lpic->rc.right = Image.cx;
    lpic->rc.bottom = Image.cy;
    
    DebugMsg(DM_TRACE,"         lpic->rc.right==%d,lpic->rc.bottom==%d",
             lpic->rc.right,lpic->rc.bottom);
    
    return TRUE;
}    

void GetCurrentIcon(LPIC lpic)
{
    //
    // gets the current icon associated with the path stored in lpic->szIconPath
    // if it can't extract the icon associated with the file, it just gets
    // the standard application icon
    //
    
    WORD wIcon = lpic->iDlgIcon;

    
    DebugMsg(DM_TRACE, "pack - GetCurrentIcon() called.");

    if (lpic->hDlgIcon)
        DestroyIcon(lpic->hDlgIcon);

    if (!lpic->szIconPath || *lpic->szIconPath == TEXT('\0'))
        lpic->hDlgIcon = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_PACKAGER),
                                   IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    else if (!(lpic->hDlgIcon = ExtractAssociatedIcon(g_hinst, lpic->szIconPath, &wIcon)))
        lpic->hDlgIcon = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_PACKAGER),
                                   IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
}


void GetDisplayName(LPTSTR szName, LPCTSTR szPath)
{
    LPTSTR pszTemp = PathFindFileName(szPath);
    lstrcpy(szName, pszTemp);
}


/* ReplaceExtension() - Replaces the extension of the temp file.
 *
 * This routine ensures that the temp file has the same extension as the
 * original file, so that the ShellExecute() will load the same server..
 */
VOID ReplaceExtension(LPTSTR lpstrTempFile,LPTSTR lpstrOrigFile)
{
    LPTSTR lpstrBack = NULL;

    DebugMsg(DM_TRACE, "            ReplaceExtension() called.");
    
    // Get temp file extension
    while (*lpstrTempFile)
    {
        if (*lpstrTempFile == '\\')
            lpstrBack = lpstrTempFile;

        lpstrTempFile++;
    }

    while (lpstrBack && *lpstrBack && *lpstrBack != '.')
        lpstrBack++;

    if (lpstrBack && *lpstrBack)
        lpstrTempFile = lpstrBack + 1;

    // Get original file extension
    while (*lpstrOrigFile)
    {
        if (*lpstrOrigFile == '\\')
            lpstrBack = lpstrOrigFile;

        lpstrOrigFile++;
    }

    while (lpstrBack && *lpstrBack && *lpstrBack != '.')
        lpstrBack++;

    if (lpstrBack && *lpstrBack)
    {
        lpstrOrigFile = lpstrBack + 1;

        // Move the extension on over
        lstrcpy(lpstrTempFile, lpstrOrigFile);
    }
    else
    {
         /* Wipe out the extension altogether */
        *lpstrTempFile = 0;
    }
}



/////////////////////////////////////////////////////////////////////////
//
// Stream Helper Functions
//
/////////////////////////////////////////////////////////////////////////

HRESULT CopyFileToStream(LPTSTR lpFileName, IStream* pstm) 
{
    //
    // copies the given file to the current seek pointer in the given stream
    // return:  S_OK            -- successfully copied
    //          E_POINTER       -- one of the pointers was NULL
    //          E_OUTOFMEMORY   -- out of memory
    //          E_FAIL          -- other error
    //
    
    LPVOID      lpMem;
    HANDLE      hFile;
    HRESULT     hr;
    DWORD       dwSizeLow;
    DWORD       dwSizeHigh;
    DWORD       dwPosLow = 0L;
    LONG        lPosHigh = 0L;
    DWORD       cbRead = BUFFERSIZE;
    DWORD       cbWritten = BUFFERSIZE;
    
    DebugMsg(DM_TRACE,"pack - CopyFileToStream called.");
    
    if (!pstm || !lpFileName) {
        DebugMsg(DM_TRACE,"          bad pointer!!");
        return E_POINTER;
    }    
    
    // Allocate memory buffer for tranfer operation...
    if (!(lpMem = (LPVOID)GlobalAlloc(GPTR, BUFFERSIZE))) {
        DebugMsg(DM_TRACE, "         couldn't alloc memory buffer!!");
        hr = E_OUTOFMEMORY;
        goto ErrRet;
    }
    
    // open file to copy to stream
    hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READWRITE, NULL, 
                       OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DebugMsg(DM_TRACE, "         couldn't open file!!");
        goto ErrRet;
    }
    
    // Figure out how much to copy...
    dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
    ASSERT(dwSizeHigh == 0);
    
    SetFilePointer(hFile, 0L, NULL, FILE_BEGIN);
        
    // read in the file, and write to stream
    while (cbRead == BUFFERSIZE && cbWritten == BUFFERSIZE)
    {
        ReadFile(hFile, lpMem, BUFFERSIZE, &cbRead, NULL);
        pstm->Write(lpMem, cbRead, &cbWritten);
    }
    
    // verify that we are now at end of block to copy
    dwPosLow = SetFilePointer(hFile, 0L, &lPosHigh, FILE_CURRENT);
    ASSERT(lPosHigh == 0);
    if (dwPosLow != dwSizeLow) {
        DebugMsg(DM_TRACE, "         error copying file!!");
        hr = E_FAIL;
        goto ErrRet;
    }
    
    hr = S_OK;
    
ErrRet:
    if (hFile)
        CloseHandle(hFile);
    if (lpMem)
        GlobalFree((HANDLE)lpMem);
    return hr;
}   

HRESULT CopyStreamToFile(IStream* pstm, LPTSTR lpFileName) 
{
    //
    // copies the contents of the given stream from the current seek pointer
    // to the end of the stream into the given file.
    //
    // NOTE: the given filename must not exist, if it does, the function fails
    // with E_FAIL
    //
    // return:  S_OK            -- successfully copied
    //          E_POINTER       -- one of the pointers was NULL
    //          E_OUTOFMEMORY   -- out of memory
    //          E_FAIL          -- other error
    //
    
    LPVOID      lpMem;
    HANDLE      hFile;
    HRESULT     hr;
    DWORD       cbRead = BUFFERSIZE;
    DWORD       cbWritten = BUFFERSIZE;
    
    ULARGE_INTEGER      uli;
    LARGE_INTEGER       li = { 0L, 0L};
    ULARGE_INTEGER       uliPos;
    
    DebugMsg(DM_TRACE,"pack - CopyStreamToFile called.");
    
    // pstm must be a valid stream that is open for reading
    // lpFileName must be a valid filename to be written
    //
    if (!pstm || !lpFileName)
        return E_POINTER;
    
    // Allocate memory buffer...
    if (!(lpMem = (LPVOID)GlobalAlloc(GPTR, BUFFERSIZE))) {
        DebugMsg(DM_TRACE, "         couldn't alloc memory buffer!!");
        hr = E_OUTOFMEMORY;
        goto ErrRet;
    }
    
    // open file to receive stream data
    hFile = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, 
                       CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DebugMsg(DM_TRACE, "         couldn't open file!!");
        goto ErrRet;
    }
    
    // get the number of bytes to copy
    pstm->Seek(li, STREAM_SEEK_CUR, &uliPos);
    pstm->Seek(li, STREAM_SEEK_END, &uli);
    ASSERT(uliPos.HighPart == uli.HighPart);    // can't copy more than a DWORD
    li.HighPart = uliPos.HighPart;
    li.LowPart = uliPos.LowPart;
    pstm->Seek(li, STREAM_SEEK_SET, NULL);
        
    // read in the stream, and write to the file
    while (cbRead == BUFFERSIZE && cbWritten == BUFFERSIZE)  
    {
        hr = pstm->Read(lpMem, BUFFERSIZE, &cbRead);
        WriteFile(hFile, lpMem, cbRead, &cbWritten, NULL);
    }
    
//    li.HighPart = 0L;
//    li.LowPart = 0L;
  
    // verify that we are now at end of stream
//    pstm->Seek(li, STREAM_SEEK_CUR, &uliPos);
//    if (uliPos.LowPart != uli.LowPart || uliPos.HighPart != uli.HighPart) {

    if (hr != S_OK) 
    {
        DebugMsg(DM_TRACE, "         error copying file!!");
        hr = E_FAIL;
        goto ErrRet;
    }
    
ErrRet:
    if (hFile)
        CloseHandle(hFile);
    if (lpMem)
        GlobalFree((HANDLE)lpMem);
    return hr;
}   

// BUGBUG: write persistence formats in UNICODE!

HRESULT StringReadFromStream(IStream* pstm, LPSTR pszBuf, UINT cchBuf)
{
    //
    // read byte by byte until we hit the null terminating char
    // return: the number of bytes read
    //
    
    UINT cch = 0;
    
    do {
        pstm->Read(pszBuf, sizeof(CHAR), NULL);
        cch++;
    } while (*pszBuf++ && cch <= cchBuf);  
    return cch;
} 

DWORD _CBString(LPCSTR psz)
{
    return sizeof(psz[0]) * (lstrlenA(psz) + 1);
}

HRESULT StringWriteToStream(IStream* pstm, LPCSTR psz, DWORD *pdwWrite)
{
    DWORD dwWrite;
    DWORD dwSize = _CBString(psz);
    HRESULT hr = pstm->Write(psz, dwSize, &dwWrite);
    if (SUCCEEDED(hr))
        *pdwWrite += dwWrite;
    return hr;
}


// parse pszPath into a unquoted path string and put the args in pszArgs
//
// returns:
//      TRUE    we verified the thing exists
//      FALSE   it may not exist
//
// taken from \ccshell\shell32\link.c
//
BOOL PathSeparateArgs(LPTSTR pszPath, LPTSTR pszArgs)
{
    LPTSTR pszT;
    
    PathRemoveBlanks(pszPath);
    
    // if the unquoted sting exists as a file just use it
    
    if (PathFileExists(pszPath))
    {
	*pszArgs = 0;
	return TRUE;
    }
    
    pszT = PathGetArgs(pszPath);
    if (*pszT)
	*(pszT - 1) = TEXT('\0');
    lstrcpy(pszArgs, pszT);
    
    PathUnquoteSpaces(pszPath);
    
    return FALSE;
}   

