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
/* $Id$ */
#include <w32k.h>

VOID
DIB_24BPP_PutPixel(SURFOBJ *SurfObj, LONG x, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + (y * SurfObj->lDelta) + (x << 1) + x;
  *(PUSHORT)(addr) = c & 0xFFFF;
  *(addr + 2) = (c >> 16) & 0xFF;
}

ULONG
DIB_24BPP_GetPixel(SURFOBJ *SurfObj, LONG x, LONG y)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x << 1) + x;
  return *(PUSHORT)(addr) + (*(addr + 2) << 16);
}

VOID
DIB_24BPP_HLine(SURFOBJ *SurfObj, LONG x1, LONG x2, LONG y, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + y * SurfObj->lDelta + (x1 << 1) + x1;
  LONG cx = x1;

  c &= 0xFFFFFF;
  while(cx < x2) {
    *(PUSHORT)(addr) = c & 0xFFFF;
    addr += 2;
    *(addr) = c >> 16;
    addr += 1;
    ++cx;
  }
}

VOID
DIB_24BPP_VLine(SURFOBJ *SurfObj, LONG x, LONG y1, LONG y2, ULONG c)
{
  PBYTE addr = SurfObj->pvScan0 + y1 * SurfObj->lDelta + (x << 1) + x;
  LONG lDelta = SurfObj->lDelta;

  c &= 0xFFFFFF;
  while(y1++ < y2) {
    *(PUSHORT)(addr) = c & 0xFFFF;
    *(addr + 2) = c >> 16;

    addr += lDelta;
  }
}

BOOLEAN
DIB_24BPP_BitBltSrcCopy(PBLTINFO BltInfo)
{
  LONG     i, j, sx, sy, xColor, f1;
  PBYTE    SourceBits, DestBits, SourceLine, DestLine;
  PBYTE    SourceBits_4BPP, SourceLine_4BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;

  DestBits = BltInfo->DestSurface->pvScan0 + (BltInfo->DestRect.top * BltInfo->DestSurface->lDelta) + BltInfo->DestRect.left * 3;

  switch(BltInfo->SourceSurface->iBitmapFormat)
  {
    case BMF_1BPP:
      sx = BltInfo->SourcePoint.x;
      sy = BltInfo->SourcePoint.y;

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        sx = BltInfo->SourcePoint.x;
        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          if(DIB_1BPP_GetPixel(BltInfo->SourceSurface, sx, sy) == 0)
          {
            DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 0));
          } else {
            DIB_24BPP_PutPixel(BltInfo->DestSurface, i, j, XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, 1));
          }
          sx++;
        }
        sy++;
      }
      break;

    case BMF_4BPP:
      SourceBits_4BPP = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + (BltInfo->SourcePoint.x >> 1);

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        DestLine = DestBits;
        sx = BltInfo->SourcePoint.x;
        f1 = sx & 1;

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest,
              (*SourceLine_4BPP & altnotmask[f1]) >> (4 * (1 - f1)));
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += BltInfo->SourceSurface->lDelta;
        DestBits += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_8BPP:
      SourceLine = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + BltInfo->SourcePoint.x;
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceBits);
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = xColor >> 8;
          SourceBits += 1;
	  DestBits += 3;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_16BPP:
      SourceBits_16BPP = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 2 * BltInfo->SourcePoint.x;

      for (j=BltInfo->DestRect.top; j<BltInfo->DestRect.bottom; j++)
      {
        SourceLine_16BPP = SourceBits_16BPP;
        DestLine = DestBits;

        for (i=BltInfo->DestRect.left; i<BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *SourceLine_16BPP);
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          SourceLine_16BPP++;
        }

        SourceBits_16BPP = (PWORD)((PBYTE)SourceBits_16BPP + BltInfo->SourceSurface->lDelta);
        DestBits += BltInfo->DestSurface->lDelta;
      }
      break;

    case BMF_24BPP:
      if (NULL == BltInfo->XlateSourceToDest || 0 != (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))
      {
	if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)
	  {
	    SourceBits = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
	    for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
	      {
		RtlMoveMemory(DestBits, SourceBits, 3 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
		SourceBits += BltInfo->SourceSurface->lDelta;
		DestBits += BltInfo->DestSurface->lDelta;
	      }
	  }
	else
	  {
	    SourceBits = BltInfo->SourceSurface->pvScan0 + ((BltInfo->SourcePoint.y + BltInfo->DestRect.bottom - BltInfo->DestRect.top - 1) * BltInfo->SourceSurface->lDelta) + 3 * BltInfo->SourcePoint.x;
	    DestBits = BltInfo->DestSurface->pvScan0 + ((BltInfo->DestRect.bottom - 1) * BltInfo->DestSurface->lDelta) + 3 * BltInfo->DestRect.left;
	    for (j = BltInfo->DestRect.bottom - 1; BltInfo->DestRect.top <= j; j--)
	      {
		RtlMoveMemory(DestBits, SourceBits, 3 * (BltInfo->DestRect.right - BltInfo->DestRect.left));
		SourceBits -= BltInfo->SourceSurface->lDelta;
		DestBits -= BltInfo->DestSurface->lDelta;
	      }
	  }
      }
      else
      {
	/* FIXME */
	DPRINT1("DIB_24BPP_Bitblt: Unhandled BltInfo->XlateSourceToDest for 16 -> 16 copy\n");
        return FALSE;
      }
      break;

    case BMF_32BPP:
      SourceLine = BltInfo->SourceSurface->pvScan0 + (BltInfo->SourcePoint.y * BltInfo->SourceSurface->lDelta) + 4 * BltInfo->SourcePoint.x;
      DestLine = DestBits;

      for (j = BltInfo->DestRect.top; j < BltInfo->DestRect.bottom; j++)
      {
        SourceBits = SourceLine;
        DestBits = DestLine;

        for (i = BltInfo->DestRect.left; i < BltInfo->DestRect.right; i++)
        {
          xColor = XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, *((PDWORD) SourceBits));
          *DestBits = xColor & 0xff;
          *(PWORD)(DestBits + 1) = xColor >> 8;
          SourceBits += 4;
	  DestBits += 3;
        }

        SourceLine += BltInfo->SourceSurface->lDelta;
        DestLine += BltInfo->DestSurface->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", BitsPerFormat(BltInfo->SourceSurface->iBitmapFormat));
      return FALSE;
  }

  return TRUE;
}

