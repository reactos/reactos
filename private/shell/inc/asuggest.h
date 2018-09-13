#ifndef _ASUGGEST_H_
#define _ASUGGEST_H_

// Private interface used between shdocvw and browseui. Currently needed
//  only for Intelliforms

// IID_IAutoCompleteDropDown in shguidp.h

#undef  INTERFACE
#define INTERFACE   IAutoCompleteDropDown

DECLARE_INTERFACE_(IAutoCompleteDropDown, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IAutoCompleteDropDown methods ***
    STDMETHOD(GetDropDownStatus)(THIS_ DWORD *pdwFlags, LPWSTR *ppwszString) PURE;
    STDMETHOD(ResetEnumerator)(THIS) PURE;
};

//
//  Flags for IAutoCompleteDropDown::GetDropDownStatus
//
#define ACDD_VISIBLE        0x0001

#endif // _ASUGGEST_H_
