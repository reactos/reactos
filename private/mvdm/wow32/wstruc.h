/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSTRUC.H
 *  WOW32 16-bit structure conversion support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Added GDI macro definitions 6/13/91, ChandanC
 *  Added MDI structures 1/20/91, BobDay
--*/
#ifndef _DEF_WSTRUC_     // if this hasn't already been included
#define _DEF_WSTRUC_

/* Structure conversion macros
 */
#define PUTINT16(vp,i)      WOW32ASSERT(!(LOW(vp)&1));\
                            *(PSHORT16)GetPModeVDMPointer(vp, sizeof(SHORT)) = (SHORT)i;

#define PUTBOOL16(vp,b)     WOW32ASSERT(!(LOW(vp)&1));\
                if (FETCHDWORD(vp))\
                    *(PWORD16)GetPModeVDMPointer(vp, sizeof(WORD)) = (WORD)b;
#define STACKORHEAPALLOC(cBytesNeeded, cBytesAvail, lpStackBuffer)   \
       ((INT)(cBytesNeeded) > (INT)(cBytesAvail)) ?  (PVOID)malloc_w(cBytesNeeded) : \
                                                        (PVOID)(lpStackBuffer)

#define STACKORHEAPFREE(lpToFree, lpStackBuffer)      \
     if ((lpToFree) && (PVOID)(lpToFree) != (PVOID)(lpStackBuffer))  free_w(lpToFree)

#define PUTINTARRAY16(vp,c,p)         putintarray16(FETCHDWORD(vp), c, p)
#define PUTINTARRAY16V(vp,c,p)        putintarray16(vp, c, p)

// GETDWORDARRAY16 is not analogous to any of the above.
#define GETDWORDARRAY16(vp,c,p)   {GETVDMPTR(vp, c*sizeof(DWORD), p);}
#define FREEDWORDARRAY16(p)       {if (p) FREEVDMPTR(p);}

#define GETRECT16(vp,p)     getrect16(FETCHDWORD(vp), p)
#define PUTRECT16(vp,p)     putrect16(FETCHDWORD(vp), p)

// COPYPOINT16 is used by ClientToScreen because the point is passed
// directly as an argument, instead of a pointer to the point.

#define COPYPOINT16(Pt16,Pt) {Pt.x = Pt16.x; Pt.y = Pt16.y;}

#define GETPOINT16(vp,p)    getpoint16(FETCHDWORD(vp), 1, p)
#define PUTPOINT16(vp,p)    putpoint16(FETCHDWORD(vp), 1, p)


//
// Get/Put point array macros.
//
// These routines will use a static buffer if the number of points to
// be thunked is small enough.  This will break badly if a thunk uses
// GETPOINTARRAY16 and then calls a function which calls back, since
// during the callback we could call another function which uses the
// same static buffer.
//
// As of 7-Jan-92 the only callers are in GDI, which doesn't call back,
// so we're OK.
//
// GETPOINTARRAY16 sometimes allocates memory, so you must call
// FREEPOINTARRAY16 once for every GETPOINTARRAY16.
//

#define PUTPOINTARRAY16(vp,c,p) putpoint16(FETCHDWORD(vp), c, p)

#define FREEMSG16(vp,lp)

#define GETWNDCLASS16(vp,lp) {\
        WORD w;\
        PWNDCLASS16 pwc16;\
        GETVDMPTR(vp, sizeof(WNDCLASS16), pwc16);\
        (lp)->style     = FETCHWORD(pwc16->style);\
        (lp)->lpfnWndProc   = (WNDPROC)FETCHDWORD(pwc16->vpfnWndProc);\
        (lp)->cbClsExtra    = FETCHWORD(pwc16->cbClsExtra);\
        (lp)->cbWndExtra    = FETCHWORD(pwc16->cbWndExtra);\
        w = FETCHWORD(pwc16->hInstance);\
        if((w == gUser16hInstance) || (w == BOGUSGDT))\
            (lp)->hInstance = ghInstanceUser32;\
        else\
            (lp)->hInstance = HMODINST32(FETCHWORD(pwc16->hInstance));\
        (lp)->hIcon     = HICON32_REGCLASS(FETCHWORD(pwc16->hIcon));\
        (lp)->hCursor   = HCURSOR32(FETCHWORD(pwc16->hCursor));\
        w = FETCHWORD(pwc16->hbrBackground);\
        if (w > COLOR_ENDCOLORS)\
            (lp)->hbrBackground = HBRUSH32(w);\
        else\
            (lp)->hbrBackground = (HBRUSH)w;\
        (lp)->lpszMenuName  = (LPSTR)FETCHDWORD(pwc16->vpszMenuName);\
        (lp)->lpszClassName = (LPSTR)FETCHDWORD(pwc16->vpszClassName);\
        FREEVDMPTR(pwc16);\
    }


