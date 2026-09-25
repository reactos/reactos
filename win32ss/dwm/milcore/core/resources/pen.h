// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilPenDuce);

struct CMilPenRealization
{
    friend class CMilPenDuce;

public:
    CMilPenRealization()
    {
        Reset();
    }

    CPlainPen *GetPlainPen() { return m_pPen; }
    CMilSlaveResource *GetBrush() { return m_pBrush; }

private:
    void Reset()
    {
        m_pPen = NULL;
        m_pBrush = NULL;
    }

    void SetPlainPen(CPlainPen *pPen) { m_pPen = pPen; }
    void SetBrush(CMilSlaveResource *pBrush) { m_pBrush = pBrush; }

    CPlainPen *m_pPen;
    CMilSlaveResource *m_pBrush;
};

// Class: CMilPenDuce
class CMilPenDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilPenDuce));

    CMilPenDuce(__in_ecount(1) CComposition*)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilPenDuce();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_PEN;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_PEN* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

public:
    virtual HRESULT GetPen(__deref_out_ecount(1) CMilPenRealization **pRealization);

    CMilPenDuce_Data m_data;

protected:
    
    override BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        )    
    {
        SetDirty(TRUE);
        return TRUE;
    }

private:
    CPlainPen m_pen;

    // Used for returning cached "realized pen" references.
    CMilPenRealization m_penRealization;
};

