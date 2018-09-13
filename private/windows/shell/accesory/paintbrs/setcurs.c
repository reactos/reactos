/****************************Module*Header******************************\
* Module Name: setcurs.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>
#include "port1632.h"

#ifdef PENWIN
#include <penwin.h>

extern VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL);
#endif // PENWIN

#include "pbrush.h"

extern struct csstat CursorStat;
extern TCHAR *cuArray[];
extern HDC imageDC;
extern int cursTool;
extern int theBrush;
extern HWND pbrushWnd[];
extern LPTSTR DrawCursor;

#if defined(JAPAN) || defined(KOREA)    //  added by Hiraisi  09 Apr. 1992  : jinwoo 11/9/92
extern BOOL bVertical; // This flag identifies the Vertical-Writing option
#endif

static int TurnedOff = FALSE;

int SetCursorOn(void)
{
   if (!TurnedOff)
       return FALSE;
   TurnedOff = FALSE;
   SetCursor(LoadCursor(NULL, IDC_ARROW));

   return TRUE;
}

int SetCursorOff(void)
{
   register HCURSOR oldc;

   if (CursorStat.allowed)
       return FALSE;

   TurnedOff = TRUE;
   oldc = SetCursor(NULL);

   return (int)oldc;
}

LPTSTR szPbCursor(int curnum)
{
   switch (curnum) {
      case HANDtool:
         return IDC_ARROW;
#ifdef PENWIN
      case TEXTtool:
         if (lpfnRegisterPenApp)
            return IDC_PEN;
         else
#ifdef JAPAN        // added by Hiraisi  09 Apr. 1992 : jinwoo 11/9/92
            if( bVertical )
                return (cuArray[curnum]);
            else
                return IDC_IBEAM;
#else
            return IDC_IBEAM;
#endif
#else
#ifdef JAPAN        // added by Hiraisi  09 Apr. 1992 : jinwoo 11/9/92
      case TEXTtool:
         if( bVertical )
             return (cuArray[curnum]);
         else
             return IDC_IBEAM;
#else
      return IDC_IBEAM;
#endif

#endif

      default:
         return (cuArray[curnum]) ? ((LPTSTR) cuArray[curnum]) : NULL;
   }
}

HANDLE HackCursor(HWND hWnd,int wtool, int wbrush) {
    HANDLE h;
    TCHAR acCursName[10];

    if (cuArray[wtool]) {
        h=LoadCursor(hInst,cuArray[wtool]);
        SETCLASSCURSOR(hWnd,h);
        return h;
    }

    switch(wtool) {
      case BRUSHtool:
        switch (wbrush) {
          case RECTcsr:
            wsprintf(acCursName,TEXT("rect%d"),theSize);
            h=LoadCursor(hInst,acCursName);
            SETCLASSCURSOR(hWnd,h);
            return h;
          case OVALcsr:
            wsprintf(acCursName,TEXT("oval%d"),theSize);
            h=LoadCursor(hInst,acCursName);
            SETCLASSCURSOR(hWnd,h);
            return h;
          case VERTcsr:
            wsprintf(acCursName,TEXT("vert%d"),theSize);
            h=LoadCursor(hInst,acCursName);
            SETCLASSCURSOR(hWnd,h);
            return h;
          case HORZcsr:
            wsprintf(acCursName,TEXT("horz%d"),theSize);
            h=LoadCursor(hInst,acCursName);
            SETCLASSCURSOR(hWnd,h);
            return h;
          case SLANTLcsr:
            wsprintf(acCursName,TEXT("slantl%d"),theSize);
            h=LoadCursor(hInst,acCursName);
            SETCLASSCURSOR(hWnd,h);
            return h;
          case SLANTRcsr:
            wsprintf(acCursName,TEXT("slantr%d"),theSize);
            h=LoadCursor(hInst,acCursName);
            SETCLASSCURSOR(hWnd,h);
            return h;

        }
        break;
      case LCUNDOtool:
        wsprintf(acCursName,TEXT("boxx%d"),theSize);
        h=LoadCursor(hInst,acCursName);
        SETCLASSCURSOR(hWnd,h);
        return h;
      case ERASERtool:
        wsprintf(acCursName,TEXT("box%d"),theSize);
        h=LoadCursor(hInst,acCursName);
        SETCLASSCURSOR(hWnd,h);
        return h;
      case COLORERASERtool:
        if (imagePlanes == 1 && imagePixels == 1)
            wsprintf(acCursName,TEXT("box%d"),theSize);
        else
            wsprintf(acCursName,TEXT("boxx%d"),theSize);
        h=LoadCursor(hInst,acCursName);
        SETCLASSCURSOR(hWnd,h);

        DB_OUTF((acDbgBfr,TEXT("LoadCursor returned %lx for %s\n"),
              (long)h,acCursName));
        return h;
      default:
        break;
    }

    DB_OUT("HC, return NULL for now\n");
    return NULL;
}


void PbSetCursor(LPTSTR curwho)
{
    DWORD   ccomp;
    HCURSOR hCurs;
    HWND theWnd;

    ccomp = (DWORD) curwho; /* type for comparison */
    CursorStat.allowed = TRUE;
    CursorStat.inrsrc = TRUE;

    theWnd = bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid];

    if (!ccomp) {
        /*
         * curwho == ccomp == NULL
         */

        CursorStat.inrsrc = FALSE;
        if (SizeCsr(cursTool, theBrush))
            hCurs = HackCursor(theWnd,cursTool,theBrush);

        else {
            SETCLASSCURSOR(theWnd, NULL);
            SetCursor(hCurs = NULL);  /* set to null */
            CursorStat.allowed = FALSE;

          DB_OUT("PbSetCursor, NULL\n");
        }
    } else if (curwho < IDC_ARROW || curwho > IDC_ICON) {
        /*
         * curwho is a cursor name from the resource file
         */
        SETCLASSCURSOR (theWnd, (hCurs = LoadCursor(hInst, curwho)));

#ifdef PENWIN
    } else if (LOWORD(ccomp) >= IDC_ARROW && LOWORD(ccomp) <= IDC_ICON) {
        // ccomp is a system cursor ID
        SetClassWord (theWnd, GCW_HCURSOR, hCurs = LoadCursor(NULL, curwho));
#endif
    } else {
        /*
         * ccomp is a system cursor ID  (dead code if PENWIN)
         */
        SETCLASSCURSOR (theWnd, (hCurs = LoadCursor(NULL, curwho)));
    }

    if (hCurs) {
        SetCursor(hCurs);
    }
}
