/****************************************************************************/
/* */
/* ECKANJI.C -                                                                    */
/* */
/* Copyright (c) 1985 - 1999, Microsoft Corporation */
/* */
/* Kanji Support Routines */
/* */
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef KANJI

/***************************************************************************\
* SysHasKanji
*
* <brief description>
*
* History:
\***************************************************************************/

BOOL SysHasKanji(
    )
{
  return (*(WORD *)&keybdInfo.Begin_First_range != 0x0FEFF ||
          *(WORD *)&keybdInfo.Begin_Second_range != 0x0FEFF);
}

/***************************************************************************\
* KAlign
*
* Make sure the given char isn't the index of the second byte of a Kanji word.
*
* History:
\***************************************************************************/

int KAlign(
     PED ped,
    int ichIn)
{
   int ichCheck;
  int ichOut;
  LPSTR lpch;

  /*
   * ichOut chases ichCheck until ichCheck > ichIn
   */
  if (ped->fSingle)
      ichOut = ichCheck = 0;
  else
      ichOut = ichCheck = ped->mpilich[IlFromIch(ped, ichIn)];

  lpch = ECLock(ped) + ichCheck;
  while (ichCheck <= ichIn) {
      ichOut = ichCheck;
      if (IsTwoByteCharPrefix(*(unsigned char *)lpch))
	{
          lpch++;
          ichCheck++;
        }

      lpch++;
      ichCheck++;
    }
  ECUnlock(ped);
  return (ichOut);
}

/***************************************************************************\
* KBump
*
* If ichMaxSel references Kanji prefix, bump dch by cxChar to bypass prefix
* char. This routine is called only from DoKey in ea1.asm.
*
* History:
\***************************************************************************/

int KBump(
     PED ped,
    int dch)
{
  unsigned char *pch;

  pch = ECLock(ped) + ped->ichMaxSel;
  if (IsTwoByteCharPrefix(*pch))
      dch += ped->cxChar;
  ECUnlock(ped);

  return (dch);
}

/***************************************************************************\
* KCombine
*
* Kanji prefix byte was found in bytestream queue. Get next byte and combine.
*
* History:
\***************************************************************************/

int KCombine(
    HWND hwnd,
    int ch)
{
    MSG msg;
    int i;

    /*
     * Loop counter to avoid the infinite loop.
     */
    i = 10;

    while (!PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
        if (--i == 0)
            return 0;
        Yield();
    }

    return (UINT)ch | ((UINT)msg.wParam << 8);
}

#endif
