#include "verpriv.h"
#include <lzexpand.h>
#include "diamondd.h"
#include "mydiam.h"
#ifdef FE_SB
#include "..\lz\lzexpand\lzapi.h"
#endif

BOOL    FileInUse(LPWSTR lpszFilePath, LPWSTR lpszFileName);
DWORD   MakeFileName(LPWSTR lpDst, LPWSTR lpDir, LPWSTR lpFile, int cchDst);

typedef struct tagVS_VERSION {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;
    WCHAR szSig[16];
    VS_FIXEDFILEINFO vInfo;
} VS_VERSION;

typedef struct tagLANGANDCP {
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCP;

WCHAR szTrans[] = TEXT("\\VarFileInfo\\Translation");
WCHAR szTempHdr[] = TEXT("temp.");


/* The routines in here will find a file on a path, and an environment
 * variable.  The constants _MAX_PATH and need to be defined
 * by the including module, plus the constant WINDOWS should
 * be defined if this is to be used in Windows, so that lstrcmpi
 * and lstrlen will not be compiled
 */


VOID Ver3IToA(LPWSTR lpwStr, int n)
{
    int nShift;
    WCHAR cTemp;

    for (nShift=8; nShift>=0; nShift-=4, ++lpwStr) {
        if ((cTemp = (WCHAR)((n>>nShift)&0x000f)) >= 10)
            *lpwStr = (WCHAR)('A' + cTemp - 10);
        else
            *lpwStr = (WCHAR)('0' + cTemp     );
    }
    *lpwStr = 0;
}


/* Convert a DOS error into an error flag
 */
DWORD FileErrFlag(int err)
{
    switch (err) {
        case 0x05:
            return (VIF_ACCESSVIOLATION);

        case 0x20:
            return (VIF_SHARINGVIOLATION);

        default:
            return (0);
    }
}


/* Create the given file with default flags; global nFileErr will
 * receive any DOS error; returns -1 on error, otherwise the DOS
 * file handle.
 */
HANDLE VerCreateFile(LPWSTR lpszFile)
{
    HANDLE hFile;

    hFile = CreateFile(lpszFile, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ, NULL, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    return (hFile);
}


VOID VerClose(HANDLE hW32File)
{
    CloseHandle(hW32File);
}


#define MyAlloc(x) ((WCHAR *)LocalAlloc(LMEM_FIXED, x))
#define MyFree(x) LocalFree((HANDLE)(x))

LPVOID MyGetFileVersionInfo(LPWSTR lpFileName)
{
    WCHAR *pInfo;
    WORD wLen = 2048;

    TryAgain:
    if (!(pInfo=MyAlloc(wLen)))
        goto Error1;
    if (!GetFileVersionInfo(lpFileName, 0L, wLen, pInfo))
        goto Error2;
    if (wLen < *(WORD *)pInfo) {
        wLen = *(WORD *)pInfo;
        MyFree(pInfo);
        goto TryAgain;
    }
    return (pInfo);

    Error2:
    MyFree(pInfo);
    Error1:
    return (NULL);
}

HINSTANCE hLz32;
DWORD cLz32Load;
typedef INT  (APIENTRY *tLZInit)( INT );
typedef INT  (APIENTRY *tLZOpenFileW)(LPWSTR, LPOFSTRUCT, WORD );
typedef VOID (APIENTRY *tLZClose)( INT );
typedef LONG (APIENTRY *tLZCopy)( INT, INT );

tLZInit      pLZInit;
tLZOpenFileW pLZOpenFileW;
tLZClose     pLZClose;
tLZCopy      pLZCopy;

DWORD
APIENTRY
VerInstallFileW(
               DWORD wFlags,
               LPWSTR lpszSrcFileName,
               LPWSTR lpszDstFileName,
               LPWSTR lpszSrcDir,
               LPWSTR lpszDstDir,
               LPWSTR lpszCurDir,
               LPWSTR lpszTmpFile,
               PUINT puTmpFileLen
               )
{
    WCHAR szSrcFile[_MAX_PATH];
    WCHAR szDstFile[_MAX_PATH];
    WCHAR szCurFile[_MAX_PATH];
    DWORD dwRetVal = 0L, dwSrcAttr;
    WORD wDirLen;
    LONG lCopy;
    HANDLE hW32Out;
    int i, fIn, fDosOut;
#ifdef FE_SB
    WCHAR szCompFile[_MAX_PATH];
    CHAR  szOemFile[_MAX_PATH];
    int   iOemString;
    BOOL  bDefaultCharUsed;
#else
    OFSTRUCT of;
#endif
    BOOL DiamondFile;

    if (!cLz32Load) {
        hLz32 = LoadLibraryW(L"LZ32.DLL");
        if (!hLz32) {
            return (VIF_CANNOTLOADLZ32);
        }
        pLZOpenFileW = (tLZOpenFileW) GetProcAddress(hLz32, "LZOpenFileW");
        pLZInit      = (tLZInit)      GetProcAddress(hLz32, "LZInit");
        pLZCopy      = (tLZCopy)      GetProcAddress(hLz32, "LZCopy");
        pLZClose     = (tLZClose)     GetProcAddress(hLz32, "LZClose");
        if (!(pLZOpenFileW && pLZInit && pLZCopy && pLZClose)) {
            FreeLibrary(hLz32);
            return (VIF_CANNOTLOADLZ32);
        }

        if (InterlockedExchangeAdd(&cLz32Load, 1) != 0) {
            // Multiple threads are attempting to LoadLib
            // Free one here.
            FreeLibrary(hLz32);
        }
    }

    LogData("inst", __LINE__, (DWORD)puTmpFileLen);

    /* LZ Open the source for reading
     */
    MakeFileName(szSrcFile, lpszSrcDir, lpszSrcFileName, ARRAYSIZE(szSrcFile));
    dwRetVal = InitDiamond();
    if (dwRetVal) {
        return (dwRetVal);
    }
#ifdef FE_SB
    if((fIn=LZCreateFileW(szSrcFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, szCompFile)) < 0) {
#else
    if ((fIn=pLZOpenFileW(szSrcFile, &of, 0)) < 0) {
#endif
        dwRetVal |= VIF_CANNOTREADSRC;
        goto doReturn;
    }

    LogData("inst", __LINE__, (DWORD)puTmpFileLen);

#ifdef FE_SB
  /*
   * now we don't have any unicode interface for Diamond API. we need to convert
   * it to OEM charset string. we will use the converted string instead of of.szPathName.
   */

   DiamondFile = FALSE;

   iOemString = WideCharToMultiByte( CP_OEMCP,            // Oem Codepage
                                     0,                   // no option flag
                                     szCompFile,          // Unicode string
                                     -1,                  // should be NULL terminated
                                     szOemFile,           // Oem string
                                     ARRAYSIZE(szOemFile),// Oem string buffer size
                                     NULL,                // use nls default char 
                                     &bDefaultCharUsed 
                                   );

   if( ( iOemString != 0               ) && // should succeed conversion.
       ( iOemString <= OFS_MAXPATHNAME ) && // should be <= 128 for OpenFile() API.
       ( bDefaultCharUsed == FALSE     )    // the def. char should not be contain.
     )
   {
      DiamondFile = IsDiamondFile(szOemFile);
   }

#else
    DiamondFile = IsDiamondFile(of.szPathName);
#endif

    if (DiamondFile) {
#ifdef FE_SB
        LZCloseFile(fIn);
#else
        pLZClose(fIn);
#endif
    }

    LogData("inst", __LINE__, (DWORD)puTmpFileLen);

    /* If the dest file exists and is read only, return immediately;
     * the calling app must change the attributes before continuing
     * In the library version, we assume the file is in use if it exists
     * and we are in a VM; check all other possible errors and then return,
     * so the calling app can override our determination of "in use"
     * on a second call, along with all other problems
     */
    wDirLen = (WORD)MakeFileName(szDstFile, lpszDstDir, lpszDstFileName, ARRAYSIZE(szDstFile));
    lstrcpy(szSrcFile, szDstFile);
    if (!HIWORD(dwSrcAttr=GetFileAttributes(szSrcFile))) {
        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
        if (LOWORD(dwSrcAttr)&0x01) {
            LogData("inst", __LINE__, (DWORD)puTmpFileLen);
            dwRetVal |= VIF_WRITEPROT;
            goto doCloseSrc;
        }

        if (FileInUse(szSrcFile, lpszDstFileName)) {
            LogData("inst", __LINE__, (DWORD)puTmpFileLen);
            dwRetVal |= VIF_FILEINUSE;
            goto doCloseSrc;
        }
    }

    /* If this is a force install and there is a temp file name from a
     * previous call to this function, use that as the temp file name
     */
    LogData("inst", __LINE__, (DWORD)puTmpFileLen);
    if ((wFlags&VIFF_FORCEINSTALL) && *lpszTmpFile) {
        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
        LogData("fnam", (DWORD)lpszDstDir, (DWORD)lpszTmpFile);
        MakeFileName(szSrcFile, lpszDstDir, lpszTmpFile, ARRAYSIZE(szSrcFile));
        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
        LogData("srcf", (DWORD)szSrcFile, *(LPDWORD)szSrcFile);
        if (!HIWORD(GetFileAttributes(szSrcFile))) {
            LogData("inst", __LINE__, (DWORD)puTmpFileLen);
            goto doCheckDstFile;
        }
    }

    /* Determine a file name that is not in use; try names of the form:
     * temp.nnn where nnn is a three digit hex number.  If we get to
     * 0xfff, we have a serious file system problem.  Create the file.
     */
    LogData("inst", __LINE__, (DWORD)puTmpFileLen);
    lstrcpy(szSrcFile+wDirLen, szTempHdr);
    for (i=0; ; ++i) {
        Ver3IToA(szSrcFile+wDirLen+lstrlen(szTempHdr), i);
        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
        if (HIWORD(GetFileAttributes(szSrcFile)))
            break;
        if (i > 0xfff) {
            dwRetVal |= VIF_CANNOTCREATE;
            goto doCloseSrc;
        }
    }
    /* Copy the file, and fill in appropriate errors
     */

    LogData("inst", __LINE__, (DWORD)puTmpFileLen);

    if (DiamondFile) {
        LZINFO lzi;
#ifdef FE_SB
        lCopy = ExpandDiamondFile(szOemFile,
#else
        lCopy = ExpandDiamondFile(of.szPathName,  // PARSED name!!
#endif
                                  szSrcFile,
                                  FALSE,
                                  &lzi);
        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
    } else {
        if ((hW32Out=VerCreateFile(szSrcFile)) == INVALID_HANDLE_VALUE) {
            dwRetVal |= VIF_CANNOTCREATE | FileErrFlag(GetLastError());
            goto doCloseSrc;
        }

        LogData("inst", __LINE__, (DWORD)puTmpFileLen);

        fDosOut = pLZInit((INT)((DWORD_PTR)hW32Out));
        lCopy = pLZCopy(fIn, fDosOut);
        pLZClose(fDosOut);
    }

    LogData("inst", __LINE__, (DWORD)puTmpFileLen);

    switch (lCopy) {
        case LZERROR_BADINHANDLE:
        case LZERROR_READ:
        case LZERROR_BADVALUE:
        case LZERROR_UNKNOWNALG:
            dwRetVal |= VIF_CANNOTREADSRC;
            goto doDelTempFile;

        case LZERROR_BADOUTHANDLE:
        case LZERROR_WRITE:
            dwRetVal |= VIF_OUTOFSPACE;
            goto doDelTempFile;

        case LZERROR_GLOBALLOC:
        case LZERROR_GLOBLOCK:
            dwRetVal |= VIF_OUTOFMEMORY;
            goto doDelTempFile;

        default:
            break;
    }

    /* If the destination exists, check the versions of the two files,
     * and only copy if the src file is at least as new as the dst, and
     * they are the same type and in the same language and codepage
     */
    doCheckDstFile:
    if (!HIWORD(dwSrcAttr)) {
        VS_VERSION *pSrcVer, *pDstVer;
        LANGANDCP *lpSrcTrans, *lpDstTrans;
        DWORD   dwSrcNum, dwDstNum;
        DWORD   dwSrcTrans, dwDstTrans;

        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
        if (!(wFlags & VIFF_FORCEINSTALL) &&
            (pDstVer=MyGetFileVersionInfo(szDstFile))) {
            LogData("inst", __LINE__, (DWORD)puTmpFileLen);
            if (!(pSrcVer=MyGetFileVersionInfo(szSrcFile))) {
                dwRetVal |= VIF_MISMATCH | VIF_SRCOLD;
            } else {
                LogData("inst", __LINE__, (DWORD)puTmpFileLen);
                if (pDstVer->vInfo.dwFileVersionMS>pSrcVer->vInfo.dwFileVersionMS
                    || (pDstVer->vInfo.dwFileVersionMS==pSrcVer->vInfo.dwFileVersionMS &&
                        pDstVer->vInfo.dwFileVersionLS>pSrcVer->vInfo.dwFileVersionLS))
                    dwRetVal |= VIF_MISMATCH | VIF_SRCOLD;

                if (pDstVer->vInfo.dwFileType!=pSrcVer->vInfo.dwFileType ||
                    pDstVer->vInfo.dwFileSubtype!=pSrcVer->vInfo.dwFileSubtype)
                    dwRetVal |= VIF_MISMATCH | VIF_DIFFTYPE;

                if (VerQueryValueW(pDstVer, szTrans, (LPVOID)&lpDstTrans, &dwDstNum) &&
                    VerQueryValueW(pSrcVer, szTrans, (LPVOID)&lpSrcTrans, &dwSrcNum)) {
                    dwDstNum /= sizeof(DWORD);
                    dwSrcNum /= sizeof(DWORD);

                    for (dwDstTrans=0; dwDstTrans<dwDstNum; ++dwDstTrans) {
                        for (dwSrcTrans=0; ; ++dwSrcTrans) {
                            if (dwSrcTrans >= dwSrcNum) {
                                dwRetVal |= VIF_MISMATCH | VIF_DIFFLANG;
                                break;
                            }

                            if (lpDstTrans[dwDstTrans].wLanguage
                                == lpSrcTrans[dwSrcTrans].wLanguage) {
                                /* OK if dst is CP0 and src is not UNICODE
                                 */
                                if (lpDstTrans[dwDstTrans].wCodePage==0 &&
                                    lpSrcTrans[dwSrcTrans].wCodePage!=1200)
                                    break;
                                if (lpDstTrans[dwDstTrans].wCodePage
                                    == lpSrcTrans[dwSrcTrans].wCodePage)
                                    break;
                            }
                        }
                    }
                }

                LogData("inst", __LINE__, (DWORD)puTmpFileLen);
                MyFree(pSrcVer);
            }
            LogData("inst", __LINE__, (DWORD)puTmpFileLen);
            MyFree(pDstVer);
        }

        /* If there were no errors, delete the currently existing file
         */
        LogData("inst", __LINE__, (DWORD)puTmpFileLen);
        if (!dwRetVal && !DeleteFile(szDstFile)) {
            dwRetVal |= VIF_CANNOTDELETE | FileErrFlag(GetLastError());
            goto doDelTempFile;
        }
    }

    /* If there were no errors, rename the temp file (any existing file
     * should have been deleted by now).  Otherwise, if we created a valid
     * temp file, then pass along the temp file name.
     */
    LogData("inst", __LINE__, (DWORD)puTmpFileLen);

    if (dwRetVal) {
        DWORD wTemp;

        if (*puTmpFileLen > (wTemp=lstrlen(szSrcFile+wDirLen))) {
            lstrcpy(lpszTmpFile, szSrcFile+wDirLen);
            dwRetVal |= VIF_TEMPFILE;
        } else {
            dwRetVal |= VIF_BUFFTOOSMALL;
            DeleteFile(szSrcFile);
        }
        *puTmpFileLen = wTemp + 1;
    } else {
        /* Delete the currently existing file; this gets done before renaming
         * the temp file in case someone has tried to mess us up with a weird
         * directory name that would allow us to delete the newly installed
         * file.
         */
        if (!(wFlags&VIFF_DONTDELETEOLD) &&
            lpszCurDir && *lpszCurDir && lstrcmpi(lpszCurDir, lpszDstDir)) {
            MakeFileName(szCurFile, lpszCurDir, lpszDstFileName, ARRAYSIZE(szCurFile));
            if (!HIWORD(GetFileAttributes(szCurFile)) &&
                (FileInUse(szCurFile, lpszDstFileName) ||
                 !DeleteFile(szCurFile)))
                dwRetVal |= VIF_CANNOTDELETECUR | FileErrFlag(GetLastError());
        }

        if (!MoveFile(szSrcFile, szDstFile)) {
            dwRetVal |= VIF_CANNOTRENAME | FileErrFlag(GetLastError());
            doDelTempFile:
            DeleteFile(szSrcFile);
        }
    }

    doCloseSrc:
    if (!DiamondFile) {
#ifdef FE_SB
        LZCloseFile(fIn);
#else
        pLZClose(fIn);
#endif
    }
    doReturn:
    LogData("inst", __LINE__, (DWORD)puTmpFileLen);
    TermDiamond();
    return (dwRetVal);
}
