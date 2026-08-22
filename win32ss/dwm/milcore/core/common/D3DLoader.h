// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Maintains primary references to D3D module and Sw
//                   rasterizer (as needed).
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Class:
//      CD3DLoader
//
//  Synopsys:
//      Provides access to top level D3D objects.
//      Implementation is based on wrapping CDisplaySet object that
//      protects against unnecessary reloads when uniqueness change
//      notification turns to be irrelevant.
//
//-----------------------------------------------------------------------------
class CD3DLoader
{
public:

    //
    // Increase D3D module load count
    //

    static void GetLoadRef(
        );

    //
    // Decrement D3D module load count
    //
    // Note: Must be paired with GetLoadRef(AndID3D), but only called
    //       after all D3D interfaces, derived from and including ID3D,
    //       have been released.
    //

    static void ReleaseLoadRef(
        );
};

//+----------------------------------------------------------------------------
//
//  Class:
//      CD3DModuleLoader
//
//  Synopsys:
//      Provides access to D3D module and top level D3D objects.
//
//-----------------------------------------------------------------------------
class CD3DModuleLoader
{
public:
    static HRESULT Startup();
    static void Shutdown();

private:
    // This class is not supposed to be used by anything
    // except CDisplaySet and CDisplayManager.
    friend class CDisplaySet;
    friend class CDisplayManager;

    //
    // Register a software rasterizer for given ID3D.
    //
    // Note: It is a caller responsibility not to call this method
    //       many times against the same pID3D.
    //
    static HRESULT RegisterSoftwareDevice(
        __inout_ecount(1) IDirect3D9 *pID3D
        );

    //
    // Get the current uniqueness value
    //
    // Note: A valid load reference is not required.
    //

    static ULONG GetDisplayUniqueness();

    //
    // Create top level D3D objects.
    //

    static HRESULT CreateD3DObjects(
        __deref_out_ecount(1) IDirect3D9 **ppID3D,
        __deref_out_ecount(1) IDirect3D9Ex **ppID3DEx
        );

    //
    // Release the module: decrease module reference count
    // unload dlls when they are no longer in use.
    //
    // Note: Must be paired with CreateD3DObjects, but only called
    //       after all D3D interfaces, derived from and including ID3D,
    //       have been released.
    //

    static void ReleaseD3DLoadRef(
        );

};



