// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains implementation for stack of matrices
//

#include "precomp.hpp"
using namespace dxlayer;
//-----------------------------------------------------------------------------
//
// CBaseMatrixStack::Push
//
// Description: Pre multiplies the matrix with the matrix on the top of the stack
//              and pushes the result on the stack. If the stack is empty
//              pMatrix is pushed as the first element onto the stack.
//
//           stack empty:
//              [] => [pMatrix]
//
//           stack non-empty:
//           if (multiply)      
//              [pTopMatrix | <rest of stack>]
//                    ==> [pMatrix * pTopMatrix | pTopMatrix | <rest of stack>]
//           else
//              [pTopMatrix | <rest of stack>]
//                    ==> [pMatrix | pTopMatrix | <rest of stack> ]
//
// pMatrix  -   matrix to push
// multiply -   bool indicating whether to multiply the matrix with
//              the current stack, or push it "as is"
//-----------------------------------------------------------------------------

HRESULT
CBaseMatrixStack::Push(
    __in_ecount(1) const CMILMatrix* pMatrix,
    bool multiply
    )
{
    HRESULT hr = S_OK;


    if (m_matrixStack.IsEmpty() || !multiply)
    {
        IFC(m_matrixStack.Push(*pMatrix));
    }
    else
    {
        CMILMatrix top;
        CMILMatrix newTop;

        IFC(m_matrixStack.Top(&top));
        newTop = *pMatrix * top;
        IFC(m_matrixStack.Push(newTop));
    }
Cleanup:
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// CBaseMatrixStack::PushOffset
//
// Description: Applies the offset to the matrix on the top of the stack and
//              pushes the result onto the stack.
//
//           stack empty:
//              [] => [pMatrix]
//
//           stack non-empty:
//              [pTopMatrix | <rest of stack>]
//                    ==> [(offsetX, offsetY) * pTopMatrix | pTopMatrix | <rest of stack>]
//-----------------------------------------------------------------------------

HRESULT
CBaseMatrixStack::PushOffset(float offsetX, float offsetY)
{
    HRESULT hr = S_OK;

    if (m_matrixStack.IsEmpty())
    {
        CMILMatrix m = matrix::get_identity();
        m._41 = offsetX;
        m._42 = offsetY;
        IFC(m_matrixStack.Push(m));
    }
    else
    {
        CMILMatrix top;

        // Get the current top matrix
        
        IFC(m_matrixStack.Top(&top));

        // Modify 'top' so that it has the result which
        // will get pushed on the stack.
        // Optimization for multiplying 2 matrices:- m*top
        // where m would be identity matrix with Offset values.

        top._41 += offsetX*top._11 + offsetY*top._21;
        top._42 += offsetX*top._12 + offsetY*top._22;

        //  Assumption:- 'top' Matrix is Affine
        ASSERT(top.Is2DAffineOrNaN());
                
        IFC(m_matrixStack.Push(top));
    }
Cleanup:
    RRETURN(hr);
}



//-----------------------------------------------------------------------------
//
// CBaseMatrixStack::PushPostOffset
//
// Description: Post translates the matrix with the matrix on the top of the
//              stack and pushes the result on the stack. If the stack is empty
//              a simple translate matrix with given offsets is pushed as the
//              first element onto the stack.
//
//           stack empty:
//              [] => [Translate]
//
//           stack non-empty:
//              [pTopMatrix | <rest of stack>]
//                    ==> [pTopMatrix * Translate | pTopMatrix | <rest of stack>]
//-----------------------------------------------------------------------------

HRESULT
CBaseMatrixStack::PushPostOffset(
    float rPostOffsetX,
    float rPostOffsetY
    )
{
    HRESULT hr = S_OK;


    if (m_matrixStack.IsEmpty())
    {
        CBaseMatrix matOffset(true);
        matOffset.SetTranslation(rPostOffsetX, rPostOffsetY);
        IFC(m_matrixStack.Push(matOffset));
    }
    else
    {
        CBaseMatrix top;

        // Get a copy of current top matrix
        IFC(m_matrixStack.Top(&top));

        top.Translate(rPostOffsetX, rPostOffsetY);

        IFC(m_matrixStack.Push(top));
    }
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CBaseMatrixStack::Pop
//
// Description: Pops the matrix at the top off the stack. Returns false if the
//              stack was empty otherwise true.
//
//              [pTopMatrix | <rest of stack>] -> [ <rest of stack> ]
//-----------------------------------------------------------------------------

VOID
CBaseMatrixStack::Pop()
{
    m_matrixStack.Pop();
}


//-----------------------------------------------------------------------------
//
// CBaseMatrixStack::Top
//
// Description: Returns the matrix at the top of the stack.
//
//-----------------------------------------------------------------------------

VOID
CBaseMatrixStack::Top(
    __out_ecount(1) CBaseMatrix *pMatrix
    )
{
    if (m_matrixStack.IsEmpty())
    {
        *pMatrix = IdentityMatrix;
    }
    else
    {
        //
        // This function handles error returns from Top() by checking for 
        // empty up front. I.e. it should never fail here.
        //
        
        Verify(SUCCEEDED(m_matrixStack.Top(CMILMatrix::ReinterpretBaseForModification(pMatrix))));
    }
}


//-----------------------------------------------------------------------------
//
// CBaseMatrixStack::GetTopByReference
//
// Description: Returns a reference to the matrix at the top of the stack or
//              NULL if the stack is empty.
//
//-----------------------------------------------------------------------------

__outro_ecount_opt(1)
CBaseMatrix const *
CBaseMatrixStack::GetTopByReference(
    ) const
{
    return m_matrixStack.GetTopByReference();
}




