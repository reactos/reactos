#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"
#include "brush.h"

BOOL VGADDIFillSolid(SURFOBJ *Surface, RECTL Dimensions, ULONG iColor)
{
  unsigned char a, b, mask;
  unsigned int pre1, i, j, newx;
  unsigned int orgpre1, orgx, midpre1;
  unsigned long leftpixs, midpixs, rightpixs, temp, len;
  long calc;

  orgx=Dimensions.left;
  len=Dimensions.right - Dimensions.left;
  Dimensions.bottom++;

  if(len<8)
  {
    for (i=Dimensions.left; i<Dimensions.left+len; i++)
      vgaPutPixel(i, Dimensions.top, iColor);
  } else {

    leftpixs=Dimensions.left;
    while(leftpixs>8) leftpixs-=8;
    temp = len;
    midpixs = 0;

    while(temp>7)
    {
      temp-=8;
      midpixs++;
    }
    if((temp>=0) && (midpixs>0)) midpixs--;

    pre1=xconv[Dimensions.left]+y80[Dimensions.top];
    orgpre1=pre1;

    // Left
    if(leftpixs==8) {
      // Left edge should be an entire middle bar
      Dimensions.left=orgx;
      leftpixs=0;
    }
    else if(leftpixs>0)
    {
      WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);                 // Set
      WRITE_PORT_UCHAR((PUCHAR)0x3cf,startmasks[leftpixs]); // the MASK

      for(j=Dimensions.top; j<Dimensions.bottom; j++)
      {
        a = READ_REGISTER_UCHAR(vidmem + pre1);
        WRITE_REGISTER_UCHAR(vidmem + pre1, iColor);

        pre1+=80;
      }

      // Middle
      Dimensions.left=orgx+(8-leftpixs)+leftpixs;

    } else {
      // leftpixs == 0
      midpixs+=1;
    }

    if(midpixs>0)
    {
      midpre1=xconv[Dimensions.left]+y80[Dimensions.top];

      // Set mask to all pixels in byte
      WRITE_PORT_UCHAR((PUCHAR)0x3ce, 0x08);
      WRITE_PORT_UCHAR((PUCHAR)0x3cf, 0xff);
      for(j=Dimensions.top; j<Dimensions.bottom; j++)
      {
        memset(vidmem+midpre1, iColor, midpixs);
        midpre1+=80;
      }
    }

    rightpixs = len - ((midpixs*8) + leftpixs);

    if((rightpixs>0))
    {
      Dimensions.left=(orgx+len)-rightpixs;

      // Go backwards till we reach the 8-byte boundary
      while(mod(Dimensions.left, 8)!=0) { Dimensions.left--; rightpixs++; }

      while(rightpixs>7)
      {
        // This is a BAD case as this should have been a midpixs

        for(j=Dimensions.top; j<Dimensions.bottom; j++)
          vgaPutByte(Dimensions.left, j, iColor);

        rightpixs-=8;
        Dimensions.left+=8;
      }

      pre1=xconv[Dimensions.left]+y80[Dimensions.top];
      WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);                // Set
      WRITE_PORT_UCHAR((PUCHAR)0x3cf,endmasks[rightpixs]); // the MASK

      for(j=Dimensions.top; j<Dimensions.bottom; j++)
      {
        a = READ_REGISTER_UCHAR(vidmem + pre1);
        WRITE_REGISTER_UCHAR(vidmem + pre1, iColor);

        pre1+=80;
      }
    }
  }

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
