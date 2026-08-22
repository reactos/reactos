// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


///////////////////////////////////////////////////////////////////////////////

//
// pixshade.cpp
//
// Direct3D Reference Device - Pixel Shader
//
///////////////////////////////////////////////////////////////////////////////
#include "precomp.h"

#pragma warning(disable:4189)

//-------------------------------------------------------------------------
//
//  Function:   RDPSTrans::RDPSTrans
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
RDPSTrans::RDPSTrans(DWORD* pCode, DWORD dwByteCodeSize, DWORD dwFlags)
    : CPSTrans()
{
    m_cInstructionDataSize = 0;
    m_pInstructionData = NULL; 
    m_cAllocated = 0; 

    Initialize(pCode, dwByteCodeSize, dwFlags);
}

//-------------------------------------------------------------------------
//
//  Function:   RDPSTrans::~RDPSTrans
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
RDPSTrans::~RDPSTrans()
{
    delete [] m_pInstructionData;
}

//-------------------------------------------------------------------------
//
//  Function:   RDPSTrans::SetOutputBufferGrowSize
//
//  Synopsis:
//     Ignored
//
//-------------------------------------------------------------------------
void 
RDPSTrans::SetOutputBufferGrowSize(DWORD dwGrowSize)
{
}

//-------------------------------------------------------------------------
//
//  Function:   RDPSTrans::GrowOutputBuffer
//
//  Synopsis:
//     Grow the output buffer
//
//-------------------------------------------------------------------------
HRESULT 
RDPSTrans::GrowOutputBuffer(DWORD dwNewSize)
{
    XRESULT hr = S_OK;

    if (dwNewSize > m_cAllocated)
    {
        m_cAllocated = dwNewSize + 4096;

        BYTE *pbNewBuffer = new BYTE[m_cAllocated];
        IFCOOM(pbNewBuffer);

        if (m_cInstructionDataSize > 0)
        {
            memcpy(pbNewBuffer, m_pInstructionData, m_cInstructionDataSize);
        }
        delete m_pInstructionData;
        m_pInstructionData =  pbNewBuffer;

    }

    m_cInstructionDataSize = dwNewSize;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   RDPSTrans::GetOutputBufferI
//
//  Synopsis:
//     Return the instruction buffer
//
//-------------------------------------------------------------------------
BYTE* 
RDPSTrans::GetOutputBufferI()
{
    return m_pInstructionData;
}



