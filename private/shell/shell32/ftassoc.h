#ifndef FTASSOC_H
#define FTASSOC_H

#include "ascstr.h"
#include "regsprtb.h"

class CFTAssocInfo : public IAssocInfo, private CRegSupportBuf
{
public:
    CFTAssocInfo();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, PVOID* ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IAssocInfo methods
    //  Init
    STDMETHOD(Init)(AIINIT aiinitFlags, LPTSTR pszStr);
    STDMETHOD(InitComplex)(AIINIT aiinitFlags1, LPTSTR pszStr1,
        AIINIT aiinitFlags2, LPTSTR pszStr2);
    //  Get
    STDMETHOD(GetString)(AISTR aistrFlags, LPTSTR pszStr, DWORD* cchStr);
    STDMETHOD(GetDWORD)(AIDWORD aidwordFlags, DWORD* pdwdata);
    STDMETHOD(GetBOOL)(AIDWORD aidwordFlags, BOOL* pBool);
    STDMETHOD(GetData)(AIDWORD aidataFlags, PBYTE pbData, DWORD* pcbData);

    //  Set
    STDMETHOD(SetString)(AISTR aistrFlags, LPTSTR pszStr);
    STDMETHOD(SetDWORD)(AIDWORD aidwordFlags, DWORD dwData);
    STDMETHOD(SetBOOL)(AIDWORD aiboolFlags, BOOL fBool);
    STDMETHOD(SetData)(AIDWORD aidataFlags, PBYTE pbData, DWORD cbData);

    //  Create
    STDMETHOD(Create)();

    //  Delete
    STDMETHOD(DelString)(AISTR aistrFlags);
    STDMETHOD(Delete)(AIALL aiallFlags);

protected:
    HRESULT _IsBrowseInPlace(BOOL* pfBool);
    HRESULT _SetBrowseInPlace(BOOL fBool);
    HRESULT _IsBrowseInPlaceEnabled(BOOL* pfBool);

    HRESULT _IsEditFlagSet(DWORD dwMask, BOOL* pfBool);
    HRESULT _SetEditFlagSet(DWORD dwMask, BOOL fBool);

    HRESULT _CreateProgID();
    HRESULT _GetProgIDActionAttributes(DWORD* pdwAttributes);
    HRESULT _GetProgIDEditFlags(DWORD* pdwEditFlags);
    HRESULT _GetOpenWithInfo(LPTSTR pszStr, DWORD* pcchStr);
    HRESULT _ExtIsAssociated(BOOL* pfIsAssociated);
    HRESULT _GetExtDocIcon(LPTSTR pszExt, BOOL fSmall, int* piIcon);
    HRESULT _GetProgIDDocIcon(BOOL fSmall, int* piIcon);
    HRESULT _GetAppIcon(BOOL fSmall, int* piIcon);
    HRESULT _GetIconLocation(LPTSTR pszStr, DWORD* pcchStr);
    HRESULT _SetIconLocation(LPTSTR pszStr);

    HRESULT _GetProgIDDefaultAction(LPTSTR pszStr, DWORD* pcchStr);
    HRESULT _SetProgIDDefaultAction(LPTSTR pszStr);

    HRESULT _GetProgIDDescr(LPTSTR pszProgIDDescr, DWORD* pcchProgIDdescr);

    HRESULT __InitImageLists();

protected:
    HRESULT _OpenSubKey(LPTSTR pszSubKey, REGSAM samDesired, HKEY* phKey);

protected:
    TCHAR                   _szInitStr1[MAX_FTMAX];
    AIINIT                  _aiinitFlags1;
    TCHAR                   _szInitStr2[MAX_FTMAX];
    AIINIT                  _aiinitFlags2;

    static HIMAGELIST       _himlSysSmall;
    static HIMAGELIST       _himlSysLarge;
private:
    LONG                    _cRef;
};

#endif //FTASSOC_H