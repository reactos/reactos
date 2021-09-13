/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Cabinet extraction using FDI API
 * COPYRIGHT:   Copyright 2018 Alexander Shaposhnikov     (sanchaez@reactos.org)
 */
#include "rapps.h"
#include <debug.h>

#include <fdi.h>
#include <fcntl.h>

/*
 * HACK: treat any input strings as Unicode (UTF-8)
 * cabinet.dll lacks any sort of a Unicode API, but FCI/FDI
 * provide an ability to use user-defined callbacks for any file or memory
 * operations. This flexibility and the magic power of C/C++ casting allows
 * us to treat input as we please.
 * This is by far the best way to extract .cab using Unicode paths.
 */

/* String conversion helper functions */

// converts CStringW to CStringA using a given codepage
inline BOOL WideToMultiByte(const CStringW& szSource,
                            CStringA& szDest,
                            UINT Codepage)
{
    // determine the needed size
    INT sz = WideCharToMultiByte(Codepage,
                                    0,
                                    szSource,
                                    -1,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
    if (!sz)
        return FALSE;

    // do the actual conversion
    sz = WideCharToMultiByte(Codepage,
                                0,
                                szSource,
                                -1,
                                szDest.GetBuffer(sz),
                                sz,
                                NULL,
                                NULL);

    szDest.ReleaseBuffer();
    return sz != 0;
}

// converts CStringA to CStringW using a given codepage
inline BOOL MultiByteToWide(const CStringA& szSource,
                            CStringW& szDest,
                            UINT Codepage)
{
    // determine the needed size
    INT sz = MultiByteToWideChar(Codepage,
                                    0,
                                    szSource,
                                    -1,
                                    NULL,
                                    NULL);
    if (!sz)
        return FALSE;

    // do the actual conversion
    sz = MultiByteToWideChar(CP_UTF8,
                                0,
                                szSource,
                                -1,
                                szDest.GetBuffer(sz),
                                sz);

    szDest.ReleaseBuffer();
    return sz != 0;
}

/* FDICreate callbacks */

FNALLOC(fnMemAlloc)
{
    return HeapAlloc(GetProcessHeap(), NULL, cb);
}

FNFREE(fnMemFree)
{
    HeapFree(GetProcessHeap(), NULL, pv);
}

FNOPEN(fnFileOpen)
{
    HANDLE hFile = NULL;
    DWORD dwDesiredAccess = 0;
    DWORD dwCreationDisposition = 0;
    ATL::CStringW szFileName;

    UNREFERENCED_PARAMETER(pmode);

    if (oflag & _O_RDWR)
    {
        dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    }
    else if (oflag & _O_WRONLY)
    {
        dwDesiredAccess = GENERIC_WRITE;
    }
    else
    {
        dwDesiredAccess = GENERIC_READ;
    }

    if (oflag & _O_CREAT)
    {
        dwCreationDisposition = CREATE_ALWAYS;
    }
    else
    {
        dwCreationDisposition = OPEN_EXISTING;
    }

    MultiByteToWide(pszFile, szFileName, CP_UTF8);

    hFile = CreateFileW(szFileName,
                        dwDesiredAccess,
                        FILE_SHARE_READ,
                        NULL,
                        dwCreationDisposition,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    return (INT_PTR) hFile;
}

FNREAD(fnFileRead)
{
    DWORD dwBytesRead = 0;

    if (ReadFile((HANDLE) hf, pv, cb, &dwBytesRead, NULL) == FALSE)
    {
        dwBytesRead = (DWORD) -1L;
    }

    return dwBytesRead;
}

FNWRITE(fnFileWrite)
{
    DWORD dwBytesWritten = 0;

    if (WriteFile((HANDLE) hf, pv, cb, &dwBytesWritten, NULL) == FALSE)
    {
        dwBytesWritten = (DWORD) -1;
    }

    return dwBytesWritten;
}

FNCLOSE(fnFileClose)
{
    return (CloseHandle((HANDLE) hf) != FALSE) ? 0 : -1;
}

FNSEEK(fnFileSeek)
{
    return SetFilePointer((HANDLE) hf, dist, NULL, seektype);
}

/* FDICopy callbacks */

FNFDINOTIFY(fnNotify)
{
    INT_PTR iResult = 0;

    switch (fdint)
    {
    case fdintCOPY_FILE:
    {
        CStringW szExtractDir, szCabFileName;

        // Append the destination directory to the file name.
        MultiByteToWide((LPCSTR) pfdin->pv, szExtractDir, CP_UTF8);
        MultiByteToWide(pfdin->psz1, szCabFileName, CP_ACP);

        if (szCabFileName.Find('\\') >= 0)
        {
            CStringW szNewDirName = szExtractDir;
            int nTokenPos = 0;
            // We do not want to interpret the filename as directory,
            // so bail out before the last token!
            while (szCabFileName.Find('\\', nTokenPos) >= 0)
            {
                CStringW token = szCabFileName.Tokenize(L"\\", nTokenPos);
                if (token.IsEmpty())
                    break;

                szNewDirName += L"\\" + token;
                if (!CreateDirectoryW(szNewDirName, NULL))
                {
                    DWORD dwErr = GetLastError();
                    if (dwErr != ERROR_ALREADY_EXISTS)
                    {
                        DPRINT1("ERROR: Unable to create directory %S (err %lu)\n", szNewDirName.GetString(), dwErr);
                    }
                }
            }
        }

        CStringW szNewFileName = szExtractDir + L"\\" + szCabFileName;

        CStringA szFilePathUTF8;
        WideToMultiByte(szNewFileName, szFilePathUTF8, CP_UTF8);

        // Open the file
        iResult = fnFileOpen((LPSTR) szFilePathUTF8.GetString(),
                             _O_WRONLY | _O_CREAT,
                             0);
    }
    break;

    case fdintCLOSE_FILE_INFO:
        iResult = !fnFileClose(pfdin->hf);
        break;

    case fdintNEXT_CABINET:
        if (pfdin->fdie != FDIERROR_NONE)
        {
            iResult = -1;
        }
        break;

    case fdintPARTIAL_FILE:
        iResult = 0;
        break;

    case fdintCABINET_INFO:
        iResult = 0;
        break;

    case fdintENUMERATE:
        iResult = 0;
        break;

    default:
        iResult = -1;
        break;
    }

    return iResult;
}

/* cabinet.dll FDI function pointers */

typedef HFDI(*fnFDICreate)(PFNALLOC,
                           PFNFREE,
                           PFNOPEN,
                           PFNREAD,
                           PFNWRITE,
                           PFNCLOSE,
                           PFNSEEK,
                           int,
                           PERF);

typedef BOOL(*fnFDICopy)(HFDI,
                         LPSTR,
                         LPSTR,
                         INT,
                         PFNFDINOTIFY,
                         PFNFDIDECRYPT,
                         void FAR *pvUser);

typedef BOOL(*fnFDIDestroy)(HFDI);

/*
 * Extraction function
 * TODO: require only a full path to the cab as an argument
 */
BOOL ExtractFilesFromCab(const ATL::CStringW& szCabName,
                         const ATL::CStringW& szCabDir,
                         const ATL::CStringW& szOutputDir)
{
    HINSTANCE hCabinetDll;
    HFDI ExtractHandler;
    ERF ExtractErrors;
    ATL::CStringA szCabNameUTF8, szCabDirUTF8, szOutputDirUTF8;
    fnFDICreate pfnFDICreate;
    fnFDICopy pfnFDICopy;
    fnFDIDestroy pfnFDIDestroy;
    BOOL bResult;

    // Load cabinet.dll and extract needed functions
    hCabinetDll = LoadLibraryW(L"cabinet.dll");

    if (!hCabinetDll)
    {
        return FALSE;
    }

    pfnFDICreate = (fnFDICreate) GetProcAddress(hCabinetDll, "FDICreate");
    pfnFDICopy = (fnFDICopy) GetProcAddress(hCabinetDll, "FDICopy");
    pfnFDIDestroy = (fnFDIDestroy) GetProcAddress(hCabinetDll, "FDIDestroy");

    if (!pfnFDICreate || !pfnFDICopy || !pfnFDIDestroy)
    {
        FreeLibrary(hCabinetDll);
        return FALSE;
    }

    // Create FDI context
    ExtractHandler = pfnFDICreate(fnMemAlloc,
                                  fnMemFree,
                                  fnFileOpen,
                                  fnFileRead,
                                  fnFileWrite,
                                  fnFileClose,
                                  fnFileSeek,
                                  cpuUNKNOWN,
                                  &ExtractErrors);

    if (!ExtractHandler)
    {
        FreeLibrary(hCabinetDll);
        return FALSE;
    }

    // Create output dir
    bResult = CreateDirectoryW(szOutputDir, NULL);

    if (bResult || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Convert wide strings to UTF-8
        bResult = WideToMultiByte(szCabName, szCabNameUTF8, CP_UTF8);
        bResult &= WideToMultiByte(szCabDir, szCabDirUTF8, CP_UTF8);
        bResult &= WideToMultiByte(szOutputDir, szOutputDirUTF8, CP_UTF8);
    }

    // Perform extraction
    if (bResult)
    {
        // Add a slash to cab name as required by the api
        szCabNameUTF8 = "\\" + szCabNameUTF8;

        bResult = pfnFDICopy(ExtractHandler,
                             (LPSTR) szCabNameUTF8.GetString(),
                             (LPSTR) szCabDirUTF8.GetString(),
                             0,
                             fnNotify,
                             NULL,
                             (void FAR *) szOutputDirUTF8.GetString());
    }

    pfnFDIDestroy(ExtractHandler);
    FreeLibrary(hCabinetDll);
    return bResult;
}
