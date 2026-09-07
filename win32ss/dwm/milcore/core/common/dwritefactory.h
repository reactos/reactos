// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Class:
//      CDWriteFactory
//
//  Synopsys:
//      Loads/unloads DWrite and provides the factory creation method
//
//-----------------------------------------------------------------------------
class CDWriteFactory
{
public:
    CDWriteFactory();
    HRESULT Startup();

    void Shutdown();
    ~CDWriteFactory();

    HRESULT DWriteCreateFactory(
        __in DWRITE_FACTORY_TYPE factoryType,
        __in REFIID iid,
        __out IUnknown **factory
        );

private:
    typedef HRESULT (WINAPI *DWRITECREATEFACTORY)(
        DWRITE_FACTORY_TYPE factoryType,
        REFIID iid,
        IUnknown **factory
        );
    
    DWRITECREATEFACTORY m_pfnDWriteCreateFactory;
    HMODULE m_hDWriteLibrary;
    CCriticalSection m_csManagement;
};

extern CDWriteFactory g_DWriteLoader;


