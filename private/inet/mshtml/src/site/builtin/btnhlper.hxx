//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       btnhelp.hxx
//
//  Contents:   the button helper class, used by the commandbutton, checkbox,
//              option button, and toggle button.
//
//  Classes:    CButtonHelper
//
//  Functions:
//
//----------------------------------------------------------------------------

#ifndef I_BTNHLPER_HXX_
#define I_BTNHLPER_HXX_
#pragma INCMSG("--- Beg 'btnhlper.hxx'")

class CRect;

//+---------------------------------------------------------------------------
//
//  Glyph is what we're calling the "check" part of a checkbox
//
//----------------------------------------------------------------------------

typedef enum
{
    GLYPHSTYLE_NONE     = 0,
    GLYPHSTYLE_CHECK    = 1,
    GLYPHSTYLE_OPTION   = 2
}
GLYPHSTYLE;


#ifdef NEVER
//+---------------------------------------------------------------------------
//
//  Constants for himetric values of metrics that need to be zoomed
//
//----------------------------------------------------------------------------

const int HI_BORDERMARGIN   = 26;
const int HI_SEPARATOR      = 26;
const int HI_GLYPHSIZE      = 13 * 26;

const int HI_HAIRLINE       = 26;
#endif // NEVER

//+---------------------------------------------------------------------------
//
//  Bit flags for keeping track of which agency pressed the button.
//
//  The button is either in a pressed or an unpressed state.  This affects how
//  it is drawn and, when it changes from pressed to unpressed the control's
//  value will change (if it holds a value).
//
//  The button will act as pressed when the user clicks on it, presses the
//  space bar while it is UI active, presses a mnemonic, or assigns a new
//  value to the Value property.
//
//  Most of these sources of "pressing" cause ui reaction, but not all.  See
//  the HasVisibleEffect procedure to determine which are which.
//
//  It is possible to redundantly press the button, for example pressing the
//  space space bar while holding the left mouse button down over the control.
//  In this case, the button is not released until all sources of pressing are
//  released.
//
//  Clicking the mouse over the button sets its pressed flag, PRESSED_MOUSE, but
//  this remains set only while the mouse remains over the button.  We capture
//  the mouse and update this flag whenever the mouse pointer enters or exits
//  the space above the button.  Note that clearing this flag, in this
//  instance, does not imply that the button is "released," rather that, if it
//  is released while the corresponding flag is set, then we take action.
//
//  2/11/97
//
//  Now, we extend this flag to contain all button status
//
//
//  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//  | f | e | d | c | b | a | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
//   |    |   |   | |<-     +     ->|             |           |   |
//   |    |   |   |         V                     |           |   |
//   |    |   |   |         Pressed status        |           |   |
//   |    |   |   V                               |           |   |
//   |    |   |   double click flag               |           |   |
//   |    |   V                                   |           |   |
//   |    |   tristate                            |           |   |
//   |                                            |           |   V
//   |                                            |           V   MK_LBUTTON
//   Has focus?                                   V           MK_RBUTTON
//                                                MK_MBUTTON
//        reserved
//----------------------------------------------------------------------------
#define    PRESSED_LBUTTON         MK_LBUTTON       // pressed left button
#define    PRESSED_RBUTTON         MK_RBUTTON       // pressed right button
#define    PRESSED_MBUTTON         MK_MBUTTON       // pressed middle button
#define    PRESSED_ASSIGNVALUE     0x0100           // Assign new Value property.
#define    PRESSED_MNEMONIC        0x0200           // Type mnemonic
#define    PRESSED_KEYBOARD        0x0400           // Press spacebar.
#define    PRESSED_MOUSE           0x0800           // Click left mouse button.
#define    FLAG_DOUBLECLICK        0x1000           // double click on/off
#define    FLAG_TRISTATE           0x2000           // tri state on/off
#define    FLAG_HASFOCUS           0x8000           // control has the focus?

//Helper macros

// reset button status bits
#define BTN_RESETSTATUS(status)     status &= FLAG_TRISTATE

// pressed?
#define BTN_PRESSED(status)         (status & (PRESSED_ASSIGNVALUE|PRESSED_MNEMONIC|PRESSED_KEYBOARD|PRESSED_MOUSE))

//
#define BTN_PRESSMIDDLE(status)     status |= PRESSED_MBUTTON
#define BTN_PRESSLEFT(status)       status |= PRESSED_LBUTTON
#define BTN_PRESSRIGHT(status)      status |= PRESSED_RBUTTON

