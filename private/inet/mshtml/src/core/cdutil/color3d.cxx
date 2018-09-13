
//+------------------------------------------------------------------------
//
//  File:       color3d.cxx
//
//  Contents:   Definitions of 3d color
//
//  History:    21-Mar-95   EricVas  Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

    //
    // Colors
    //

static inline COLORREF
DoGetColor ( BOOL fUseSystem, COLORREF coColor, int lColorIndex )
{
    return fUseSystem ? GetSysColorQuick( lColorIndex ) : coColor;
}
            
COLORREF ThreeDColors::BtnFace ( void )
    { return DoGetColor( _fUseSystem, _coBtnFace, COLOR_BTNFACE ); }

COLORREF ThreeDColors::BtnLight ( void )
    { return DoGetColor( _fUseSystem, _coBtnLight, COLOR_3DLIGHT); }

COLORREF ThreeDColors::BtnShadow ( void )
    { return DoGetColor( _fUseSystem, _coBtnShadow, COLOR_BTNSHADOW ); }
    
COLORREF ThreeDColors::BtnHighLight ( void )
    { return DoGetColor( _fUseSystem, _coBtnHighLight, COLOR_BTNHIGHLIGHT ); }

COLORREF ThreeDColors::BtnDkShadow ( void )
    { return DoGetColor( _fUseSystem, _coBtnDkShadow, COLOR_3DDKSHADOW ); }
    
COLORREF ThreeDColors::BtnText ( void )
    { return DoGetColor( TRUE, 0, COLOR_BTNTEXT ); }


    //
    // Brushes
    //

    
static inline HBRUSH
DoGetBrush ( BOOL fUseSystem, COLORREF coColor )
{
    return GetCachedBrush( coColor );
}

HBRUSH ThreeDColors::BrushBtnFace ( void )
    { return DoGetBrush( _fUseSystem, BtnFace() ); }

HBRUSH ThreeDColors::BrushBtnLight ( void )
    { return DoGetBrush( _fUseSystem, BtnLight() );}

HBRUSH ThreeDColors::BrushBtnShadow ( void )
    { return DoGetBrush( _fUseSystem, BtnShadow() ); }
    
HBRUSH ThreeDColors::BrushBtnHighLight ( void )
    { return DoGetBrush( _fUseSystem, BtnHighLight() ); }
    
HBRUSH ThreeDColors::BrushBtnText ( void )
    { return DoGetBrush( TRUE, BtnText() ); }

//+------------------------------------------------------------------------
//
//  Functions:  LightenColor & DarkenColor
//
//  Synopsis:   These take RGBs and return RGBs which are "lighter" or
//              "darker"
//
//-------------------------------------------------------------------------

static unsigned Increase ( unsigned x )
{
    x = x * 5 / 3;

    return x > 255 ? 255 : x;
}

static unsigned Decrease ( unsigned x )
{
    return x * 3 / 5;
}

static COLORREF LightenColor ( COLORREF cr )
{
    return
        (Increase( (cr & 0x000000FF) >>  0 ) <<  0) |
        (Increase( (cr & 0x0000FF00) >>  8 ) <<  8) |
        (Increase( (cr & 0x00FF0000) >> 16 ) << 16);
}

static COLORREF DarkenColor ( COLORREF cr )
{
    return
        (Decrease( (cr & 0x000000FF) >>  0 ) <<  0) |
        (Decrease( (cr & 0x0000FF00) >>  8 ) <<  8) |
        (Decrease( (cr & 0x00FF0000) >> 16 ) << 16);
}

//+------------------------------------------------------------------------
//
//  Member:     ThreeDColors::SetBaseColor
//
//  Synopsis:   This is called to reestablish the 3D colors, based on a
//              single color.
//
//-------------------------------------------------------------------------

