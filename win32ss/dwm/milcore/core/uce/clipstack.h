// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Clip Stack
//
//-----------------------------------------------------------------------------

class CBaseClipStack
{
public:
    virtual ~CBaseClipStack() {};

    VOID Pop();

public:
    VOID Clear();
    BOOL IsEmpty() const { return m_clipStack.IsEmpty(); }
    VOID Optimize() { m_clipStack.Optimize(); }

    UINT GetSize() { return m_clipStack.GetSize(); }

protected:
    HRESULT Push(
        __in_ecount(1) const MilRectF &rcClip
        );
    HRESULT PushExact(
        __in_ecount(1) const MilRectF &rcClip
        );
    void Top(__out_ecount(1) CMilRectF *prcClip);

private:
    CWatermarkStack<CMilRectF, 8 /* MinCapacity */, 2 /* GrowFactor */, 8 /* TrimCount */> m_clipStack;
};


class CGenericClipStack : public CBaseClipStack
{
public:
    HRESULT Push(
        __in_ecount(1) const MilRectF &rcClip
        )
    {
        return CBaseClipStack::Push(rcClip);
    }

    HRESULT PushExact(
        __in_ecount(1) const MilRectF &rcClip
        )
    {
        return CBaseClipStack::PushExact(rcClip);
    }

    void Top(__out_ecount(1) CMilRectF *prcClip)
    {
        CBaseClipStack::Top(prcClip);
    }
};


template <typename CoordSpace>
class CClipStack : public CBaseClipStack
{
public:
    HRESULT Push(
        __in_ecount(1) const CRectF<CoordSpace> &rcClip
        )
    {
        return CBaseClipStack::Push(rcClip);
    }

    HRESULT PushExact(
        __in_ecount(1) const CRectF<CoordSpace> &rcClip
        )
    {
        return CBaseClipStack::PushExact(rcClip);
    }

    void Top(__out_ecount(1) CRectF<CoordSpace> *prcClip)
    {
        CBaseClipStack::Top(prcClip);
    }
};


