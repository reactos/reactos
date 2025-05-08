// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//
//    Classes to cache system setting data and dependent data.
//
//  System settings data includes:
//  - amount of displays (monitors) attached to the computer;
//  - relative location of each display and other per-display data;
//  - system-wide and per-display registry settings to specify rendering modes;
//  - data obtained from SystemParametersInfo call.
//
//  System settings dependent data includes:
//  - IDirect3D9 object;
//  - redundant rendering data: gamma lookup tables, text rendering modes, etc.
//
//  Common feature of the data in question is that it is almost constant.
//  Changes happen when user enables/disables some monitors, or changes
//  their resolution or relative locations, or adjusts text rendering modes.
//  When it happens, we need to re-read settings and re-build dependent data.
//
//  Due to multithreaded model, this switch can not be done at once.
//  Some threads may be continue holding on old data while others are already
//  switched to new. That is the reason why we don't use static instances
//  to keep setting dependent data.
//
//  Instead, we gather all settings dependent data in the instance of special
//  class, called CDisplaySet. Typically we use only one instance, but during
//  transition there can be two or more.
//
//  The important fact is that every CDisplaySet instance is essentially
//  constant. It is created, filled with data just once and then never
//  changed. This means that we need not to worry about interthread
//  precautions when using this class.
//
//  Life time of CDisplaySet is controlled by regular AddRef/Release, so when
//  every object will switch to new settings, old CDisplaySet will go away.
//
//
//
//  Classes:    CDisplayManager
//              CDisplaySet
//              CDisplay
//
//  Structs:    DisplaySettings
//
//------------------------------------------------------------------------

#pragma once

class CDisplaySet;
class CDisplay;
struct DisplaySettings;
class CGlyphRunResource;

//+----------------------------------------------------------------------------
//
//  Class:
//      DisplayId
//
//  Synopsis:
//      Display identifier which matches a specific CDisplay or is none.
//
//-----------------------------------------------------------------------------

class DisplayId
{
public:
    bool operator==(DisplayId const &cmp) const { return value == cmp.value; }
    bool operator!=(DisplayId const &cmp) const { return value != cmp.value; }

    DisplayId() {}

    static const DisplayId None;

    bool IsNone() const { return *this == None; }

private:

    friend class CDisplay;  // To cast display index to DisplayId
    DisplayId(UINT _value) : value(_value) {}

    friend class CDisplaySet;   // To pull display index from DisplayId
    operator UINT() const;

    UINT value;

};


//+------------------------------------------------------------------------
//
// Struct:  DisplaySettings
//
// Synopsis:
//      Encapsulates rendering modes, mostly for text rendering.
//      There can be three kinds of display settings:
//      1. hard coded - static const values defined in the code;
//      2. default settings - correspond to values that are currently 
//         established in system.
//      3. display settings - correspond to values that are currently
//         established in system for particular monitor.
//      1st is never changed, 2nd and 3rd can change when Windows is
//      being adjusted; however this event happens extremely seldom.
//-------------------------------------------------------------------------

typedef enum 
{
    BiLevel,
    Grayscale,
    ClearType
} RenderingMode;



struct GlyphBlendingParameters
{
    float ContrastEnhanceFactor;   // ContrastEnhanceFactor >= 0.0f
    float BlueSubpixelOffset;
    UINT GammaIndex;        
};

struct DisplaySettings
{    
    DisplaySettings() 
    { 
        pIDWriteRenderingParams = NULL; 
    }
    ~DisplaySettings() { ReleaseInterface(pIDWriteRenderingParams); }

    // This varies per monitor, and may be different from pIDWriteRenderingParams->GetPixelGeometry()
    // See modification in CDisplaySet::ReadIndividualDisplaySettings
    DWRITE_PIXEL_GEOMETRY PixelStructure;    // PIXEL_STRUCTURE_FLAT/RGB/BGR

    // This is a system wide setting that is the same for every display
    RenderingMode DisplayRenderingMode;
    
    bool AllowGamma;