void
ThreeDColors::SetBaseColor ( OLE_COLOR coBaseColor )
{
        //
        // Sentinal color (0xffffffff) means use default which is button face
        //

    _fUseSystem = (coBaseColor & 0x80ffffff) == DEFAULT_BASE_COLOR;
    
#ifdef UNIX
    _fUseSystem = ( _fUseSystem ||
                    (CColorValue(coBaseColor).IsUnixSysColor()));
#endif

    if (_fUseSystem)
        return;

        
        //
        // Ok, now synthesize some colors! 
        //
        // First, use the base color as the button face color
        //
    
    _coBtnFace = ColorRefFromOleColor( coBaseColor );

    _coBtnLight = _coBtnFace;
        //
        // Dark shadow is always black and button face
        // (or so Win95 seems to indicate)
        //

    _coBtnDkShadow = 0;
    
        //
        // Now, lighten/darken colors
        //

    _coBtnShadow = DarkenColor( _coBtnFace );

    HDC hdc = TLS(hdcDesktop);

    if (!hdc)
        return;

    COLORREF coRealBtnFace = GetNearestColor( hdc, _coBtnFace );

    _coBtnHighLight = LightenColor( _coBtnFace );

    if (GetNearestColor( hdc, _coBtnHighLight ) == coRealBtnFace)
    {
        _coBtnHighLight = LightenColor( _coBtnHighLight );

        if (GetNearestColor( hdc, _coBtnHighLight ) == coRealBtnFace)
            _coBtnHighLight = RGB( 255, 255, 255 );
    }

    _coBtnShadow = DarkenColor( _coBtnFace );

    if (GetNearestColor( hdc, _coBtnShadow ) == coRealBtnFace)
    {
        _coBtnShadow = DarkenColor( _coBtnFace );

        if (GetNearestColor( hdc, _coBtnShadow ) == coRealBtnFace)
            _coBtnShadow = RGB( 0, 0, 0 );
    }
}
//
// Function: RGB2YIQ
//
// Parameter: c -- the color in RGB.
//
// Note: Converts RGB to YIQ. The standard conversion matrix obtained
//       from Foley Van Dam -- 2nd Edn.
//
// Returns: Nothing
//
inline void CYIQ::RGB2YIQ (COLORREF   c)
{
    int R = GetRValue(c);
    int G = GetGValue(c);
    int B = GetBValue(c);

    _Y = (30 * R + 59 * G + 11 * B + 50) / 100;
    _I = (60 * R - 27 * G - 32 * B + 50) / 100;
    _Q = (21 * R - 52 * G + 31 * B + 50) / 100;
}

//
// Function: YIQ2RGB
//
// Parameter: pc [o] -- the color in RGB.
//
// Note: Converts YIQ to RGB. The standard conversion matrix obtained
//       from Foley Van Dam -- 2nd Edn.
//
// Returns: Nothing
//
inline void CYIQ::YIQ2RGB (COLORREF *pc)
{
    int R, G, B;

    R = (100 * _Y +  96 * _I +  62 * _Q + 50) / 100;
    G = (100 * _Y -  27 * _I -  64 * _Q + 50) / 100;
    B = (100 * _Y - 111 * _I + 170 * _Q + 50) / 100;

    // Needed because of round-off errors
    if (R < 0) R = 0; else if (R > 255) R = 255;
    if (G < 0) G = 0; else if (G > 255) G = 255;
    if (B < 0) B = 0; else if (B > 255) B = 255;

    *pc = RGB(R,G,B);
}

//
// Function: Luminance
//
// Parameter: c -- The color whose luminance is to be returned
//
// Returns: The luminance value [0,255]
//
static inline int Luminance (COLORREF c)
{
    return CYIQ(c)._Y;
}

