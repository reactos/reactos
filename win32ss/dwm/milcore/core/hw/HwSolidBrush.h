// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSolidBrush declaration
//


//  This class is only used in scratch capacity with CHwBrushPool.  It could
//  very well just be a member of that class.
MtExtern(CHwSolidBrush);

//+----------------------------------------------------------------------------
//
//  Class:     CHwSolidBrush
//
//  Synopsis:  This class implements the primary color source interface for a
//             solid brush
//
//             Since this class and it color source are very simple the two
//             have been combined together in this single class.  This saves
//             some allocation and management time.

class CHwSolidBrush : 
    public CHwBrush, 
    public CHwConstantMilColorFColorSource
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwSolidBrush));

    CHwSolidBrush(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );


    ~CHwSolidBrush()
    {
        Assert(m_cRef == 0);
    }

    // AddRef is called by Pipeline builder when this is a color source.
    override STDMETHOD_(ULONG, AddRef)()
    {
        Assert(++m_cRef > 0);
        return 1;
    }

    // Release is expected by the caller of DeriveHwBrush (which calls
    // CHwBrushPool::GetHwBrush ...) and by Pipeline builder when this
    // is acting as a color source.
    override STDMETHOD_(ULONG, Release)()
    {
        Assert(m_cRef-- > 0);
        return 0;
    }

    void SetColor(
        __in_ecount(1) MilColorF const &color
        );

    override HRESULT SendOperations(
        __inout_ecount(1) CHwPipelineBuilder *pBuilder
        );
};