    bool IsEquivalentTo(
        __in_ecount(1) DisplaySettings const &ds
        ) const
    {
        return PixelStructure                                   == ds.PixelStructure 
            && pIDWriteRenderingParams != NULL
            && ds.pIDWriteRenderingParams != NULL
            && pIDWriteRenderingParams->GetGamma()              == ds.pIDWriteRenderingParams->GetGamma()
            && pIDWriteRenderingParams->GetEnhancedContrast()   == ds.pIDWriteRenderingParams->GetEnhancedContrast()
            && pIDWriteRenderingParams->GetClearTypeLevel()     == ds.pIDWriteRenderingParams->GetClearTypeLevel()
            && DisplayRenderingMode                             == ds.DisplayRenderingMode;
    }

    // Primary data
    IDWriteRenderingParams *pIDWriteRenderingParams;

    // Secondary data
    GlyphBlendingParameters DisplayGlyphParameters;
};

MtExtern(CDisplaySet);

//+------------------------------------------------------------------------
//
// Class:       CDisplaySet
//
// Synopsis:    Contains data about every display in the system,
//              holds on D3D object
//
//-------------------------------------------------------------------------
class CDisplaySet
{
    using DpiAwarenessContextValue = wpf::util::DpiAwarenessContextValue;
    using DpiAwarenessContext = wpf::util::DpiAwarenessContext;

private:
    friend class CDisplayManager;

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CDisplaySet));

    CDisplaySet(
        ULONG ulDisplayUniquenessLoader,
        ULONG ulExternalUpdateCount
        );
    
    ~CDisplaySet();
    HRESULT Init();
    HRESULT EnumerateDevices();
    HRESULT EnumerateMonitors();
    HRESULT ArrangeDXAdapters();
    HRESULT ReadDisplayModes();

    static BOOL CALLBACK MonitorEnumProc(
        __in HMONITOR hMonitor,
        __in HDC hdcMonitor,
        __in LPRECT lprcMonitor,
        __in LPARAM dwData
        );

    static HRESULT GetMonitorDescription(
        __in HMONITOR hMonitor,
        __out_ecount(1) MONITORINFOEX *pMonitorInfo
        );

    __out_ecount_opt(1) CDisplay* FindDisplayByName(__in_ecount(1) const MONITORINFOEX *pmi);
    int FindDisplayByHMonitor(__in HMONITOR hMonitor) const;

    static HRESULT ValidateDeviceName(
        __in_bcount(cbBuffer) LPCTSTR pstrDeviceName,
        __in size_t cbBuffer
        );

    void ComputeDisplayBounds();
    HRESULT ReadDefaultDisplaySettings();
    HRESULT ReadIndividualDisplaySettings();
    HRESULT ReadGraphicsAccelerationCaps();
    void ReadRequiredVideoDriverDate();

    bool IsEquivalentTo(
        __in_ecount(1) CDisplaySet const * pDisplaySet
        ) const;

    void UpdateUniqueness(
        __in_ecount(1) CDisplaySet const * pDisplaySet
        ) const;

    bool IsUpToDate() const;

    inline std::vector<DpiAwarenessContextValue> GetValidDpiAwarenessContextValues() const
    {
        if (m_defaultDpiAwarenessContextValue != DpiAwarenessContextValue::Invalid)
        {
            return DpiAwarenessContext::GetValidDpiAwarenessContextValues();
        }

        return { m_defaultDpiAwarenessContextValue };
    }

