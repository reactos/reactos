// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_transform
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(TranslateTransformResource, MILRender, "TranslateTransform Resource");
MtDefine(CMilTranslateTransformDuce, TranslateTransformResource, "CMilTranslateTransformDuce");

CMilTranslateTransformDuce::~CMilTranslateTransformDuce()
{    
    UnRegisterNotifiers();
}

//-----------------------------------------------------------------------------
//
// CMilPathGeometryDuce::Create
//
//-----------------------------------------------------------------------------
HRESULT 
CMilTranslateTransformDuce::Create(
     __in_ecount(1) MilPoint2F *pTranslateBy,
    __deref_out CMilTranslateTransformDuce **ppTranslateTransform
    )
{
    HRESULT hr = S_OK;

    CMilTranslateTransformDuce *pTranslateTransform = NULL;

    IFCOOM(pTranslateTransform = new CMilTranslateTransformDuce(pTranslateBy));    
    pTranslateTransform->AddRef();

    *ppTranslateTransform = pTranslateTransform; // Transitioning ref count to out argument
    pTranslateTransform = NULL;
   
Cleanup:
    ReleaseInterface(pTranslateTransform);
    RRETURN(hr);
}

/*++

Routine Description:

    CMilTranslateTransformDuce::GetMatrixCore

--*/

HRESULT CMilTranslateTransformDuce::GetMatrixCore(
    CMILMatrix *pmat
    )
{
    HRESULT hr = S_OK;
    Assert(pmat != NULL);

    DOUBLE offsetX;
    DOUBLE offsetY;

    IFC(SynchronizeAnimatedFields());
    
    offsetX = m_data.m_X;
    offsetY = m_data.m_Y;

    *pmat = 
        matrix::get_translation(
            static_cast<FLOAT>(offsetX), 
            static_cast<FLOAT>(offsetY), 
            0.0f);

Cleanup:
    RRETURN(hr);
}