BOOLEAN
DIB_24BPP_BitBlt(PBLTINFO BltInfo)
{
   ULONG DestX, DestY;
   ULONG SourceX, SourceY;
   ULONG PatternY = 0;
   ULONG Dest, Source = 0, Pattern = 0;
   BOOL UsesSource;
   BOOL UsesPattern;
   PBYTE DestBits;

   UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);
   UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);

   SourceY = BltInfo->SourcePoint.y;
   DestBits = (PBYTE)(
      BltInfo->DestSurface->pvScan0 +
      (BltInfo->DestRect.left << 1) + BltInfo->DestRect.left +
      BltInfo->DestRect.top * BltInfo->DestSurface->lDelta);

   if (UsesPattern)
   {
      if (BltInfo->PatternSurface)
      {
         PatternY = (BltInfo->DestRect.top + BltInfo->BrushOrigin.y) %
                    BltInfo->PatternSurface->sizlBitmap.cy;
      }
      else
      {
         Pattern = BltInfo->Brush->iSolidColor;
      }
   }

   for (DestY = BltInfo->DestRect.top; DestY < BltInfo->DestRect.bottom; DestY++)
   {
      SourceX = BltInfo->SourcePoint.x;

      for (DestX = BltInfo->DestRect.left; DestX < BltInfo->DestRect.right; DestX++, DestBits += 3, SourceX++)
      {
         Dest = *((PUSHORT)DestBits) + (*(DestBits + 2) << 16);

         if (UsesSource)
         {
            Source = DIB_GetSource(BltInfo->SourceSurface, SourceX, SourceY, BltInfo->XlateSourceToDest);
         }

         if (BltInfo->PatternSurface)
	 {
            Pattern = DIB_GetSource(BltInfo->PatternSurface, (DestX + BltInfo->BrushOrigin.x) % BltInfo->PatternSurface->sizlBitmap.cx, PatternY, BltInfo->XlatePatternToDest);
         }

         Dest = DIB_DoRop(BltInfo->Rop4, Dest, Source, Pattern) & 0xFFFFFF;
         *(PUSHORT)(DestBits) = Dest & 0xFFFF;
         *(DestBits + 2) = Dest >> 16;
      }

      SourceY++;
      if (BltInfo->PatternSurface)
      {
         PatternY++;
         PatternY %= BltInfo->PatternSurface->sizlBitmap.cy;
      }
      DestBits -= (BltInfo->DestRect.right - BltInfo->DestRect.left) * 3;
      DestBits += BltInfo->DestSurface->lDelta;
   }

   return TRUE;
}

