/*
 * PROJECT:     ReactOS CabView Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FDI API wrapper
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"
#include "cabview.h"
#include "util.h"
#include <fcntl.h>

struct EXTRACTCABINETINTERNALDATA
{
    LPCWSTR destination;
    EXTRACTCALLBACK callback;
    LPVOID cookie;
};

static LPWSTR BuildPath(LPCWSTR Dir, LPCSTR File, UINT Attr)
{
    UINT cp = Attr & _A_NAME_IS_UTF ? CP_UTF8 : CP_ACP;
    UINT cchfile = MultiByteToWideChar(cp, 0, File, -1, 0, 0);
    SIZE_T lendir = lstrlenW(Dir), cch = lendir + 1 + cchfile;
    LPWSTR path = (LPWSTR)SHAlloc(cch * sizeof(*path));
    if (path)
    {
        lstrcpyW(path, Dir);
        if (lendir && !IsPathSep(path[lendir - 1]))
            path[lendir++] = '\\';

        LPWSTR dst = &path[lendir];
        MultiByteToWideChar(cp, 0, File + IsPathSep(*File), -1, dst, cchfile);
        for (SIZE_T i = 0; dst[i]; ++i)
        {
            if (dst[i] == L':' && lendir) // Don't allow absolute paths
                dst[i] = L'_';
            if (dst[i] == L'/') // Normalize
                dst[i] = L'\\';
        }
    }
    return path;
}

static HRESULT HResultFrom(const ERF &erf)
{
    switch (erf.fError ? erf.erfOper : FDIERROR_NONE)
    {
        case FDIERROR_NONE:
            return erf.fError ? HRESULT_FROM_WIN32(erf.erfType) : S_OK;
        case FDIERROR_CABINET_NOT_FOUND:
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        case FDIERROR_ALLOC_FAIL:
            return E_OUTOFMEMORY;
        case FDIERROR_USER_ABORT:
            return S_FALSE;
        default:
            return erf.erfType ? HRESULT_FROM_WIN32(erf.erfType) : E_FAIL;
    }
}

FNFREE(CabMemFree)
{
    SHFree(pv);
}

FNALLOC(CabMemAlloc)
{
    return SHAlloc(cb);
}

FNCLOSE(CabClose)
{
    return CloseHandle((HANDLE)hf) ? 0 : -1;
}

static INT_PTR CabOpenEx(LPCWSTR path, UINT access, UINT share, UINT disp, UINT attr)
{
    return (INT_PTR)CreateFileW(path, access, share, NULL, disp, attr, NULL);
}

FNOPEN(CabOpen)
{
    UINT disp = (oflag & _O_CREAT) ? CREATE_ALWAYS : OPEN_EXISTING;
    UINT access = GENERIC_READ;
    if (oflag & _O_RDWR)
        access = GENERIC_READ | GENERIC_WRITE;
    else if (oflag & _O_WRONLY)
        access = GENERIC_WRITE;
    UNREFERENCED_PARAMETER(pmode);
    WCHAR buf[MAX_PATH * 2];
    MultiByteToWideChar(CP_UTF8, 0, pszFile, -1, buf, _countof(buf));
    return CabOpenEx(buf, access, FILE_SHARE_READ, disp, FILE_ATTRIBUTE_NORMAL);
}

FNREAD(CabRead)
{
    DWORD dwBytesRead;
    return ReadFile((HANDLE)hf, pv, cb, &dwBytesRead, NULL) ? dwBytesRead : -1;
}

FNWRITE(CabWrite)
{
    DWORD dwBytesWritten;
    return WriteFile((HANDLE)hf, pv, cb, &dwBytesWritten, NULL) ? dwBytesWritten : -1;
}

FNSEEK(CabSeek)
{
    return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
}

static HRESULT Init(HFDI &hfdi, ERF &erf)
{
    const int cpu = cpuUNKNOWN;
    hfdi = FDICreate(CabMemAlloc, CabMemFree, CabOpen, CabRead, CabWrite, CabClose, CabSeek, cpu, &erf);
    return hfdi ? S_OK : HResultFrom(erf);
}

FNFDINOTIFY(ExtractCabinetCallback)
{
    const UINT attrmask = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE;
    EXTRACTCABINETINTERNALDATA &ecd = *(EXTRACTCABINETINTERNALDATA*)pfdin->pv;
    EXTRACTCALLBACKDATA noti;
    HRESULT hr;
    FILETIME ft;

    noti.pfdin = pfdin;
    switch (fdint)
    {
        case fdintCOPY_FILE:
            hr = ecd.callback(ECM_FILE, noti, ecd.cookie);
            if (hr == S_OK)
            {
                hr = E_OUTOFMEMORY;
                LPWSTR path = BuildPath(ecd.destination, pfdin->psz1, pfdin->attribs);
                if (path)
                {
                    // Callee is using SHPPFW_IGNOREFILENAME so we don't need to remove the name.
                    /*LPWSTR file = PathFindFileNameW(path);
                    if (file > path)
                    {
                        file[-1] = L'\0';*/
                        noti.Path = path;
                        ecd.callback(ECM_PREPAREPATH, noti, ecd.cookie);
                    /*  file[-1] = L'\\';
                    }*/
                    UINT attr = pfdin->attribs & attrmask;
                    UINT access = GENERIC_READ | GENERIC_WRITE, share = FILE_SHARE_DELETE;
                    INT_PTR handle = CabOpenEx(path, access, share, CREATE_NEW, attr | FILE_FLAG_SEQUENTIAL_SCAN);
                    noti.hr = HResultFromWin32(GetLastError());
                    SHFree(path);
                    if (handle != (INT_PTR)-1)
                        return handle;
                    if (ecd.callback(ECM_ERROR, noti, ecd.cookie) != E_NOTIMPL)
                        hr = noti.hr;
                }
            }
            return hr == S_FALSE ? 0 : -1;

        case fdintCLOSE_FILE_INFO:
            if (DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
                SetFileTime((HANDLE)(pfdin->hf), NULL, NULL, &ft);
            return !CabClose(pfdin->hf);

        case fdintNEXT_CABINET:
            if (pfdin->fdie && pfdin->fdie != FDIERROR_USER_ABORT)
            {
                if (pfdin->fdie == FDIERROR_CABINET_NOT_FOUND)
                    noti.hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                else
                    noti.hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                ecd.callback(ECM_ERROR, noti, ecd.cookie);
            }
            return pfdin->fdie ? -1 : 0;

        case fdintPARTIAL_FILE:
            return 0;

        case fdintCABINET_INFO:
            return 0;

        case fdintENUMERATE:
            return 0;
    }
    return -1;
}

