#include "shellprv.h"
#pragma  hdrstop

#include "defext.h"

//===========================================================================
// Common : thunks
//===========================================================================

STDMETHODIMP Common_QueryInterface(void * punk, REFIID riid, LPVOID * ppvObj)
{
    CCommonUnknown *this = IToCommonUnknown(punk);

    return(this->unk.lpVtbl->QueryInterface(&(this->unk), riid, ppvObj));
}

STDMETHODIMP_(ULONG) Common_AddRef(void * punk)
{
    CCommonUnknown *this = IToCommonUnknown(punk);

    return(this->unk.lpVtbl->AddRef(&(this->unk)));
}

STDMETHODIMP_(ULONG) Common_Release(void * punk)
{
    CCommonUnknown *this = IToCommonUnknown(punk);

    //
    // This is a fatal assertion; it will cause a stack fault.
    //
    ASSERT(punk!=(LPVOID)&this->unk);

    return(this->unk.lpVtbl->Release(&(this->unk)));
}