#define GETBITMAP16(vp, lp) {\
        PBITMAP16 pbm16;\
        GETVDMPTR(vp, sizeof(BITMAP16), pbm16);\
        (lp)->bmType    = FETCHSHORT(pbm16->bmType);\
        (lp)->bmWidth   = FETCHSHORT(pbm16->bmWidth);\
        (lp)->bmHeight  = FETCHSHORT(pbm16->bmHeight);\
        (lp)->bmWidthBytes  = FETCHSHORT(pbm16->bmWidthBytes);\
        (lp)->bmPlanes  = pbm16->bmPlanes;\
        (lp)->bmBitsPixel   = pbm16->bmBitsPixel;\
            GETMISCPTR(pbm16->bmBits, (lp)->bmBits);\
        FREEVDMPTR(pbm16);\
    }


#define PUTBITMAP16(vp,cb,lp) {\
        PBITMAP16 pbm16;\
            GETVDMPTR(vp, ((int)cb), pbm16);\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmType) + sizeof(pbm16->bmType)))\
                STORESHORT(pbm16->bmType,       (lp)->bmType);\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmWidth) + sizeof(pbm16->bmWidth)))\
                STORESHORT(pbm16->bmWidth,      (lp)->bmWidth);\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmHeight) + sizeof(pbm16->bmHeight)))\
                STORESHORT(pbm16->bmHeight,     (lp)->bmHeight);\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmWidthBytes) + sizeof(pbm16->bmWidthBytes)))\
                STORESHORT(pbm16->bmWidthBytes, \
                      ((int)((((lp)->bmWidth * (lp)->bmBitsPixel + 15) / 16) * 2)));\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmPlanes) + sizeof(pbm16->bmPlanes)))\
                pbm16->bmPlanes               = (BYTE)(lp)->bmPlanes;\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmBitsPixel) + sizeof(pbm16->bmBitsPixel)))\
                pbm16->bmBitsPixel            = (BYTE)(lp)->bmBitsPixel;\
            if (cb >= (FIELD_OFFSET(BITMAP16, bmBits) + sizeof(pbm16->bmBits)))\
                pbm16->bmBits                 = (VPBYTE)NULL;\
            FLUSHVDMPTR(vp, cb, pbm16);\
        FREEVDMPTR(pbm16);\
    }



#define GETDCB16(vp,lp)     // not implemented yet
#define PUTDCB16(vp,lp)     // not implemented yet
#define PUTCOMSTAT16(vp,lp) // not implemented yet

#define PUTSEGINFO16(vp,lp) // not implemented yet
#define GETLOADPARMS16(vp,lp)   // not implemented yet
#define PUTOFSTRUCT16(vp,lp)    // not implemented yet

#define GETCATCHBUF16(vp,lp)    // not implemented yet
#define PUTCATCHBUF16(vp,lp)    // not implemented yet


#define GETLOGPEN16(vp, lp)  {\
            PLOGPEN16 plp16;\
            GETVDMPTR(vp, sizeof(LOGPEN16), plp16);\
            (lp)->lopnStyle     = FETCHWORD(plp16->lopnStyle);\
            (lp)->lopnWidth.x   = FETCHSHORT(plp16->lopnWidth.x);\
            (lp)->lopnColor     = FETCHDWORD(plp16->lopnColor);\
            FREEVDMPTR(plp16);\
        }

#define PUTLOGPEN16(vp,cb,lp) {\
        PLOGPEN16 plp16;\
        GETVDMPTR(vp, cb, plp16);\
            if (cb >= 2)\
        STOREWORD(plp16->lopnStyle,     (lp)->lopnStyle);\
        if (cb >= 4)\
        STOREWORD(plp16->lopnWidth.x,   (lp)->lopnWidth.x);\
        if (cb >= 6)\
        STOREWORD(plp16->lopnWidth.y,   (lp)->lopnWidth.y);\
        if (cb >= 10)\
        STOREDWORD(plp16->lopnColor,    (lp)->lopnColor);\
        FLUSHVDMPTR(vp, cb, plp16);\
        FREEVDMPTR(plp16);\
    }