HRESULT ExtractCabinet(LPCWSTR cab, LPCWSTR destination, EXTRACTCALLBACK callback, LPVOID cookie)
{
    BOOL quick = !destination;
    if (!destination)
        destination = L"?:"; // Dummy path for callers that enumerate without extracting
    EXTRACTCABINETINTERNALDATA data = { destination, callback, cookie };
    EXTRACTCALLBACKDATA noti;
    ERF erf = { };
    HFDI hfdi;
    UINT total = 0, files = 0;
    HRESULT hr = Init(hfdi, erf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    UINT share = FILE_SHARE_READ | FILE_SHARE_DELETE;
    INT_PTR hf = quick ? -1 : CabOpenEx(cab, GENERIC_READ, share, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
    if (hf != -1)
    {
        FDICABINETINFO ci;
        if (FDIIsCabinet(hfdi, hf, &ci))
        {
            total = ci.cbCabinet;
            files = ci.cFiles;
        }
        CabClose(hf);
    }

    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    char buf[MAX_PATH * 2], *name = 0;
    if (!WideCharToMultiByte(CP_UTF8, 0, cab, -1, buf, _countof(buf), NULL, NULL))
    {
        *buf = '\0';
        hr = E_INVALIDARG;
    }
    for (UINT i = 0; buf[i]; ++i)
    {
        if (buf[i] == '\\' || buf[i] == '/')
            name = &buf[i + 1];
    }
    if (name > buf && *name)
    {
        // Format the name the way FDI likes it
        name[-1] = ANSI_NULL;
        char namebuf[MAX_PATH];
        namebuf[0] = '\\';
        lstrcpyA(namebuf + 1, name);
        name = namebuf;

        FDINOTIFICATION fdin;
        fdin.cb = total;
        fdin.hf = files;
        noti.Path = cab;
        noti.pfdin = &fdin;
        callback(ECM_BEGIN, noti, cookie);

        hr = FDICopy(hfdi, name, buf, 0, ExtractCabinetCallback, NULL, &data) ? S_OK : HResultFrom(erf);
    }
    FDIDestroy(hfdi);
    return hr;
}
