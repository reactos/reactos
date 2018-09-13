// colorsrc.h : main header file for the PBRUSH application
//

#ifndef __COLORSRC_H__
#define __COLORSRC_H__

/******************************************************************************/

class CColors : public CObject
    {
    DECLARE_DYNCREATE( CColors )

    public:

    CColors();
   ~CColors();

    enum { MAXCOLORS = 256 };

    private:

    COLORREF*   m_colors;
    COLORREF*   m_monoColors;
    int         m_nColorCount;
    BOOL        m_bMono;

    public:

    void        SetMono ( BOOL bMono = TRUE );
    COLORREF    GetColor( int nColor );
    void        SetColor( int nColor, COLORREF color );

    int         GetColorCount() const { return m_nColorCount; }
    BOOL        GetMonoFlag  () const { return m_bMono; }

    void        EditColor( BOOL bLeft, BOOL bTrans );
    void        ResetColors(int nColors=256);
    void        CmdEditColor();
    #if 0 // unused lame features
    void        CmdLoadColors();
    void        CmdSaveColors();
    #endif
    };

/******************************************************************************/

extern CColors* g_pColors;

#endif
