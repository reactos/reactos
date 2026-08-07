// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


///////////////////////////////////////////////////////////////////////////////

//
// pshader.h
//
// Direct3D Reference Device - Pixel Shader
//
///////////////////////////////////////////////////////////////////////////////

class RDPSTrans : public CPSTrans
{
protected:
    void            SetOutputBufferGrowSize(DWORD dwGrowSize); // implementing pure method
    HRESULT         GrowOutputBuffer(DWORD dwNewSize);         // implementing pure method 
    BYTE*           GetOutputBufferI();                        // implementing pure method

public:
    RDPSTrans(DWORD* pCode, DWORD ByteCodeSize, DWORD Flags);
    ~RDPSTrans();

private:
    UINT32  m_cInstructionDataSize;   // Number of valid entries in the queue
    BYTE    *m_pInstructionData;       // Request queue
    UINT32  m_cAllocated;             // Number of allocated entries
};




