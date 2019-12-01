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
#include <sys/stat.h>

static CStringW DoGetZipName(const CStringW& filename)
{
    WCHAR szPath[MAX_PATH];
    StringCbCopyW(szPath, sizeof(szPath), filename);
    PathRemoveExtensionW(szPath);

    CStringW ret = szPath;
    ret += L".zip";

    UINT i = 2;
    while (PathFileExistsW(ret))
    {
        ret = szPath;

        CStringW str;
        str.Format(L" (%u).zip", i++);
        ret += str;
    }

    return ret;
}

static CStringA DoGetUTF8Name(const CStringW& filename)
{
    CHAR buf[MAX_PATH * 3 + 1];
    WideCharToMultiByte(CP_UTF8, 0, filename, -1, buf, _countof(buf), NULL, NULL);
    return buf;
}

static CStringW DoGetBaseName(const CStringW& filename)
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
    CStringW ret = filename;

    if (filename.Find(basename) == 0)
        ret = ret.Mid(basename.GetLength());

    ret.Replace(L'\\', L'/');

    return DoGetUTF8Name(ret);
}

static BOOL
DoReadAllOfFile(const CStringW& filename, CSimpleArray<BYTE>& contents,
                zip_fileinfo *pzi)
{
    contents.RemoveAll();

    struct _stat st;
    if (!_wstat(filename, &st))
        pzi->dosDate = st.st_mtime;
    else
        pzi->dosDate = 0;

    FILE *fp = _wfopen(filename, L"rb");
    if (fp == NULL)
        return FALSE;

    char buf[512];
    for (;;)
    {
        SIZE_T count = fread(buf, 1, sizeof(buf), fp);
        if (count == 0)
            break;

        for (SIZE_T i = 0; i < count; ++i)
        {
            contents.Add(buf[i]);
        }
    }

    fclose(fp);
    return TRUE;
}

static void
DoAddItem(CSimpleArray<CStringW>& files, const CStringW& item)
{
    if (!PathIsDirectoryW(item))
    {
        files.Add(item);
        return;
    }

    WCHAR szDir[MAX_PATH], szPath[MAX_PATH];
    StringCbCopyW(szPath, sizeof(szPath), item);
    PathAppendW(szPath, L"*");

    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    StringCbCopyW(szDir, sizeof(szDir), item);

    do
    {
        if (wcscmp(find.cFileName, L".") == 0 ||
            wcscmp(find.cFileName, L"..") == 0)
        {
            continue;
        }

        StringCbCopyW(szPath, sizeof(szPath), szDir);
        PathAppendW(szPath, find.cFileName);

        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            DoAddItem(files, szPath);
        }
        else
        {
            files.Add(szPath);
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);
}

struct CZipCreatorImpl
{
    CSimpleArray<CStringW> m_items;
    CStringW m_strBaseName;

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

CZipCreator* CZipCreator::DoCreate()
{
    return new CZipCreator();
}

static unsigned __stdcall
create_zip_function(void *arg)
{
    CZipCreator *pCreater = reinterpret_cast<CZipCreator *>(arg);
    return pCreater->m_pimpl->JustDoIt();
}

BOOL CZipCreator::runThread(CZipCreator *pCreater)
{
    unsigned tid = 0;

    HANDLE hThread = reinterpret_cast<HANDLE>(
        _beginthreadex(NULL, 0, create_zip_function, pCreater, 0, &tid));

    if (hThread)
    {
        CloseHandle(hThread);
        return TRUE;
    }

    DPRINT1("hThread == NULL\n");

    CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
    CStringW strText(MAKEINTRESOURCEW(IDS_CANTSTARTTHREAD));
    MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);

    delete pCreater;
    return FALSE;
}

void CZipCreator::DoAddItem(LPCWSTR pszFile)
{
    m_pimpl->m_items.Add(pszFile);
}

unsigned CZipCreatorImpl::JustDoIt()
{
    // TODO: Show progress.

    if (m_items.GetSize() <= 0)
    {
        DPRINT1("GetSize() <= 0\n");
        return -1;
    }

    CStringW szBaseName = DoGetBaseName(m_items[0]);
    CStringW strZipName = DoGetZipName(m_items[0]);

    CSimpleArray<CStringW> files;
    for (INT iItem = 0; iItem < m_items.GetSize(); ++iItem)
    {
        DoAddItem(files, m_items[iItem]);
    }

    if (files.GetSize() <= 0)
    {
        DPRINT1("files.GetSize() <= 0\n");

        CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
        CStringW strText;
        strText.Format(IDS_NOFILES, static_cast<LPCWSTR>(m_items[0]));
        MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);

        return -2;
    }

    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64W(&ffunc);

    zipFile zf = zipOpen2_64(strZipName, APPEND_STATUS_CREATE, NULL, &ffunc);
    if (zf == 0)
    {
        DPRINT1("zf == 0\n");

        CStringW strTitle(MAKEINTRESOURCEW(IDS_ERRORTITLE));
        CStringW strText;
        strText.Format(IDS_CANTOPENFILE, static_cast<LPCWSTR>(strZipName));
        MessageBoxW(NULL, strText, strTitle, MB_ICONERROR);

        return -1;
    }

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    int zip64 = 1; // always zip64

    // TODO: password
    const char *password = NULL;

    int err = 0;
    for (INT iFile = 0; iFile < files.GetSize(); ++iFile)
    {
        CStringW& strFile = files[iFile];
        unsigned long crc = 0;
        if (password)
        {
            // TODO: crc = ...;
        }

        CSimpleArray<BYTE> contents;
        if (!DoReadAllOfFile(strFile, contents, &zi))
        {
            DPRINT1("DoReadAllOfFile failed\n");
            err = 9999;
            break;
        }

        CStringA strNameInZip = DoGetNameInZip(szBaseName, strFile);
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
    }

    return err;
}
