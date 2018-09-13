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

#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"


void UpdatImg(void)
{
   if (updateFlag)
   {
       PasteDownRect(0, 0, 0, 0);
       UpdFlag(FALSE);

       if (!gfLoading)
           AdviseDataChange();
   }
}

void UndoImg()
{
   if (updateFlag) {
       WorkImageExchange();

       /* OLE: Similar for undo */
       if (!gfLoading)
           AdviseDataChange();
   }
}

void UpdFlag(int how)
{
   if (how) {
       updateFlag = TRUE;
       EnableMenuItem(ghMenuFrame, EDITundo, MF_ENABLED);
   } else {
       updateFlag = FALSE;
       EnableMenuItem(ghMenuFrame, EDITundo, MF_GRAYED);
   }
}
