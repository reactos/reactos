#ifndef _NEW_WSBCONTROL_H
#define _NEW_WSBCONTROL_H

//
//  When a screen reader is running, we switch to system metrics rather
//  than using the app metrics.  All the metrics are kept in this structure
//  so we can switch between them easily.
//
typedef struct WSBMETRICS {
    int cxVSBArrow;     //  x size of arrow for vertical scrollbar.
    int cxHSBArrow;
    int cxHSBThumb;

    int cyVSBArrow;
    int cyHSBArrow;
    int cyVSBThumb;

} WSBMETRICS, *PWSBMETRICS;

typedef struct WSBState {
    PWSBMETRICS pmet;       // The metrics that are active
    int style;          //  Win style.
                            //  px: Current coord. Used in Tracking.
    int px;                 //  Mouse message coord.
    int pxStart;            //  back to pxStart if tracking out of box.
    int dpxThumb;           //  pxThumbTop - px
    int pxBottom;       
    int pxDownArrow;
    int pxLeft;
    int pxOld;
    int pxRight;
    int pxThumbBottom;
    int pxThumbTop;
    int pxTop;
    int pxUpArrow;
    int cpxThumb;           //  cpx: Current size.
    int cpxArrow;
    int cpxSpace;

    int cmdSB;              //  Current scroll command.
    int posOld;             //  Thumb pos of last time.
    int posNew;             //  To support GetScrollInfo with SIF_TRACKPOS
    int posStart;           //  Thumb pos when we start tracking.

    void ( * pfnSB )(struct WSBState *, int, WPARAM, LPARAM);
    BOOL    fVertSB;        //  This variable shows if the last valid 
                            //  computation is on Vertical SB.
    BOOL    fHitOld;
    BOOL    fTrackVert;     //  This variable shows which scrollbar we are
                            //  tracking.
    BOOL    fTracking;      //  Critical section lock for locMouse.

    BOOL    fVActive;       //  Is mouse hovering on vertical SB?
    BOOL    fHActive;
    int     fInDoScroll;    //  Are we in the middle of a DoScroll?

    UINT_PTR hTimerSB;
    UINT_PTR hTrackSB;

    RECT rcSB;
    RECT rcClient;
    RECT rcTrack;
    
    int vStyle;             //  Style.
    int hStyle;

#define WSB_MOUSELOC_OUTSIDE    0
#define WSB_MOUSELOC_ARROWUP    1
#define WSB_MOUSELOC_ARROWDN    2
#define WSB_MOUSELOC_V_THUMB    3
#define WSB_MOUSELOC_V_GROOVE   4
#define WSB_MOUSELOC_ARROWLF    5
#define WSB_MOUSELOC_ARROWRG    6
#define WSB_MOUSELOC_H_THUMB    7
#define WSB_MOUSELOC_H_GROOVE   8

    POINT ptMouse;          //  to left-top corner of window
    int locMouse;

    COLORREF col_VSBBkg;
    COLORREF col_HSBBkg;
    HBRUSH hbr_VSBBkg;
    HBRUSH hbr_HSBBkg;
    HBRUSH hbr_Bkg;
    HBITMAP hbm_Bkg;
    HPALETTE hPalette;
    HWND sbHwnd;

    int sbFlags;
    int sbHMinPos;
    int sbHMaxPos;
    int sbHPage;
    int sbHThumbPos;
    int sbVMinPos;
    int sbVMaxPos;
    int sbVPage;
    int sbVThumbPos;
    int sbGutter;

    //
    //  Since OLEACC assumes that all scrollbars are the standard size,
    //  we revert to normal-sized scrollbars when a screenreader is running.
    //  The pmet member tells us which of these two is the one to use.
    WSBMETRICS metApp;      // The metrics the app selected
    WSBMETRICS metSys;      // The metrics from the system
} WSBState;

//
//  These macros let you get at the current metrics without realizing that
//  they could be shunted between the app metrics and system metrics.
//
#define x_VSBArrow      pmet->cxVSBArrow
#define x_HSBArrow      pmet->cxHSBArrow
#define x_HSBThumb      pmet->cxHSBThumb
#define y_VSBArrow      pmet->cyVSBArrow
#define y_HSBArrow      pmet->cyHSBArrow
#define y_VSBThumb      pmet->cyVSBThumb

#define WSB_HORZ_LF  0x0001  // Represents the Left arrow of the horizontal scroll bar.
#define WSB_HORZ_RT  0x0002  // Represents the Right arrow of the horizontal scroll bar.
#define WSB_VERT_UP  0x0004  // Represents the Up arrow of the vert scroll bar.
#define WSB_VERT_DN  0x0008  // Represents the Down arrow of the vert scroll bar.

#define WSB_VERT   (WSB_VERT_UP | WSB_VERT_DN)
#define WSB_HORZ   (WSB_HORZ_LF | WSB_HORZ_RT)

#define LTUPFLAG    0x0001
#define RTDNFLAG    0x0002
#define WFVPRESENT  0x00000002L
#define WFHPRESENT  0x00000004L

#define SBO_MIN     0
#define SBO_MAX     1
#define SBO_PAGE    2
#define SBO_POS     3   

#define TestSTYLE(STYLE, MASK) ((STYLE) & (MASK))
#define DMultDiv(A, B, C)   (((C) == 0)? (A):(MulDiv((A), (B), (C))))

#define VMODE(WSTATE)   ((WSTATE)->vStyle == FSB_FLAT_MODE) ? (WSTATE)->vMode \
                                :(((WSTATE)->vStyle == FSB_ENCARTA_MODE)?   \
                                WSB_2D_MODE : WSB_3D_MODE))
#define HMODE(WSTATE)   ((WSTATE)->hStyle == FSB_FLAT_MODE) ? (WSTATE)->hMode \
                                :(((WSTATE)->hStyle == FSB_ENCARTA_MODE)?   \
                                WSB_2D_MODE : WSB_3D_MODE))
#define ISINACTIVE(WSTATE) ((WSTATE) == WSB_UNINIT_HANDLE || (WSTATE)->fScreenRead)


//  This IDSYS_SCROLL has the same value we used in 'user' code.
#define IDSYS_SCROLL    0x0000FFFEL
//  Following ID is for tracking. I hope it won't conflict with 
//  interest of anybody else.
//  IDWSB_TRACK is now following the ID_MOUSExxxx we use in TrackMe.c
#define IDWSB_TRACK     0xFFFFFFF2L

#define MINITHUMBSIZE       10

#define WSB_SYS_FONT        TEXT("MARLETT")

#define WSB_UNINIT_HANDLE   ((WSBState *)-1)

#endif  //  _NEW_WSBCONTROL_H
