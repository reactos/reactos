// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      See comments in *.h file
//
//------------------------------------------------------------------------

#include "precomp.hpp"

#include "strsafe.h"

#include "hw\D3DDeviceManager.h"
#include "hw\HwGraphicsCards.h"

using DpiAwarenessContext = wpf::util::DpiAwarenessContext;
using DpiAwarenssContextValue = wpf::util::DpiAwarenessContextValue;
using DpiUtil = wpf::util::DpiUtil;
template<typename TDpiAwarenessScopeSource> using DpiAwarenessScope = wpf::util::DpiAwarenessScope<TDpiAwarenessScopeSource>;

// ALWAYS_READ_DISPLAY_ORIENTATION will query a displays rotation information
// when not provided by D3D.  There is a slight startup perf hit to this
// functionality.  There isn't a current need for it so leave it off currently.
#define ALWAYS_READ_DISPLAY_ORIENTATION 0


// Defined in Hw\D3DDeviceManager.cpp
extern HRESULT
CheckDisplayFormat(
    __in_ecount(1) IDirect3D9 *pID3D,
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT DisplayFormat,
    MilRTInitialization::Flags RTInitFlags
    );

MtDefine(CDisplaySet, Mem, "CDisplaySet");
MtDefine(CDisplay, CDisplaySet, "CDisplay");

