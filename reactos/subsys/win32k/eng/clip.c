/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: clip.c,v 1.13 2003/07/11 15:59:37 royce Exp $
 * 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Clipping Functions
 * FILE:              subsys/win32k/eng/clip.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 21/8/1999: Created
 */

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include "objects.h"
#include "clip.h"
#include <include/object.h>

#define NDEBUG
#include <win32k/debug1.h>

VOID STDCALL IntEngDeleteClipRegion(CLIPOBJ *ClipObj)
{
  HCLIP HClip      = AccessHandleFromUserObject(ClipObj);
  FreeGDIHandle(HClip);
}

CLIPOBJ STDCALL * IntEngCreateClipRegion( ULONG count, PRECTL pRect, RECTL rcBounds )
{
	HCLIP hClip;
	CLIPGDI* clipInt;
    CLIPOBJ* clipUser;
	DPRINT("IntEngCreateClipRegion count: %d\n", count);
	if( count > 1 ){
		hClip = (HCLIP)CreateGDIHandle( sizeof( CLIPGDI ) + count*sizeof(RECTL),
	                                       sizeof( CLIPOBJ ) );

		if( hClip ){
			clipInt = (CLIPGDI*)AccessInternalObject( hClip );
			RtlCopyMemory( clipInt->EnumRects.arcl, pRect, count*sizeof(RECTL));
			clipInt->EnumRects.c=count;

			clipUser = (CLIPOBJ*)AccessUserObject( hClip );
			ASSERT( clipUser );

			clipUser->iDComplexity = DC_COMPLEX;
			clipUser->iFComplexity = (count <= 4)? FC_RECT4: FC_COMPLEX;
			clipUser->iMode 	   = TC_RECTANGLES;
			RtlCopyMemory( &(clipUser->rclBounds), &rcBounds, sizeof( RECTL ) );

			return clipUser;
		}
		return NULL;
	}
	else{
		hClip = (HCLIP)CreateGDIHandle( sizeof( CLIPGDI ),
	                                       sizeof( CLIPOBJ ) );
		if( hClip ){
			clipInt = (CLIPGDI*)AccessInternalObject( hClip );
			RtlCopyMemory( clipInt->EnumRects.arcl, &rcBounds, sizeof( RECTL ));
			clipInt->EnumRects.c = 1;

			clipUser = (CLIPOBJ*)AccessUserObject( hClip );
			ASSERT( clipUser );

			clipUser->iDComplexity = ((rcBounds.top==rcBounds.bottom)&&(rcBounds.left==rcBounds.right))?
										DC_TRIVIAL:DC_RECT;
			clipUser->iFComplexity = FC_RECT;
			clipUser->iMode 	   = TC_RECTANGLES;
			DPRINT("IntEngCreateClipRegion: iDComplexity: %d\n", clipUser->iDComplexity);
			RtlCopyMemory( &(clipUser->rclBounds), &rcBounds, sizeof( RECTL ) );
			return clipUser;
		}
	}
	return NULL;
}

/*
 * @implemented
 */
CLIPOBJ * STDCALL
EngCreateClip(VOID)
{
  return EngAllocMem(FL_ZERO_MEMORY, sizeof(CLIPOBJ), 0);
}

/*
 * @implemented
 */
VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
  EngFreeMem(ClipRegion);
}

/*
 * @unimplemented
 */
ULONG STDCALL
CLIPOBJ_cEnumStart(IN PCLIPOBJ ClipObj,
		   IN BOOL ShouldDoAll,
		   IN ULONG ClipType,
		   IN ULONG BuildOrder,
		   IN ULONG MaxRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);

  ClipGDI->EnumPos     = 0;
  ClipGDI->EnumMax = (MaxRects>0)? MaxRects : ClipGDI->EnumRects.c;

  if( !((BuildOrder == CD_ANY) || (BuildOrder == CD_LEFTDOWN ))){
  	UNIMPLEMENTED;
  }
  ClipGDI->EnumOrder = BuildOrder;

  // Return the number of rectangles enumerated
  if( (MaxRects > 0) && (ClipGDI->EnumRects.c>MaxRects) )
  {
    return 0xFFFFFFFF;
  }

  return ClipGDI->EnumRects.c;
}

/*
 * @implemented
 */
BOOL STDCALL
CLIPOBJ_bEnum(IN PCLIPOBJ ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);
  ULONG nCopy;
  PENUMRECTS pERects = (PENUMRECTS)EnumRects;

  //calculate how many rectangles we should copy
  nCopy = MIN( ClipGDI->EnumMax - ClipGDI->EnumPos,
               MIN( ClipGDI->EnumRects.c - ClipGDI->EnumPos,
                    (ObjSize - sizeof(ULONG)) / sizeof(RECTL)));

  RtlCopyMemory( pERects->arcl, ClipGDI->EnumRects.arcl + ClipGDI->EnumPos,
                 nCopy * sizeof(RECTL) );
  pERects->c = nCopy;

  ClipGDI->EnumPos+=nCopy;

  return ClipGDI->EnumPos < ClipGDI->EnumRects.c;
}
/* EOF */