public:
    static HRESULT InspectLastError();

    void AddRef() const {InterlockedIncrement(&m_cRef);}
    void Release() const;

    bool DangerousHasDisplayStateChanged() const;
    bool IsNonLocalDisplayPresent() const { return m_fNonLocalDevicePresent; }

    HRESULT GetD3DObjectNoRef(
        __deref_out_ecount(1) IDirect3D9 **pID3D
        ) const;

    __out_ecount(1) IDirect3D9* D3DObject() const {return m_pID3D;}
    __out_ecount(1) IDirect3D9Ex* D3DExObject() const {return m_pID3DEx;}

    // Get HRESULT to detect why D3DObject() returned NULL.
    HRESULT GetD3DInitializationError() const {return m_hrD3DInitialization;}

    UINT GetNumD3DRecognizedAdapters() const { return m_uD3DAdapterCount; }

    __outro_ecount(1) CMILSurfaceRect const &GetBounds() const;

    UINT GetDisplayCount() const {return m_rgpDisplays.GetCount();}

    HRESULT GetDisplay(
        UINT uDisplayIndex,
        __deref_outro_ecount(1) CDisplay const * * ppDisplay
        ) const;

    HRESULT GetDisplayIndexFromMonitor(
        __in HMONITOR hMonitor,
        __out_ecount(1) UINT &uDisplayIndex
        ) const;

    HRESULT GetDisplayIndexFromDisplayId(
        DisplayId displayId,
        __out_ecount(1) UINT &uDisplayIndex
        ) const;

    __outro_ecount(1) CDisplay const * Display(UINT uDisplayIndex) const;

    __outro_ecount(1) DisplaySettings const* GetDisplaySettings(UINT uDisplayIndex) const;

    __outro_ecount(1) DisplaySettings const* GetDefaultDisplaySettings() const {return &m_defaultDisplaySettings;}

    void GetGraphicsAccelerationCaps(
        bool fReturnCommonMinimum,
        __out_ecount_opt(1) ULONG *pulDisplayUniqueness,
        __out_ecount(1) MilGraphicsAccelerationCaps *pGraphicsAccelerationCaps
        ) const;

    UINT64 GetRequiredVideoDriverDate() const {return m_u64RequiredVideoDriverDate;}

    ULONG GetUniqueness() const {return m_ulDisplayUniquenessLoader;}

    static HRESULT CompileSettings(__in_ecount(1) IDWriteRenderingParams *pParams, 
                                   DWRITE_PIXEL_GEOMETRY pixelGeometry, 
                                   __in_ecount_opt(1) IDWriteGlyphRunAnalysis *pAnalysis,
                                   __out_ecount(1) GlyphBlendingParameters *pGBP);
    
    HRESULT GetGammaTable(UINT gammaIndex, 
                          __deref_out_ecount(1) GammaTable const** ppTable
                          ) const;
    
    HRESULT GetEnhancedContrastTable(float k, 
                                     __deref_out_ecount(1) EnhancedContrastTable **ppTable
                                     ) const;

public:
    HRESULT EnsureSwRastIsRegistered() const;

    static MilGraphicsAccelerationCaps GetNoHardwareAccelerationCaps();

private:
    HRESULT GetDWriteFactoryNoRef(IDWriteFactory **ppIDWriteFactory);

    // place it first to ensure alignment
    UINT64 m_u64RequiredVideoDriverDate;

    mutable volatile LONG m_cRef;

    IDirect3D9* m_pID3D;
    IDirect3D9Ex *m_pID3DEx;

    HRESULT m_hrD3DInitialization;
    mutable HRESULT m_hrSwRastRegistered;

    UINT m_uD3DAdapterCount;

    //
    // These two values can be corrected when state change has been reported
    // but we did not find real state changes affecting current display set.
    //
    mutable ULONG m_ulDisplayUniquenessLoader;
    mutable ULONG m_ulDisplayUniquenessEx;


    bool m_fNonLocalDevicePresent;

    std::map<DpiAwarenessContextValue, CMILSurfaceRect> m_rcDisplayBounds;

    // Array of information about each display.
    // Includes both physical and mirroring devices.
    DynArrayIA<CDisplay*, 4> m_rgpDisplays;

    static const MilGraphicsAccelerationCaps sc_NoHardwareAccelerationCaps;

    mutable MilGraphicsAccelerationCaps m_commonMinCaps;
    mutable bool m_fCachedCommonMinCaps;

    DisplaySettings m_defaultDisplaySettings;

    mutable GammaTable* m_prgGammaTables[MAX_GAMMA_INDEX+1];

    mutable DynArray<EnhancedContrastTable *> m_pEnhancedContrastTables;

    IDWriteFactory *m_pIDWriteFactory;

    /// <remarks>
    /// If this value is <see cref="DpiAwarenessContextValue::Invalid"/>, then
    /// this is an older OS platform (pre Win10 v1607) that does not
    /// support querying thread DPI_AWARENESS_CONTEXT value
    /// </remarks>
    DpiAwarenessContextValue m_defaultDpiAwarenessContextValue;
};

