#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include "brush.h"


BOOL VGADDIFillSolid(SURFOBJ *Surface, RECTL Dimensions, ULONG iColor)
{
	int x = Dimensions.left;
	int y = Dimensions.top;
	int w = Dimensions.right - Dimensions.left;
	int h = Dimensions.bottom - Dimensions.top;
	unsigned char *vp, *vpX;
	volatile unsigned char dummy;
	int byte_per_line;
	int i;

	ASSIGNVP4(x, y, vpX)
	get_masks(x, w);
	byte_per_line = SCREEN_X >> 3;
	WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);	/* write mode 2 */
	saved_GC_mode = READ_PORT_UCHAR((PUCHAR)GRA_D);
	WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x02);
	WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);	/* replace */
	saved_GC_fun = READ_PORT_UCHAR((PUCHAR)GRA_D);
	WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0x00);
	WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x08);	/* bit mask */
	saved_GC_mask = READ_PORT_UCHAR((PUCHAR)GRA_D);
	if (leftMask) {
		WRITE_PORT_UCHAR((PUCHAR)GRA_D, leftMask);	/* bit mask */
		/* write to video */
		vp = vpX;
		for (i=h; i>0; i--) {
			dummy = *vp; *vp = iColor;
			vp += byte_per_line;
		}
		vpX++;
	}
	if (byteCounter) {
		WRITE_PORT_UCHAR((PUCHAR)GRA_D, 0xff);	/* bit mask */
		/* write to video */
		vp = vpX;
		for (i=h; i>0; i--) {
			memset(vp, iColor, byteCounter);
			vp += byte_per_line;
		}
		vpX += byteCounter;
	}
	if (rightMask) {
		WRITE_PORT_UCHAR((PUCHAR)GRA_D, rightMask);	/* bit mask */
		/* write to video */
		vp = vpX;
		for (i=h; i>0; i--) {
			dummy = *vp; *vp = iColor;
			vp += byte_per_line;
		}
	}
	/* reset GC register */
	WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mask);
	WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x03);
	WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_fun);
	WRITE_PORT_UCHAR((PUCHAR)GRA_I, 0x05);
	WRITE_PORT_UCHAR((PUCHAR)GRA_D, saved_GC_mode);

	return TRUE;
}

BOOL VGADDIPaintRgn(SURFOBJ *Surface, CLIPOBJ *ClipRegion, ULONG iColor, MIX Mix,
                    BRUSHINST *BrushInst, POINTL *BrushPoint)
{
   RECT_ENUM RectEnum;
   BOOL EnumMore;

   switch(ClipRegion->iMode) {

      case TC_RECTANGLES:

      /* Rectangular clipping can be handled without enumeration.
         Note that trivial clipping is not possible, since the clipping
         region defines the area to fill */

      if (ClipRegion->iDComplexity == DC_RECT)
      {
         VGADDIFillSolid(Surface, ClipRegion->rclBounds, iColor);
      } else {
         /* Enumerate all the rectangles and draw them */

/*         CLIPOBJ_cEnumStart(ClipRegion, FALSE, CT_RECTANGLES, CD_ANY,
                            ENUM_RECT_LIMIT);

         do {
            EnumMore = CLIPOBJ_bEnum(ClipRegion, sizeof(RectEnum), (PVOID) &RectEnum);

            VGADDIFillSolid(Surface, &RectEnum.arcl[0], iColor);

         } while (EnumMore); */
      }

      return(TRUE);

      default:
         return(FALSE);
   }
}

BOOL VGADDIPaint(IN SURFOBJ *Surface, IN CLIPOBJ *ClipRegion,
                 IN BRUSHOBJ *Brush,  IN POINTL *BrushOrigin,
                 IN MIX  Mix)
{
   ULONG iSolidColor;

   iSolidColor = Brush->iSolidColor; // FIXME: Realizations and the like

   // If the foreground and background Mixes are the same,
   // (LATER or if there's no brush mask)
   // then see if we can use the solid brush accelerators

   // FIXME: Put in the mix switch below
   // Brush color parameter doesn't matter for these rops
   return(VGADDIPaintRgn(Surface, ClipRegion, iSolidColor, Mix, NULL, BrushOrigin));

   if ((Mix & 0xFF) == ((Mix >> 8) & 0xFF))
   {
      switch (Mix & 0xFF)
      {
         case 0:
            break;

         // FIXME: Implement all these millions of ROPs
         // For now we don't support brushes -- everything is solid

         case R2_MASKNOTPEN:
         case R2_NOTCOPYPEN:
         case R2_XORPEN:
         case R2_MASKPEN:
         case R2_NOTXORPEN:
         case R2_MERGENOTPEN:
         case R2_COPYPEN:
         case R2_MERGEPEN:
         case R2_NOTMERGEPEN:
         case R2_MASKPENNOT:
         case R2_NOTMASKPEN:
         case R2_MERGEPENNOT:

         // Rops that are implicit solid colors
         case R2_NOT:
         case R2_WHITE:
         case R2_BLACK:

	// FIXME: The Paint region belongs HERE

         case R2_NOP:
            return(TRUE);

         default:
            break;
      }
   }

doBitBlt:

   // If VGADDIPaint can't do it, VGADDIBitBlt can.. or it might just loop back
   // here and we have a nice infinite loop

/*   return( VGADDIBitBlt(Surface, (SURFOBJ *)NULL, (SURFOBJ *)NULL, ClipRegion,
                       (XLATEOBJ *)NULL, &ClipRegion->rclBounds,
                       NULL, (POINTL *)NULL, Brush, BrushOrigin,
                       NULL) ); UNIMPLEMENTED */
}
