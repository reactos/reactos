//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       csite.hxx
//
//  Contents:   CSite and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_CSITE_HXX_
#define I_CSITE_HXX_
#pragma INCMSG("--- Beg 'csite.hxx'")

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#define _hxx_
#include "csite.hdl"

#define COLORREF_NONE           ((COLORREF)(0xFFFFFFFF))
#define FDESIGN     0x20    // Keystroke is design mode only
#define FRUN        0x40    // Keystroke is runmode only
#define FDISPATCH   0x80    // do also dispatch message on this key

#define MAX_BORDER_SPACE  1000

//
// Forward declarations
//

class CSite;
class CBaseBag;
class CDoc;
class CClassTable;
class CCreateInfo;
class CFormDrawInfo;
class CLabelElement;

//+---------------------------------------------------------------------------
//
//  Class:      CBorderInfo
//
//  Purpose:    Class used to hold a collection of border information.  This is
//              really just a struct with a constructor.
//
//----------------------------------------------------------------------------
class CBorderInfo
{
public:
    CBorderInfo()            { Init(); }
    CBorderInfo(BOOL fDummy) { /* do nothing - work postponed */ }
    void Init()
    {
        memset( this, 0, sizeof(CBorderInfo) );
        // Have to set up some default widths.
        CUnitValue cuv;
        cuv.SetValue( 4 /*MEDIUM*/, CUnitValue::UNIT_PIXELS );
        aiWidths[BORDER_TOP] = aiWidths[BORDER_BOTTOM] = cuv.GetPixelValue ( NULL, CUnitValue::DIRECTION_CY, 0, 0 );
        aiWidths[BORDER_RIGHT] = aiWidths[BORDER_LEFT] = cuv.GetPixelValue ( NULL, CUnitValue::DIRECTION_CX, 0, 0 );
    }

    // The collection of border data:
    BOOL            fNotDraw;               // do not draw
    // These values reflect essentials of the border
    // (These are those needed by CSite::GetClientRect)
    BYTE            abStyles[4];            // top,right,bottom,left
    int             aiWidths[4];            // top,right,bottom,left
    WORD            wEdges;                 // which edges to draw

    // These values reflect all remaining details
    // (They are set only if GetBorderInfo is called with fAll==TRUE)
    RECT            rcSpace;                // top, bottom, right and left space
    SIZE            sizeCaption;            // location of the caption
    int             offsetCaption;          // caption offset on top
    int             xyFlat;                 // hack to support combined 3d and flat
    COLORREF        acrColors[4][3];        // top,right,bottom,left:base color, inner color, outer color
};

//+---------------------------------------------------------------------------
//
//  Class:      CSite (site)
//
//  Purpose:    Base class for all site objects used by CDoc.
//
//----------------------------------------------------------------------------

class CSite : public CElement
{
    DECLARE_CLASS_TYPES(CSite, CElement)

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:
    // Construct / Destruct

    CSite (ELEMENT_TAG etag, CDoc *pDoc);  // Normal constructor.

    virtual HRESULT Init();

    // IUnknown methods

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IControl methods

    #define _CSite_
    #include "csite.hdl"

    //
    // Helper methods
    //

    // only derived classes should call this
    virtual HRESULT CreateLayout() { Assert(FALSE); RRETURN(E_FAIL); }

    //
    // Notify site that it is being detached from the form
    //   Children should be detached and released
    //-------------------------------------------------------------------------
    //  +override : add behavior
    //  +call super : last
    //  -call parent : no
    //  +call children : first
    //-------------------------------------------------------------------------

    virtual float Detach() { return 0.0; } // to catch remaining detach impls - remove later

    struct DRAGINFO
    {
        DRAGINFO()
            {   _pBag = NULL; }
        virtual ~DRAGINFO();

        CBaseBag *  _pBag;
    };
};

int RTCCONV CompareElementsByZIndex(const void *pv1, const void *pv2);

DWORD GetBorderInfoHelper(CTreeNode * pNodeContext, CDocInfo * pdci,
             CBorderInfo *pborderinfo, BOOL fAll = FALSE);
void GetBorderColorInfoHelper(
                            CTreeNode *     pNodeContext,
                            CDocInfo *      pdci,
                            CBorderInfo *   pborderinfo);
void DrawBorder(CFormDrawInfo *pDI, LPRECT lprc, CBorderInfo *pborderinfo);

void CalcBgImgRect(CTreeNode *pNode, CFormDrawInfo * pDI,
           const SIZE * psizeObj, const SIZE * psizeImg,
           CPoint * pptBackOrg, RECT * prcBackClip);

// creates a data object containing html like "<INPUT TYPE = BUTTON>" for commands like IDM_BUTTON
HRESULT CreateHtmlDOFromIDM (UINT cmd, LPTSTR lptszParam, IDataObject ** ppHtmlDO);

HRESULT ClsidParamStrFromClsid (CLSID * pClsid, LPTSTR ptszParam, int cbParam);
HRESULT ClsidParamStrFromClsidStr (LPTSTR ptszClsid, LPTSTR ptszParam, int cbParam);

#pragma INCMSG("--- End 'csite.hxx'")
#else
#pragma INCMSG("*** Dup 'csite.hxx'")
#endif
