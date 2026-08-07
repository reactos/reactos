// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Templatized class for delay loading a module
//
//  Notes:
//      Implementation is thread safe.
//
//      Once loaded the module will not be unloaded until the class is
//      destroyed.
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Class:
//      CDelayLoadedModule
//
//  Synopsis:
//      Loads a module specified by template information, when requested by a
//      caller, but does not unload module until respective instance of class
//      is destroyed.
//
//      Template type should be a struct with a static constant member named
//      sc_szFileName which is a string of the module to load:
//          struct SomeModuleInfo {
//              static const TCHAR sc_szFileName[];
//          };
//          const TCHAR sc_szFileName[] = _T("foo");
//
//      Optionally the template struct may contain a static method named
//      CheckLoadAvailability which CDelayLoadedModule will call before
//      attempting to actually load specified module.  If caller allows
//      multiple threads to attempt module load simultaneously, then
//      CheckLoadAvailability must be prepared to also handle this call pattern
//      and must return the same result for all calls.  Example declaration:
//          static HRESULT CheckLoadAvailability();
//
//-----------------------------------------------------------------------------

template<typename ModuleInfo>
class CDelayLoadedModule
{
public:
    CDelayLoadedModule()
      : m_hrLoad(WGXERR_NOTINITIALIZED),
        m_hModule(NULL)
    {
    }
    
    ~CDelayLoadedModule() 
    { 
        if (m_hModule)
        {
            FreeLibrary(m_hModule);
        }
    }

    HRESULT Load()
    {
        HRESULT hr = m_hrLoad;

        if (hr == WGXERR_NOTINITIALIZED)
        {
            __if_exists (ModuleInfo::CheckLoadAvailability)
            {
                hr = ModuleInfo::CheckLoadAvailability();
            }
            // else
            __if_not_exists (ModuleInfo::CheckLoadAvailability)
            {
                hr = S_OK;
            }

            if (SUCCEEDED(hr))
            {
                HMODULE hNew;

                MIL_TW32_NOSLE(hNew = LoadLibrary(ModuleInfo::sc_szFileName));

                if (SUCCEEDED(hr))
                {
                    Assert(hNew != NULL);

                    HMODULE hCurrent = reinterpret_cast<HMODULE>(
                        InterlockedCompareExchangePointer(
                            (PVOID volatile *)&m_hModule,
                            hNew,
                            NULL));

                    // If m_hModule was already updated then release this
                    // unneeded load reference.
                    if (hCurrent != NULL)
                    {
                        // LoadLibrary should always return the same HMODULE
                        Assert(hCurrent == hNew);
                        FreeLibrary(hNew);
                    }

                    // At this point we need to be sure result is written
                    // before result status is set.  We rely on
                    // InterlockecdCompareExchangePointer to have committed
                    // results by now.
                }
                else
                {
                    // No need to update m_hModule when load fails.  No one
                    // else should have udpated it to a non-zero value either.
                    Assert(m_hModule == NULL);
                }
            }

            // Note for future users - the assert can be removed if
            // CheckLoadAvailabity wants to delay loading until certain
            // conditions, but in that case other logic depending on a
            // deterministic result will need to be checked.  For example
            // threading module/protection may need to change and callers that
            // use a single function pointer to intialially point to a "load"
            // function routine will have to expect this case as well.
            Assert(hr != WGXERR_NOTINITIALIZED);
            // Save results
            m_hrLoad = hr;
        }

        RRETURN(hr);
    }

    __out HMODULE Handle() const
    {
        Assert(SUCCEEDED(m_hrLoad));
        return m_hModule;
    }

    FARPROC
    GetProcAddress(
        __in PCSTR pProcName
        ) const
    {
        Assert(SUCCEEDED(m_hrLoad));
        return ::GetProcAddress(m_hModule, pProcName);
    }

    FARPROC
    LoadProcAddress(
        __in PCSTR pProcName
        )
    {
        return SUCCEEDED(Load()) ? ::GetProcAddress(m_hModule, pProcName) : NULL;
    }

private:

    HRESULT volatile m_hrLoad;
    HMODULE volatile m_hModule;

};


