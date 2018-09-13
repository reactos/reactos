
//+------------------------------------------------------------------------
//
//  File:       color3d.hxx
//
//  Contents:   Declarations for 3D color synthesis
//
//  History:    21-Mar-95   EricVas  Created
//
//-------------------------------------------------------------------------

#ifndef I_3DCOLOR_HXX_
#define I_3DCOLOR_HXX_
#pragma INCMSG("--- Beg 'color3d.hxx'")

//+---------------------------------------------------------------------------
//
//  Class:      ThreeDColors (c3d)
//
//  Purpose:    Synthesizes 3D beveling colors
//
//  History:    21-Mar-95   EricVas      Created
//
//----------------------------------------------------------------------------

#define DEFAULT_BASE_COLOR OLECOLOR_FROM_SYSCOLOR(COLOR_BTNFACE)

class ThreeDColors
{
public:
    enum INITIALIZER {NO_COLOR};
    ThreeDColors ( INITIALIZER ) {}
    
    ThreeDColors ( OLE_COLOR coInitialBaseColor = DEFAULT_BASE_COLOR )
    {
        SetBaseColor( coInitialBaseColor );
    }

    void SetBaseColor ( OLE_COLOR );
    void NoDither();

        //
        // Use these to fetch the synthesized colors
        //

    COLORREF BtnFace      ( void );
    COLORREF BtnLight     ( void );
    COLORREF BtnShadow    ( void );
    COLORREF BtnHighLight ( void );
    COLORREF BtnDkShadow  ( void );
    
    virtual COLORREF BtnText      ( void );

        //
        // Use these to fetch cached brushes (must call ReleaseCachedBrush)
        // when finished.
        //

    HBRUSH BrushBtnFace      ( void );
    HBRUSH BrushBtnLight     ( void );
    HBRUSH BrushBtnShadow    ( void );
    HBRUSH BrushBtnHighLight ( void );

    virtual HBRUSH BrushBtnText ( void );
    
private:

    BOOL _fUseSystem;

    COLORREF _coBtnFace;
    COLORREF _coBtnLight;
    COLORREF _coBtnShadow;
    COLORREF _coBtnHighLight;
    COLORREF _coBtnDkShadow;
};

class CYIQ {
public:
    CYIQ (COLORREF c)
    {
        RGB2YIQ(c);
    }
    
    inline void RGB2YIQ (COLORREF c);
    inline void YIQ2RGB (COLORREF *pc);

    // Data members
    int _Y, _I, _Q;

    // Constants
    enum {MAX_LUM=255, MIN_LUM=0};
};

void ContrastColors (COLORREF &c1, COLORREF &c2, int LumDiff);

#pragma INCMSG("--- End 'color3d.hxx'")
#else
#pragma INCMSG("*** Dup 'color3d.hxx'")
#endif
