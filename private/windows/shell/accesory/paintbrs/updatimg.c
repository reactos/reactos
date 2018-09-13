/****************************Module*Header******************************\
* Module Name: updatimg.c                                               *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation			*
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include "onlypbr.h"
#undef NOMENUS
#undef NOCLIPBOARD
#undef NOATOM
#undef NOKERNEL
#include <windows.h>
#include "port1632.h"

#include "pbrush.h"
#include "pbserver.h"			/* OLE */

void UpdatImg(void)
{
   if (updateFlag) {
       PasteDownRect(0, 0, 0, 0);
       UpdFlag(FALSE);

#ifdef NON_GRANULAR
	/* OLE:  Tell the library the document has changed as granularly
	 * as the Undo mechanism (whenever the tool changes)
	 */
       if (fOLE && !fLoading)
	   SendDocChangeMsg(vpdoc, ECD_CHANGED);
#endif
   }
}

void UndoImg()
{
   if (updateFlag) {
       WorkImageExchange();

       /* OLE: Similar for undo */
       if (fOLE && !fLoading)
	   SendDocChangeMsg(vpdoc, OLE_CHANGED);
   }
}

void UpdFlag(int how)
{
   HMENU hMenu;

   hMenu = GetMenu(pbrushWnd[PARENTid]);

   if (how) {
       updateFlag = TRUE;
       EnableMenuItem(hMenu, EDITundo, MF_ENABLED);
   } else {
       updateFlag = FALSE;
       EnableMenuItem(hMenu, EDITundo, MF_GRAYED);
   }
}