// set status bit
#define BTN_SETSTATUS(status, flag) (status | flag)

// get status bit
#define BTN_GETSTATUS(status, flag) (status & flag)

// reset status bit
#define BTN_RESSTATUS(status, flag) (status & ~flag)

// reverse status bit
#define BTN_REVSTATUS(status, flag) (status ^ flag)

// Get button press status
#define BTN_GETPRESSSTATUS(status) (status & (PRESSED_LBUTTON | PRESSED_MBUTTON | PRESSED_RBUTTON))

class CElement;

class CBtnHelper
{
public:

    // drawing and layout

#ifdef NEVER
    HRESULT BtnRender(CDrawInfo * pDI, const RECT * prc, BOOL fFocus);

    HRESULT BtnAdjustRectForPressedAndDefault(RECT * prc);

    HRESULT BtnAdjustRectlForGlyph(RECTL * prcl);
    HRESULT BtnAdjustRectForGlyph(RECT * prc);



    // CallBacks

    virtual BOOL            BtnTakesFocus()         { return TRUE; }
    virtual BOOL            BtnStaysDown(void)      { return TRUE; }
    virtual BOOL            BtnOffsetPressed(void)  { return FALSE; }
    virtual BOOL            BtnHasGlyph(void)       { return FALSE; }

    virtual OLE_TRISTATE    BtnValue(void);
    virtual HRESULT         BtnValueChanged(OLE_TRISTATE triValue);

    virtual BOOL            BtnIsTripleState(void)  { return FALSE; }
    virtual void            BtnSetValue(OLE_TRISTATE triValue)  { return S_OK; }
    virtual HRESULT         BtnGetExtent(SIZEL * sizel) { *psizel = _sizel; return S_OK; }
    virtual fmButtonEffect  BtnGetEffect(void)      { return fmButtonEffectFlat; }
    virtual fmBorderStyle   BtnGetEdgeState(void)   { return Pressed() ?
                                                            fmBorderStyleSunken :
                                                            fmBorderStyleRaised; }


    HRESULT DoLayout(CDrawInfo * pDI, const RECT * prc, SIZE * psizeSizeToFit);

    // Callback for when anything has changed in the button which would
    // jiggle the ui elements around (used for autosize)

    virtual HRESULT OnBtnRelayout ( void );
#endif

    // window messages

    HRESULT BtnHandleMessage(CMessage * pMessage);

    virtual GLYPHSTYLE      BtnStyle(void)   { return GLYPHSTYLE_NONE; }

    virtual CElement * GetElement ( void ) = 0;

    virtual void    ChangePressedLook();

    void    PressButton(WORD wWhoPressed);          // Press the button.
    void    ReleaseButton(WORD wWhoPressed, CMessage * pMessage);        // Release the button.

    WORD    _wBtnStatus;                            // Button status
    void AdjustInsetForSize(
                        CCalcInfo *     pci,
                        SIZE *          psizeText,
                        SIZE *          psize,
                        CBorderInfo *   pbInfo,
                        SIZE *          psizeFontForShortStr,
                        LONG            lCaret,
                        BOOL            fWidthNotSet,
                        BOOL            fHeightNotSet,
                        BOOL            fRightToLeft);
    CSize   _sizeInset;                             // Default inset amount (when button is not depressed)

    BOOL    GetBtnWasHidden(void)           {return _fButtonWasHidden;}
    void    SetBtnWasHidden(BOOL fHidden)   { _fButtonWasHidden = fHidden; }

protected:
    WORD    _fButtonHasCapture          :1;
    WORD    _fBtnHelperRequestsCurrency :1;
    WORD    _fButtonWasHidden           :1; // Was this button hidden?

    void    BtnTakeCapture(BOOL fTake);
    BOOL    BtnHasCapture();

private:
    //  helper methods
    BOOL    MouseIsOver(LONG x, LONG y);            // In Pixels.
    BOOL    PressedLooksDifferent(WORD wWhoPressed);// True if pressed looks different.

//    BOOL            Pressed();
//    OLE_TRISTATE    NextValue(void);

};

#pragma INCMSG("--- End 'btnhlper.hxx'")
#else
#pragma INCMSG("*** Dup 'btnhlper.hxx'")
#endif
