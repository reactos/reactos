#ifndef _WIN32K_SCROLL_H
#define _WIN32K_SCROLL_H

typedef struct tagSBCALC
{
    INT posMin;
    INT posMax;
    INT page;
    INT pos;
    INT pxTop;
    INT pxBottom;
    INT pxLeft;
    INT pxRight;
    INT cpxThumb;
    INT pxUpArrow;
    INT pxDownArrow;
    INT pxStart;
    INT pxThumbBottom;
    INT pxThumbTop;
    INT cpx;
    INT pxMin;
} SBCALC, *PSBCALC;

typedef VOID (NEAR NTAPI *PFN_SCROLLBAR)(PWND, UINT, WPARAM, LPARAM, PSBCALC);

typedef struct tagSBTRACK
{
    ULONG    fHitOld:1;
    ULONG    fTrackVert:1;
    ULONG    fCtlSB:1;
    ULONG    fTrackRecalc:1;
    PWND     spwndTrack;
    PWND     spwndSB;
    PWND     spwndSBNotify;
    RECT     rcTrack;
    PFN_SCROLLBAR xxxpfnSB;
    UINT     cmdSB;
    UINT_PTR hTimerSB;
    INT      dpxThumb;
    INT      pxOld;
    INT      posOld;
    INT      posNew;
    INT      nBar;
    PSBCALC  pSBCalc;
} SBTRACK, *PSBTRACK;


typedef struct _SBINFOEX
{
  SCROLLBARINFO ScrollBarInfo;
  SCROLLINFO ScrollInfo;
} SBINFOEX, *PSBINFOEX;

#define IntGetScrollbarInfoFromWindow(Window, i) \
  ((PSCROLLBARINFO)(&((Window)->pSBInfo + i)->ScrollBarInfo))

#define IntGetScrollInfoFromWindow(Window, i) \
  ((LPSCROLLINFO)(&((Window)->pSBInfo + i)->ScrollInfo))

#define SBOBJ_TO_SBID(Obj)	((Obj) - OBJID_HSCROLL)
#define SBID_IS_VALID(id)	(id == SB_HORZ || id == SB_VERT || id == SB_CTL)

BOOL FASTCALL co_IntCreateScrollBars(PWINDOW_OBJECT Window);
BOOL FASTCALL IntDestroyScrollBars(PWINDOW_OBJECT Window);

#endif /* _WIN32K_SCROLL_H */
