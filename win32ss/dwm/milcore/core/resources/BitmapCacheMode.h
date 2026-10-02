// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Bitmap cache mode resource header.
//
//-----------------------------------------------------------------------------

//class CDirtyRegion2;

MtExtern(CMilBitmapCacheDuce);

// Class: CMilBitmapCacheDuce
class CMilBitmapCacheDuce : public CMilCacheModeDuce
{
    friend class CResourceFactory;

private:
    CComposition* m_pCompositionNoRef;
    CMilBitmapCacheDuce_Data m_data;

    CMilBitmapCacheDuce(
        __in CComposition *pComposition,
        __in double renderAtScale,
        __in bool snapsToDevicePixels,
        __in bool enableClearType
        );
    
protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilBitmapCacheDuce));

    CMilBitmapCacheDuce(__in_ecount(1) CComposition* pComposition)
    {
        m_pCompositionNoRef = pComposition;
    }

    ~CMilBitmapCacheDuce() { }
    
public:

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_BITMAPCACHE* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_BITMAPCACHE || CMilCacheModeDuce::IsOfType(type);
    }

    static HRESULT Create(
        __in CComposition *pComposition,
        __in double renderAtScale,
        __in bool snapsToDevicePixels,
        __in bool enableClearType,
        __deref_out CMilBitmapCacheDuce **pCacheMode
        );

    bool IsStatic()
    {
        return false; 
    }

    bool SnapsToDevicePixels() const
    {
        return !!m_data.m_SnapsToDevicePixels;
    }

    bool IsClearTypeEnabled() const
    {
        return !!m_data.m_EnableClearType;
    }
    
    double GetScale();
};





