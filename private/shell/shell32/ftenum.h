#ifndef FTENUM_H
#define FTENUM_H

#include "ascstr.h"

class CFTEnumAssocInfo : public IEnumAssocInfo
{
public:
    CFTEnumAssocInfo();
    ~CFTEnumAssocInfo();

public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, PVOID* ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IEnumAssocInfo methods
    //  Initialization
    STDMETHOD(Init)(ASENUM asenumFlags, LPTSTR pszStr,
        AIINIT aiinitFlags);

    //  Standard IEnum methods
    STDMETHOD(Next)(IAssocInfo** ppAI);
    STDMETHOD(Skip)(DWORD dwSkip);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumAssocInfo* pEnum);

protected:
    STDMETHOD(_EnumHKCR)(ASENUM asenumFlags, LPTSTR pszStr, 
                                        DWORD* pcchStr);
    STDMETHOD(_EnumProgIDActions)(LPTSTR pszStr, DWORD* pcchStr);
    STDMETHOD_(BOOL, _EnumKCRSkip)(DWORD asenumFlags, LPTSTR pszExt);
    STDMETHOD_(BOOL, _EnumKCRStop)(DWORD asenumFlags, LPTSTR pszExt);

protected:
    LONG                    _cRef;

    ASENUM                  _asenumFlags;
    TCHAR                   _szInitStr[MAX_FTMAX];
    AIINIT                  _aiinitFlags;

    BOOL                    _fFirstExtFound;

    DWORD                   _dwIndex;
};

#endif //FTENUM_H