#define GETLOGBRUSH16(vp,lp) {\
            PLOGBRUSH16 plb16;\
            WORD    wStyle;\
            GETVDMPTR(vp, sizeof(LOGBRUSH16), plb16);\
            wStyle              = FETCHWORD(plb16->lbStyle);\
            (lp)->lbStyle       = wStyle;\
            (lp)->lbColor       = FETCHDWORD(plb16->lbColor);\
            if (wStyle == BS_PATTERN) { \
                (lp)->lbHatch   = (DWORD)HBITMAP32(FETCHWORD(plb16->lbHatch));\
            } else { \
                (lp)->lbHatch   = FETCHWORD(plb16->lbHatch);\
            } \
            FREEVDMPTR(plb16);\
        }
#define PUTLOGBRUSH16(vp,cb,lp) {\
            PLOGBRUSH16 plb16;\
            GETVDMPTR(vp, cb, plb16);\
            if (cb >= 2)\
        STOREWORD(plb16->lbStyle, (lp)->lbStyle);\
            if (cb >= 6)\
            STOREDWORD(plb16->lbColor,      (lp)->lbColor);\
        if (cb >= 8){\
          if( ((lp)->lbStyle == BS_PATTERN) || ((lp)->lbStyle == BS_DIBPATTERN)){\
        STORESHORT(plb16->lbHatch,  GETHBITMAP16((HAND32)(lp)->lbHatch));\
        }\
          else {\
        STORESHORT(plb16->lbHatch,  (lp)->lbHatch);\
          }\
        }\
            FLUSHVDMPTR(vp, cb, plb16);\
            FREEVDMPTR(plb16);\
    }

#define GETLOGFONT16(vp,p)      getlogfont16(FETCHDWORD(vp), p)
#define PUTLOGFONT16(vp,cb,p)   putlogfont16(FETCHDWORD(vp), cb, p)
#define PUTENUMLOGFONT16(vp,p)  putenumlogfont16(FETCHDWORD(vp), p)

#define PUTTEXTMETRIC16(vp,p)           puttextmetric16(FETCHDWORD(vp), p)
#define PUTNEWTEXTMETRIC16(vp,p)        putnewtextmetric16(FETCHDWORD(vp), p)
#define PUTOUTLINETEXTMETRIC16(vp,c,p)  putoutlinetextmetric16(vp,c,p)

#define ALLOCHANDLETABLE16(c,p)     p=malloc_w(c*sizeof(HANDLE))
#define GETHANDLETABLE16(vp,c, p)   gethandletable16(FETCHDWORD(vp), c, p)
#define PUTHANDLETABLE16(vp,c, p)   puthandletable16(FETCHDWORD(vp), c, p)
#define FREEHANDLETABLE16(p)        {if (p) free_w(p);}

#define FREEDEVMODE32(p)   {if (p) free_w(p);}

#define GETCLIENTCREATESTRUCT16(vp,lp)   { \
            PCLIENTCREATESTRUCT16 pccs16;\
            GETVDMPTR(vp, sizeof(CLIENTCREATESTRUCT16), pccs16);\
            (lp)->hWindowMenu   = HMENU32(FETCHWORD(pccs16->hWindowMenu));\
            (lp)->idFirstChild  = FETCHWORD(pccs16->idFirstChild);\
            FREEVDMPTR(pccs16);\
        }
#define PUTCLIENTCREATESTRUCT16(vp,lp)   { \
            PCLIENTCREATESTRUCT16 pccs16;\
            GETVDMPTR(vp, sizeof(CLIENTCREATESTRUCT16), pccs16);\
            STOREWORD(pccs16->hWindowMenu, GETHMENU16((lp)->hWindowMenu));\
            STOREWORD(pccs16->idFirstChild, (lp)->idFirstChild);\
        }


// following 2 for allocating a maximum sized BITMAPINFO structure from stack
#define MAXDIBCOLORS 256  // for biBitCount == 8
typedef struct _tagSTACKBMI32 {
    BITMAPINFOHEADER  bmiHeader;
    RGBQUAD           bmiColors[MAXDIBCOLORS];
} STACKBMI32, *LPSTACKBMI32;


/* Function prototypes
 */
