/*
 * PROJECT:     ReactOS Zip Shell Extension
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Create a zip file
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include "atlsimpcoll.h"
#include "minizip/zip.h"
#include "minizip/iowin32.h"
#include <process.h>

static CStringW DoGetZipName(LPCWSTR filename)
{
    WCHAR szPath[MAX_PATH];
    StringCbCopyW(szPath, sizeof(szPath), filename);
    PathRemoveExtensionW(szPath);

    CStringW ret = szPath;
    ret += L".zip";

    UINT i = 2;
    while (PathFileExistsW(ret))
    {
        CStringW str;
        str.Format(L" (%u).zip", i++);

        ret = szPath;
        ret += str;
    }

    return ret;
}

static CStringA DoGetAnsiName(LPCWSTR filename)
{
    CHAR buf[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, filename, -1, buf, _countof(buf), NULL, NULL);
    return buf;
}

static CStringW DoGetBaseName(LPCWSTR filename)
{
    WCHAR szBaseName[MAX_PATH];
    StringCbCopyW(szBaseName, sizeof(szBaseName), filename);
    PathRemoveFileSpecW(szBaseName);
    PathAddBackslashW(szBaseName);
    return szBaseName;
}

static CStringA
DoGetNameInZip(const CStringW& basename, const CStringW& filename)
{
    CStringW basenameI = basename, filenameI = filename;
    basenameI.MakeUpper();
    filenameI.MakeUpper();

    CStringW ret;
    if (filenameI.Find(basenameI) == 0)
        ret = filename.Mid(basename.GetLength());
    else
        ret = filename;

    ret.Replace(L'\\', L'/');

    return DoGetAnsiName(ret);
}

static BOOL
DoReadAllOfFile(LPCWSTR filename, CSimpleArray<BYTE>& contents,
                zip_fileinfo *pzi)
{
    contents.RemoveAll();

    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DPRINT1("%S: cannot open\n", filename);
        return FALSE;
    }

    FILETIME ft, ftLocal;
    ZeroMemory(pzi, sizeof(*pzi));
    if (GetFileTime(hFile, NULL, NULL, &ft))
    {
        SYSTEMTIME st;
        FileTimeToLocalFileTime(&ft, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);
        pzi->tmz_date.tm_sec = st.wSecond;
        pzi->tmz_date.tm_min = st.wMinute;
        pzi->tmz_date.tm_hour = st.wHour;
        pzi->tmz_date.tm_mday = st.wDay;
        pzi->tmz_date.tm_mon = st.wMonth - 1;
        pzi->tmz_date.tm_year = st.wYear;
    }

    const DWORD cbBuff = 0x7FFF;
    LPBYTE pbBuff = reinterpret_cast<LPBYTE>(CoTaskMemAlloc(cbBuff));
    if (!pbBuff)
    {
        DPRINT1("Out of memory\n");
        CloseHandle(hFile);
        return FALSE;
    }

    for (;;)
    {
        DWORD cbRead;
        if (!ReadFile(hFile, pbBuff, cbBuff, &cbRead, NULL) || !cbRead)
            break;

        for (DWORD i = 0; i < cbRead; ++i)
            contents.Add(pbBuff[i]);
    }

    CoTaskMemFree(pbBuff);
    CloseHandle(hFile);

    return TRUE;
}

static void
DoAddFilesFromItem(CSimpleArray<CStringW>& files, LPCWSTR item)
{
    if (!PathIsDirectoryW(item))
    {
        files.Add(item);
        return;
    }

    WCHAR szPath[MAX_PATH];
    StringCbCopyW(szPath, sizeof(szPath), item);
    PathAppendW(szPath, L"*");

    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (wcscmp(find.cFileName, L".") == 0 ||
            wcscmp(find.cFileName, L"..") == 0)
        {
            continue;
        }

        StringCbCopyW(szPath, sizeof(szPath), item);
        PathAppendW(szPath, find.cFileName);

        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            DoAddFilesFromItem(files, szPath);
        else
            files.Add(szPath);
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);
}

struct CZipCreatorImpl
{
    CSimpleArray<CStringW> m_items;

    unsigned JustDoIt();
};

CZipCreator::CZipCreator() : m_pimpl(new CZipCreatorImpl)
{
    InterlockedIncrement(&g_ModuleRefCnt);
}

CZipCreator::~CZipCreator()
{
    InterlockedDecrement(&g_ModuleRefCnt);
    delete m_pimpl;
}

static unsigned __stdcall
create_zip_function(void *arg)
{
    CZipCreator *pCreator = reinterpret_cast<CZipCreator *>(arg);
    return pCreator->m_pimpl->JustDoIt();
}

BOOL CZipCreator::runThread(CZipCreator *pCreator)
{
    unsigned tid = 0;
    HANDLE hThread = reinterpret_cast<HANDLE>(
        _beginthreadex(NULL, 0, create_zip_function, pCreator, 0, &tid));

    if (hThread)
    {
        CloseHandle(hThread);
        return TRUE;
    }

    DPRINT1("hThread == NULL\n");

    CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
    CStringW strText(MAKEINTRESOURCEW(IDS_CANTSTARTTHREAD));
    MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);

    delete pCreator;
    return FALSE;
}

void CZipCreator::DoAddItem(LPCWSTR pszFile)
{
    // canonicalize path
    WCHAR szPath[MAX_PATH];
    GetFullPathNameW(pszFile, _countof(szPath), szPath, NULL);

    m_pimpl->m_items.Add(szPath);
}

enum CZC_ERROR
{
    CZCERR_ZEROITEMS = 1,
    CZCERR_NOFILES,
    CZCERR_CREATE,
    CZCERR_READ
};

unsigned CZipCreatorImpl::JustDoIt()
{
    // TODO: Show progress.

    if (m_items.GetSize() <= 0)
    {
        DPRINT1("GetSize() <= 0\n");
        return CZCERR_ZEROITEMS;
    }

    CSimpleArray<CStringW> files;
    for (INT iItem = 0; iItem < m_items.GetSize(); ++iItem)
    {
        DoAddFilesFromItem(files, m_items[iItem]);
    }

    if (files.GetSize() <= 0)
    {
        DPRINT1("files.GetSize() <= 0\n");

        CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
        CStringW strText;
        strText.Format(IDS_NOFILES, static_cast<LPCWSTR>(m_items[0]));
        MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);

        return CZCERR_NOFILES;
    }

    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64W(&ffunc);

    CStringW strZipName = DoGetZipName(m_items[0]);
    zipFile zf = zipOpen2_64(strZipName, APPEND_STATUS_CREATE, NULL, &ffunc);
    if (zf == 0)
    {
        DPRINT1("zf == 0\n");

        int err = CZCERR_CREATE;

        CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
        CStringW strText;
        strText.Format(IDS_CANTCREATEZIP, static_cast<LPCWSTR>(strZipName), err);
        MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);

        return err;
    }

    // TODO: password
    const char *password = NULL;
    int zip64 = 1; // always zip64
    zip_fileinfo zi;

    int err = 0;
    CStringW strTarget, strBaseName = DoGetBaseName(m_items[0]);
    for (INT iFile = 0; iFile < files.GetSize(); ++iFile)
    {
        const CStringW& strFile = files[iFile];

        CSimpleArray<BYTE> contents;
        if (!DoReadAllOfFile(strFile, contents, &zi))
        {
            DPRINT1("DoReadAllOfFile failed\n");
            err = CZCERR_READ;
            strTarget = strFile;
            break;
        }

        unsigned long crc = 0;
        if (password)
        {
            // TODO: crc = ...;
        }

        CStringA strNameInZip = DoGetNameInZip(strBaseName, strFile);
        err = zipOpenNewFileInZip3_64(zf,
                                      strNameInZip,
                                      &zi,
                                      NULL,
                                      0,
                                      NULL,
                                      0,
                                      NULL,
                                      Z_DEFLATED,
                                      Z_DEFAULT_COMPRESSION,
                                      0,
                                      -MAX_WBITS,
                                      DEF_MEM_LEVEL,
                                      Z_DEFAULT_STRATEGY,
                                      password,
                                      crc,
                                      zip64);
        if (err)
        {
            DPRINT1("zipOpenNewFileInZip3_64\n");
            break;
        }

        err = zipWriteInFileInZip(zf, contents.GetData(), contents.GetSize());
        if (err)
        {
            DPRINT1("zipWriteInFileInZip\n");
            break;
        }

        err = zipCloseFileInZip(zf);
        if (err)
        {
            DPRINT1("zipCloseFileInZip\n");
            break;
        }
    }

    zipClose(zf, NULL);

    if (err)
    {
        DeleteFileW(strZipName);

        CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));

        CStringW strText;
        if (err < 0)
            strText.Format(IDS_CANTCREATEZIP, static_cast<LPCWSTR>(strZipName), err);
        else
            strText.Format(IDS_CANTREADFILE, static_cast<LPCWSTR>(strTarget));

        MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);
    }
    else
    {
        WCHAR szFullPath[MAX_PATH];
        GetFullPathNameW(strZipName, _countof(szFullPath), szFullPath, NULL);
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, szFullPath, NULL);
    }

    return err;
}
