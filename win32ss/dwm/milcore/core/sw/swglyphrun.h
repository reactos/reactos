// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CSWGlyphRun);

class CSWGlyphRun : public CBaseGlyphRun
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSWGlyphRun));

    CSWGlyphRun(){}

    ~CSWGlyphRun()
    {
        DiscardAlphaArray();
    }

    __xcount(size known only to caller) BYTE* GetAlphaArray() const {return m_pAlphaArray;}

    HRESULT Validate(
        __inout_ecount(1) CSWGlyphRunPainter* pPainter
        );

    void DiscardAlphaArray();

private:
    BYTE* m_pAlphaArray;
    UINT32 m_alphaArraySize;
};


