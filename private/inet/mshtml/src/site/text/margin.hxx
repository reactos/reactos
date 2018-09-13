#ifndef I_MARGIN_HXX_
#define I_MARGIN_HXX_
#pragma INCMSG("--- Beg 'margin.hxx'")

class CRecalcLinePtr;
class CDisplay;
class CMeasurer;
class CLSMeasurer;

//+------------------------------------------------------------------------
//
//  Class:      CMarginInfo
//
//  Synopsis:   Stores all the information related to margins
//
//-------------------------------------------------------------------------
class CMarginInfo
{
    friend CRecalcLinePtr;
    friend CDisplay;
    friend CMeasurer;
    friend CLSMeasurer;
    friend class CLineServices;

    // The following vars are used by recalclineptr & the measurer
    LONG _xLeftMargin;              // Display's current left margin
    LONG _xRightMargin;             // Display's current right margin
    BOOL _fAddLeftFrameMargin  : 1; // Notes if the current line needs either
    BOOL _fAddRightFrameMargin : 1; //     a left or right frame margin
    BOOL _fClearLeft           : 1; // Do we clear left  margin?
    BOOL _fClearRight          : 1; // Do we clear right margin?
    BOOL _fAutoClear           : 1; // Just clear the next thing and try again.
    
    // The following are used only by the recalclineptr.
    LONG _yLeftMargin;              // Display's current height of left margin
    LONG _yRightMargin;             // Display's current height of right margin
    LONG _yBottomLeftMargin;        // Display's lowest edge of objects aligned left
    LONG _yBottomRightMargin;       // Display's lowest edge of objects aligned right
    
public:    
    CMarginInfo() { Init(); }

    void Init()
    {
        _xLeftMargin = 0;
        _xRightMargin = 0;
        _fAddLeftFrameMargin  = FALSE;
        _fAddRightFrameMargin = FALSE;
        _fClearLeft  = FALSE;
        _fClearRight = FALSE;
        _fAutoClear = FALSE;

        _yLeftMargin        = MAXLONG;
        _yRightMargin       = MAXLONG;
        _yBottomLeftMargin  = MINLONG;
        _yBottomRightMargin = MINLONG;
    }

    BOOL HasLeftMargin()       const {return !!_xLeftMargin;}
    BOOL HasRightMargin()      const {return !!_xRightMargin;}
    BOOL HasLeftFrameMargin()  const {return HasLeftMargin()  && _fAddLeftFrameMargin;}
    BOOL HasRightFrameMargin() const {return HasRightMargin() && _fAddRightFrameMargin;}
};

#pragma INCMSG("--- End 'margin.hxx'")
#else
#pragma INCMSG("*** Dup 'margin.hxx'")
#endif
