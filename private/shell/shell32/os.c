#include "shellprv.h"
#pragma  hdrstop

#ifdef WINNT
//
// extra headers needed for IsExeTSAware
//
#include <ntimage.h>    
#include <ntrtl.h>
#endif // WINNT

BOOL _ShouldWeRetry(LPCTSTR pszFileName)
{
    BOOL fRetry = FALSE;
    if (GetLastError() == ERROR_ACCESS_DENIED)
    {
        HRESULT hres = OleInitialize(0);
        if (SUCCEEDED(hres))
        {
            LPDATAOBJECT pdtobj;
            HRESULT hres = OleGetClipboard(&pdtobj);
            if (hres == S_OK)
            {
                FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM medium;
                hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium);
                if (SUCCEEDED(hres))
                {
                    HDROP hdrop = medium.hGlobal;
                    if (DragQueryFile(hdrop, (UINT)-1, NULL, 0) == 1)
                    {
                        TCHAR szPath[MAX_PATH];
                        if (DragQueryFile(hdrop, 0, szPath, ARRAYSIZE(szPath)))
                        {
                            DebugMsg(DM_TRACE, TEXT("sh TR - _ShouldWeRetry found %s in clipboard (%s)"),
                                     szPath, pszFileName);
                            if (lstrcmpi(szPath, pszFileName)==0)
                            {
                                fRetry = TRUE;
                            }
                        }
                    }
                    ReleaseStgMedium(&medium);
                }
                pdtobj->lpVtbl->Release(pdtobj);
            }

            if (fRetry)
            {
                DebugMsg(DM_TRACE, TEXT("sh TR - _ShouldWeRetry emptying clipboard"));
                if (OpenClipboard(NULL))
                {
                    EmptyClipboard();
                    CloseClipboard();
                }
                else
                {
                    DebugMsg(DM_TRACE, TEXT("sh TR - _ShouldWeRetry OpenClipboard failed"));
                    fRetry = FALSE;
                }
            }

            OleUninitialize();
        }

        //
        // We need to restore the last error.
        //
        SetLastError(ERROR_ACCESS_DENIED);
    }
    return fRetry;
}