//NOTE: If you change something here, please do the same in other dibXXbpp.c files!
BOOLEAN DIB_24BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode)
{
   int SrcSizeY;
   int SrcSizeX;
   int DesSizeY;
   int DesSizeX;      
   int sx;
   int sy;
   int DesX;
   int DesY;
   int color;

   SrcSizeY = SourceRect->bottom;
   SrcSizeX = SourceRect->right;
  
   DesSizeY = DestRect->bottom;
   DesSizeX = DestRect->right;   

    switch(SourceSurf->iBitmapFormat)
    {
      case BMF_1BPP:
		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

 		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{				
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY);                 				

				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

				    if(DIB_1BPP_GetPixel(SourceSurf, sx, sy) == 0)
					{
					 DIB_24BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, 0));
					 } else {
                     DIB_24BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, 1));
                     }					
					}
				}
      break;

      case BMF_4BPP:
		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{				
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				

				if (sy > SourceRect->bottom) break;


				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_4BPP_GetPixel(SourceSurf, sx, sy);
					DIB_24BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}
      break;

      case BMF_8BPP:		
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_8BPP_GetPixel(SourceSurf, sx, sy);
					DIB_24BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}
		break;

      case BMF_16BPP:
		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_16BPP_GetPixel(SourceSurf, sx, sy);
					DIB_24BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}
	  break;

      case BMF_24BPP:
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_24BPP_GetPixel(SourceSurf, sx, sy);
					DIB_24BPP_PutPixel(DestSurf, DesX, DesY, color);
					}
				}		  
      break;

      case BMF_32BPP:
  		   /* FIXME :  MaskOrigin, BrushOrigin, ClipRegion, Mode ? */
   		   /* This is a reference implementation, it hasn't been optimized for speed */

		  	for (DesY=0; DesY<DestRect->bottom; DesY++)
			{
				if (DesSizeY>SrcSizeY)
				    sy = (int) ((ULONG) SrcSizeY * (ULONG) DesY) / ((ULONG) DesSizeY);
				else
                    sy = (int) ((ULONG) DesSizeY * (ULONG) DesY) / ((ULONG) SrcSizeY); 
                				
				if (sy > SourceRect->bottom) break;

				for (DesX=0; DesX<DestRect->right; DesX++)
				{
					if (DesSizeY>SrcSizeY)
						sx = (int) ((ULONG) SrcSizeX * (ULONG) DesX) / ((ULONG) DesSizeX);
					else
						sx = (int) ((ULONG) DesSizeX * (ULONG) DesX) / ((ULONG) SrcSizeX);                 				       	            
					
					if (sx > SourceRect->right) break;

					color = DIB_32BPP_GetPixel(SourceSurf, sx, sy);
					DIB_24BPP_PutPixel(DestSurf, DesX, DesY, XLATEOBJ_iXlate(ColorTranslation, color));
					}
				}		  
      break;

      default:
      //DPRINT1("DIB_24BPP_StretchBlt: Unhandled Source BPP: %u\n", BitsPerFormat(SourceSurf->iBitmapFormat));
      return FALSE;
    }
  
  return TRUE;
}

BOOLEAN
DIB_24BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  POINTL  *SourcePoint,
                         XLATEOBJ *ColorTranslation, ULONG iTransColor)
{
  ULONG X, Y, SourceX, SourceY, Source, wd, Dest;
  BYTE *DestBits;

  SourceY = SourcePoint->y;
  DestBits = (BYTE*)(DestSurf->pvScan0 +
                      (DestRect->left << 2) +
                      DestRect->top * DestSurf->lDelta);
  wd = DestSurf->lDelta - ((DestRect->right - DestRect->left) << 2);

  for(Y = DestRect->top; Y < DestRect->bottom; Y++)
  {
    SourceX = SourcePoint->x;
    for(X = DestRect->left; X < DestRect->right; X++, DestBits += 3, SourceX++)
    {
      Source = DIB_GetSourceIndex(SourceSurf, SourceX, SourceY);
      if(Source != iTransColor)
      {
        Dest = XLATEOBJ_iXlate(ColorTranslation, Source) & 0xFFFFFF;
         *(PUSHORT)(DestBits) = Dest & 0xFFFF;
         *(DestBits + 2) = Dest >> 16;
      }
    }

    SourceY++;
    DestBits = (BYTE*)((ULONG_PTR)DestBits + wd);
  }

  return TRUE;
}

/* EOF */
