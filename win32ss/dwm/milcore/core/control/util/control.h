// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    control.h
//
//---------------------------------------------------------------------------------

#define DEBUGCONTROL_VERSION 3

__if_not_exists(ARGB) {
struct ARGB;
}

//---------------------------------------------------------------------------------
// CPerformanceCounter
// 
// Synopsis: 
//    Helper class for collecting performance statistics.
//
// Remark:
//    Before the class can be used to collect statistics, the static method
//    CPerformanceCounter::Initialize must be called.
//---------------------------------------------------------------------------------

class CPerformanceCounter
{
public:
    static void CPerformanceCounter::Initialize(); 
    
    CPerformanceCounter(UINT minIntervalMilliseconds);

    void Inc() { m_counter++; }
    
    UINT GetCurrentRate();

private:

    UINT m_samplingIntervalInMilliseconds; // Always at least 1000ms.
    LARGE_INTEGER m_startTime;     // Start time for sampling interval.
    UINT m_counter;                // Current count.
    UINT m_currentRate;            // Current rate.
   
    static BOOL s_qpcSupported;           // Inidcates that QPC is supported.
    static LARGE_INTEGER s_qpcFrequency;  // QPC Frequency.
};

//---------------------------------------------------------------------------------
// CMediaControlFile
// 
// Synopsis: 
//    Structure of the Media Control File.
//---------------------------------------------------------------------------------

struct CMediaControlFile
{
    public:
        BOOL ShowDirtyRegionOverlay;
        BOOL ClearBackBufferBeforeRendering;
        BOOL DisableDirtyRegionSupport;
        BOOL EnableTranslucentRendering;
        DWORD FrameRate;
        DWORD DirtyRectAddRate;
        DWORD PercentElapsedTimeForComposition;
    
        DWORD TrianglesPerFrame;
        DWORD TrianglesPerFrameMax;
        DWORD TrianglesPerFrameCumulative;

        DWORD PixelsFilledPerFrame;
        DWORD PixelsFilledPerFrameMax;
        DWORD PixelsFilledPerFrameCumulative;

        DWORD TextureUpdatesPerFrame;
        DWORD TextureUpdatesPerFrameMax;
        DWORD TextureUpdatesPerFrameCumulative;
    
        DWORD VideoMemoryUsage;
        DWORD VideoMemoryUsageMin;
        DWORD VideoMemoryUsageMax;

        DWORD NumSoftwareRenderTargets;
        DWORD NumHardwareRenderTargets;

        // Provides a per-frame count of hw IRTs
        DWORD NumHardwareIntermediateRenderTargets;
        DWORD NumHardwareIntermediateRenderTargetsMax;

        // Provides a per-frame count of sw IRTs
        DWORD NumSoftwareIntermediateRenderTargets;
        DWORD NumSoftwareIntermediateRenderTargetsMax;
        
        BOOL AlphaEffectsDisabled;
        BOOL PrimitiveSoftwareFallbackDisabled;
        BOOL RecolorSoftwareRendering;
        BOOL FantScalerDisabled;
        BOOL Draw3DDisabled;
};

//---------------------------------------------------------------------------------
// CMediaControl
// 
// Synopsis: 
//    The compositor control provides the infrastructure to configure the
//    compositor from another process. Note that all the flags are initialized
//    to false.
//---------------------------------------------------------------------------------

class CMediaControl
{
private:
    struct MemoryMappedFile
    {
        DWORD _dwVersion;
        CMediaControlFile _data;
    };

private:
    CMediaControl();
    HRESULT Initialize(
        __in PCWSTR lpName);
    HRESULT InitializeAttach(
        __in PCWSTR lpName);
    void CMediaControl::UpdateMaxValuePair(
        __inout_ecount(1) DWORD* pdwMaxValue,
        __inout_ecount(1) DWORD* pdwCurrentValue);
public:
    static __checkReturn HRESULT Create(
        __in PCWSTR lpName,
        __deref_out_ecount(1) CMediaControl** ppMediaControl);
    
    virtual ~CMediaControl();

    __out_ecount(1) CMediaControlFile* GetDataPtr();

    void UpdatePerFrameCounters();
    
    static void TintARGBBitmap(
        __inout_bcount(cbStride*(uHeight-1)+uWidth*sizeof(ARGB)) ARGB * pbBitmap,
        UINT uWidth,
        UINT uHeight,
        UINT cbStride
        );
    
public:
    // Warning: Attach and CanAttach should not be called from with-in the compositor since
    //  they are currently not robust. See the comment to GetFileSizeEx in InitializeAttach.

    static __checkReturn HRESULT Attach(
        __in PCWSTR lpName,
        __deref_out_ecount(1) CMediaControl** ppMediaControl);

    static BOOL CanAttach(__in PCWSTR lpName);


private:
    HANDLE _hMemoryMappedFile;
    MemoryMappedFile* _pFile;
};