STDAPI_(BOOL) SHMoveFile(LPCTSTR pszExisting, LPCTSTR pszNew, LONG lEvent)
{
    BOOL res;
    BOOL fRetried = FALSE;

#ifdef WINNT
    //
    // On NT, CreateDirectory fails if the directory name being created does
    // not have room for an 8.3 name to be tagged onto the end of it,
    // i.e., lstrlen(new_directory_name)+12 must be less or equal to MAX_PATH.
    // However, NT does not impose this restriction on MoveFile -- which the
    // shell sometimes uses to manipulate directory names.  So, in order to
    // maintain consistency, we now check the length of the name before we
    // move the directory...
    //
    // the magic # "12" is 8 + 1 + 3 for and 8.3 name.


    if(IsDirPathTooLongForCreateDir(pszNew))
    {
        if (GetFileAttributes(pszExisting) & FILE_ATTRIBUTE_DIRECTORY)
        {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            return FALSE;
        }
    }
#endif

    do {
        res = MoveFile(pszExisting, pszNew);

        if (FALSE == res)
        {
            // If we couldn't move the file, see if it had the readonly or system attributes.
            // If so, clear them, move the file, and set them back on the destination

            DWORD dwAttributes = GetFileAttributes(pszExisting);
            if (-1 != dwAttributes && (dwAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
            {
                if (SetFileAttributes(pszExisting, dwAttributes  & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
                {
                    res = MoveFile(pszExisting, pszNew);
                    if (res)
                    {
                        SetFileAttributes(pszNew, dwAttributes);
                    }
                    else
                    {
                        SetFileAttributes(pszExisting, dwAttributes); // if move failed, return attributes.
                    }
                }
            }
        }
    } while (!res && !fRetried && (fRetried = _ShouldWeRetry(pszExisting)));

    if (res)
        SHChangeNotify(lEvent, SHCNF_PATH, pszExisting, pszNew);

    return res;
}


STDAPI_(BOOL) Win32MoveFile(LPCTSTR pszExisting, LPCTSTR pszNew, BOOL fDir)
{
    return SHMoveFile(pszExisting, pszNew, fDir ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM);
}

STDAPI_(BOOL) Win32DeleteFile(LPCTSTR pszFileName)
{
    BOOL res;
    BOOL fRetried = FALSE;
    do {
        res = DeleteFile(pszFileName);

        if (FALSE == res)
        {
            // If we couldn't delete the file, see if it has the readonly or
            // system bits set.  If so, clear them and try again

            DWORD dwAttributes = GetFileAttributes(pszFileName);
            if (-1 != dwAttributes && (dwAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
            {
                if (SetFileAttributes(pszFileName, dwAttributes  & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
                {
                    res = DeleteFile(pszFileName);
                }
            }
        }

    } while (!res && !fRetried && (fRetried = _ShouldWeRetry(pszFileName)));

    if (res)
        SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, pszFileName, NULL);

    return res;
}

STDAPI_(BOOL) Win32CreateDirectory(LPCTSTR pszPath, LPSECURITY_ATTRIBUTES lpsa)
{
    BOOL res = CreateDirectory(pszPath, lpsa);

    if (res)
        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, pszPath, NULL);

    return res;
}

//
// Some filesystems (like NTFS, perchance) actually pay attention to
// the readonly bit on folders.  So, in order to pretend we're sort of
// FAT and dumb, we clear the attribute before trying to delete the
// directory.
//
STDAPI_(BOOL) Win32RemoveDirectory(LPCTSTR pszDir)
{
    BOOL res = RemoveDirectory(pszDir);

    if (FALSE == res) {
        DWORD dwAttr = GetFileAttributes(pszDir);
        if ((-1 != dwAttr) && (dwAttr & FILE_ATTRIBUTE_READONLY))
        {
            dwAttr &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pszDir, dwAttr);
            res = RemoveDirectory(pszDir);
        }
    }
    
    if (res)
        SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pszDir, NULL);

    return res;
}

STDAPI_(HANDLE) Win32CreateFile(LPCTSTR pszFileName, DWORD dwAttrib)
{
#ifdef WINNT
    HANDLE hFile = CreateFile(pszFileName, GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, CREATE_ALWAYS, dwAttrib & FILE_ATTRIBUTE_VALID_FLAGS,
                                      NULL);
#else
    HANDLE hFile = CreateFile(pszFileName, GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, CREATE_ALWAYS, dwAttrib,
                                      NULL);
#endif
    if (INVALID_HANDLE_VALUE != hFile)
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszFileName, NULL);

    return hFile;
}

STDAPI_(BOOL) CreateWriteCloseFile(HWND hwnd, LPCTSTR pszFile, void *pData, DWORD cbData)
{
    HANDLE hfile = Win32CreateFile(pszFile, 0);
    if (hfile != INVALID_HANDLE_VALUE)
    {
        if (cbData)
        {
            DWORD dwBytesWritten;
            WriteFile(hfile, pData, cbData, &dwBytesWritten, 0);
        }
        CloseHandle(hfile);
        return TRUE;
    } 
    else 
    {
        TCHAR szPath[MAX_PATH];

        lstrcpyn(szPath, pszFile, ARRAYSIZE(szPath));
        PathRemoveExtension(szPath);

        SHSysErrorMessageBox(hwnd, NULL, IDS_CANNOTCREATEFILE,
                GetLastError(), PathFindFileName(szPath),
                MB_OK | MB_ICONEXCLAMATION);
    }
    return FALSE;
}

STDAPI_(DWORD) SHGetProcessDword(DWORD idProcess, LONG iIndex)
{
#ifdef WINNT
    return 0;
#else
    return GetProcessDword(idProcess, iIndex);
#endif
}

STDAPI_(BOOL) SHSetShellWindowEx(HWND hwnd, HWND hwndChild)
{
#ifdef WINNT
    return SetShellWindowEx(hwnd, hwndChild);
#else
    return SetShellWindow(hwnd);
#endif    
}


#ifdef WINNT

#define ISEXETSAWARE_MAX_IMAGESIZE  (64 * 1024) // allocate at most a 64k block to hold the image header

//
// this is a function that takes a full path to an executable and returns whether or not
// the exe has the TS_AWARE bit set in the image header
//
STDAPI_(BOOL) IsExeTSAware(LPCTSTR pszExe)
{
    BOOL bRet = FALSE;
    HANDLE hFile = CreateFile(pszExe,
                              GENERIC_READ, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING, 
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD cbImageSize = GetFileSize(hFile, NULL);
        LPBYTE pBuffer;
        
        if (cbImageSize > ISEXETSAWARE_MAX_IMAGESIZE)
        {
            // 64k should be enough to get the image header for everything...
            cbImageSize = ISEXETSAWARE_MAX_IMAGESIZE;
        }

        pBuffer = LocalAlloc(LPTR, cbImageSize);

        if (pBuffer)
        {
            HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, cbImageSize, NULL);

            if (hMap)
            {
                // map the first 64k of the file in
                LPBYTE pFileMapping = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, cbImageSize);

                if (pFileMapping) 
                {
                    _try
                    {
                        memcpy(pBuffer, pFileMapping, cbImageSize);
                    }
                    _except(UnhandledExceptionFilter(GetExceptionInformation()))
                    {
                        // We hit an exception while copying! doh!
                        LocalFree(pBuffer);
                        pBuffer = NULL;
                    }
                    
                    UnmapViewOfFile(pFileMapping);
                }
                else
                {
                    LocalFree(pBuffer);
                    pBuffer = NULL;
                }

                CloseHandle(hMap);
            }
            else
            {
                LocalFree(pBuffer);
                pBuffer = NULL;
            }

            if (pBuffer)
            {
                PIMAGE_NT_HEADERS pImageNTHeader;

                // NOTE: this should work ok for 64-bit images too, since both the IMAGE_NT_HEADERS and the IMAGE_NT_HEADERS64
                // structs have a ->Signature and ->OptionalHeader that is identical up to the DllCharacteristics offset.
                pImageNTHeader = RtlImageNtHeader(pBuffer);

                if (pImageNTHeader)
                {
                    if (pImageNTHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
                    {
                        // yes, this is a TSAWARE executable!
                        bRet = TRUE;
                    }
                }

                LocalFree(pBuffer);
            }
        }

        CloseHandle(hFile);
    }

    return bRet;
}
#endif // WINNT
