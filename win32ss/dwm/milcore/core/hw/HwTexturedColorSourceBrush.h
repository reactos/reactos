// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwTexturedColorSourceBrush declaration
//


//+----------------------------------------------------------------------------
//
//  Class:     CHwTexturedColorSourceBrush
//
//  Synopsis:  This class implements the primary color source interface for a
//             texture based color source.  It is intended to just be a helper
//             when a textured color source need to be send thruough the Hw
//             pipeline.
//

class CHwTexturedColorSourceBrush : public CHwBrush
{
private:

    //
    // Illegal allocation operators
    //
    //   These are declared, but not defined such that any use that gets around
    //   the private protection will generate a link time error.
    //

    __allocator __out_bcount(cb) void * operator new(size_t cb);
    __allocator __out_bcount(cb) void * operator new[](size_t cb);
    __out_bcount(cb) void * operator new(size_t cb, __out_bcount(cb) void * pv);

public:

    CHwTexturedColorSourceBrush(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CHwTexturedColorSource *pTexturedSource
        );
    virtual ~CHwTexturedColorSourceBrush();

    // AddRef/Release are not supported
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IHwPrimaryColorSource methods

    override HRESULT SendOperations(
        __inout_ecount(1) CHwPipelineBuilder *pBuilder
        );

private:

    // Textured Color Source
    CHwTexturedColorSource *m_pTexturedSource;

};



