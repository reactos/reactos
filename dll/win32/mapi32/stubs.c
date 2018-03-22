#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

typedef PVOID LPMAPIFORMMGR, LPADDRESSBOOK;

HRESULT
WINAPI
MAPIOpenFormMgr(
    LPMAPISESSION pSession,
    LPMAPIFORMMGR *ppmgr)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


HRESULT
WINAPI
OpenTnefStream(
    LPVOID lpvSupport,
    LPSTREAM lpStream,
    LPTSTR lpszStreamName, 
    ULONG ulFlags,
    LPMESSAGE lpMessage,
    WORD wKey,
    LPSTREAM *lppTNEF)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
OpenTnefStreamEx(
    LPVOID lpvSupport,
    LPSTREAM lpStream,
    LPTSTR lpszStreamName,
    ULONG ulFlags,
    LPMESSAGE lpMessage,
    WORD wKeyVal,
    LPADDRESSBOOK lpAdressBook,
    LPSTREAM *lppTNEF)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
GetTnefStreamCodepage(
    LPSTREAM lpStream,
    ULONG *lpulCodepage,
    ULONG *lpulSubCodepage)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
RTFSync(
    LPMESSAGE lpMessage,
    ULONG ulFlags,
    BOOL *lpfMessageUpdated)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
HrGetOmiProvidersFlags(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
HrSetOmiProvidersFlagsInvalid(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

ULONG
WINAPI
GetOutlookVersion(void)
{
    UNIMPLEMENTED;
    return 0;
}

HRESULT
WINAPI
FixMAPI(void)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
