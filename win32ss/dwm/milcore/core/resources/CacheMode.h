// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Cache mode resource header.
//
//-----------------------------------------------------------------------------

// Forward declarations
class CDrawingContext;

MtExtern(CMilCacheModeDuce);

// Class: CMilEffectDuce
class CMilCacheModeDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

private:
    CComposition* m_pCompositionNoRef;


protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilCacheModeDuce));

    CMilCacheModeDuce(__in_ecount(1) CComposition* pComposition)
    {
        m_pCompositionNoRef = pComposition;
    }

    CMilCacheModeDuce() { }
    

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_CACHEMODE;
    }
};



