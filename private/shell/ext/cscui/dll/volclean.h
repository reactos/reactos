//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       volclean.h
//
//--------------------------------------------------------------------------

#ifndef _VOLFREE_H_
#define _VOLFREE_H_

#include "purge.h"

class CCscVolumeCleaner : public IEmptyVolumeCache2
{
    LONG m_cRef;
    BOOL m_fPinned; // TRUE if this instance frees pinned files, FALSE for auto-cached files
    CCachePurger *m_pPurger;
    LPEMPTYVOLUMECACHECALLBACK m_pDiskCleaner;
    DWORDLONG m_dwlSpaceToFree;
    DWORDLONG m_dwlSpaceFreed;
    CCachePurgerSel m_PurgerSel;

public:
    CCscVolumeCleaner(BOOL fPinned) : m_cRef(1), m_fPinned(fPinned), m_pPurger(NULL),
        m_pDiskCleaner(NULL), m_dwlSpaceToFree(0), m_dwlSpaceFreed(0)
    {
        DllAddRef();
    }
    ~CCscVolumeCleaner()
    {
        delete m_pPurger;
        DllRelease();
    }

    static HRESULT WINAPI CreateInstance(REFIID riid, LPVOID *ppv);
    static HRESULT WINAPI CreateInstance2(REFIID riid, LPVOID *ppv);

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEmptyVolumeCache methods
    STDMETHODIMP Initialize(HKEY hkRegKey,
                            LPCWSTR pcwszVolume,
                            LPWSTR *ppwszDisplayName,
                            LPWSTR *ppwszDescription,
                            LPDWORD pdwFlags);
    STDMETHODIMP GetSpaceUsed(DWORDLONG *pdwlSpaceUsed,
                              LPEMPTYVOLUMECACHECALLBACK picb);
    STDMETHODIMP Purge(DWORDLONG dwlSpaceToFree,
                       LPEMPTYVOLUMECACHECALLBACK picb);
    STDMETHODIMP ShowProperties(HWND hwnd);
    STDMETHODIMP Deactivate(LPDWORD pdwFlags);

    // IEmptyVolumeCache methods
    STDMETHODIMP InitializeEx(HKEY hkRegKey,
                              LPCWSTR pcwszVolume,
                              LPCWSTR pcwszKeyName,
                              LPWSTR *ppwszDisplayName,
                              LPWSTR *ppwszDescription,
                              LPWSTR *ppwszBtnText,
                              LPDWORD pdwFlags);

private:
    BOOL ScanCallback(CCachePurger *pPurger);
    BOOL DeleteCallback(CCachePurger *pPurger);

    static BOOL CALLBACK CachePurgerCallback(CCachePurger *pPurger);
    static HRESULT WINAPI Create(BOOL fPinned, REFIID riid, LPVOID *ppv);
};
typedef CCscVolumeCleaner *PCSCVOLCLEANER;

#endif  // _VOLFREE_H_
