/*
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

#include <win32k/debug1.h>

VOID IntEngDeleteClipRegion(CLIPOBJ *ClipObj)
{
  HCLIP HClip      = AccessHandleFromUserObject(ClipObj);
  FreeGDIHandle(HClip);
}

CLIPOBJ * IntEngCreateClipRegion( ULONG count, PRECTL pRect, RECTL rcBounds )
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

CLIPOBJ * STDCALL
EngCreateClip(VOID)
{
  return EngAllocMem(FL_ZERO_MEMORY, sizeof(CLIPOBJ), 0);
}

VOID STDCALL
EngDeleteClip(CLIPOBJ *ClipRegion)
{
  EngFreeMem(ClipRegion);
}

ULONG STDCALL
CLIPOBJ_cEnumStart(IN CLIPOBJ *ClipObj,
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

BOOL STDCALL
CLIPOBJ_bEnum(IN CLIPOBJ *ClipObj,
	      IN ULONG ObjSize,
	      OUT ULONG *EnumRects)
{
  CLIPGDI *ClipGDI = (CLIPGDI*)AccessInternalObjectFromUserObject(ClipObj);
  ULONG nCopy;
  PENUMRECTS pERects = (PENUMRECTS)EnumRects;

  //calculate how many rectangles we should copy
  nCopy = min( ClipGDI->EnumMax-ClipGDI->EnumPos,
  				min( ClipGDI->EnumRects.c, (ObjSize-sizeof(ULONG))/sizeof(RECTL)));

  RtlCopyMemory( &(pERects->arcl), &(ClipGDI->EnumRects.arcl), nCopy*sizeof(RECTL) );
  pERects->c = nCopy;

  ClipGDI->EnumPos+=nCopy;

  if(ClipGDI->EnumPos > ClipGDI->EnumRects.c)
    return FALSE;
  else
    return TRUE;
}
