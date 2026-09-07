// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Matrix Transform Stack
//
//-----------------------------------------------------------------------------

#pragma once

class CBaseMatrixStack
{
protected:
    // protected ctor to prevent direct use
    CBaseMatrixStack() {}

    // This method pre-multiplies the current top matrix by the incoming matrix, and
    // pushes the result.
    HRESULT Push(__in_ecount(1) const CMILMatrix* pMatrix, bool multiply = true);

    // This method *post*-offsets the current top matrix by give offsets, and
    // pushes the result.
    HRESULT PushPostOffset(
        float rPostOffsetX,
        float rPostOffsetY
        );

    HRESULT PushOffset(float offsetX, float offsetY);

    VOID Top(__out_ecount(1) CBaseMatrix* pMatrix);

    __outro_ecount_opt(1)
    CBaseMatrix const *
        GetTopByReference() const;

public:
    VOID Pop();

    VOID Clear() { m_matrixStack.Clear(); }
    BOOL IsEmpty() const { return m_matrixStack.IsEmpty(); }
    VOID Optimize() { m_matrixStack.Optimize(); }

    UINT GetSize() { return m_matrixStack.GetSize(); }    

private:
    CWatermarkStack<CBaseMatrix, 8 /* MinCapacity */, 2 /* GrowFactor */, 8 /* TrimCount */> m_matrixStack;
};


class CGenericMatrixStack : public CBaseMatrixStack
{
public:

    // This method pre-multiplies the current top matrix by the incoming matrix, and
    // pushes the result.
    HRESULT Push(__in_ecount(1) const CMILMatrix* pMatrix, bool multiply = true)
    {
        return CBaseMatrixStack::Push(pMatrix, multiply);
    }

    // This method *post*-offsets the current top matrix by give offsets, and
    // pushes the result.
    HRESULT PushPostOffset(
        float rPostOffsetX,
        float rPostOffsetY
        )
    {
        return CBaseMatrixStack::PushPostOffset(rPostOffsetX, rPostOffsetY);
    }

    HRESULT PushOffset(float offsetX, float offsetY)
    {
        return CBaseMatrixStack::PushOffset(offsetX, offsetY);
    }

    VOID Top(__out_ecount(1) CMILMatrix* pMatrix)
    {
        CBaseMatrixStack::Top(pMatrix);
    }

};

template <typename InCoordSpace, typename OutCoordSpace>
class CMatrixStack : public CBaseMatrixStack
{
public:

    // This method pre-multiplies the current top matrix by the incoming matrix, and
    // pushes the result.
    HRESULT Push(__in_ecount(1) const CMILMatrix* pMatrix, bool multiply = true)
    {
        return CBaseMatrixStack::Push(pMatrix, multiply);
    }

    // This method *post*-offsets the current top matrix by given offsets, and
    // pushes the result.
    HRESULT PushPostOffset(
        float rPostOffsetX,
        float rPostOffsetY
        )
    {
        return CBaseMatrixStack::PushPostOffset(rPostOffsetX, rPostOffsetY);
    }

    HRESULT PushOffset(float offsetX, float offsetY)
    {
        return CBaseMatrixStack::PushOffset(offsetX, offsetY);
    }

    VOID Top(__out_ecount(1) CMatrix<InCoordSpace,OutCoordSpace> *pMatrix)
    {
        CBaseMatrixStack::Top(pMatrix);
    }

    __outro_ecount_opt(1)
    CMatrix<InCoordSpace,OutCoordSpace> const *GetTopByReference() const
    {
        return CMatrix<InCoordSpace,OutCoordSpace>::ReinterpretBase
            (CBaseMatrixStack::GetTopByReference());
    }


};