//
// Function: MoveColorBy
//
// Parameters: pcColor [i,o] The color to be moved
//             nLums         Move so that new color is this bright
//                           or darker than the original: a signed
//                           number whose abs value is betn 0 and 255
//
// Returns: Nothing
//
static void MoveColorBy (COLORREF *pcColor, int nLums)
{
    CYIQ yiq(*pcColor);
    int Y;

    Y = yiq._Y;
    
    if (Y + nLums > CYIQ::MAX_LUM)
    {
        // Cannot move more in the lighter direction.
        *pcColor = RGB(255,255,255);
    }
    else if (Y + nLums < CYIQ::MIN_LUM)
    {
        // Cannot move more in the darker direction.
        *pcColor = RGB(0,0,0);
    }
    else
    {
        Y += nLums;
        if (Y < 0)
            Y = 0;
        if (Y > 255)
            Y =255;

        yiq._Y = Y;
        yiq.YIQ2RGB (pcColor);
    }
}

//
// Function: ContrastColors
//
// Parameters: c1,c2 [i,o]: Colors to be contrasted
//             Luminance:   Contrast so that diff is luminance is atleast
//                          this much: A number in the range [0,255]
//
// IMPORTANT: If you change this function, make sure, not to change the order
//            of the colors because some callers depend it.  For example if
//            both colors are white, we need to guarantee that only the
//            first color (c1) is darkens and never the second (c2).
//
// Returns: Nothing
//
void ContrastColors (COLORREF &c1, COLORREF &c2, int LumDiff)
{
    COLORREF *pcLighter, *pcDarker;
    int      l1, l2, lLighter, lDarker;
    int      lDiff, lPullApartBy;

    Assert ((LumDiff >= CYIQ::MIN_LUM) && (LumDiff <= CYIQ::MAX_LUM));

    l1 = Luminance(c1);
    l2 = Luminance(c2);

    // If both the colors are black, make one slightly bright so
    // things work OK below ...
    if ((l1 == 0) && (l2 == 0))
    {
        c1 = RGB(1,1,1);
        l1 = Luminance (c1);
    }
    
    // Get their absolute difference
    lDiff = l1 < l2 ? l2 - l1 : l1 - l2;

    // Are they different enuf? If yes get out
    if (lDiff >= LumDiff)
        return;

    // Attention:  Don't change the order of the two colors as some callers
    // depend on this order. In case both colors are the same they need
    // to know which color is made darker and which is made lighter.
    if (l1 > l2)
    {
        // c1 is lighter than c2
        pcLighter = &c1;
        pcDarker = &c2;
        lDarker = l2;
    }
    else
    {
        // c1 is darker than c2
        pcLighter = &c2;
        pcDarker = &c1;
        lDarker = l1;
    }

    //
    // STEP 1: Move lighter color
    //
    // Each color needs to be pulled apart by this much
    lPullApartBy = (LumDiff - lDiff + 1) / 2;
    Assert (lPullApartBy > 0);
    // First pull apart the lighter color -- in +ve direction
    MoveColorBy (pcLighter, lPullApartBy);

    //
    // STEP 2: Move darker color
    //
    // Need to move darker color in the darker direction.
    // Note: Since the lighter color may not have moved enuf
    // we compute the distance the darker color should move
    // by recomuting the luminance of the lighter color and
    // using that to move the darker color.
    lLighter = Luminance (*pcLighter);
    lPullApartBy = lLighter - LumDiff - lDarker;
    // Be sure that we are moving in the darker direction
    Assert (lPullApartBy <= 0);
    MoveColorBy (pcDarker, lPullApartBy);

    //
    // STEP 3: If necessary, move lighter color again
    //
    // Now did we have enuf space to move in darker region, if not,
    // then move in the lighter region again
    lPullApartBy = Luminance (*pcDarker) + LumDiff - lLighter;
    if (lPullApartBy > 0)
    {
        MoveColorBy (pcLighter, lPullApartBy);
    }

#ifdef DEBUG    
    {
        int l1 = Luminance (c1);
        int l2 = Luminance (c2);
        int diff = l1 - l2; 
    }
#endif    

    return;
}


void
ThreeDColors::NoDither()
{
    _coBtnFace          |= 0x02000000;
    _coBtnLight			|= 0x02000000;
    _coBtnShadow        |= 0x02000000;
    _coBtnHighLight     |= 0x02000000;
    _coBtnDkShadow      |= 0x02000000;
}