MtExtern(CDisplay);

//+------------------------------------------------------------------------
//
// Class:       CDisplay
//
// Synopsis:    Contains the monitor handle, bounding
//              rectangle of a display
//              and other per-display info.
//              Instances of this class exist as parts of CDisplaySet
//
//-------------------------------------------------------------------------
class CDisplay
{
    using DpiAwarenessContextValue = wpf::util::DpiAwarenessContextValue;
    using DpiAwarenessContext = wpf::util::DpiAwarenessContext;

private:
    friend class CDisplaySet;

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CDisplay));

    CDisplay(
        __in_ecount(1) const CDisplaySet * pDisplaySet,
        __in UINT uDisplayIndex,
        __in_ecount(1) const DISPLAY_DEVICE *pdd
        );
    //~CDisplay(); not required

    HRESULT SetMonitorInfo(
        __in HMONITOR hMonitor,
        __in_ecount(1) LPCRECT prcMonitor
        );

    HRESULT ReadMode(
        __in_ecount_opt(1) IDirect3D9 *pID3D,
        __in_ecount_opt(1) IDirect3D9Ex *pID3DEx
        );

    void ReadGraphicsAccelerationCaps(
        __in_ecount(1) IDirect3D9 *pID3D
        );

    bool IsEquivalentTo(
        __in_ecount(1) CDisplay const * pDisplay
        ) const;

    bool CheckForRecentDriver(__in PCTSTR pstrDriver) const;

    void CheckBadDeviceDrivers();

    inline std::vector<DpiAwarenessContextValue> GetValidDpiAwarenessContextValues() const
    {
        if (m_defaultDpiAwarenessContextValue != DpiAwarenessContextValue::Invalid)
        {
            return DpiAwarenessContext::GetValidDpiAwarenessContextValues();
        }

        return { m_defaultDpiAwarenessContextValue };
    }

public:
    void AddRef() const {m_pDisplaySet->AddRef();}
    void Release() const {m_pDisplaySet->Release();}
    bool IsRecentDriver() const { return m_fIsRecentDriver; }

    bool IsMirrorDevice() const {return (m_dwStateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0;}

    __outro PCTSTR DeviceName() const { return m_szDeviceName; }

    __outro_ecount(1) CDisplaySet const * DisplaySet() const {return m_pDisplaySet;}
    UINT GetDisplayIndex() const {return m_uDisplayIndex;}
    DisplayId GetDisplayId() const { return GetDisplayIndex(); }
    HRESULT GetGraphicsAccelerationCaps(
        __out_ecount(1) MilGraphicsAccelerationCaps *pGraphicsAccelerationCaps
        ) const;
    TierType GetTier() const { return m_Caps.TierValue; }
    UINT GetMemorySize() const { return m_uMemorySize; }
    HRESULT GetMode(
        __out_xcount_part(sizeof(*pDisplayMode), pDisplayMode->Size) D3DDISPLAYMODEEX *pDisplayMode,
        __out_ecount_opt(1) D3DDISPLAYROTATION *pDisplayRotation
        ) const;
    D3DFORMAT GetFormat() const {return m_DisplayMode.Format;}
    UINT GetBitsPerPixel() const {return m_Caps.BitsPerPixel;}
    UINT GetRefreshRate() const {return m_DisplayMode.RefreshRate;}
    __outro_ecount(1) CMILSurfaceRect const &GetDisplayRect() const;
    __outro_ecount(1) DisplaySettings const * GetDisplaySettings() const {return &m_settings;}
    
    __out_ecount(1) IDirect3D9* D3DObject() const
    {
        return m_pDisplaySet->D3DObject();
    }

    __out_ecount_opt(1) IDirect3D9Ex* D3DExObject() const
    {
        return m_pDisplaySet->D3DExObject();
    }

    LUID GetLUID() const { return m_luidD3DAdapter;}
    __out HMONITOR GetHMONITOR() const { return m_hMonitor; }

    bool IsDeviceDriverBad() const { return m_fIsBadDriver; }
    DWORD GetVendorId() const { return m_uGraphicsCardVendorId; }
    DWORD GetDeviceId() const { return m_uGraphicsCardDeviceId; }

private:
    
    friend class CDisplaySet;
    CDisplaySet const *                                 m_pDisplaySet;
    UINT                                                m_uDisplayIndex;
    LUID                                                m_luidD3DAdapter;
    HMONITOR                                            m_hMonitor;
    std::map<DpiAwarenessContextValue, CMILSurfaceRect> m_rcBounds;

    TCHAR               m_szDeviceName[32];
    DWORD               m_dwStateFlags;

    DisplaySettings     m_settings;

    UINT                m_uMemorySize;

    bool                m_fIsRecentDriver;
    bool                m_fIsBadDriver;
    __nullterminated TCHAR m_szInstalledDisplayDrivers[MAX_PATH];

    DWORD               m_uGraphicsCardVendorId;
    DWORD               m_uGraphicsCardDeviceId;

    D3DDISPLAYMODEEX    m_DisplayMode;
    D3DDISPLAYROTATION  m_DisplayRotation;

    MilGraphicsAccelerationCaps m_Caps;

    /// <remarks>
    /// If this value is <see cref="DpiAwarenessContextValue::Invalid"/>, then
    /// this is an older OS platform (pre Win10 v1607) that does not
    /// support querying thread DPI_AWARENESS_CONTEXT value
    /// </remarks>
    DpiAwarenessContextValue m_defaultDpiAwarenessContextValue;
};

