#ifndef FTASCSTR_H
#define FTASCSTR_H

#include "ascstr.h"

class CFTAssocStore : public IAssocStore
{
public:
    CFTAssocStore();
    ~CFTAssocStore();
public:
	//IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, PVOID* ppv);
	STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

	//IAssocStore methods
    //	Enum
    STDMETHOD(EnumAssocInfo)(ASENUM asenumFlags, LPTSTR pszStr, 
		AIINIT aiinitFlags, IEnumAssocInfo** ppEnum);
    //  Get/Set
    STDMETHOD(GetAssocInfo)(LPTSTR pszStr, AIINIT aiinitFlags, 
		IAssocInfo** ppAI);
    STDMETHOD(GetComplexAssocInfo)(LPTSTR pszStr1, AIINIT aiinitFlags1, 
		LPTSTR pszStr2, AIINIT aiinitFlags2, IAssocInfo** ppAI);

    // S_OK: We have create/delete access,
    // S_FALSE: we do not have create and/or delete access to HKCR
    STDMETHOD(CheckAccess)();

private:
	friend class CFTEnumAssocInfo;
	static HRESULT __GetProgIDDescr(LPTSTR pszProgID, LPTSTR pszProgIDdescr,
		                    DWORD cchProgIDdescr);
private:
    HRESULT                _hresCoInit;
    LONG                   _cRef;
    static HRESULT         _hresAccess;
};

#endif //FTASCSTR_H