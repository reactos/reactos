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
//  $Description:
//      define the classes CMILGlyphRun and CD3DSubGlyph that hold information
//      required for rendering glyphs
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CD3DGlyphRun);

class CMILGlyphRun;
class CD3DSubGlyph;
class CD3DGlyphRunPainter;

//class TChain: holds a list of items
template<class T>
class TChain
{
    // no new/delete: chain exist only as a member of another class
    // that should declare DECLARE_METERHEAP_CLEAR

public:
    ~TChain() { Clean(); }

    T* GetFirst() const { return m_pList; }
    bool IsEmpty() const { return m_pList == NULL; }

    void AddAsFirst(T* p) { p->m_pNext = m_pList; m_pList = p;}

    void Clean()
    {
        while (m_pList)
        {
            T* p = m_pList;
            m_pList = p->m_pNext;
            delete p;
        }
    }

private:
    T* m_pList;
};

class CD3DSubGlyph
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CD3DGlyphRun));

    ~CD3DSubGlyph();

    HRESULT ValidateAlphaMap(CD3DGlyphRunPainter* pPainter);
    void FreeAlphaMap();

    // Accessors
    CD3DGlyphTank* GetTank() { return m_pTank; }
    IDirect3DTexture9* GetTextureNoAddref() const {Assert(m_pTank); return m_pTank->GetTextureNoAddref();}
    float GetWidTextureRc() const {Assert(m_pTank); return m_pTank->GetWidTextureRc();}
    float GetHeiTextureRc() const {Assert(m_pTank); return m_pTank->GetHeiTextureRc();}
    RECT const& GetFilteredRect() const {return m_rcFiltered;}
    SIZE const& GetOffset() const {return m_offset;}

    bool IsAlphaMapValid() const {return m_pTank != 0 && m_pTank->IsValid();}
    bool WasEvicted() const {return m_pTank != 0 && !m_pTank->IsValid();}
    CD3DSubGlyph* GetNext() const {return m_pNext;}

private:
    friend class CD3DGlyphRun;

    friend class CD3DSubGlyphChain;
    friend class TChain<CD3DSubGlyph>;

    CD3DSubGlyph *m_pNext;  // chain of subglyphs: required when the glyph is too big and need splitting

    CD3DGlyphTank* m_pTank; // container
    RECT m_rcFiltered;      // bounding rectangle in filtered space.
                            // Includes borders.
                            // Borders of neighbouring subglyphs overlap one another.
    SIZE m_offset;
    // location in the tank can be obtained by taking m_rcFiltered and shifting it by m_offset
};

class CD3DGlyphRun : public CBaseGlyphRun
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CD3DGlyphRun));

    //CD3DGlyphRun();   not required
    //~CD3DGlyphRun();  not required

    HRESULT ValidateGeometry(CD3DGlyphRunPainter* pPainter);

    CD3DSubGlyph* GetFirstSubGlyph() const { return m_subglyphs.GetFirst(); }

    void DiscardAlphaArrayAndResources();

private:
    HRESULT MakeGeometry(CD3DGlyphRunPainter* pPainter);

    TChain<CD3DSubGlyph> m_subglyphs;

    UINT64 m_uCacheSignature;

};

#define DX9_SUBGLYPH_OVERLAP_X 3
#define DX9_SUBGLYPH_OVERLAP_Y 1


