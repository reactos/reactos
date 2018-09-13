//+------------------------------------------------------------------------
//
//  File:       ctrlutil.hxx
//
//  Contents:   Random decls & functions used by all/most of the CD controls
//
//  History:    28-Sep-93   SumitC      Created
//               8-Oct-93   SumitC      Merged in DonCl's global defn stuff
//              21-Mar-94   DonCl       WBSTR->BSTR
//              27-Apr-94   SumitC      Added DELEGATE_TO_CONTROLBASECLASS* macros
//
//-------------------------------------------------------------------------

#ifndef I_CTRLUTIL_HXX_
#define I_CTRLUTIL_HXX_
#pragma INCMSG("--- Beg 'ctrlutil.hxx'")

#ifndef X_CTRLCNST_HXX_
#define X_CTRLCNST_HXX_
#include "ctrlcnst.hxx"
#endif

//+------------------------------------------------------------------------
//  Function:   CTargetDcHelper
//
//  Notes:      This is used to get a dc from a DVTARGETDEVICE and a HIC
//
//-------------------------------------------------------------------------

class CTargetDcHelper
{
public:

    CTargetDcHelper ( void )
        : _hdcPrinter( 0 ), _hdcScreen( 0 ), _hdc( 0 ) { }

    ~ CTargetDcHelper ( void )
    {
        // Make sure we have been deinitialized.  I'd like to do this in the
        // destructor, but forms^3 code style prevents me from doing so.
        Assert( !_hdcPrinter && !_hdcScreen );
    }

    HRESULT Init ( DVTARGETDEVICE * ptd, HDC hicTargetDev )
    {
        if (ptd && hicTargetDev)
        {
            _hdc = _hdcPrinter = ::CreateDC(
                PTCHAR(PCHAR(ptd) + ptd->tdDriverNameOffset),
                PTCHAR(PCHAR(ptd) + ptd->tdDeviceNameOffset),
                0, 0 );
        }
        else
        {
            _hdc = _hdcScreen = TLS(hdcDesktop);
        }

        return _hdc ? S_OK : E_FAIL;
    }

    void DeInit ( void )
    {
        if (_hdcPrinter) { ::DeleteDC( _hdcPrinter ); _hdcPrinter = 0; }
        if (_hdcScreen)  { _hdcScreen = 0; }
    }

    operator HDC ( void ) { return _hdc; }

    HDC _hdc;

private:

    HDC _hdcPrinter;
    HDC _hdcScreen;
};

//  Global data

extern LONG g_cxCtrlDef;
extern LONG g_cyCtrlDef;
extern LONG g_cyLstBox;
extern LONG g_cxHScroll;
extern LONG g_cyHScroll;
extern LONG g_cxVScroll;
extern LONG g_cyVScroll;


//
//      Defines applicable to all controls
//

#define WS_BASECONTROLSTYLE (WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS)


//
//  Macro for delegating property implementations to the ControlBase classes
//

#define DELEGATE_PROP_IMPL_TO_BASECLASS(_CtlClass_, _PropType_, _PropName_) \
                                                                            \
STDMETHODIMP _CtlClass_::Get##_PropName_(_PropType_ * p##_PropName_)        \
{ return CWrappedControl::Get##_PropName_(p##_PropName_); }                        \
                                                                            \
STDMETHODIMP _CtlClass_::Set##_PropName_(_PropType_ _PropName_)             \
{ return CWrappedControl::Set##_PropName_(_PropName_); }


#if DBG == 1
//
//      prototypes for functions in .cxx file
//
HRESULT EditProperties(void * pv, HWND hWnd);
HRESULT PutUpContextMenu(HWND hWnd, POINT pt);
HRESULT FireMethods(void *pv, int i, HWND hWnd);

#endif

//-------------------------------------------------------------------------
//
// Command Bar Drawing Helpers
//
// CONSIDER (rodc): Move these definitions to a better location when this
// code is generalized.  For now, it's only used by the Toolbox.
//

//
// This is the style of the command bar button to be drawn.
//
enum CMDBARSTYLE
{
    cbsNormal       = 0,
    cbsHighlight    = 1,
    cbsDown1        = 2,
    cbsDown2        = 3,
    cbsChecked      = 4
};

//
// These functions draw the different parts of a command bar button.
//
void    CmdBarDrawBtnEdge(HDC hdc, RECT * prc, CMDBARSTYLE cbs);
void    CmdBarDrawBtnFace(HDC hdc, RECT * prc, CMDBARSTYLE cbs);
void    CmdBarDrawBtnPict(HDC hdc, RECT * prc, CMDBARSTYLE cbs, HBITMAP hbmp);

#pragma INCMSG("--- End 'ctrlutil.hxx'")
#else
#pragma INCMSG("*** Dup 'ctrlutil.hxx'")
#endif
