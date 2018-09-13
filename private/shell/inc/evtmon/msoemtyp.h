/*****************************************************************************
    msoemtyp.h

    Owner: DaleG
    Copyright (c) 1997 Microsoft Corporation

    Typedef file for Rules Engine of Event Monitor.

*****************************************************************************/

#ifndef MSOEMTYP_H
#define MSOEMTYP_H

#ifndef MSO_H
#pragma message ("MsoEMTyp.h file included before Mso.h.  Including Mso.h.")
#include "mso.h"
#endif

MSOEXTERN_C_BEGIN   // ***************** Begin extern "C" ********************

#ifndef MSOCP_DEFINED
typedef long MSOCP;                                     // Character position
#define msocpNil ((MSOCP) -1)
#define msocp0 ((MSOCP) 0)
#define msocpMax ((MSOCP) 0x7FFFFFFF)
#define MSOCP_DEFINED
#endif /* !MSOCP_DEFINED */


// Define Bit Field type
typedef unsigned short MSOBF;


#if !defined(WORD_BUILD)  &&  !defined(WIN_CALLBACK)
#define WIN_CALLBACK    OFC_CALLBACK
#endif /* !WORD_BUILD */


// Character Byte Manipulators
#define MsoLowByteWch(w)      (unsigned char)((w) & 0x00ff)
#define MsoHighByteWch(w)     (unsigned char)(((WORD)(w) >> 8) & 0x00ff)

#ifndef cbXchar
#ifndef ANSI_XCHAR
typedef unsigned short XCHAR;
#define cbXchar    2

#else /* ANSI_XCHAR */

typedef unsigned char XCHAR;
#define cbXchar    1
#endif /* !ANSI_XCHAR */
#endif /* !cbXchar */

#ifndef ANSI_XCHAR

#define MsoLowByteXch(xch)      MsoLowByteWch(xch)
#define MsoHighByteXch(xch)     MsoHighByteWch(xch)
#define CchXsz(xsz)             MsoCchWzLen(xsz)
#define SzFromXsz(xszFrom, szTo) \
            MsoWzToSz(xszFrom, szTo)
#define CchCopyRgxchToRgch  MsoRgwchToRgch
#define CopyRgxchToRgch(pxchSrc, pchDest, pcch) \
            (*pcch = MsoRgwchToRgch(pxchSrc, *pcch, pchDest, 2 * (*pcch)))

#else /* ANSI_XCHAR */

#define MsoLowByteXch(xch)      (xch)
#define MsoHighByteXch(xch)     (0)
#define CchXsz(xsz)             MsoCchSzLen(xsz)
#define SzFromXsz(xszFrom, szTo) \
            strcpy(szTo, xszFrom)
#define CchCopyRgxchToRgch(rgchFrom, cchFrom, rgchTo, cchTo) \
            (PbCopyRgb((rgchFrom), (rgchTo), (cchFrom)), (cchFrom))
#define CopyRgxchToRgch(pxchSrc, pchDest, pcch) \
            memmove(pchDest, pxchSrc, *(pcch))

#endif /* !ANSI_XCHAR */

int FNeNcRgxch(const XCHAR *pxch1, const XCHAR *pxch2, int cch);


#if !WORD_BUILD  &&  !STANDALONE_WORD

typedef void *LPV;

// Split value struct. Overlays two shorts on long, byte-reversable indifferent
typedef struct _SV
    {
#ifndef MAC
    short                   wValue1;                    // Low, short
    short                   wValue2;                    // High, long
#else /* MAC */
    short                   wValue2;                    // High, long
    short                   wValue1;                    // Low, short
#endif /* !MAC */
    } SV;

// SV as seen as chars
typedef struct _SVC
    {
#ifndef MAC
    char                    ch1;                        // Low, 1st byte
    char                    ch2;                        // 2nd byte
    char                    ch3;                        // 3rd byte
    char                    ch4;                        // 4th byte
#else /* MAC */
    char                    ch3;                        // 3rd byte
    char                    ch4;                        // 4th byte
    char                    ch1;                        // Low, 1st byte
    char                    ch2;                        // 2nd byte
#endif /* !MAC */
    } SVC;

// SV unioned with long
typedef union _SVL
    {
    long                    lValue;                     // As long
    SV                      sv;                         // Split version
    SVC                     svc;                        // As chars
    } SVL;

#endif /* !WORD_BUILD  &&  !STANDALONE_WORD */


#ifndef NORULES
#include "emrule.h"
#include "emutil.h"
#include "emact.h"
#ifdef DYN_RULES
#include "emruloci.h"
#endif /* DYN_RULES */
#endif /* !NORULES */


#if defined(DEBUG)  &&  !defined(STANDALONE)

MSOAPI_(void) MsoInitEmMemMark(                         // Init EM Mem marking
    struct MSOINST     *hMsoInst,
    long                lparam,
    int                 bt
    );
MSOAPI_(void) MsoMarkEmPv(void *pv);                    // Mark EM Memory

#else /* !DEBUG  ||  STANDALONE */

#define MsoInitEmMemMark(hMsoInst, lparam, bt)
#define MsoMarkEmPv(pv)

#endif /* DEBUG  &&  !STANDALONE */

MSOEXTERN_C_END     // ****************** End extern "C" *********************

#endif /* !MSOEMTYP_H */


