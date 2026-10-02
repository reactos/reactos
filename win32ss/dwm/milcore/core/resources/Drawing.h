// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Abstract base Drawing class definition.  
//


//+----------------------------------------------------------------------------
//
//  Class:     CMilDrawingDuce
//
//  Synopsis:  Base class that defines the interface for the Draw method which
//             all Drawing's must implement.  Clients call Draw to render
//             a Drawing's content to a render context.
//
//-----------------------------------------------------------------------------
class CMilDrawingDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    CMilDrawingDuce(__in_ecount(1) CComposition*)
    {
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DRAWING;
    }

    virtual HRESULT Draw(__in_ecount(1) CDrawingContext *pDrawingContext) = 0;
};



