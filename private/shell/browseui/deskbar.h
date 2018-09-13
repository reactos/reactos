// coming soon: new deskbar (old deskbar moved to browbar base class)
#ifndef DESKBAR_H_
#define DESKBAR_H_

#include "dockbar.h"

#ifndef NOCDESKBAR

class CDeskBar : public CDockingBar
               , public IRestrict
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void)   { return CDockingBar::AddRef(); }
    virtual STDMETHODIMP_(ULONG) Release(void)  { return CDockingBar::Release(); }
    virtual STDMETHODIMP         QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IPersistStreamInit ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IRestrict ***
    virtual STDMETHODIMP IsRestricted(const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult);
    
    CDeskBar();

protected:
    BITBOOL _fRestrictionsInited :1;        // Have we read in the restrictions?
    BITBOOL _fRestrictDDClose :1;           // Restrict: Add, Close, Drag & Drop
    BITBOOL _fRestrictMove :1;              // Restrict: Move
};

#endif

class CDeskBarPropertyBag : public CDockingBarPropertyBag
{
};

#endif
