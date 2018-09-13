//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CAsyncMk.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-25-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

class FAR CAsyncMoniker :  public IMoniker
{
public:

                // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        // Release has to implemented by the derived class

        // *** IPersistStream methods ***
        STDMETHOD(IsDirty) (THIS);
        STDMETHOD(Load) (THIS_ LPSTREAM pStm);
        STDMETHOD(Save) (THIS_ LPSTREAM pStm,BOOL fClearDirty);
        STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

        // *** IMoniker methods ***
        STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                REFIID riidResult, LPVOID FAR* ppvResult);
        STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD(Reduce) (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*
                ppmkToLeft, LPMONIKER FAR * ppmkReduced);
        STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
                LPMONIKER FAR* ppmkComposite);
        STDMETHOD(Enum) (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);
        STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker);
        STDMETHOD(Hash) (THIS_ LPDWORD pdwHash);
        STDMETHOD(IsRunning) (THIS_ LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER
                pmkNewlyRunning);
        STDMETHOD(GetTimeOfLastChange) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                FILETIME FAR* pfiletime);
        STDMETHOD(Inverse) (THIS_ LPMONIKER FAR* ppmk);
        STDMETHOD(CommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
                ppmkPrefix);
        STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
                ppmkRelPath);
        STDMETHOD(GetDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                LPWSTR FAR* lplpszDisplayName);
        STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
                LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
                LPMONIKER FAR* ppmkOut);
        STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);

private:
        ULONG   m_refs;

protected:

        CAsyncMoniker(void) { m_refs = 0; }
};

