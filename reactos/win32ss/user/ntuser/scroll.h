#pragma once

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

/*
typedef struct _SBINFOEX
{
  SCROLLBARINFO ScrollBarInfo;
  SCROLLINFO ScrollInfo;
} SBINFOEX, *PSBINFOEX;
*/
#define IntGetScrollbarInfoFromWindow(Window, i) \
  ((PSCROLLBARINFO)(&((Window)->pSBInfoex + i)->ScrollBarInfo))

#define IntGetScrollInfoFromWindow(Window, i) \
  ((LPSCROLLINFO)(&((Window)->pSBInfoex + i)->ScrollInfo))

#define SBOBJ_TO_SBID(Obj)	((Obj) - OBJID_HSCROLL)
#define SBID_IS_VALID(id)	(id == SB_HORZ || id == SB_VERT || id == SB_CTL)

BOOL FASTCALL co_IntCreateScrollBars(PWND Window);
BOOL FASTCALL IntDestroyScrollBars(PWND Window);