//+------------------------------------------------------------------------
//
//  Function:   IsMultiAdapterCodeEnabled
//
//  Synopsis:
//      Checks to see if our multi-adapter code should be enabled for this
//      application.  The default is for the multi-adapter code to be enabled,
//      which means that everything is rendered to the correct graphics
//      adapter, resulting in the highest possible rendering performance.
//      However, there is an unknown bug somewhere in the multi-adapter code
//      which can result in black stripes appearing on WPF windows when in
//      multi-adapter scenarios.  This code
//      was added to allow the multi-adapter code to be disabled if there
//      is a regkey set in
//          HKCU\Software\Microsoft\Avalon.Graphics\MultiAdapterSupport
//      or
//          HKLM\Software\Microsoft\Avalon.Graphics\MultiAdapterSupport
//      with a DWORD name of the full path to the .exe which wants to disable
//      this code and a value of 0.
//
//      The regkey is checked in the first call to this function, and the
//      value is cached for all future calls in this process.
//
//-------------------------------------------------------------------------
bool IsMultiAdapterCodeEnabled()
{
    enum MultiAdapterCodeState
    {
        MACS_Uninitialized,
        MACS_Enabled,
        MACS_Disabled,
    };
    static MultiAdapterCodeState s_macs = MACS_Uninitialized;
    if (s_macs == MACS_Uninitialized)
    {
        TCHAR szFullPath[MAX_PATH];

        DWORD cFullPath = GetModuleFileName(NULL, szFullPath, sizeof(szFullPath)/sizeof(szFullPath[0]));
        if (cFullPath > 0)
        {
            // First check for a value in HKCU (which should override any HKLM setting).
            HKEY hKey = NULL;
            LONG r = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                _T("Software\\Microsoft\\Avalon.Graphics\\MultiAdapterSupport"),
                0,
                KEY_QUERY_VALUE,
                &hKey
                );

            if (r == ERROR_SUCCESS)
            {
                DWORD dwValue = 1; // default to enabled
                if (RegGetDword(hKey, szFullPath, &dwValue))
                {
                    s_macs = (dwValue == 0) ? MACS_Disabled : MACS_Enabled;
                }

                RegCloseKey(hKey);
            }

            // If we didn't set a value from HKCU, try HKLM.
            if (s_macs == MACS_Uninitialized)
            {
                r = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    _T("Software\\Microsoft\\Avalon.Graphics\\MultiAdapterSupport"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey
                    );

                if (r == ERROR_SUCCESS)
                {
                    DWORD dwValue = 1; // default to enabled
                    if (RegGetDword(hKey, szFullPath, &dwValue))
                    {
                        s_macs = (dwValue == 0) ? MACS_Disabled : MACS_Enabled;
                    }

                    RegCloseKey(hKey);
                }
            }
        }

        // If we still haven't set a value, then either we failed to get the full path
        // or no value was set in the registry.  Default to enabled.
        if (s_macs == MACS_Uninitialized)
        {
            s_macs = MACS_Enabled;
        }
    }
    return (s_macs == MACS_Enabled);
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplayRegKey::CDisplayRegKey
//
//  Synopsis:   Constructor.
//              Open registry key <hKeyRoot>\Software\Microsoft\Avalon.Graphics\<pszDeviceName>.
//              hKeyRoot can be either HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE.
//
//-------------------------------------------------------------------------
CDisplayRegKey::CDisplayRegKey(
    __in HKEY hKeyRoot,
    __in PCTSTR pszDeviceName
    )
{
    HKEY hKeyAvalonGraphics = NULL;
    m_fOpened = false;

    LONG r = RegOpenKeyEx(
        hKeyRoot,
        _T("Software\\Microsoft\\Avalon.Graphics"),
        0,
        KEY_QUERY_VALUE,
        &hKeyAvalonGraphics
        );

    if (r == ERROR_SUCCESS)
    {
        // skip slashes in device name
        const TCHAR *pszCleanName = pszDeviceName;
        while (*pszDeviceName)
        {
            if (*pszDeviceName++ == _T('\\'))
                pszCleanName = pszDeviceName;
        }

        r = RegOpenKeyEx(
            hKeyAvalonGraphics,
            pszCleanName,
            0,
            KEY_QUERY_VALUE,
            &m_hKey
            );
        RegCloseKey(hKeyAvalonGraphics);

        if (r == ERROR_SUCCESS)
        {
            m_fOpened = true;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplayRegKey::CDisplayRegKey
//
//  Synopsis:   Constructor.
//              Open registry key <HKLM>\<pszDeviceKey>.
//
//-------------------------------------------------------------------------
CDisplayRegKey::CDisplayRegKey(
    __in PCTSTR pszDeviceKey
    )
{
    m_fOpened = TW32(0, ERROR_SUCCESS == RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        pszDeviceKey,
        0,
        KEY_QUERY_VALUE,
        &m_hKey
        ));
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplayRegKey::~CDisplayRegKey
//
//  Synopsis:   Destructor.
//              Close registry key if was opened.
//
//-------------------------------------------------------------------------
CDisplayRegKey::~CDisplayRegKey()
{
    if (m_fOpened)
    {
        RegCloseKey(m_hKey);
        m_fOpened = false;
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CDisplayRegKey::ReadDWORD
//
//  Synopsis:  Read DWORD value
//
//-------------------------------------------------------------------------
bool
CDisplayRegKey::ReadDWORD(
    __in PCTSTR pName,
    __out_ecount(1) DWORD *pValue
    )
{
    bool fSuccess = false;

    if (m_fOpened)
    {
        DWORD dwValue = 0;
        DWORD dwDataSize = sizeof(dwValue);

        LONG r = RegQueryValueEx(
            m_hKey,
            pName,
            NULL,
            NULL,
            (LPBYTE)&dwValue,
            &dwDataSize
            );

        if (   r == ERROR_SUCCESS
            && dwDataSize == sizeof(dwValue))
        {
            *pValue = dwValue;
            fSuccess = true;
        }
    }

    return fSuccess;
}


//+------------------------------------------------------------------------
//
//  Function:  CDisplayRegKey::ReadString
//
//  Synopsis:  Read string value
//
//-------------------------------------------------------------------------
bool
CDisplayRegKey::ReadString(
    __in PCTSTR pName,
    DWORD cb,
    __out_bcount(cb) PTSTR pstr
    )
{
    bool fSuccess = false;
    DWORD dwType;

    if (m_fOpened)
    {
        LONG r = RegQueryValueEx(
            m_hKey,
            pName,
            NULL,
            &dwType,
            (LPBYTE)pstr,
            &cb
            );

        fSuccess = (r == ERROR_SUCCESS) &&
                   (dwType == REG_SZ || dwType == REG_MULTI_SZ);
    }

    if (fSuccess)
    {

        // fSuccess indicates that pstr[0] is non-empty.
        #pragma prefast(push)
        #pragma prefast (disable : 37001 37002 37003)

        // RegQueryValueEx doesn't always null-terminate.
        pstr[cb/sizeof(pstr[0])-1] = 0;

        #pragma prefast(push)

    }

    return fSuccess;
}



//==========================================================DisplayId

const DisplayId DisplayId::None(UINT_MAX);

DisplayId::operator UINT() const
{
    // Today a display id is the same as a display index, assuming the display
    // id is not Unknown.
    return value;
}


//==========================================================CDisplayManager


// unique instance of CDisplayManager
CDisplayManager g_DisplayManager;

//+------------------------------------------------------------------------
//
//  Function:  CDisplayManager::CDisplayManager
//
//  Synopsis:  Constructor
//
//-------------------------------------------------------------------------
CDisplayManager::CDisplayManager()
{
    m_pCurrentDisplaySet = NULL;
    m_cD3DUsage = 0;
    m_ulExternalUpdateCount = 0;
}

//+------------------------------------------------------------------------
//
//  Function:  CDisplayManager::Init
//
//  Synopsis:  Complete initialization that couldn't be done in constructor.
//
//-------------------------------------------------------------------------
HRESULT
CDisplayManager::Init()
{
    HRESULT hr = S_OK;

    Assert(!m_csManagement.IsValid());
    Assert(m_pCurrentDisplaySet == NULL);

    hr = m_csManagement.Init();

    Assert(m_csManagement.IsValid() || FAILED(hr));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDisplayManager::DangerousGetLatestDisplaySet
//
//  Synopsis:
//      Creates new CDisplaySet or returns existing one.
//      Caller is responsible for releasing obtained display set.
//
//      Note that this function is DANGEROUS to use inside the rendering
//      stack since that layer assumes the DisplaySet will not change
//      throughout the course of processing a single frame.
//
//      Returned display set may contain either valid D3D object
//      or NULL.
//
//      Many threads can share CDisplaySet without any
//      thread precautions because all the information
//      inside CDisplaySet is constant. It is filled
//      once on creation and then never changes.
//
//-------------------------------------------------------------------------
HRESULT
CDisplayManager::DangerousGetLatestDisplaySet(
    __deref_out_ecount(1) CDisplaySet const * * const ppDisplaySet
    )
{
    HRESULT hr = S_OK;

    CDisplaySet const * pDisplaySet = NULL;
    CDisplaySet const * pObsoleteDisplaySet = NULL;

    // try to use existing display set

    {
        //
        // Copy volatile m_pCurrentDisplaySet to local variable
        // and AddRef it so that it would not be destroyed by
        // other threads.
        //
        CGuard<CCriticalSection> oGuard(m_csManagement);

        SetInterface(pDisplaySet, m_pCurrentDisplaySet);
    }

    if (pDisplaySet != NULL)
    {
        if (pDisplaySet->IsUpToDate())
        {
            // everything okay, we're done
            goto Cleanup;
        }
    }

    ReleaseInterface(pDisplaySet);

    for (int attempt = 0; attempt < 5; attempt++)
    {
        // Create new display set.

        // Since it is possible that initializing D3D can trigger a mode change and
        // allow the NT window manager to send messages to various windows that may
        // be processed before execution returns to this code, we need to be
        // careful about holding a critical section.  It is possible for this
        // thread to trigger a mode change that will result in another thread of
        // this process trying to access D3D and therefore try to acquire our
        // critical section.  If a window serviced by this thread then waits on the
        // other thread there would be a deadlock.  So, following call should not
        // be protected.

        ULONG ulExternalUpdateCount = m_ulExternalUpdateCount;
        ULONG ulDisplayUniquenessLoader = CD3DModuleLoader::GetDisplayUniqueness();

        hr = CreateNewDisplaySet(ulDisplayUniquenessLoader, ulExternalUpdateCount, &pDisplaySet);

        if (hr == WGXERR_DISPLAYSTATEINVALID ||
            ulExternalUpdateCount != m_ulExternalUpdateCount ||
            ulDisplayUniquenessLoader != CD3DModuleLoader::GetDisplayUniqueness())
        {
            // System display settings have been changed while we were
            // creating display set. Try to redo the work.
            ReleaseInterface(pDisplaySet);
            continue;
        }

        // quit on errors other than WGXERR_DISPLAYSTATEINVALID
        IFC(hr);

        Assert(pDisplaySet);

        {
            //
            // Now we are about to update m_pCurrentDisplaySet.
            // Several threads can come to this point,
            // each having own new display set.
            //

            CGuard<CCriticalSection> oGuard(m_csManagement);

            bool fCurrentIsUpToDate = m_pCurrentDisplaySet && m_pCurrentDisplaySet->IsUpToDate();
            bool fNewIsUpToDate = pDisplaySet->IsUpToDate();

            if (!fCurrentIsUpToDate && !fNewIsUpToDate)
            {
                //
                // Worst luck. System display settings
                // have been changed after we've created display set.
                // Continue looping to redo the work.
                //
                ReleaseInterface(pDisplaySet);
                continue;
            }

            if (m_pCurrentDisplaySet != NULL && !fCurrentIsUpToDate)
            {
                //
                // Current display set is reported out-of-date,
                // but we don't trust it since it might be false
                // alarm. Compare new and old display sets and, if
                // they are the same, reuse current one.
                //

                Assert(fNewIsUpToDate);


                //
                // [2006/07/11 mikhaill, iourit]
                // The condition against m_pID3DEx below was added to
                // prohibit this optimization for XPDM.
                // See . In certain scenarios d3d9 fails
                // to report D3DERR_DEVICELOST. We need to force
                // re-creating devices even if no essential mode
                // changes happened.
                //

                if (m_pCurrentDisplaySet->m_pID3DEx != NULL &&
                    m_pCurrentDisplaySet->IsEquivalentTo(pDisplaySet))
                {
                    m_pCurrentDisplaySet->UpdateUniqueness(pDisplaySet);
                    fCurrentIsUpToDate = true;
                }
            }

            if (fCurrentIsUpToDate)
            {
                //
                // Current display set eventually considered good, use it
                //
                ReplaceInterface(pDisplaySet, m_pCurrentDisplaySet);
            }
            else
            {
                Assert(fNewIsUpToDate);

                //
                // Replace m_pCurrentDisplaySet with new one. This should be
                // done before releasing old display set, otherwise CheckInUse()
                // may be confused.
                //

                pObsoleteDisplaySet = m_pCurrentDisplaySet;     // transfer reference
                m_pCurrentDisplaySet = NULL;

                SetInterface(m_pCurrentDisplaySet, pDisplaySet);
            }

            goto Cleanup;
        }
    }

    // too many failures or not allowed to get a new one
    IFC(WGXERR_DISPLAYSTATEINVALID);

Cleanup:

    Assert(SUCCEEDED(hr) || pDisplaySet == NULL);

    *ppDisplaySet = pDisplaySet; // Transfer reference

    if (pObsoleteDisplaySet)
    {
        //
        // We've just replaced current display set.
        // Notify device manager about this.
        // We need to do this after leaving critical section.
        //
        CD3DDeviceManager::NotifyDisplayChange(pObsoleteDisplaySet, pDisplaySet);
        ReleaseInterfaceNoNULL(pObsoleteDisplaySet);
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplayManager::GetCurrentDisplaySet
//
//  Synopsis:
//      Returns the DisplaySet for the current frame.  This method DOES NOT
//      update the DisplaySet if it is out-of-date; see GetLatestDisplaySet.
//      This method is safe to call from within the rendering stack since it
//      should always return the same DisplaySet in processing a single frame.
//
//-----------------------------------------------------------------------------
void
CDisplayManager::GetCurrentDisplaySet(
    __deref_out_ecount(1) CDisplaySet const * * const ppDisplaySet
    )
{
    // This method should only have been called after we've updating the
    // DisplaySet for the current frame.  If we pass out a NULL ptr the
    // caller will AV so break here.
    if (m_pCurrentDisplaySet == NULL)
    {
        DebugBreak();
    }

    SetInterface(*ppDisplaySet, m_pCurrentDisplaySet);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplayManager::IncrementUseRef
//
//  Synopsis:
//      Increment D3D usage count for caller.  This guarentees the caller that
//      D3D module won't be unloaded until a call to DecrementUseRef is called.
//
//-----------------------------------------------------------------------------

void
CDisplayManager::IncrementUseRef(
    )
{
    InterlockedIncrement(&m_cD3DUsage);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplayManager::DecrementUseRef
//
//  Synopsis:
//      Release 1 D3D usage count for caller.  If the count goes to zero then
//      check if any currently help CDisplaySet may be released.
//
//-----------------------------------------------------------------------------

void
CDisplayManager::DecrementUseRef(
    )
{
    LONG cD3DUsage = InterlockedDecrement(&m_cD3DUsage);

    if (cD3DUsage == 0)
    {
        CheckInUse();
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplayManager::CreateNewDisplaySet
//
//  Synopsis:   Creates and initializes new instance of display set.
//              New created set has refCount = 1.
//-------------------------------------------------------------------------
HRESULT
CDisplayManager::CreateNewDisplaySet(
    ULONG ulDisplayUniquenessLoader,
    ULONG ulExternalUpdateCount,
    __deref_out_ecount(1) CDisplaySet const * * ppDisplaySet
    )
{
    Assert(ppDisplaySet);
    HRESULT hr = S_OK;

    CDisplaySet * pNewDisplaySet = new CDisplaySet(ulDisplayUniquenessLoader, ulExternalUpdateCount);
    IFCOOM(pNewDisplaySet);

    {
        //
        // CDisplaySet::Init makes calls to various system components
        // that use to break FPU state by switching it from
        // "float" to "double" precision.
        //
        FPUStateSandbox oGuard;
        IFC( pNewDisplaySet->Init() );
    }

    *ppDisplaySet = pNewDisplaySet;
    pNewDisplaySet = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewDisplaySet);
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:   CDisplayManager::ScheduleUpdate
//
//  Synopsis:   Register UI thread notification about settings change.
//              Eventually this will cause new display set creation
//              and switching all the consumers onto it.
//-------------------------------------------------------------------------
void
CDisplayManager::ScheduleUpdate()
{
    InterlockedIncrement(reinterpret_cast<volatile LONG *>(&m_ulExternalUpdateCount));
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplayManager::GenerateUniqueAdapterLUIDForXPDM
//
//  Synopsis:
//      Generates a unique adapter LUID which can be used to identify an
//      adpater as unique. This method can be used to generate a LUID when
//      ID3DDirect9Ex is unavailable.
//
//  Note:
//      When a display set is changed, the LUID returned here for the adapter
//      will be different the second time around. This is a key difference
//      between this method and D3D's GetAdapterLUID method.
//

HRESULT
CDisplayManager::GenerateUniqueAdapterLUIDForXPDM(
    __out_ecount(1) LUID *pLUID
    )
{
    HRESULT hr = S_OK;

    MIL_TW32_NOSLE(AllocateLocallyUniqueId(pLUID));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplayManager::CheckInUse
//
//  Synopsis:   Detect whether current display set is in use.
//              If not then release it.
//-------------------------------------------------------------------------
void
CDisplayManager::CheckInUse()
{
    CGuard<CCriticalSection> oGuard(g_DisplayManager.m_csManagement);

    if (m_pCurrentDisplaySet != NULL)
    {
        //
        // m_pCurrentDisplaySet is not NULL and this object owns a reference to
        // it.
        //
        // This test is reading volatile memory (even under crit sec).  If the
        // value is changing either any way (inc 1->2 or dec 2->1) either
        // before or after this check won't matter.  (It can't go below 1
        // because this object has a reference.)  We are guaranteed to get
        // another call to this routine after an increment is made and we
        // handle the case of already freeing m_pCurrentDisplaySet just fine,
        // even if this current display set is released during a CheckInUse
        // triggered by another CDisplaySet's count going to 1.
        //

        LONG cCurrentDisplayRef = m_pCurrentDisplaySet->m_cRef;

        Assert(cCurrentDisplayRef > 0);

        if (cCurrentDisplayRef == 1)
        {
            //
            // This test is reading volatile memory (even under crit sec).  If
            // the value is changing either any way (inc 0->1 or dec 1->0)
            // either before or after this check won't matter.  We are
            // guaranteed to get another call to this routine after an
            // increment is made and we handle the case of already freeing
            // m_pCurrentDisplaySet just fine, since m_pCurrentDisplaySet is
            // only modified under the critical section.
            //

            if (m_cD3DUsage == 0)
            {
                // It might happen that m_cD3DUsage virtually went to zero
                // but increased after the check above is done. In this case
                // D3D will be unloaded and most likely immediately reloaded,
                // which means performance losses. It might also happen that
                // m_cD3DUsage virtually went to zero but increased before the
                // check, so losses will be luckily avoided. Caller should
                // not rely upon this taking the reference on time and keeping
                // it while D3D unloading is not desirable.

                ReleaseInterface(m_pCurrentDisplaySet);
            }
        }
    }
}

//==============================================================CDisplaySet

const MilGraphicsAccelerationCaps CDisplaySet::sc_NoHardwareAccelerationCaps =
{
    /* TierValue = */               MIL_TIER(0,0),
    /* HasWDDMSupport = */          FALSE,
    /* PixelShaderVersion = */      D3DPS_VERSION(0,0),
    /* VertexShaderVersion = */     D3DVS_VERSION(0,0),
    /* MaxTextureWidth = */         0,
    /* MaxTextureHeight = */        0,
    /* WindowCompatibleMode = */    FALSE,
    /* BitsPerPixel = */            0,
    /* HasSSE2Support = */          FALSE
};

MilGraphicsAccelerationCaps
CDisplaySet::GetNoHardwareAccelerationCaps()
{
    MilGraphicsAccelerationCaps caps = sc_NoHardwareAccelerationCaps;
    caps.HasSSE2Support = CCPUInfo::HasSSE2ForEffects();
    return caps;
}

HRESULT
CDisplaySet::GetDWriteFactoryNoRef(IDWriteFactory **ppIDWriteFactory)
{
    HRESULT hr = S_OK;

    if (!m_pIDWriteFactory)
    {
        IUnknown *pIUnknown = NULL;

        // Initialize DWriteFactory object
        IFC(g_DWriteLoader.DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            &pIUnknown
            ));

        IFC(pIUnknown->QueryInterface(__uuidof(IDWriteFactory),
                                        reinterpret_cast<void**>(&(m_pIDWriteFactory))
                                        ));

        IFCOOM(m_pIDWriteFactory);
        pIUnknown->Release();
    }

    *ppIDWriteFactory = m_pIDWriteFactory;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::CDisplaySet
//
//  Synopsis:
//      Constructor
//
//  Arguments:
//      ulDisplayUniquenessLoader:
//          Display set uniqueness signature that comes from
//          CD3DModuleLoader::GetDisplayUniqueness().
//
//      ulExternalUpdateCount:
//          External display uniqueness signature that comes from
//          CDisplayManager::m_ulExternalUpdateCount.
//
//-------------------------------------------------------------------------
CDisplaySet::CDisplaySet(
    ULONG ulDisplayUniquenessLoader,
    ULONG ulExternalUpdateCount
    )
    : m_cRef(1)
    , m_pID3D(NULL)
    , m_pID3DEx(NULL)
    , m_hrD3DInitialization(WGXERR_NOTINITIALIZED)
    , m_hrSwRastRegistered(WGXERR_NOTINITIALIZED)
    , m_uD3DAdapterCount(0)
    , m_ulDisplayUniquenessLoader(ulDisplayUniquenessLoader)
    , m_ulDisplayUniquenessEx(ulExternalUpdateCount)
    , m_fNonLocalDevicePresent(false)
    , m_fCachedCommonMinCaps(false)
    , m_defaultDpiAwarenessContextValue(DpiAwarenessContext::GetThreadDpiAwarenessContextValue())
{
    // Default required video driver date: Nov 1, 2004
    m_u64RequiredVideoDriverDate = 127437408000000000;

    // Following comment came from DanWo's change 125352 on 2005/10/27:
    /*
      AFAIK the only way to convert from a date to a FILETIME is to
      call SystemTimeToFileTime, so that's what I did.  There's a
      command that goes the other way:

      E:\nt2\windows\mil\core\dll>w32tm /ntte 127437408000000000
      147497 00:00:00.0000000 - 10/31/2004 5:00:00 PM (local time)

      E:\nt2\windows\mil\core\dll>w32tm /tz
      Time zone: Current:TIME_ZONE_ID_DAYLIGHT Bias: 480min (UTC=LocalTime+Bias)
      [Standard Name:"Pacific Standard Time" Bias:0min Date:(M:10 D:5 DoW:0)]
      [Daylight Name:"Pacific Standard Time" Bias:-60min Date:(M:4 D:1 DoW:0)]
    */

    for (auto dpiContextValue : GetValidDpiAwarenessContextValues())
    {
        m_rcDisplayBounds[dpiContextValue].SetEmpty();
    }

    ZeroMemory(&m_prgGammaTables, sizeof(m_prgGammaTables));
    ZeroMemory(&m_commonMinCaps, sizeof(MilGraphicsAccelerationCaps));
    // m_defaultDisplaySettings remains uninitialized
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::~CDisplaySet
//
//  Synopsis:   Destructor
//
//-------------------------------------------------------------------------
CDisplaySet::~CDisplaySet()
{
    Assert(m_cRef == 0);

    for (int i = 0; i <= MAX_GAMMA_INDEX; i++)
    {
        GammaTable* pTable = m_prgGammaTables[i];
        if (pTable)
        {
            WPFFree(ProcessHeap, pTable);
        }
    }

    UINT count = m_pEnhancedContrastTables.GetCount();
    for (UINT k = 0; k < count; k++)
    {
        delete m_pEnhancedContrastTables[k];
    }

    for (int j = m_rgpDisplays.GetCount(); --j >= 0; )
    {
        delete m_rgpDisplays[j];
    }

    if (m_pID3D != NULL)
    {
        ReleaseInterfaceNoNULL(m_pID3D);
        ReleaseInterfaceNoNULL(m_pID3DEx);
        CD3DModuleLoader::ReleaseD3DLoadRef();
    }
    else
    {
        Assert(m_pID3DEx == NULL);
    }

    ReleaseInterface(m_pIDWriteFactory);
}

//+------------------------------------------------------------------------
//
//  Function:   Release()
//
//  Synopsis:   Decrease reference count and destroy if necessary.
//
//  Note:
//      Straightforward implementation can be of one line:
//      {
//          if (InterlockedDecrement(&m_cRef) == 0) delete this;
//      }
//      It could be acceptable, but right now we don't want to change
//      d3d.dll module life time. So we need to detect if CDisplayManager
//      is the only remaining holder, and if so, release this CDisplaySet.
//-------------------------------------------------------------------------
void
CDisplaySet::Release() const
{
    LONG cHolders = InterlockedDecrement(&m_cRef);

    if (cHolders == 0)
    {
        delete this;
    }
    else if (cHolders == 1)
    {
        g_DisplayManager.CheckInUse();
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::Init
//
//  Synopsis:
//      Read current system setting and initialize
//      the instance correspondingly.
//
//-------------------------------------------------------------------------
HRESULT
CDisplaySet::Init()
{
    HRESULT hr = S_OK;

    //
    // Get IDirect3D9 and IDirect3D9Ex objects and keep HRESULT.
    // Failure here is not fatal.
    //
    m_hrD3DInitialization = CD3DModuleLoader::CreateD3DObjects(&m_pID3D, &m_pID3DEx);

    Assert(FAILED(m_hrD3DInitialization) == (m_pID3D == NULL));

    ReadRequiredVideoDriverDate();

    IFC( EnumerateDevices() );

    IFC( EnumerateMonitors() );

    if (m_pID3D)
    {
        IFC( ArrangeDXAdapters() );
    }

    IFC(ReadDisplayModes());

    ComputeDisplayBounds();

    IFC(ReadDefaultDisplaySettings());

    IFC(ReadIndividualDisplaySettings());

    IFC(ReadGraphicsAccelerationCaps());

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::DangerousHasDisplayStateChanged
//
//  Synopsis:
//      Returns true if display device state has changed since set creation.
//      If the DisplaySet has changed, the caller needs to treat this as a
//      mode change since it means the current DisplaySet for the frame has been
//      overwritten.
//
//-----------------------------------------------------------------------------

bool
CDisplaySet::DangerousHasDisplayStateChanged() const
{
    //
    // Do a quick check
    //

    if (IsUpToDate())
    {
        return false;
    }
    else
    {
        CDisplaySet const *pDisplaySet = NULL;

        //
        // Delegate CD3DDeviceManager notification to g_DisplayManager.GetDisplaySet().
        //

        IGNORE_HR(g_DisplayManager.DangerousGetLatestDisplaySet(&pDisplaySet));

        // On HR failure we'll get NULL in pDisplaySet
        // and treat it as display change


        bool fChanged = pDisplaySet != this;

        ReleaseInterface(pDisplaySet);

        return fChanged;
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::IsUpToDate
//
//  Synopsis:
//      Check whether any display state change was declared since this
//      display set has been created or updated by UpdateUniqueness().
//
//  Returns:
//
//      false:
//          No changes declared, this display set is reliably up to date.
//
//      true:
//          Changes were declared, this display set is suspected obsolete.
//          It might happen however that changes were irrelevant and
//          display set can persist.
//
//-------------------------------------------------------------------------
bool
CDisplaySet::IsUpToDate() const
{
    bool fUpToDate = CD3DModuleLoader::GetDisplayUniqueness() == m_ulDisplayUniquenessLoader;

    bool fUpToDateEx = m_ulDisplayUniquenessEx == g_DisplayManager.m_ulExternalUpdateCount;

    return fUpToDate && fUpToDateEx;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::GetD3DObjectNoRef
//
//  Synopsis:
//      Return top level Direct3D interface if successfully acquired.
//
//-----------------------------------------------------------------------------

HRESULT
CDisplaySet::GetD3DObjectNoRef(
    __deref_out_ecount(1) IDirect3D9 **pID3D
    ) const
{
    Assert(m_pID3D || FAILED(m_hrD3DInitialization));
    *pID3D = m_pID3D;
    return m_hrD3DInitialization;
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::GetDisplay
//
//  Synopsis:   Get the pointer to CDisplay that corresponds
//              to given display index and AddRef it.
//
//-------------------------------------------------------------------------
HRESULT
CDisplaySet::GetDisplay(
    UINT uDisplayIndex,
    __deref_outro_ecount(1) CDisplay const * * ppDisplay
    ) const
{
    Assert(ppDisplay);

    HRESULT hr = S_OK;

    if (uDisplayIndex >= m_rgpDisplays.GetCount())
    {
        IFC(E_FAIL);
    }

    *ppDisplay = m_rgpDisplays[uDisplayIndex];
    (*ppDisplay)->AddRef();

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::GetDisplayIndexFromMonitor
//
//  Synopsis:
//      Get the set index of given HMONITOR.
//
//-----------------------------------------------------------------------------

HRESULT
CDisplaySet::GetDisplayIndexFromMonitor(
    __in HMONITOR hMonitor,
    __out_ecount(1) UINT &uDisplayIndex
    ) const
{
    HRESULT hr = S_OK;

    UINT i = m_rgpDisplays.GetCount();

    while (i-- > 0)
    {
        if (m_rgpDisplays[i]->m_hMonitor == hMonitor)
        {
            uDisplayIndex = i;
            goto Done;
        }
    }

    // Unknown HMONITOR
    MIL_THR(E_INVALIDARG);

Done:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::GetDisplayIndexFromDisplayId
//
//  Synopsis:
//      Converts a display id to a display index.
//

HRESULT
CDisplaySet::GetDisplayIndexFromDisplayId(
    DisplayId displayId,
    __out_ecount(1) UINT &uDisplayIndex
    ) const
{
    HRESULT hr = S_OK;

    // Today a display id is the same as a display index, assuming the display
    // id is not DisplayId::None (cast operator asserts !None.)
    uDisplayIndex = static_cast<UINT>(displayId);

    if (uDisplayIndex >= GetDisplayCount())
    {
        IFC(E_INVALIDARG);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::Display
//
//  Synopsis:   Get the pointer to CDisplay that corresponds
//              to given display index. Don't AddRef it.
//
//-------------------------------------------------------------------------
__outro_ecount(1) CDisplay const *
CDisplaySet::Display(UINT uDisplayIndex) const
{
    Assert(uDisplayIndex < m_rgpDisplays.GetCount());
    return m_rgpDisplays[uDisplayIndex];
}


//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::EnumerateDevices
//
//  Synopsis:   Figure out how many usable monitors are there
//              in system. Allocate CDisplay slot for each.
//
//              This first pass does not initialize CDisplay data
//              completely, we only store device name and state
//              flags.
//
//------------------------------------------------------------------------
HRESULT
CDisplaySet::EnumerateDevices()
{
    HRESULT hr = S_OK;
    CDisplay *pDisplay = NULL;

    for (UINT uDevice = 0; UNCONDITIONAL_EXPR(true); uDevice++)
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);

        if (!EnumDisplayDevices(NULL, uDevice, &dd, 0))
            break; // no more devices

        if (!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue; // skip unusable device

        if (dd.StateFlags & (DISPLAY_DEVICE_REMOTE | DISPLAY_DEVICE_MIRRORING_DRIVER))
        {
            m_fNonLocalDevicePresent = true;

            if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            {
                continue;  // mirror devices are considered hidden and should
                           // be excluded as display.  Remembering that a
                           // non-local device is present is sufficient for
                           // now.
            }
        }

        // Ensure that returned dd.DeviceName is good one.
        IFC ( ValidateDeviceName(dd.DeviceName, sizeof(dd.DeviceName) ) );

        // Create instance of CDisplay, store initial data in it.
        pDisplay = new CDisplay(this, m_rgpDisplays.GetCount(), &dd);
        IFCOOM(pDisplay);

        IFC( m_rgpDisplays.Add(pDisplay) );
        pDisplay = NULL;

        // DevDiv Servicing :
        // If this app has asked to disable the multi-adapter code, then just
        // break out of this loop after we've filled in the first device so we
        // only use this one device.
        if (!IsMultiAdapterCodeEnabled())
        {
            break;
        }
    }

Cleanup:
    delete pDisplay;
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Struct:     EnumDisplayMonitorsData
//
//  Synopsis:   Local pack of data to pass arguments to MonitorEnumProc.
//
//------------------------------------------------------------------------
struct EnumDisplayMonitorsData
{
    HRESULT hr;
    CDisplaySet *pDisplaySet;
};

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::EnumerateMonitors
//
//  Synopsis:   Second enumaration pass that follows EnumerateDevices.
//              We need it to obtain HMONITOR values for each display.
//
//              Unfortunately it is impossible to do the job in one pass,
//              neither EnumDisplayDevices nor EnumDisplayMonitors
//              provide complete information.
//
//------------------------------------------------------------------------
HRESULT
CDisplaySet::EnumerateMonitors()
{
    HRESULT hr = S_OK;

    for (auto dpiContextValue: GetValidDpiAwarenessContextValues())
    {
        DpiAwarenessContext dpiContext(dpiContextValue);
        
        // DpiScope will fail gracefully when dpiContextValue == Invalid
        DpiAwarenessScope<DpiAwarenessContext> dpiScope(dpiContext);

        hr = S_OK;

        EnumDisplayMonitorsData data;
        data.hr = S_OK;
        data.pDisplaySet = this;

        SetLastError(ERROR_SUCCESS);
        BOOL fSuccess = TW32(0, EnumDisplayMonitors(
            NULL,
            NULL,
            &MonitorEnumProc,
            reinterpret_cast<LPARAM>(&data)
        ));

        // Check if something went wrong inside MonitorEnumProc
        IFC(data.hr);

        // Check EnumDisplayMonitors return value
        if (!fSuccess)
        {
            IFC(InspectLastError());
        }
    }

    // Check that all displays are initialized now.
    for (int i = m_rgpDisplays.GetCount(); --i>=0;)
    {
        const CDisplay *pDisplay = m_rgpDisplays[i];
        Assert(pDisplay);
        if (!pDisplay->m_hMonitor)
        {
            // This guy has been missed on this pass.
            // We suppose it happened due to a mode change.
            IFC(WGXERR_DISPLAYSTATEINVALID);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CDisplaySet::MonitorEnumProc
//
//  Synopsis:   CALLBACK used by EnumDisplayMonitors.  Sets the monitor handle
//              & monitor rectangle for a single display.
//
//  Returns:   TRUE to continue the enumeration, FALSE to stop it
//
//------------------------------------------------------------------------
BOOL CALLBACK
CDisplaySet::MonitorEnumProc(
  __in HMONITOR hMonitor,  // handle to display monitor
  __in HDC hdcMonitor,     // handle to monitor DC
  __in LPRECT lprcMonitor, // monitor intersection rectangle
  __in LPARAM lpData       // this pointer
)
{
    Assert(lpData);
    Assert(hdcMonitor == NULL);
    HRESULT hr = S_OK;
    MONITORINFOEX mi;

    EnumDisplayMonitorsData *pData = reinterpret_cast<EnumDisplayMonitorsData*>(lpData);
    CDisplaySet *pDisplaySet = pData->pDisplaySet;

    // Get monitor info
    IFC( GetMonitorDescription(hMonitor, &mi) );

    {
        CDisplay *pDisplay = pDisplaySet->FindDisplayByName(&mi);
        if (pDisplay)
        {
            IFC( pDisplay->SetMonitorInfo(hMonitor, lprcMonitor) );
        }
    }

Cleanup:
    // Bad HRESULT should not be reported more than once
    Assert(SUCCEEDED(pData->hr));

    pData->hr = hr;

    // Stop enumerating if anything went wrong
    return SUCCEEDED(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CDisplaySet::GetMonitorDescription
//
//  Synopsis:   Call GetMonitorInfo, convert possible failure
//              code to HRESULT.
//
//------------------------------------------------------------------------
HRESULT
CDisplaySet::GetMonitorDescription(
    __in HMONITOR hMonitor,
    __out_ecount(1) MONITORINFOEX *pMonitorInfo
    )
{
    HRESULT hr = S_OK;

    Assert(pMonitorInfo);

    pMonitorInfo->cbSize = sizeof(MONITORINFOEX);

    SetLastError(ERROR_SUCCESS);
    if (0 == TW32(0,GetMonitorInfo(hMonitor, pMonitorInfo)))
    {
        IFC( InspectLastError() );
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CDisplaySet::FindDisplayByName
//
//  Synopsis:   Look thru existing instances of CDisplay, find
//              the one that has DeviceName that matches the one
//              in given MONITORINFOEX.
//
//------------------------------------------------------------------------
__out_ecount_opt(1) CDisplay*
CDisplaySet::FindDisplayByName(__in_ecount(1) const MONITORINFOEX *pmi)
{
    for (int i = m_rgpDisplays.GetCount(); --i>=0;)
    {
        CDisplay *pDisplay = m_rgpDisplays[i];
        Assert(pDisplay);

        Assert( sizeof(pmi->szDevice) == sizeof(pDisplay->m_szDeviceName) );
        // pDisplay->m_szDeviceName is known to be null terminated,
        // so following call will never cause buffer overrun.
        if (0 == _tcscmp(pmi->szDevice, pDisplay->m_szDeviceName))
        {
            return pDisplay;
        }
    }
    return NULL;
}

//+-----------------------------------------------------------------------
//
//  Member:    CDisplaySet::FindDisplayByHMonitor
//
//  Synopsis:   Look thru existing instances of CDisplay, find
//              the one that has given HMONOTIR value.
//              Return index in m_rgpDisplays or -1 if not found.
//
//------------------------------------------------------------------------
int
CDisplaySet::FindDisplayByHMonitor(__in HMONITOR hMonitor) const
{
    // DevDiv Servicing :
    // If this app has asked to disable the multi-adapter code, then just
    // always use the first display (if we have one).
    if (!IsMultiAdapterCodeEnabled() && m_rgpDisplays.GetCount() > 0 && m_rgpDisplays[0]->m_hMonitor != NULL)
        return 0;

    for (int i = m_rgpDisplays.GetCount(); --i>=0;)
    {
        const CDisplay *pDisplay = m_rgpDisplays[i];
        Assert(pDisplay);
        if (pDisplay->m_hMonitor == hMonitor)
            return i;
    }
    return -1;
}


//+-----------------------------------------------------------------------
//
//  Member:    CDisplaySet::InspectLastError
//
//  Synopsis:  Retrieves last error code value and converts it to HRESULT.
//
//------------------------------------------------------------------------
HRESULT
CDisplaySet::InspectLastError()
{
    HRESULT hr = S_OK;
    DWORD dwLastError = GetLastError();

    if (dwLastError == ERROR_INVALID_MONITOR_HANDLE ||
        dwLastError == ERROR_INVALID_PRINTER_NAME)
    {
        //
        // CreateDC returns ERROR_INVALID_PRINTER_NAME when it can't
        // find the display with specified name. This happens when
        // display state has been changed (after successful call to
        // GetMonitorInfo that provided pMonitorInfo->szDevice).
        //
        // ERROR_INVALID_MONITOR_HANDLE can be returned from GetMonitorInfo
        // because of similar reasons.
        //
        // In both cases we should return WGXERR_DISPLAYSTATEINVALID, thus
        // allowing CDisplayManager::GetDisplaySet to make another
        // attempt.
        //

        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    hr = HRESULT_FROM_WIN32(dwLastError);

    if (SUCCEEDED(hr))
    {
        // We do know that an error happened, so good hresult
        // should not be returned.
        // We suppose that Win32 routines may fail without
        // reporting an error due to display state changes.
        // So we return WGXERR_DISPLAYSTATEINVALID value to
        // tell caller that it'll be reasonable to retry
        // display enumeration.

        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     CDisplaySet::ValidateDeviceName
//
//  Synopsis:   Validates a device name is NON-NULL, is NULL-terminated,
//              and is not an empty string.
//
//  Returns:    S_OK if the device name is valid
//              WGXERR_INVALIDPARAMETER if the devicename is invalid
//
//------------------------------------------------------------------------
HRESULT
CDisplaySet::ValidateDeviceName(
    __in_bcount(cbBuffer) LPCTSTR pstrDeviceName,
        // Device name to validate
    __in size_t cbBuffer
        // Size of pstrDeviceName, including NULL-terminator, in bytes
    )
{
    HRESULT hr = S_OK;

    Assert(pstrDeviceName);

    size_t cbDeviceName = 0;

    // Get the String Length
    // This call also verifies the string is NON-NULL and NULL-TERMINATED
    if ( FAILED(StringCbLength(pstrDeviceName, cbBuffer, &cbDeviceName)))
    {
        IFC(WGXERR_INVALIDPARAMETER);
    }

    if (cbDeviceName < sizeof(TCHAR) )
    {
        IFC(WGXERR_INVALIDPARAMETER);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    CDisplaySet::ArrangeDXAdapters
//
//  Synopsis:  Associate each CDisplay with DirectX adapter index.
//
//------------------------------------------------------------------------
HRESULT
CDisplaySet::ArrangeDXAdapters()
{
    Assert(m_pID3D);
    HRESULT hr = S_OK;

    m_uD3DAdapterCount = m_pID3D->GetAdapterCount();

    // DevDiv Servicing :
    // If this app has asked to disable the multi-adapter code, then always
    // pretend like there is a single adapter, to match the single display
    // created in EnumerateDevices().
    if (!IsMultiAdapterCodeEnabled() && m_uD3DAdapterCount > 1)
    {
        m_uD3DAdapterCount = 1;
    }

    // If DX reported adapters that we don't have, assume it's due to mode change.
    if (m_uD3DAdapterCount > m_rgpDisplays.GetCount())
    {
        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    for (UINT i = 0; i < m_uD3DAdapterCount; i++)
    {
        HMONITOR hMonitor = m_pID3D->GetAdapterMonitor(i);

        // DevDiv Servicing :
        // If this app has asked to disable the multi-adapter code, then always
        // use the monitor for the one and only display we should have created.
        if (!IsMultiAdapterCodeEnabled() && m_rgpDisplays.GetCount() > 0)
        {
            hMonitor = m_rgpDisplays[0]->m_hMonitor;
        }

        if (hMonitor)
        {
            int j = FindDisplayByHMonitor(hMonitor);
            if (j < 0)
            {
                // DX has an adapter that we have not.
                // Assume this happened due to mode change.
                IFC(WGXERR_DISPLAYSTATEINVALID);
            }

            // We need same enumenation order as DX use.
            // Rearrange array if it is not so.
            if (i != static_cast<UINT>(j))
            {
                Assert (i < static_cast<UINT>(j));

                CDisplay *pDisplay_i = m_rgpDisplays[i];
                CDisplay *pDisplay_j = m_rgpDisplays[j];
                Assert(pDisplay_i && pDisplay_j);

                m_rgpDisplays[i] = pDisplay_j;
                m_rgpDisplays[j] = pDisplay_i;

                m_rgpDisplays[i]->m_uDisplayIndex = i;
                m_rgpDisplays[j]->m_uDisplayIndex = j;
            }

            if (m_pID3DEx)
            {
                IGNORE_HR(m_pID3DEx->GetAdapterLUID(
                    i,
                    &m_rgpDisplays[i]->m_luidD3DAdapter
                    ));
            }
            else
            {
                //
                // If D3DEx is not available, let us set the LUID to our own
                // uniqueness count. This allows us to use these LUIDs internally
                // to measure uniqueness of two adapters during the lifetime of a
                // CDisplaySet.
                //
                // The uniqueness generated here does not span display sets like
                // the LUID returned by the GetAdapterLUID function does.
                //

                IGNORE_HR(g_DisplayManager.GenerateUniqueAdapterLUIDForXPDM(
                    &m_rgpDisplays[i]->m_luidD3DAdapter
                    ));
            }
        }
    }

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::ReadDisplayModes
//
//  Synopsis:
//      Iterate through displays and have their display modes read.
//

HRESULT
CDisplaySet::ReadDisplayModes(
    )
{
    HRESULT hr = S_OK;

    UINT uDisplays = m_rgpDisplays.GetCount();

    for (UINT i = 0; i < uDisplays; i++)
    {
        IFC(m_rgpDisplays[i]->ReadMode(
            (i < m_uD3DAdapterCount) ? m_pID3D : NULL,
            (i < m_uD3DAdapterCount) ? m_pID3DEx : NULL
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CDisplaySet::ComputeDisplayBounds
//
//  Synopsis:  Union display bounds to find bounds of display set
//

void
CDisplaySet::ComputeDisplayBounds()
{
    for (auto dpiContextValue : GetValidDpiAwarenessContextValues())
    {
        Assert(m_rcDisplayBounds[dpiContextValue].IsEmpty());

        for (UINT i = 0; i < m_rgpDisplays.GetCount(); i++)
        {
            m_rcDisplayBounds[dpiContextValue].Union(m_rgpDisplays[i]->m_rcBounds[dpiContextValue]);
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::ReadDefaultDisplaySettings
//
//  Synopsis:   Read text rendering settings related to every display.
//
//              This routine fails. If anything is getting wrong,
//              hardcoded defaults are used.
//
//-------------------------------------------------------------------------
HRESULT
CDisplaySet::ReadDefaultDisplaySettings()
{
    HRESULT hr = S_OK;

    // set hard coded values
    BOOL smoothing = FALSE;
    UINT smoothingType = FE_FONTSMOOTHINGSTANDARD;

    // Update them with SPI data, ignoring failures
    SystemParametersInfo(SPI_GETFONTSMOOTHING           , 0, &smoothing           , 0);
    SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE       , 0, &smoothingType       , 0);

    // SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST) is not used here on purpose.
    // This setting traditionally was mixed with display gamma curve.
    // In oppose to GDI, we use separate setting for monitor gamma and text contrast.

    // Here we are trying to get the system settings for font smoothing. This will not
    // be the final decision for the rendering/blending mode we use, but it will be an
    // input into it.
    RenderingMode drm = BiLevel;
    if (!!smoothing)
    {
        switch(smoothingType)
        {
            case FE_FONTSMOOTHINGCLEARTYPE:
                drm = ClearType;
                break;
            case FE_FONTSMOOTHINGSTANDARD:
                __fallthrough;
            default:
                drm = Grayscale;
                break;
        }
    }

    IDWriteFactory *pIDWriteFactoryNoRef = NULL;

    ReleaseInterface(m_defaultDisplaySettings.pIDWriteRenderingParams);

    IFC(GetDWriteFactoryNoRef(&pIDWriteFactoryNoRef));
    IFC(pIDWriteFactoryNoRef->CreateRenderingParams(&(m_defaultDisplaySettings.pIDWriteRenderingParams)));

    m_defaultDisplaySettings.DisplayRenderingMode   = drm;
    m_defaultDisplaySettings.PixelStructure = m_defaultDisplaySettings.pIDWriteRenderingParams->GetPixelGeometry();

    IFC(CompileSettings(m_defaultDisplaySettings.pIDWriteRenderingParams, m_defaultDisplaySettings.PixelStructure, NULL, &(m_defaultDisplaySettings.DisplayGlyphParameters)));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::CompileSettings
//
//  Synopsis:   For given primary data in given DisplaySettings structure,
//              calculate redundant ones. We are doing this just once
//              thus saving time on rendering.
//
//-------------------------------------------------------------------------
HRESULT
CDisplaySet::CompileSettings(__in_ecount(1) IDWriteRenderingParams *pParams,
                             DWRITE_PIXEL_GEOMETRY pixelGeometry,
                             __in_ecount_opt(1) IDWriteGlyphRunAnalysis *pAnalysis,
                             __out_ecount(1) GlyphBlendingParameters *pGBP)

{
    HRESULT hr = S_OK;

    float clearTypeLevel;
    float gammaLevel;

    if (pAnalysis == NULL)
    {
        gammaLevel                   = pParams->GetGamma();
        clearTypeLevel               = pParams->GetClearTypeLevel();
        pGBP->ContrastEnhanceFactor  = pParams->GetEnhancedContrast();
    }
    else
    {
        IFC(pAnalysis->GetAlphaBlendParams(pParams, &gammaLevel, &(pGBP->ContrastEnhanceFactor), &clearTypeLevel));
    }

    pGBP->GammaIndex = gammaLevel < 1.0f ? 0 : static_cast<UINT>((gammaLevel - 1.0f) * 10.0f);

    if (pGBP->GammaIndex > MAX_GAMMA_INDEX)
    {
        pGBP->GammaIndex = MAX_GAMMA_INDEX;
    }

    pGBP->BlueSubpixelOffset = float(1./3)*clearTypeLevel;
    if (pixelGeometry == DWRITE_PIXEL_GEOMETRY_BGR)
    {
        pGBP->BlueSubpixelOffset = -pGBP->BlueSubpixelOffset;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::GetEnhancedContrastTable
//
//  Synopsis:   Get a pointer to an enhanced contrast table for a specified k value
//
//-------------------------------------------------------------------------
HRESULT
CDisplaySet::GetEnhancedContrastTable(float k, __deref_out_ecount(1) EnhancedContrastTable **ppTable) const
{
    HRESULT hr = S_OK;

    EnhancedContrastTable *pTable = NULL;
    *ppTable = NULL;

    for (UINT i = 0; i < m_pEnhancedContrastTables.GetCount(); i++)
    {
        if (m_pEnhancedContrastTables[i]->GetContrastValue() == k)
        {
            pTable = m_pEnhancedContrastTables[i];
            break;
        }
    }

    //
    // If we didn't find an entry, add one.
    // This list grows linearly, and thus search will be O(n), however we expect
    // that there should never be many ECTs during one process lifetime. There will be
    // at most one for GDI compatible text with contrast boost (k value of
    // 0.5), and two for each monitor for natural text (the contrast value for that monitor as specified
    // by the registry, and a boosted contrast value for particular fonts for that monitor)
    //
    if (pTable == NULL)
    {
        pTable = new EnhancedContrastTable();
        IFCOOM(pTable);
        pTable->ReInit(k);
        IFC(m_pEnhancedContrastTables.Add(pTable));
    }

    *ppTable = pTable;
    pTable = NULL;

Cleanup:
    delete pTable;
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::GetGammaTable
//
//  Synopsis:   Get a pointer to a gamma table for a specific index
//
//-------------------------------------------------------------------------

HRESULT
CDisplaySet::GetGammaTable(UINT gammaIndex, __deref_out_ecount(1) GammaTable const** ppTable) const
{
    HRESULT hr = S_OK;

    // Check if we already have suitable gamma table.
    GammaTable *& pTable = m_prgGammaTables[gammaIndex];
    if (pTable == NULL)
    {
        // No luck. Need to allocate and fill with data.
        pTable = WPFAllocType(GammaTable *,
            ProcessHeap,
            Mt(CDisplaySet),
            sizeof(GammaTable)
            );
        IFCOOM(pTable);
        CGammaHandler::CalculateGammaTable(pTable, gammaIndex);
    }

    *ppTable = pTable;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::ReadIndividualDisplaySettings
//
//  Synopsis:   Read text rendering settings related to particular display.
//
//-------------------------------------------------------------------------
HRESULT
CDisplaySet::ReadIndividualDisplaySettings()
{
    HRESULT hr = S_OK;

    for (UINT i = 0; i < m_rgpDisplays.GetCount(); i++)
    {
        CDisplay *pDisplay = m_rgpDisplays[i];
        Assert(pDisplay);
        DisplaySettings &ds = pDisplay->m_settings;

        // copy default primary settings
        ds.DisplayGlyphParameters.GammaIndex            = m_defaultDisplaySettings.DisplayGlyphParameters.GammaIndex       ;
        ds.DisplayGlyphParameters.ContrastEnhanceFactor = m_defaultDisplaySettings.DisplayGlyphParameters.ContrastEnhanceFactor;
        ds.DisplayRenderingMode                         = m_defaultDisplaySettings.DisplayRenderingMode;
        ds.AllowGamma                                   = true;

        ReleaseInterface(ds.pIDWriteRenderingParams);

        IDWriteFactory *pIDWriteFactoryNoRef = NULL;
        IFC(GetDWriteFactoryNoRef(&pIDWriteFactoryNoRef));
        IFC(pIDWriteFactoryNoRef->CreateMonitorRenderingParams(
            pDisplay->GetHMONITOR(),
            &(ds.pIDWriteRenderingParams)));

        // Start with pixel geometry specified by monitor settings, and modify
        // as necessary depending on device type and bit depth.
        ds.PixelStructure = ds.pIDWriteRenderingParams->GetPixelGeometry();

        // Read primary settings. On failure defaults will persist.
        if (pDisplay->IsMirrorDevice())
        {
            // Don't allow clear type, use defaultgamma correction.
            ds.PixelStructure = DWRITE_PIXEL_GEOMETRY_FLAT;
        }
        else
        {
            if (pDisplay->GetBitsPerPixel() < 16)
            {
                // Don't allow clear type and gamma correction
                // when display has low color resolution.
                ds.PixelStructure = DWRITE_PIXEL_GEOMETRY_FLAT;
                ds.AllowGamma = false;
            }
        }

        IFC(CompileSettings(ds.pIDWriteRenderingParams, ds.PixelStructure, NULL, &(ds.DisplayGlyphParameters)));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::ReadGraphicsAccelerationCaps
//
//  Synopsis:
//      Call each display to read its acceleration capabilities
//

HRESULT
CDisplaySet::ReadGraphicsAccelerationCaps(
    )
{
    // If DX reported adapters that we don't have, assume it's due to mode change.
    if (m_uD3DAdapterCount > m_rgpDisplays.GetCount())
    {
        RRETURN(WGXERR_DISPLAYSTATEINVALID);
    }

    // Only process D3D recognized adapters (there might be other GDI-recognized-only
    // displays, like for remoting, which will render in software only).  The displays
    // are arranged in DX's adapter order (see ArrangeDXAdapters) so it's correct to
    // iterate through this way.
    for (UINT i = 0; i < m_uD3DAdapterCount; i++)
    {
        m_rgpDisplays[i]->ReadGraphicsAccelerationCaps(m_pID3D);
    }

    RRETURN(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::ReadRequiredVideoDriverDate
//
//  Synopsis:
//      Read registry string value from
//      HKLM\Software\Microsoft\Avalon.Graphics\RequiredVideoDriverDate,
//      convert it to binary format.
//

void
CDisplaySet::ReadRequiredVideoDriverDate(
    )
{
    CDisplayRegKey regkey(HKEY_CURRENT_USER, _T(""));
    TCHAR str[11]; // "YYYY/MM/DD"
    if (regkey.ReadString(_T("RequiredVideoDriverDate"), sizeof(str), str))
    {
        UINT64 binaryTime;
        SYSTEMTIME  time;
        time.wYear          = static_cast<USHORT>(_ttoi(str + 0));
        time.wMonth         = static_cast<USHORT>(_ttoi(str + 5));
        time.wDayOfWeek     = 0;
        time.wDay           = static_cast<USHORT>(_ttoi(str + 8));
        time.wHour          = 0;
        time.wMinute        = 0;
        time.wSecond        = 0;
        time.wMilliseconds  = 0;

        BOOL ok = SystemTimeToFileTime(&time, reinterpret_cast<LPFILETIME>(&binaryTime));
        if (ok)
        {
            m_u64RequiredVideoDriverDate = binaryTime;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CDisplaySet::GetDisplaySettings
//
//  Synopsis:   Get display settings for patricular display specified with
//              given index
//
//-------------------------------------------------------------------------
DisplaySettings const*
CDisplaySet::GetDisplaySettings(UINT uDisplayIndex) const
{
    if (uDisplayIndex >= m_rgpDisplays.GetCount())
    {
        return &m_defaultDisplaySettings;
    }
    else
    {
        const CDisplay *pDisplay = m_rgpDisplays[uDisplayIndex];
        Assert(pDisplay);

        return pDisplay->GetDisplaySettings();
    }
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::GetGraphicsAccelerationCaps
//
//  Synopsis:
//      Returns the graphic acceleration tier and basic rendering statistics
//      (maximum texture size, display format, etc.).  Can be used to query the
//      primary display as well to return a common minimum across all displays.
//
//-----------------------------------------------------------------------------

void
CDisplaySet::GetGraphicsAccelerationCaps(
    bool fReturnCommonMinimum,
    __out_ecount_opt(1) ULONG *pulDisplayUniqueness,
    __out_ecount(1) MilGraphicsAccelerationCaps *pGraphicsAccelerationCaps
    ) const
{
    MilGraphicsAccelerationCaps caps;

    if (pulDisplayUniqueness)
    {
        *pulDisplayUniqueness = m_ulDisplayUniquenessLoader;
    }

    if (m_rgpDisplays.GetCount() == 0 || m_fNonLocalDevicePresent)
    {
        //
        // No display - no acceleration
        //

        caps = GetNoHardwareAccelerationCaps();
    }
    else
    {
        //
        // Start with primary caps
        //

        caps = m_rgpDisplays[0]->m_Caps;

        //
        // If requested, iterate over the secondary displays and calculate
        // the common minimum capabilities subset.
        //

        // Cache the common min caps so we don't recreate them.
        // This is safe since if the device is lost or changes, the DisplaySet is re-created.
        if (fReturnCommonMinimum && !m_fCachedCommonMinCaps)
        {
            m_commonMinCaps = caps;

            for (UINT i = 1; i < m_rgpDisplays.GetCount(); i++)
            {
                const MilGraphicsAccelerationCaps &secondaryCaps =
                    m_rgpDisplays[i]->m_Caps;

                // Calculate the minimum common capability information
                #define SET_COMMON_MINIMUM(f) m_commonMinCaps.f = min(caps.f, secondaryCaps.f)

                SET_COMMON_MINIMUM(TierValue);
                SET_COMMON_MINIMUM(HasWDDMSupport);
                SET_COMMON_MINIMUM(MaxTextureWidth);
                SET_COMMON_MINIMUM(MaxTextureHeight);
                SET_COMMON_MINIMUM(PixelShaderVersion);
                SET_COMMON_MINIMUM(VertexShaderVersion);
                SET_COMMON_MINIMUM(MaxPixelShader30InstructionSlots);
                SET_COMMON_MINIMUM(WindowCompatibleMode);
                SET_COMMON_MINIMUM(HasSSE2Support);
                #undef SET_COMMON_MINIMUM
            }

            m_fCachedCommonMinCaps = true;
        }
    }

    // Report the discovered graphics tier and other statistics
    if (fReturnCommonMinimum && m_fCachedCommonMinCaps)
    {
        *pGraphicsAccelerationCaps = m_commonMinCaps;
    }
    else
    {
        *pGraphicsAccelerationCaps = caps;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::EnsureSwRastIsRegistered
//
//  Synopsis:
//      Make sure a software rasterizer is registered for given ID3D
//
//      If given ID3D is not the most current version then
//      WGXERR_DISPLAYSTATEINVALID will be returned.
//
//-----------------------------------------------------------------------------

HRESULT
CDisplaySet::EnsureSwRastIsRegistered() const
{
    HRESULT hr = S_OK;

    if (DangerousHasDisplayStateChanged())
    {
        IFC(WGXERR_DISPLAYSTATEINVALID);
    }

    if (FAILED(m_hrSwRastRegistered))  // if SUCCEEDED then is registered already
    {
        CGuard<CCriticalSection> oGuard(g_DisplayManager.m_csManagement);

        // check m_hrSwRastRegistered again after entering critical section

        if (m_hrSwRastRegistered == WGXERR_NOTINITIALIZED)
        {
            m_hrSwRastRegistered = CD3DModuleLoader::RegisterSoftwareDevice(m_pID3D);
        }
    }

    hr = m_hrSwRastRegistered;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::UpdateUniqueness
//
//  Synopsis:
//      Copy uniqueness values from given display set origin.
//
//-----------------------------------------------------------------------------

void
CDisplaySet::UpdateUniqueness(
    __in_ecount(1) CDisplaySet const * pDisplaySet
    ) const
{
    m_ulDisplayUniquenessLoader = pDisplaySet->m_ulDisplayUniquenessLoader ;
    m_ulDisplayUniquenessEx     = pDisplaySet->m_ulDisplayUniquenessEx     ;
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplaySet::IsEquivalentTo
//
//  Arguments:
//
//       fIgnoreDesktopCompositionState: Indicates that the desktop composition state should
//                  be ignored for the equivalence test.
//
//  Synopsis:
//      Compare two instances of CDisplaySet.
//      Don't compare mutable fields.
//
//-----------------------------------------------------------------------------

bool
CDisplaySet::IsEquivalentTo(
    __in_ecount(1) CDisplaySet const * pDisplaySet
) const
{
    if (m_u64RequiredVideoDriverDate != pDisplaySet->m_u64RequiredVideoDriverDate) return false;
    if (m_uD3DAdapterCount != pDisplaySet->m_uD3DAdapterCount) return false;

    if (m_fNonLocalDevicePresent != pDisplaySet->m_fNonLocalDevicePresent) return false;

    auto validDpiAwarenessContextValues = GetValidDpiAwarenessContextValues();
    auto otherDisplaySetValidDpiAwarenessContextValues = pDisplaySet->GetValidDpiAwarenessContextValues();

    if (validDpiAwarenessContextValues.size() != otherDisplaySetValidDpiAwarenessContextValues.size()) return false;
    for (auto dpiAwarenessContextValue : validDpiAwarenessContextValues)
    {
        if (pDisplaySet->m_rcDisplayBounds.find(dpiAwarenessContextValue) == pDisplaySet->m_rcDisplayBounds.end()) return false;
        if (!m_rcDisplayBounds.at(dpiAwarenessContextValue).IsEquivalentTo(pDisplaySet->m_rcDisplayBounds.at(dpiAwarenessContextValue))) return false;
    }

    UINT uCount = m_rgpDisplays.GetCount();
    if (uCount != pDisplaySet->m_rgpDisplays.GetCount()) return false;

    for (UINT i = 0; i < uCount; i++)
    {
        if (!m_rgpDisplays[i]->IsEquivalentTo(pDisplaySet->m_rgpDisplays[i])) return false;
    }

    return true;
}


//=================================================================CDisplay

//+------------------------------------------------------------------------
//
//  Function:  CDisplay::CDisplay
//
//  Synopsis:  Constructor
//
//-------------------------------------------------------------------------
CDisplay::CDisplay(
    __in_ecount(1) const CDisplaySet * pDisplaySet,
    __in UINT uDisplayIndex,
    __in_ecount(1) const DISPLAY_DEVICE *pdd
    ) : 
    m_defaultDpiAwarenessContextValue(DpiAwarenessContext::GetThreadDpiAwarenessContextValue())
{
    Assert(pDisplaySet);
    Assert(pdd);

    m_pDisplaySet = pDisplaySet;
    m_uDisplayIndex = uDisplayIndex;
    m_luidD3DAdapter.LowPart = 0;
    m_luidD3DAdapter.HighPart = 0;
    m_dwStateFlags = pdd->StateFlags;

    C_ASSERT( sizeof(m_szDeviceName) == sizeof(pdd->DeviceName) );
    memcpy( m_szDeviceName, pdd->DeviceName, sizeof(m_szDeviceName) );

    m_hMonitor = NULL;

    m_uMemorySize = 0;
    m_fIsRecentDriver = false;
    m_fIsBadDriver = false;
    m_szInstalledDisplayDrivers[0] = _T('\0'); // set to empty string

    m_uGraphicsCardVendorId = GraphicsCardVendorUnknown;
    m_uGraphicsCardDeviceId = GraphicsCardUnknown;

    m_DisplayMode.Size = 0;
    m_DisplayMode.Width = 0;
    m_DisplayMode.Height = 0;
    m_DisplayMode.RefreshRate = 0;
    m_DisplayMode.Format = D3DFMT_UNKNOWN;
    m_DisplayMode.ScanLineOrdering = D3DSCANLINEORDERING(0);
    m_DisplayRotation = static_cast<D3DDISPLAYROTATION>(0);  // Invalid rotation

    size_t KeyLength = 0;
    const size_t c_RegKeyPrefix = 18;   // "\Registry\Machine\"

    if (   SUCCEEDED(StringCchLength(pdd->DeviceKey, ARRAYSIZE(pdd->DeviceKey), &KeyLength))
        && KeyLength > c_RegKeyPrefix)
    {
        CDisplayRegKey keyDev(&pdd->DeviceKey[c_RegKeyPrefix]);

        //
        // Attempt to read true memory size out of registry
        //
        keyDev.ReadUINT(_T("HardwareInformation.MemorySize"), &m_uMemorySize);

        //
        // The idea of reading the InstalledDisplayDrivers key and checking the
        // file times of that driver file came from the dxdiag code.
        // Since we don't want to check the date on WDDM, we store the string
        // and run the check after we've determined our driver model.
        //
        if (!keyDev.ReadString(
                _T("InstalledDisplayDrivers"),
                sizeof(m_szInstalledDisplayDrivers),
                m_szInstalledDisplayDrivers))
        {
            m_szInstalledDisplayDrivers[0] = _T('\0');
        }
    }

    //
    // Default Graphics Acceleration Caps indicating no acceleration
    //

    m_Caps = CDisplaySet::GetNoHardwareAccelerationCaps();

    // Members that remain uninitialized:
    // m_rcBounds
    // m_settings
}

CMILSurfaceRect const & CDisplaySet::GetBounds() const
{
    auto dpiContext = DpiAwarenessContext::GetThreadDpiAwarenessContextValue();
    try
    {
        return m_rcDisplayBounds.at(dpiContext);
    }
    catch (const std::exception&)
    {
        // Not present in the map
    }

    return m_rcDisplayBounds.at(m_defaultDpiAwarenessContextValue);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplay::SetMonitorInfo
//
//  Synopsis:
//      Set monitor association and covered desktop bounds
//
//-----------------------------------------------------------------------------

HRESULT
CDisplay::SetMonitorInfo(
    __in HMONITOR hMonitor,
    __in_ecount(1) LPCRECT prcMonitor
    )
{
    Assert(hMonitor);
    Assert(prcMonitor);

    HRESULT hr = S_OK;

    auto dpiContextValue = DpiAwarenessContext::GetThreadDpiAwarenessContextValue();

    if (m_hMonitor != nullptr)
    {
        // This instance has been already initialized.
        // But this could be a partially constructed instance
        if (m_rcBounds.find(dpiContextValue) != m_rcBounds.end())
        {
            // Suppose it happened due to a mode change.
            IFC(WGXERR_DISPLAYSTATEINVALID);
        }
    }
    else
    {
        m_hMonitor = hMonitor;
    }
    
    
    m_rcBounds[dpiContextValue] = *prcMonitor;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplay::ReadMode
//
//  Synopsis:
//      Read display mode information from D3D and/or GDI.
//

HRESULT
CDisplay::ReadMode(
    __in_ecount_opt(1) IDirect3D9 *pID3D,
    __in_ecount_opt(1) IDirect3D9Ex *pID3DEx
    )
{
    Assert(m_pDisplaySet);

    if (pID3D || pID3DEx)
    {
        Assert(m_uDisplayIndex < m_pDisplaySet->GetNumD3DRecognizedAdapters());
    }

    HRESULT hr = S_OK;

    //
    // Assert initial state
    //

    Assert(m_DisplayMode.Size == 0);
    Assert(m_DisplayMode.RefreshRate == 0);
    Assert(m_DisplayMode.Format == D3DFMT_UNKNOWN);
    Assert(m_DisplayMode.ScanLineOrdering == D3DSCANLINEORDERING(0));
    Assert(m_DisplayRotation == 0);

    if (pID3DEx)
    {
        //
        // Read display mode from D3DEx which has complete information.
        //

        m_DisplayMode.Size = sizeof(m_DisplayMode);

        MIL_THR(pID3DEx->GetAdapterDisplayModeEx(
            m_uDisplayIndex,
            &m_DisplayMode,
            &m_DisplayRotation
            ));
    }
    else if (pID3D)
    {
        //
        // Read display mode from D3D which has basic information.
        //

        D3DDISPLAYMODE displayMode;

        MIL_THR(pID3D->GetAdapterDisplayMode(m_uDisplayIndex, &displayMode));

        if (SUCCEEDED(hr))
        {
            // DISPLAYMODE is a direct substructure of DISPLAYMODEEX
            m_DisplayMode.Size = offsetof(D3DDISPLAYMODEEX, ScanLineOrdering);
            m_DisplayMode.Width = displayMode.Width;
            m_DisplayMode.Height = displayMode.Height;
            m_DisplayMode.RefreshRate = displayMode.RefreshRate;
            m_DisplayMode.Format = displayMode.Format;
        }
    }

    // If this adapter is no longer enabled or the monitor is disconnected, the call above
    // will fail with the following error.  This can occur sometimes when remoting into a machine
    // with certain video drivers, or when the display driver is disabled or uninstalled while WPF
    // is running.  Ignoring the failure is okay - the function will proceed with reading display
    // information from GDI instead.
    if (hr == D3DERR_NOTAVAILABLE)
    {
        hr = S_OK;
    }
    else
    {
        // Fail upon encountering any other error codes.
        IFC(hr);
    }

    //
    // For adapters D3D does not report, get information from GDI.
    //
    if (   (m_DisplayMode.Format == D3DFMT_UNKNOWN)
#if ALWAYS_READ_DISPLAY_ORIENTATION
           // D3D9Ex will report rotation, but D3D9 will not.  Get rotation from GDI
           // in this case.
        || (m_DisplayRotation == static_cast<D3DDISPLAYROTATION>(0))
#endif
       )
    {
        //
        // Read display mode from GDI
        //

        DEVMODE displayModeGDI;

        displayModeGDI.dmSize = sizeof(displayModeGDI);

        SetLastError(ERROR_SUCCESS);
        if (0 == TW32(0,EnumDisplaySettings(
            m_szDeviceName,
            ENUM_CURRENT_SETTINGS,
            &displayModeGDI
            )))
        {
            IFC( m_pDisplaySet->InspectLastError() );
        }

        if (m_DisplayMode.Format == D3DFMT_UNKNOWN)
        {
            //
            // Fill in information from GDI
            //

            m_DisplayMode.Size = offsetof(D3DDISPLAYMODEEX, ScanLineOrdering);
            m_DisplayMode.Width = displayModeGDI.dmPelsWidth;
            m_DisplayMode.Height = displayModeGDI.dmPelsHeight;
            m_DisplayMode.RefreshRate = displayModeGDI.dmDisplayFrequency;

            // Interpret bits per pixel to D3D format
            switch (displayModeGDI.dmBitsPerPel)
            {
            case 32:
                // Note - may also be D3DFMT_A2R10G10B10
                m_DisplayMode.Format = D3DFMT_X8R8G8B8;
                break;

            case 24:
                m_DisplayMode.Format = D3DFMT_R8G8B8;
                break;

            case 16:
                // Note - may also be D3DFMT_X1R5G5B5
                m_DisplayMode.Format = D3DFMT_R5G6B5;
                break;

            case 8:
                m_DisplayMode.Format = D3DFMT_P8;
                break;

            default:
                m_DisplayMode.Format = D3DFMT_UNKNOWN;
                break;
            }
        }

        //
        // Fill in rotation from GDI when available
        //

        if (   (m_DisplayRotation == static_cast<D3DDISPLAYROTATION>(0))
            && (displayModeGDI.dmFields & DM_DISPLAYORIENTATION))
        {
            C_ASSERT(DMDO_DEFAULT+1 == D3DDISPLAYROTATION_IDENTITY);
            C_ASSERT(DMDO_90+1 == D3DDISPLAYROTATION_90);
            C_ASSERT(DMDO_180+1 == D3DDISPLAYROTATION_180);
            C_ASSERT(DMDO_270+1 == D3DDISPLAYROTATION_270);
            m_DisplayRotation = static_cast<D3DDISPLAYROTATION>
                (displayModeGDI.dmDisplayOrientation+1);
        }

        m_Caps.BitsPerPixel = displayModeGDI.dmBitsPerPel;
    }
    else
    {
        m_Caps.BitsPerPixel = 8*D3DFormatSize(m_DisplayMode.Format);
    }

Cleanup:

    if (hr == D3DERR_DEVICELOST || hr == D3DERR_DRIVERINTERNALERROR)
    {
        // These errors can be obtained because of display state change,
        // though it is not guaranteed to be the only possible reason.
        // We are replacing this HR to WGXERR_DISPLAYSTATEINVALID in order
        // to force retry attempt CDisplayManager::DangerousGetLatestDisplaySet().

        MIL_THR(WGXERR_DISPLAYSTATEINVALID);
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplay::ReadGraphicsAccelerationCaps
//
//  Synopsis:
//      Returns the hardware capability tier and basic rendering
//      and capabilities statistics (maximum texture size, fullscreen
//      capability, etc.) for this display.
//

void
CDisplay::ReadGraphicsAccelerationCaps(
    __in_ecount(1) IDirect3D9 *pID3D
    )
{
    // Report tier 0 by default - should already be set
    Assert(m_Caps.TierValue == MIL_TIER(0,0));

    // Bit depth should have already been set by ReadMode
    Assert(m_Caps.BitsPerPixel > 0);

    // Check for uninitialized caps default values
    Assert(m_Caps.WindowCompatibleMode == FALSE);


    CheckBadDeviceDrivers();

    //
    // If the device driver explicitly considered bad
    // on CheckBadDeviceDrivers() call, leave m_Caps unchanged.
    //

    if (!IsDeviceDriverBad())
    {
        D3DCAPS9 caps;

        m_Caps.WindowCompatibleMode = SUCCEEDED(::CheckDisplayFormat(
            pID3D,
            GetDisplayIndex(),
            D3DDEVTYPE_HAL,
            GetFormat(),
            MilRTInitialization::Default   // Not fullscreen; not need_dst_alpha
            ));

        if (m_Caps.WindowCompatibleMode)
        {
            if (SUCCEEDED(pID3D->GetDeviceCaps(
                GetDisplayIndex(),
                D3DDEVTYPE_HAL,
                &caps
                )))
            {
                // Copy the maximum texture size for this display
                m_Caps.MaxTextureWidth = caps.MaxTextureWidth;
                m_Caps.MaxTextureHeight = caps.MaxTextureHeight;

                // Check if the display has WDDM support
                m_Caps.HasWDDMSupport = HwCaps::IsLDDMDevice(caps);

                // Copy the pixel and vertex shader versions
                m_Caps.PixelShaderVersion = caps.PixelShaderVersion;
                m_Caps.VertexShaderVersion = caps.VertexShaderVersion;

                // Copy the max number of instruction slots for PS 3.0
                m_Caps.MaxPixelShader30InstructionSlots = caps.MaxPixelShader30InstructionSlots;

                if (m_Caps.HasWDDMSupport)
                {
                    m_fIsRecentDriver = true;
                }
                else
                {
                    m_fIsRecentDriver = CheckForRecentDriver(m_szInstalledDisplayDrivers);
                }

                if (m_fIsRecentDriver)
                {
                    // Get the tier value for this display
                    m_Caps.TierValue =
                        GraphicsAccelerationTier::GetTier(GetMemorySize(), caps);
                }

                // Determine if the processor has SSE2 support
                m_Caps.HasSSE2Support = CCPUInfo::HasSSE2ForEffects();
            }
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CDisplay::IsEquivalentTo
//
//  Synopsis:
//      Compare two instances of CDisplay.
//
//-----------------------------------------------------------------------------

bool
CDisplay::IsEquivalentTo(
    __in_ecount(1) CDisplay const * pDisplay
    ) const
{
    if (m_uDisplayIndex != pDisplay->m_uDisplayIndex) return false;
    if (m_luidD3DAdapter != pDisplay->m_luidD3DAdapter) return false;
    if (m_hMonitor != pDisplay->m_hMonitor) return false;

    if (m_rcBounds.size() != pDisplay->m_rcBounds.size()) return false;
    for (auto item : m_rcBounds)
    {
        const auto& dpiContextValue = item.first;
        const auto& rcBound = item.second;

        if (pDisplay->m_rcBounds.find(dpiContextValue) == pDisplay->m_rcBounds.end()) return false;
        
        const auto& rcBound2 = pDisplay->m_rcBounds.at(dpiContextValue);
        if (!rcBound.IsEquivalentTo(rcBound2)) return false;
    }

    if (0 != _tcscmp(m_szDeviceName, pDisplay->m_szDeviceName)) return false;

    if (m_dwStateFlags != pDisplay->m_dwStateFlags) return false;

    if (!m_settings.IsEquivalentTo(pDisplay->m_settings)) return false;
    if (m_uMemorySize != pDisplay->m_uMemorySize) return false;

    if (m_fIsRecentDriver != pDisplay->m_fIsRecentDriver) return false;
    if (m_fIsBadDriver != pDisplay->m_fIsBadDriver) return false;

    if (m_uGraphicsCardVendorId != pDisplay->m_uGraphicsCardVendorId) return false;
    if (m_uGraphicsCardDeviceId != pDisplay->m_uGraphicsCardDeviceId) return false;

    if (0 != memcmp(&m_DisplayMode, &pDisplay->m_DisplayMode, sizeof(m_DisplayMode))) return false;
    if (m_DisplayRotation != pDisplay->m_DisplayRotation) return false;
    if (0 != memcmp(&m_Caps, &pDisplay->m_Caps, sizeof(m_Caps))) return false;

    return true;
}


HRESULT
GetDriverDate(
    __in PCTSTR pstrDriver,
    __out_ecount(1) unsigned __int64 *pui64DriverDate
    )
{
    HRESULT hr = S_OK;

    TCHAR rgPath[MAX_PATH];
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    CDisableWow64FsRedirection disabler;

    DWORD count = GetSystemDirectory(rgPath, ARRAY_SIZE(rgPath));
    if (!count || count > ARRAY_SIZE(rgPath))
    {
        IFC(E_FAIL);
    }

    IFC(StringCchCat(rgPath,
                     ARRAY_SIZE(rgPath),
                     _T("\\")));

    IFC(StringCchCat(rgPath,
                     ARRAY_SIZE(rgPath),
                     pstrDriver));

    IFC(StringCchCat(rgPath,
                     ARRAY_SIZE(rgPath),
                     _T(".dll")));

    hFind = FindFirstFile(rgPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        IFCW32(UNCONDITIONAL_EXPR(NULL));
    }
    FindClose(hFind);

    // Give the benefit of the doubt
    *pui64DriverDate = reinterpret_cast<unsigned __int64&>(findFileData.ftLastWriteTime);
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CDisplay::GetMode
//
//  Synopsis:  Return display mode
//

HRESULT
CDisplay::GetMode(
    __out_xcount_part(sizeof(*pDisplayMode), pDisplayMode->Size) D3DDISPLAYMODEEX *pDisplayMode,
    __out_ecount_opt(1) D3DDISPLAYROTATION *pDisplayRotation
    ) const
{
    // Note: This code does not care about dynamic size nature of
    //       D3DDISPLAYMODEEX.  It simply assumes caller is using same version
    //       of structure and does a simple copy.
    *pDisplayMode = m_DisplayMode;

    if (pDisplayRotation)
    {
        // May be invalid (0)
        *pDisplayRotation = m_DisplayRotation;
    }

    return (m_DisplayMode.Size > 0) ? S_OK : WGXERR_NOTINITIALIZED;
}


//+----------------------------------------------------------------------------
//
//  Member:    CDisplay::CheckForRecentDriver
//
//  Synopsis:  Checks to see if we have an video driver newer than our
//             required freshness date.
//

bool
CDisplay::CheckForRecentDriver(
    __in PCTSTR pstrDriver
    ) const
{
    bool fDriverIsGood = true;

    unsigned __int64 ui64DriverDate = 0;

    if (   pstrDriver[0] != _T('\0')
        && SUCCEEDED(GetDriverDate(
                pstrDriver,
                &ui64DriverDate)))
    {
        if (IsTagEnabled(tagWarning))
        {
            SYSTEMTIME SystemTime;
            if (FileTimeToSystemTime(reinterpret_cast<LPFILETIME>(&ui64DriverDate), &SystemTime))
            {
                TCHAR DriverDate[MAX_PATH];
                if (GetDateFormat(LOCALE_USER_DEFAULT,
                                  DATE_SHORTDATE,
                                  &SystemTime,
                                  NULL,
                                  DriverDate,
                                  ARRAY_SIZE(DriverDate)
                                  ) != 0)
                {
                    TraceTag((tagWarning, "MIL-HW(adapter=%d): Device driver date is %S", m_uDisplayIndex, DriverDate));
                }
                else
                {
                    TraceTag((tagWarning, "MIL-HW(adapter=%d): Couldn't convert to display date.  Device driver date is %I64u", m_uDisplayIndex, ui64DriverDate));
                }
            }
        }

        if (ui64DriverDate < m_pDisplaySet->GetRequiredVideoDriverDate())
        {
            TraceTag((tagWarning, "MIL-HW(adapter=%d): Device driver too old: falling back to SW.", m_uDisplayIndex));
            fDriverIsGood = false;
        }
    }
    else
    {
        // If we fail to get driver info then driver will be considered recent.
        TraceTag((tagWarning, "MIL-HW(adapter=%d): Couldn't find device driver date: driver will be considered recent.", m_uDisplayIndex));
    }

    return fDriverIsGood;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDisplay::CheckBadDeviceDrivers
//
//  Synopsis:
//      Explicitly checks driver vendor and id, sets m_fIsBadDriver = true
//      for the drivers that are known to contain bugs.
//      Fills m_uGraphicsCardVendorId and m_uGraphicsCardDeviceId along the way.
//      Keeps other fields unchanged.
//
//-------------------------------------------------------------------------
void
CDisplay::CheckBadDeviceDrivers()
{
    HRESULT hr = S_OK;

    IDirect3D9 *pD3D = D3DObject();
    IDirect3D9Ex *pD3DEx = D3DExObject();

    D3DADAPTER_IDENTIFIER9 identifier = { 0 };

#if !defined(D3DENUM_NO_DRIVERVERSION)
#define D3DENUM_NO_DRIVERVERSION                0x00000004L
#endif
    // D3DENUM_NO_DRIVERVERSION allows D3D to avoid the perf hit of getting the
    // driver's file information. D3DADAPTER_IDENTIFIER9::DriverVersion will
    // not be valid, nor will DriverVersion be integrated into the
    // D3DADAPTER_IDENTIFIER9::DeviceIdentifier.  This is only available on
    // D3D9Ex devices.
    if (pD3DEx != NULL)
    {
        hr = pD3DEx->GetAdapterIdentifier(
            m_uDisplayIndex,
            D3DENUM_NO_DRIVERVERSION,
            &identifier);
    }

    if (pD3DEx == NULL || FAILED(hr))
    {
        // If GetAdapterIdentifier doesn't work with D3DENUM_NO_DRIVERVERSION
        // then try without it, since not all versions of D3D support it.
        IFC(pD3D->GetAdapterIdentifier(
            m_uDisplayIndex,
            0,
            &identifier
            ));
    }

    //
    // Store adapter identifier data for later using by CD3DDeviceLevel1
    // to correct device caps for buggy drivers.
    //
    m_uGraphicsCardVendorId = identifier.VendorId;
    m_uGraphicsCardDeviceId = identifier.DeviceId;

    //
    // Intel 845 is disabled for the following reasons:
    //
    //    1. As of 3/15/2003, our software rasterizer is faster than the
    //       Intel 845 running with the HW pipeline for most scenario
    //       based tests.
    //
    //    2. A Gouraud shading bug
    //
    // Note that the Gouraud shading bug can be avoided by always texturing,
    // but there is no need for this now that our software engine is faster
    // for the scenario tests.
    //

    if (identifier.VendorId == GraphicsCardVendorIntel
        && identifier.DeviceId == GraphicsCardIntel_845G)
    {
        IFC(E_FAIL);
    }

Cleanup:
    //
    // On any fail driver is concluded bad.
    //
    m_fIsBadDriver = !!FAILED(hr);
}

CMILSurfaceRect const &CDisplay::GetDisplayRect() const 
{
    auto dpiContext = DpiAwarenessContext::GetThreadDpiAwarenessContextValue();
    try
    {
        return m_rcBounds.at(dpiContext);
    }
    catch (const std::exception&)
    {
        // Not present in the map
    }

    return m_rcBounds.at(m_defaultDpiAwarenessContextValue); 
}