//+------------------------------------------------------------------------
//
// Class:       CDisplayManager
//
// Synopsis:
//      Creates and holds on CDisplaySet.
//
//-------------------------------------------------------------------------
class CDisplayManager
{
public:
    CDisplayManager();
    HRESULT Init();

    HRESULT DangerousGetLatestDisplaySet(
        __deref_out_ecount(1) CDisplaySet const * * const ppDisplaySet
        );

    bool HasCurrentDisplaySet() const
    {
        return m_pCurrentDisplaySet != NULL;
    }

    void GetCurrentDisplaySet(
        __deref_out_ecount(1) CDisplaySet const * * const ppDisplaySet
        );

    //
    // Increment D3D usage count
    //

    void IncrementUseRef();

    //
    // Decrement D3D usage count
    //
    // Note: Must be paired with IncrementUseRef, but only
    //       called after all D3D interfaces, derived from and including ID3D,
    //       have been released.
    //

    void DecrementUseRef();

    void ScheduleUpdate();

    HRESULT GenerateUniqueAdapterLUIDForXPDM(
        __out_ecount(1) LUID *pLUID
        );

private:
    HRESULT CreateNewDisplaySet(
        ULONG ulDisplayUniquenessLoader,
        ULONG ulExternalUpdateCount, 
        __deref_out_ecount(1) CDisplaySet const * * ppDisplaySet
        );

    void CheckInUse();

private:
    friend class CDisplaySet;

    CCriticalSection m_csManagement;
    CDisplaySet const * m_pCurrentDisplaySet;
    volatile LONG m_cD3DUsage;
    volatile ULONG m_ulExternalUpdateCount;

};

// unique instance of CDisplayManager
extern CDisplayManager g_DisplayManager;


//+------------------------------------------------------------------------
//
//  Class:  CDisplayRegKey
//
//  Synopsis:  Helper to read Avalon.Graphics or display device registry values
//
//-------------------------------------------------------------------------

class CDisplayRegKey
{
public:
    CDisplayRegKey(
        __in HKEY hKeyRoot, 
        __in PCTSTR pszDeviceName
        );

    CDisplayRegKey(
        __in PCTSTR pszDeviceKey
        );

    ~CDisplayRegKey();

    bool IsValid() const { return m_fOpened; }

    bool ReadDWORD(
        __in PCTSTR pName,
        __out_ecount(1) DWORD *pValue  // unchanged on failure
        );

    bool ReadUINT(
        __in PCTSTR pName,
        __out_ecount(1) UINT *pValue  // unchanged on failure
        )
    {
        return ReadDWORD(pName, reinterpret_cast<DWORD *>(pValue));
    }

    bool ReadString(
        __in PCTSTR pName,
        DWORD cb,
        __out_bcount(cb) PTSTR str
        );

private:
    bool m_fOpened;
    HKEY m_hKey;
};


