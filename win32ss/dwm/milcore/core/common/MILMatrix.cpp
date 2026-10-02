// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of the general matrix class used by the MIL.
//      This class derives from CBaseMatrix which derives from D3DMATRIX, and
//      adds no additional data to the memory footprint.  This is done to
//      maximize interchangeability between matrix classes and minimize
//      overhead.
//

#include "precomp.hpp"
#include <strsafe.h>

using namespace dxlayer;

MtDefine(CMILMatrix, MILApi, "CMILMatrix");
MtExtern(matrix_t_get_scaling);

const CMILMatrix IdentityMatrix(true);


/**************************************************************************\
*
* Function Description:
*
*   Infer an affine transformation matrix
*   from a rectangle-to-rectangle mapping
*
* Arguments:
*
*   [IN] destRect - Specifies the destination rectangle
*   [IN] srcRect  - Specifies the source rectangle
*
* Created:
*
*   3/10/1999 DCurtis
*
\**************************************************************************/

void CMILMatrix::InferAffineMatrix(
    __in_ecount(1) const MilPointAndSizeF &destRect,
    __in_ecount(1) const MilPointAndSizeF &srcRect
    )
{
    SetToIdentity();

    // Division by zero is okay
    _11 = destRect.Width / srcRect.Width;
    _22 = destRect.Height / srcRect.Height;
    _41 = destRect.X - (_11 * srcRect.X);
    _42 = destRect.Y - (_22 * srcRect.Y);

}


BOOL CMILMatrix::Invert()
{
    bool success = false;
    try
    {
        *this = this->inverse();
        success = true;
    }
    catch (dxlayer_exception)
    {
        // do nothing
    }

    return success ? TRUE : FALSE;
}


//+------------------------------------------------------------------------
//
//  Function:  MILMatrixAdjoint
//
//  Synopsis:  Computes the adjoint of a matrix.  Returns a pointer to the
//             output matrix so that the result of the function can be used in
//             an expression.
//------------------------------------------------------------------------
__out_ecount(1) CMILMatrix *
MILMatrixAdjoint(
    __out_ecount(1) CMILMatrix *pOut,
    __in_ecount(1) const CMILMatrix *pM)
{

    // XXXlorenmcq - The code was designed to work on a processor with more
    //  than 4 general-purpose registers.  Is there a more optimal way of
    //  doing this on X86?

    float fX00, fX01, fX02;
    float fX10, fX11, fX12;
    float fX20, fX21, fX22;
    float fX30, fX31, fX32;
    float fY01, fY02, fY03, fY12, fY13, fY23;
    float fZ02, fZ03, fZ12, fZ13, fZ22, fZ23, fZ32, fZ33;

#define fX03 fX01
#define fX13 fX11
#define fX23 fX21
#define fX33 fX31
#define fZ00 fX02
#define fZ10 fX12
#define fZ20 fX22
#define fZ30 fX32
#define fZ01 fX03
#define fZ11 fX13
#define fZ21 fX23
#define fZ31 fX33
#define fDet fY01
#define fRcp fY02

    // read 1st two columns of matrix
    fX00 = pM->_11;
    fX01 = pM->_12;
    fX10 = pM->_21;
    fX11 = pM->_22;
    fX20 = pM->_31;
    fX21 = pM->_32;
    fX30 = pM->_41;
    fX31 = pM->_42;

    // compute all six 2x2 determinants of 1st two columns
    fY01 = fX00 * fX11 - fX10 * fX01;
    fY02 = fX00 * fX21 - fX20 * fX01;
    fY03 = fX00 * fX31 - fX30 * fX01;
    fY12 = fX10 * fX21 - fX20 * fX11;
    fY13 = fX10 * fX31 - fX30 * fX11;
    fY23 = fX20 * fX31 - fX30 * fX21;

    // read 2nd two columns of matrix
    fX02 = pM->_13;
    fX03 = pM->_14;
    fX12 = pM->_23;
    fX13 = pM->_24;
    fX22 = pM->_33;
    fX23 = pM->_34;
    fX32 = pM->_43;
    fX33 = pM->_44;

    // compute all 3x3 cofactors for 2nd two columns
    fZ33 = fX02 * fY12 - fX12 * fY02 + fX22 * fY01;
    fZ23 = fX12 * fY03 - fX32 * fY01 - fX02 * fY13;
    fZ13 = fX02 * fY23 - fX22 * fY03 + fX32 * fY02;
    fZ03 = fX22 * fY13 - fX32 * fY12 - fX12 * fY23;
    fZ32 = fX13 * fY02 - fX23 * fY01 - fX03 * fY12;
    fZ22 = fX03 * fY13 - fX13 * fY03 + fX33 * fY01;
    fZ12 = fX23 * fY03 - fX33 * fY02 - fX03 * fY23;
    fZ02 = fX13 * fY23 - fX23 * fY13 + fX33 * fY12;

    // compute all six 2x2 determinants of 2nd two columns
    fY01 = fX02 * fX13 - fX12 * fX03;
    fY02 = fX02 * fX23 - fX22 * fX03;
    fY03 = fX02 * fX33 - fX32 * fX03;
    fY12 = fX12 * fX23 - fX22 * fX13;
    fY13 = fX12 * fX33 - fX32 * fX13;
    fY23 = fX22 * fX33 - fX32 * fX23;

    // read 1st two columns of matrix
    fX00 = pM->_11;
    fX01 = pM->_12;
    fX10 = pM->_21;
    fX11 = pM->_22;
    fX20 = pM->_31;
    fX21 = pM->_32;
    fX30 = pM->_41;
    fX31 = pM->_42;

    // compute all 3x3 cofactors for 1st two columns
    fZ30 = fX11 * fY02 - fX21 * fY01 - fX01 * fY12;
    fZ20 = fX01 * fY13 - fX11 * fY03 + fX31 * fY01;
    fZ10 = fX21 * fY03 - fX31 * fY02 - fX01 * fY23;
    fZ00 = fX11 * fY23 - fX21 * fY13 + fX31 * fY12;
    fZ31 = fX00 * fY12 - fX10 * fY02 + fX20 * fY01;
    fZ21 = fX10 * fY03 - fX30 * fY01 - fX00 * fY13;
    fZ11 = fX00 * fY23 - fX20 * fY03 + fX30 * fY02;
    fZ01 = fX20 * fY13 - fX30 * fY12 - fX10 * fY23;

    // take transpose of fZ
    pOut->_11 = fZ00;
    pOut->_12 = fZ10;
    pOut->_13 = fZ20;
    pOut->_14 = fZ30;
    pOut->_21 = fZ01;
    pOut->_22 = fZ11;
    pOut->_23 = fZ21;
    pOut->_24 = fZ31;
    pOut->_31 = fZ02;
    pOut->_32 = fZ12;
    pOut->_33 = fZ22;
    pOut->_34 = fZ32;
    pOut->_41 = fZ03;
    pOut->_42 = fZ13;
    pOut->_43 = fZ23;
    pOut->_44 = fZ33;

    return pOut;
}




