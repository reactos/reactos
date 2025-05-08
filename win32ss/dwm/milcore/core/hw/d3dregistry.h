// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DRegistryDatabase
//

MtExtern(CD3DRegistryDatabase);

//------------------------------------------------------------------------------
//
//  Class: CD3DRegistryDatabase
//
//  Description:
//      Accesses the registry to determine if we can run hw accelerated on
//      the current driver
//
//      Note that all methods and data here are static so that we can be
//      smart enough to only access the registry once to query this 
//      information.
//
//------------------------------------------------------------------------------

class CD3DRegistryDatabase  
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DRegistryDatabase));

    //
    // CD3DRegistryDatabase methods
    //

    static HRESULT InitializeFromRegistry(
        __in_ecount(1) IDirect3D9 *pD3D
        );

    static HRESULT IsAdapterEnabled(
        UINT uAdapter, 
        __out_ecount(1) bool *pfEnabled
        );

    static bool ShouldSkipDriverCheck();

    static HRESULT DisableAdapter(
        UINT uAdapter
        );

    static HRESULT HandleAdapterUnexpectedError(
        UINT uAdapter
        );

    static void Cleanup();
    
private:
    static void EnableAllAdapters(
        bool fEnabled
        );

    static HRESULT InitializeDriversFromRegistry(
        __inout_ecount(1) IDirect3D9 *pD3D
        );

private:
    static bool m_fInitialized;
    static UINT m_cNumAdapters;

    // Error count is number of errors associated with this adapter.  If
    // it is greater than or equal to c_uMaxErrorCount the adapter is
    // disabled.
    static UINT *m_prguErrorCount;
    static bool m_fSkipDriverCheck;
};