VOID   getstr16(VPSZ vpszSrc, LPSZ lpszDst, INT cb);
VOID   putstr16(VPSZ vpszDst, LPCSTR lpszSrc, INT cb);
LPRECT getrect16(VPRECT16 vpRect, LPRECT lpRect);
VOID   putrect16(VPRECT16 vpRect, LPRECT lpRect);
VOID   getpoint16(VPPOINT16 vpPoint, INT c, LPPOINT lpPoint);
VOID   putpoint16(VPPOINT16 vpPoint, INT c, LPPOINT lpPoint);
VOID   getintarray16(VPINT16 vpInt, INT c, LPINT lpInt);
VOID   putintarray16(VPINT16 vpInt, INT c, LPINT lpInt);
VOID   getuintarray16(VPWORD vp, INT c, PUINT puint);
VOID   getdrawitem16(VPDRAWITEMSTRUCT16 vpDI16, LPDRAWITEMSTRUCT lpDI);
VOID   putdrawitem16(VPDRAWITEMSTRUCT16 vpDI16, LPDRAWITEMSTRUCT lpDI);
VOID   getmeasureitem16(VPMEASUREITEMSTRUCT16 vpMI16, LPMEASUREITEMSTRUCT lpMI, HWND16 hwnd16);
VOID   putmeasureitem16(VPMEASUREITEMSTRUCT16 vpMI16, LPMEASUREITEMSTRUCT lpMI);
VOID   getdeleteitem16(VPDELETEITEMSTRUCT16 vpDI16, LPDELETEITEMSTRUCT lpDI);
VOID   putdeleteitem16(VPDELETEITEMSTRUCT16 vpDI16, LPDELETEITEMSTRUCT lpDI);
VOID   getcompareitem16(VPCOMPAREITEMSTRUCT16 vpCI16, LPCOMPAREITEMSTRUCT lpCI);
VOID   putcompareitem16(VPCOMPAREITEMSTRUCT16 vpCI16, LPCOMPAREITEMSTRUCT lpCI);
VOID   getmsg16(VPMSG16 vpmsg16, LPMSG lpmsg, LPMSGPARAMEX lpmpex);
ULONG  putmsg16(VPMSG16 vpmsg16, LPMSG lpmsg);
VOID   getlogfont16(VPLOGFONT16 vplf, LPLOGFONT lplf);
VOID   putlogfont16(VPLOGFONT16 vplf, INT cb, LPLOGFONT lplf);
VOID   putenumlogfont16(VPENUMLOGFONT16 vpelf, LPENUMLOGFONT lpelf);
VOID   puttextmetric16(VPTEXTMETRIC16 vptm, LPTEXTMETRIC lptm);
VOID   putnewtextmetric16(VPNEWTEXTMETRIC16 vpntm, LPNEWTEXTMETRIC lpntm);
VOID   putoutlinetextmetric16(VPOUTLINETEXTMETRIC16 vpotm, INT cb, LPOUTLINETEXTMETRIC lpotm);
VOID   gethandletable16(VPWORD vpht, UINT c, LPHANDLETABLE lpht);
VOID   puthandletable16(VPWORD vpht, UINT c, LPHANDLETABLE lpht);
VOID   putkerningpairs16(VPWORD vp, UINT cb, LPKERNINGPAIR lp);

#ifdef NOTUSED
BOOL   getdropfilestruct16(HAND16 hand16, PHANDLE phand32);
#endif
INT    GetBMI16Size(PVPVOID vpbmi16, WORD fuColorUse, LPDWORD lpdwClrUsed);
INT    GetBMI32Size(LPBITMAPINFO lpbmi32, WORD fuColorUse);
LPBITMAPINFO       CopyBMI16ToBMI32(PVPVOID vpbmi16, LPBITMAPINFO lpbmi32, WORD fuColorUse);
LPBITMAPINFOHEADER CopyBMIH16ToBMIH32(PVPVOID vpbmih16, LPBITMAPINFOHEADER lpbmih);


VOID    getwindowpos16( VPWINDOWPOS16 vpwp, LPWINDOWPOS lpwp );
VOID    putwindowpos16( VPWINDOWPOS16 vpwp, LPWINDOWPOS lpwp );
VOID    W32CopyMsgStruct(VPMSG16 vpmsg16, LPMSG lpmsg, BOOL fThunk16To32);
VOID    getpaintstruct16(VPVOID vp, LPPAINTSTRUCT lp);
VOID    putpaintstruct16(VPVOID vp, LPPAINTSTRUCT lp);
VOID FASTCALL getmenuiteminfo16(VPVOID vp, LPMENUITEMINFO pmii32);
VOID FASTCALL putmenuiteminfo16(VPVOID vp, LPMENUITEMINFO pmii32);

LPDEVMODE ThunkDevMode16to32(VPDEVMODE31 vpdm16);
BOOL      ThunkDevMode32to16(VPDEVMODE31 vpdm16, LPDEVMODE lpdm32, UINT nBytes);

#endif  // ifndef _DEF_WSTRUC_  THIS SHOULD BE THE LAST LINE IN THIS FILE
