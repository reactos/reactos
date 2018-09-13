/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMMSTRUC.H
 *  WOW32 16-bit MultiMedia structure conversion support
 *
 *  History:
 *  Created  13-Feb-1992 by Mike Tricker (miketri) after jeffpar
 *
--*/


/*++

  Hack to make the code work with current MIPS compiler, whereby the compiler
  can't work out the correct size of a MMTIME16 structure.

--*/
#define MIPS_COMPILER_PACKING_BUG

/**********************************************************************\
*
*   The following macros are used to set or clear the done bit in a
*   16 bit wave|midi header structure.
*
\**********************************************************************/
#define COPY_WAVEOUTHDR16_FLAGS( x, y )             \
{                                                   \
    PWAVEHDR16  pWavHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pWavHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    STOREDWORD( pWavHdr->dwFlags, dw );             \
}


#define COPY_MIDIOUTHDR16_FLAGS( x, y )             \
{                                                   \
    PMIDIHDR16  pMidHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pMidHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    STOREDWORD( pMidHdr->dwFlags, dw );             \
}

#define COPY_WAVEINHDR16_FLAGS( x, y )              \
{                                                   \
    PWAVEHDR16  pWavHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pWavHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    STOREDWORD( pWavHdr->dwFlags, dw );             \
    dw   = (y).dwBytesRecorded;                     \
    STOREDWORD( pWavHdr->dwBytesRecorded, dw );     \
}


#define COPY_MIDIINHDR16_FLAGS( x, y )              \
{                                                   \
    PMIDIHDR16  pMidHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pMidHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    STOREDWORD( pMidHdr->dwFlags, dw );             \
    dw   = (y).dwBytesRecorded;                     \
    STOREDWORD( pMidHdr->dwBytesRecorded, dw );     \
}


/*++

  Call definitions

--*/

#define GETMMTIME16(vp,lp)          getmmtime16(FETCHDWORD(vp),lp)
#define GETWAVEHDR16(vp,lp)         getwavehdr16(FETCHDWORD(vp), lp)
#define GETMIDIHDR16(vp,lp)         getmidihdr16(FETCHDWORD(vp), lp)

#define PUTMMTIME16(vp,lp)          putmmtime16(FETCHDWORD(vp),lp)
#define PUTWAVEHDR16(vp,lp)         putwavehdr16(FETCHDWORD(vp), lp)
#define PUTWAVEFORMAT16(vp,lp)      putwaveformat16(FETCHDWORD(vp), lp)
#define PUTWAVEOUTCAPS16(vp,lp,c)   putwaveoutcaps16(FETCHDWORD(vp), lp, c)
#define PUTWAVEINCAPS16(vp,lp,c)    putwaveincaps16(FETCHDWORD(vp), lp, c)
#define PUTMIDIHDR16(vp,lp)         putmidihdr16(FETCHDWORD(vp), lp)
#define PUTAUXCAPS16(vp,lp,c)       putauxcaps16(FETCHDWORD(vp), lp, c)
#define PUTTIMECAPS16(vp,lp,c)      puttimecaps16(FETCHDWORD(vp), lp, c)
#define PUTMIDIINCAPS16(vp,lp,c)    putmidiincaps16(FETCHDWORD(vp), lp, c)
#define PUTMIDIOUTCAPS16(vp,lp,c)   putmidioutcaps16(FETCHDWORD(vp), lp, c)
#define PUTJOYCAPS16(vp,lp,c)       putjoycaps16(FETCHDWORD(vp), lp, c)
#define PUTJOYINFO16(vp,lp)         putjoyinfo16(FETCHDWORD(vp), lp)

#ifndef DEBUG
#define FREEMMTIME(p)
#define FREEWAVEHDR(p)
#define FREEWAVEOUTCAPS(p)
#define FREEWAVEINCAPS(p)
#define FREEMIDIHDR(p)
#define FREEAUXCAPS(p)
#define FREETIMECAPS(p)
#define FREEMIDIINCAPS(p)
#define FREEMIDIOUTCAPS(p)
#define FREEJOYCAPS(p)
#define FREEJOYINFO(p)
#else
#define FREEMMTIME(p)       p=NULL
#define FREEWAVEHDR(p)      p=NULL
#define FREEWAVEOUTCAPS(p)  p=NULL
#define FREEWAVEINCAPS(p)   p=NULL
#define FREEMIDIHDR(p)      p=NULL
#define FREEAUXCAPS(p)      p=NULL
#define FREETIMECAPS(p)     p=NULL
#define FREEMIDIINCAPS(p)   p=NULL
#define FREEMIDIOUTCAPS(p)  p=NULL
#define FREEJOYCAPS(p)      p=NULL
#define FREEJOYINFO(p)      p=NULL
#endif

/*++

 Function prototypes

--*/
PWAVEHDR16 getwavehdr16(VPWAVEHDR16 vpwhdr, LPWAVEHDR lpwhdr);
VOID       putwavehdr16(VPWAVEHDR16 vpwhdr, LPWAVEHDR lpwhdr);

PMIDIHDR16 getmidihdr16(VPMIDIHDR16 vpmhdr, LPMIDIHDR lpmhdr);
VOID       putmidihdr16(VPMIDIHDR16 vpmhdr, LPMIDIHDR lpmhdr);

ULONG getmmtime16      (VPMMTIME16 vpmmt, LPMMTIME lpmmt);
ULONG putmmtime16      (VPMMTIME16 vpmmt, LPMMTIME lpmmt);
ULONG putwaveoutcaps16 (VPWAVEOUTCAPS16 vpwoc, LPWAVEOUTCAPS lpwoc, UINT uSize);
ULONG putwaveincaps16  (VPWAVEINCAPS16 vpwic, LPWAVEINCAPS lpwic, UINT uSize);
ULONG putauxcaps16     (VPAUXCAPS16 vpauxc, LPAUXCAPS lpauxc, UINT uSize);
ULONG puttimecaps16    (VPTIMECAPS16 vptimec, LPTIMECAPS lptimec, UINT uSize);
ULONG putmidiincaps16  (VPMIDIINCAPS16 vpmic, LPMIDIINCAPS lpmic, UINT uSize);
ULONG putmidioutcaps16 (VPMIDIOUTCAPS16 vpmoc, LPMIDIOUTCAPS lpmoc, UINT uSize);
ULONG putjoycaps16     (VPJOYCAPS16 vpjoyc, LPJOYCAPS lpjoyc, UINT uSize);
ULONG putjoyinfo16     (VPJOYINFO16 vpjoyi, LPJOYINFO lpjoyi);
