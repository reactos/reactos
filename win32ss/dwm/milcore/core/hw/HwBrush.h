// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains CHwBrush declaration and definition
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwBrush
//
//  Synopsis:
//      Provides base interface and implementation for HW brush implementations
//
//------------------------------------------------------------------------------

class CHwBrush : 
    public IMILRefCount, 
    public IHwPrimaryColorSource
{
protected:

    CHwBrush(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        )
        : m_pDevice(pDevice)
    {
        Assert(pDevice);
    }

protected:

    //
    // Reference to the HW device abstraction which may be used to create HW
    // resources and set states as required.
    //
    // Note that there is not reference count held for this as brushes may be
    // cached by the device abstraction.  Such a situation would yeild a
    // circular reference.  As there is not reference held it is only valid to
    // use this reference when in a context that guarentees it availablilty.
    // The one such case is during a primitve call, as there is a render target
    // which holds a device reference.  And example of an unacceptable time to
    // access the device is during clean up unless other arrangements have been
    // made.
    //
    CD3DDeviceLevel1 * const m_pDevice;

};



