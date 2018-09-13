//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       volclean.cpp
//
//  Authors;
//    Jeff Saathoff (jeffreys)
//
//  Notes;
//    CSC disk cleanup implementation (IEmptyVolumeCache)
//--------------------------------------------------------------------------
#include "pch.h"
#include "folder.h"


int
CoTaskLoadString(HINSTANCE hInstance, UINT idString, LPWSTR *ppwsz)
{
    int nResult = 0;

    *ppwsz = NULL;

    ULONG cchString = SizeofStringResource(hInstance, idString);
    if (cchString)
    {
        cchString++;    // for NULL
        *ppwsz = (LPWSTR)CoTaskMemAlloc(cchString * SIZEOF(WCHAR));
        if (*ppwsz)
            nResult = LoadStringW(hInstance, idString, *ppwsz, cchString);
    }

    return nResult;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IClassFactory::CreateInstance support                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
CCscVolumeCleaner::CreateInstance(REFIID riid, LPVOID *ppv)
{
    return Create(FALSE, riid, ppv);
}

HRESULT WINAPI
CCscVolumeCleaner::CreateInstance2(REFIID riid, LPVOID *ppv)
{
    return Create(TRUE, riid, ppv);
}

HRESULT WINAPI
CCscVolumeCleaner::Create(BOOL fPinned, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    CCscVolumeCleaner *pThis = new CCscVolumeCleaner(fPinned);
    if (pThis)
    {
        hr = pThis->QueryInterface(riid, ppv);
        pThis->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IUnknown implementation                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCscVolumeCleaner::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CCscVolumeCleaner, IEmptyVolumeCache),
        QITABENT(CCscVolumeCleaner, IEmptyVolumeCache2),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CCscVolumeCleaner::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CCscVolumeCleaner::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IEmptyVolumeCache implementation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CCscVolumeCleaner::Initialize(HKEY  /*hkRegKey*/,
                              LPCWSTR pcwszVolume,
                              LPWSTR *ppwszDisplayName,
                              LPWSTR *ppwszDescription,
                              LPDWORD pdwFlags)
{
    BOOL bSettingsMode;
    CSCSPACEUSAGEINFO sui = {0};

    USES_CONVERSION;

    TraceEnter(TRACE_SHELLEX, "IEmptyVolumeCache::Initialize");
    TraceAssert(pcwszVolume != NULL);
    TraceAssert(ppwszDisplayName != NULL);
    TraceAssert(ppwszDescription != NULL);
    TraceAssert(pdwFlags != NULL);
    TraceAssert(IsCSCEnabled());

    bSettingsMode = (BOOL)((*pdwFlags) & EVCF_SETTINGSMODE);

    *ppwszDisplayName = NULL;
    *ppwszDescription = NULL;
    *pdwFlags = 0;

    // If this isn't the volume containing the CSC database, then we have
    // nothing to free. Note that we don't use the space usage data
    // returned here.
    GetCscSpaceUsageInfo(&sui);
    if (!bSettingsMode && !PathIsSameRoot(sui.szVolume, W2CT(pcwszVolume)))
        TraceLeaveResult(S_FALSE);

    m_PurgerSel.SetFlags(m_fPinned ? PURGE_FLAG_PINNED : (PURGE_FLAG_UNPINNED | PURGE_IGNORE_ACCESS));
    m_pPurger = new CCachePurger(m_PurgerSel,
                                 CachePurgerCallback,
                                 this);
    if (!m_pPurger)
        TraceLeaveResult(E_FAIL);

    // If we're freeing auto-cached files, we want to be enabled by default,
    // but not if we're freeing pinned files.
    *pdwFlags = 0;
    if (!m_fPinned)
        *pdwFlags = EVCF_ENABLEBYDEFAULT | EVCF_ENABLEBYDEFAULT_AUTO;

    // If policy allows, turn on the "Details" button which launches the viewer
    if (!CConfig::GetSingleton().NoCacheViewer())
        *pdwFlags |= EVCF_HASSETTINGS;

    // Load the display name string
    CoTaskLoadString(g_hInstance,
                     m_fPinned ? IDS_APPLICATION : IDS_DISKCLEAN_DISPLAY,
                     ppwszDisplayName);

    // Load the description string
    CoTaskLoadString(g_hInstance,
                     m_fPinned ? IDS_DISKCLEAN_PIN_DESCRIPTION : IDS_DISKCLEAN_DESCRIPTION,
                     ppwszDescription);

    TraceLeaveResult(S_OK);
}

STDMETHODIMP
CCscVolumeCleaner::GetSpaceUsed(DWORDLONG *pdwlSpaceUsed,
                                LPEMPTYVOLUMECACHECALLBACK picb)
{
    m_pDiskCleaner = picb;
    m_pPurger->Scan();
    if (m_pDiskCleaner)
        m_pDiskCleaner->ScanProgress(m_dwlSpaceToFree,
                                     EVCCBF_LASTNOTIFICATION,
                                     NULL);
    *pdwlSpaceUsed = m_dwlSpaceToFree;
    return S_OK;
}

STDMETHODIMP
CCscVolumeCleaner::Purge(DWORDLONG /*dwlSpaceToFree*/,
                         LPEMPTYVOLUMECACHECALLBACK picb)
{
    m_pDiskCleaner = picb;
    m_pPurger->Delete();
    if (m_pDiskCleaner)
        m_pDiskCleaner->PurgeProgress(m_dwlSpaceFreed,
                                      0,
                                      EVCCBF_LASTNOTIFICATION,
                                      NULL);
    return S_OK;
}

STDMETHODIMP
CCscVolumeCleaner::ShowProperties(HWND /*hwnd*/)
{
    // Launch the viewer
    COfflineFilesFolder::Open();
    return S_FALSE;
}

STDMETHODIMP
CCscVolumeCleaner::Deactivate(LPDWORD /*pdwFlags*/)
{
    // nothing to do here
    return S_OK;
}

// IEmptyVolumeCache2 method
STDMETHODIMP
CCscVolumeCleaner::InitializeEx(HKEY hkRegKey,
                                LPCWSTR pcwszVolume,
                                LPCWSTR pcwszKeyName,
                                LPWSTR *ppwszDisplayName,
                                LPWSTR *ppwszDescription,
                                LPWSTR *ppwszBtnText,
                                LPDWORD pdwFlags)
{
    HRESULT hr = Initialize(hkRegKey,
                            pcwszVolume,
                            ppwszDisplayName,
                            ppwszDescription,
                            pdwFlags);
    if (S_OK == hr)
        CoTaskLoadString(g_hInstance, IDS_DISKCLEAN_BTN_TEXT, ppwszBtnText);
    return hr;
}

BOOL
CCscVolumeCleaner::ScanCallback(CCachePurger *pPurger)
{
    BOOL bContinue = TRUE;

    // If the pinned state matches what we're looking for, add the
    // size to the total.
    if (pPurger->WillDeleteThisFile())
        m_dwlSpaceToFree += pPurger->FileBytes();

    if (m_pDiskCleaner)
        bContinue = SUCCEEDED(m_pDiskCleaner->ScanProgress(m_dwlSpaceToFree,
                                                           0,
                                                           NULL));
    return bContinue;
}

BOOL
CCscVolumeCleaner::DeleteCallback(CCachePurger *pPurger)
{
    BOOL bContinue = TRUE;

    // Don't let this go below zero
    m_dwlSpaceToFree -= min(pPurger->FileBytes(), m_dwlSpaceToFree);
    m_dwlSpaceFreed += pPurger->FileBytes();

    if (m_pDiskCleaner)
        bContinue = SUCCEEDED(m_pDiskCleaner->PurgeProgress(m_dwlSpaceFreed,
                                                            m_dwlSpaceToFree,
                                                            0,
                                                            NULL));
    return bContinue;
}

BOOL CALLBACK
CCscVolumeCleaner::CachePurgerCallback(CCachePurger *pPurger)
{
    PCSCVOLCLEANER pThis = (PCSCVOLCLEANER)pPurger->CallbackData();
    switch (pPurger->Phase())
    {
    case PURGE_PHASE_SCAN:
        return pThis->ScanCallback(pPurger);
    case PURGE_PHASE_DELETE:
        return pThis->DeleteCallback(pPurger);
    }
    return FALSE;
}
