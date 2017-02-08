/*
 * PROJECT:         ReactOS crt library
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Compiler support for COM
 * PROGRAMMER:      Thomas Faber (thomas.faber@reactos.org)
 */

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#include <comdef.h>

/* comdef.h */
typedef void WINAPI COM_ERROR_HANDLER(HRESULT, IErrorInfo *);
static COM_ERROR_HANDLER *com_error_handler;

void WINAPI _com_raise_error(HRESULT hr, IErrorInfo *perrinfo)
{
    throw _com_error(hr, perrinfo);
}

void WINAPI _set_com_error_handler(COM_ERROR_HANDLER *phandler)
{
    com_error_handler = phandler;
}

void WINAPI _com_issue_error(HRESULT hr)
{
    com_error_handler(hr, NULL);
}

void WINAPI _com_issue_errorex(HRESULT hr, IUnknown *punk, REFIID riid)
{
    void *pv;
    IErrorInfo *perrinfo = NULL;

    if (SUCCEEDED(punk->QueryInterface(riid, &pv)))
    {
        ISupportErrorInfo *pserrinfo = static_cast<ISupportErrorInfo *>(pv);
        if (pserrinfo->InterfaceSupportsErrorInfo(riid) == S_OK)
            (void)GetErrorInfo(0, &perrinfo);
        pserrinfo->Release();
    }

    com_error_handler(hr, perrinfo);
}

/* comutil.h */
_variant_t vtMissing(DISP_E_PARAMNOTFOUND, VT_ERROR);

namespace _com_util
{

BSTR WINAPI ConvertStringToBSTR(const char *pSrc)
{
    DWORD cwch;
    BSTR wsOut(NULL);

    if (!pSrc) return NULL;

    /* Compute the needed size with the NULL terminator */
    cwch = ::MultiByteToWideChar(CP_ACP, 0, pSrc, -1, NULL, 0);
    if (cwch == 0) return NULL;

    /* Allocate the BSTR (without the NULL terminator) */
    wsOut = ::SysAllocStringLen(NULL, cwch - 1);
    if (!wsOut)
    {
        ::_com_issue_error(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
        return NULL;
    }

    /* Convert the string */
    if (::MultiByteToWideChar(CP_ACP, 0, pSrc, -1, wsOut, cwch) == 0)
    {
        /* We failed, clean everything up */
        cwch = ::GetLastError();

        ::SysFreeString(wsOut);
        wsOut = NULL;

        ::_com_issue_error(!IS_ERROR(cwch) ? HRESULT_FROM_WIN32(cwch) : cwch);
    }

    return wsOut;
}

char* WINAPI ConvertBSTRToString(BSTR pSrc)
{
    DWORD cb, cwch;
    char *szOut = NULL;

    if (!pSrc) return NULL;

    /* Retrieve the size of the BSTR with the NULL terminator */
    cwch = ::SysStringLen(pSrc) + 1;

    /* Compute the needed size with the NULL terminator */
    cb = ::WideCharToMultiByte(CP_ACP, 0, pSrc, cwch, NULL, 0, NULL, NULL);
    if (cb == 0)
    {
        cwch = ::GetLastError();
        ::_com_issue_error(!IS_ERROR(cwch) ? HRESULT_FROM_WIN32(cwch) : cwch);
        return NULL;
    }

    /* Allocate the string */
    szOut = (char*)::operator new(cb * sizeof(char));
    if (!szOut)
    {
        ::_com_issue_error(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
        return NULL;
    }

    /* Convert the string and NULL-terminate */
    szOut[cb - 1] = '\0';
    if (::WideCharToMultiByte(CP_ACP, 0, pSrc, cwch, szOut, cb, NULL, NULL) == 0)
    {
        /* We failed, clean everything up */
        cwch = ::GetLastError();

        ::operator delete(szOut);
        szOut = NULL;

        ::_com_issue_error(!IS_ERROR(cwch) ? HRESULT_FROM_WIN32(cwch) : cwch);
    }

    return szOut;
}

}
