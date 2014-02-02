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
