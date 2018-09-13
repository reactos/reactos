/*********************************************************************
   Interlace.cpp

   Definition file for interlace module.
 *********************************************************************/
#include "stdafx.h"
#include "Image.h"
#include "Interlac.h"

/*--------------------------------------------------------------------
            Data Structures and Definitions.
 --------------------------------------------------------------------*/
#define  ADAM7_BLOCK_SIZE    8

short kgacPassScanLines[NUM_PASSES] = { 1, 1, 1, 2, 2, 4, 4 };
short kgacPassStartHorzPosn[NUM_PASSES] = { 0, 4, 0, 2, 0, 1, 0 };
short kgacPassStartVertPosn[NUM_PASSES] = { 0, 0, 4, 0, 2, 0, 1 };
short kgacPassVertIncrements[NUM_PASSES] = { 8, 8, 8, 4, 4, 2, 2 };
short kgacPassHorzIncrements[NUM_PASSES] = { 8, 8, 4, 4, 2, 2, 1 };

/*--------------------------------------------------------------------
            Local Function Prototypes.
 --------------------------------------------------------------------*/

long iFindPass(pADAM7_STRUCT);
long iFindImageLine(pADAM7_STRUCT);

/*--------------------------------------------------------------------
            Export Function Definitions.
 --------------------------------------------------------------------*/

//************************************************************************************
// Given an image described by the parameters of the ADAM7_STRUCT, calculate the
// number of scan lines in the image file which has been interlaced using the Adam7
// scheme.
//************************************************************************************
int iADAM7CalculateNumberOfScanLines(pADAM7_STRUCT ptAdam7)
{
   if (ptAdam7 == NULL)
   {
      return 0;
   }

   if (ptAdam7->iImageHeight == 0 || ptAdam7->iImageWidth == 0)
   {
      return 0;
   }

   if (ptAdam7->iImageHeight < ADAM7_BLOCK_SIZE)
   {
      switch(ptAdam7->iImageHeight)
      {
      case 1:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 1;
         ptAdam7->cPassScanLines[4] = 1;
         ptAdam7->cPassScanLines[5] = 1;
         ptAdam7->cPassScanLines[6] = 0;
         ptAdam7->cTotalScanLines  =  6;
         break;

      case 2:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 1;
         ptAdam7->cPassScanLines[4] = 1;
         ptAdam7->cPassScanLines[5] = 1;
         ptAdam7->cPassScanLines[6] = 1;
         ptAdam7->cTotalScanLines  =  7;
         break;

      case 3:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 1;
         ptAdam7->cPassScanLines[4] = 1;
         ptAdam7->cPassScanLines[5] = 2;
         ptAdam7->cPassScanLines[6] = 1;
         ptAdam7->cTotalScanLines  =  8;
         break;

      case 4:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 1;
         ptAdam7->cPassScanLines[4] = 1;
         ptAdam7->cPassScanLines[5] = 2;
         ptAdam7->cPassScanLines[6] = 2;
         ptAdam7->cTotalScanLines  =  9;
         break;

      case 5:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 2;
         ptAdam7->cPassScanLines[4] = 1;
         ptAdam7->cPassScanLines[5] = 3;
         ptAdam7->cPassScanLines[6] = 2;
         ptAdam7->cTotalScanLines  = 11;
         break;

      case 6:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 2;
         ptAdam7->cPassScanLines[4] = 1;
         ptAdam7->cPassScanLines[5] = 3;
         ptAdam7->cPassScanLines[6] = 3;
         ptAdam7->cTotalScanLines  = 12;
         break;

      case 7:
         ptAdam7->cPassScanLines[0] = 1;
         ptAdam7->cPassScanLines[1] = 1;
         ptAdam7->cPassScanLines[2] = 1;
         ptAdam7->cPassScanLines[3] = 2;
         ptAdam7->cPassScanLines[4] = 2;
         ptAdam7->cPassScanLines[5] = 4;
         ptAdam7->cPassScanLines[6] = 3;
         ptAdam7->cTotalScanLines  = 14;
         break;
      }

      return ptAdam7->cTotalScanLines;
   }


   ptAdam7->cScanBlocks = ptAdam7->iImageHeight / ADAM7_BLOCK_SIZE;
   int iExtraLines = ptAdam7->iImageHeight % ADAM7_BLOCK_SIZE;

   ptAdam7->cTotalScanLines = 0;

   for (int i = 0; i < NUM_PASSES; i++)
   {
      ptAdam7->cPassScanLines[i] = ptAdam7->cScanBlocks * kgacPassScanLines[i];
      ptAdam7->cTotalScanLines += ptAdam7->cPassScanLines[i];
   }

   switch(iExtraLines)
   {
   case 0:
      break;

   case 1:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 0;  // Yes, I could have left these out: hopefully
      ptAdam7->cPassScanLines[3] += 1;
      ptAdam7->cPassScanLines[4] += 0;  // these will help someone else figure out the
      ptAdam7->cPassScanLines[5] += 1;
      ptAdam7->cPassScanLines[6] += 0;  // Adam7 de-interlacing scheme.
      ptAdam7->cTotalScanLines +=  4;
      break;

   case 2:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 0;
      ptAdam7->cPassScanLines[3] += 1;
      ptAdam7->cPassScanLines[4] += 0;
      ptAdam7->cPassScanLines[5] += 1;
      ptAdam7->cPassScanLines[6] += 1;
      ptAdam7->cTotalScanLines +=  5;
      break;

   case 3:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 0;
      ptAdam7->cPassScanLines[3] += 1;
      ptAdam7->cPassScanLines[4] += 1;
      ptAdam7->cPassScanLines[5] += 2;
      ptAdam7->cPassScanLines[6] += 1;
      ptAdam7->cTotalScanLines += 7;
      break;

  case 4:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 0;
      ptAdam7->cPassScanLines[3] += 1;
      ptAdam7->cPassScanLines[4] += 1;
      ptAdam7->cPassScanLines[5] += 2;
      ptAdam7->cPassScanLines[6] += 2;
      ptAdam7->cTotalScanLines += 8;
      break;

   case 5:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 1;
      ptAdam7->cPassScanLines[3] += 2;
      ptAdam7->cPassScanLines[4] += 1;
      ptAdam7->cPassScanLines[5] += 3;
      ptAdam7->cPassScanLines[6] += 2;
      ptAdam7->cTotalScanLines += 11;
      break;

   case 6:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 1;
      ptAdam7->cPassScanLines[3] += 2;
      ptAdam7->cPassScanLines[4] += 1;
      ptAdam7->cPassScanLines[5] += 3;
      ptAdam7->cPassScanLines[6] += 3;
      ptAdam7->cTotalScanLines += 12;
      break;

   case 7:
      ptAdam7->cPassScanLines[0] += 1;
      ptAdam7->cPassScanLines[1] += 1;
      ptAdam7->cPassScanLines[2] += 1;
      ptAdam7->cPassScanLines[3] += 2;
      ptAdam7->cPassScanLines[4] += 2;
      ptAdam7->cPassScanLines[5] += 4;
      ptAdam7->cPassScanLines[6] += 3;
      ptAdam7->cTotalScanLines += 14;
      break;

   default: /* Should never, ever get here!  */
      break;
   }

   return ptAdam7->cTotalScanLines;
}


//************************************************************************************
// Functions to generate a deinterlaced DIB; i.e., each pixel is in BGR in the case
// of RGB/RGBA image classes, and raster line data is stored in a contiguous block.
//************************************************************************************
LPBYTE *ppbADAM7InitDIBPointers(LPBYTE pbDIB, pADAM7_STRUCT ptAdam7, DWORD cbImageLine)
{
   if (ptAdam7 == NULL)
   {
      return NULL;
   }

   if (ptAdam7->iImageHeight == 0 || ptAdam7->iImageWidth == 0)
   {
      return NULL;
   }

   BYTE **ppbRowPtrs = (BYTE **)HeapAlloc(GetProcessHeap(),
           0, sizeof(BYTE *) * ptAdam7->iImageHeight);
   if (ppbRowPtrs == NULL)
   {
      return NULL;
   }

   /*  DIBs are bottom up  */
   for (int i = 0; i < ptAdam7->iImageHeight; i++)
   {
      ppbRowPtrs[i] = pbDIB +
                         ((DWORD)(ptAdam7->iImageHeight - i - 1) * (DWORD)cbImageLine);
   }

   int iScanLines = iADAM7CalculateNumberOfScanLines(ptAdam7);

   ptAdam7->iPassLine = 0;

   return ppbRowPtrs;
}

// The following returns TRUE if the scan line was an empty scan line.
BOOL ADAM7AddRowToDIB(LPBYTE *ppbDIBPtrs, LPBYTE pbScanLine, pADAM7_STRUCT ptAdam7)
{
   BYTE *pbScan;
   BYTE *pbImage;
   BYTE *pbCurrentImageLine;

   long iCurrentPass = iFindPass(ptAdam7);
   long iImageLine = iFindImageLine(ptAdam7);
   long  i;
   if (iImageLine < ptAdam7->iImageHeight)
   {

      pbCurrentImageLine = ppbDIBPtrs[iImageLine];
      for (pbImage = pbCurrentImageLine + (kgacPassStartHorzPosn[iCurrentPass] * ptAdam7->cbPixelSize),
           pbScan = pbScanLine, i = kgacPassStartHorzPosn[iCurrentPass];
           i < ptAdam7->iImageWidth;
           pbImage += (kgacPassHorzIncrements[iCurrentPass] * ptAdam7->cbPixelSize),
           pbScan += ptAdam7->cbPixelSize,
           i += kgacPassHorzIncrements[iCurrentPass])
      {
         {
            switch (ptAdam7->Class)
            {
               case IFLCL_GRAY:
               case IFLCL_GRAYA:
               case IFLCL_PALETTE:
                                 memcpy(pbImage, pbScan, ptAdam7->cbPixelSize);
                                 break;
               case IFLCL_RGBA:
                  *(pbImage + 3) = *(pbScan + 3); // And fall through . . .
               case IFLCL_RGB:
                  *pbImage = *(pbScan + 2);
                  *(pbImage + 1) = *(pbScan + 1);
                  *(pbImage + 2) = *pbScan;
                            break;
               default:
                  return TRUE;
            }
         }
      }
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}

//************************************************************************************
// Generate a deinterlaced image; i.e., each pixel is in RGB in the case
// of RGB/RGBA image classes, and raster line data may not necessarily be stored
// in one contiguous block of memory.
//************************************************************************************

// The following returns TRUE if the scan line was an empty scan line.
BOOL ADAM7AddRowToImageBuffer(LPBYTE ppbImageBuffer[], LPBYTE pbScanLine, pADAM7_STRUCT ptAdam7)
{
   BYTE *pbScan;
   BYTE *pbImage;
   BYTE *pbCurrentImageLine;

   long iCurrentPass = iFindPass(ptAdam7);
   long iImageLine = iFindImageLine(ptAdam7);
   long i;
   if (iImageLine < ptAdam7->iImageHeight)
   {

      pbCurrentImageLine = ppbImageBuffer[iImageLine];
      for (pbImage = pbCurrentImageLine + (kgacPassStartHorzPosn[iCurrentPass] * ptAdam7->cbPixelSize),
           pbScan = pbScanLine, i = kgacPassStartHorzPosn[iCurrentPass];
           i < ptAdam7->iImageWidth;
           pbImage += (kgacPassHorzIncrements[iCurrentPass] * ptAdam7->cbPixelSize),
           pbScan += ptAdam7->cbPixelSize,
           i += kgacPassHorzIncrements[iCurrentPass])
      {
         {
            switch (ptAdam7->Class)
            {
               case IFLCL_GRAY:
               case IFLCL_GRAYA:
               case IFLCL_PALETTE:
                                 memcpy(pbImage, pbScan, ptAdam7->cbPixelSize);
                                 break;
                       case IFLCL_RGBA:
                  *(pbImage + 3) = *(pbScan + 3); // And fall through . . .
               case IFLCL_RGB:
                  *pbImage = *pbScan;
                  *(pbImage + 1) = *(pbScan + 1);
                  *(pbImage + 2) = *(pbScan + 2);
                            break;
               default:
                  return TRUE;
            }
         }
      }
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}

//************************************************************************************
// Generate a deinterlaced alpha channel data block.
//************************************************************************************
BOOL ADAM7RMFDeinterlaceAlpha(LPWORD *ppwInterlaced, LPWORD *ppwDeinterlaced,
                              IFL_ALPHA_CHANNEL_INFO  *ptAlphaInfo)
{
   ADAM7_STRUCT tAdam7;

   if (ppwInterlaced == NULL || ppwDeinterlaced == NULL || ptAlphaInfo == NULL)
   {
      return FALSE;
   }

   tAdam7.iImageHeight = ptAlphaInfo->dwHeight;
   tAdam7.iImageWidth = ptAlphaInfo->dwWidth;
   tAdam7.cbPixelSize = sizeof(WORD);
   tAdam7.iPassLine = 0;
   /* Simulate a class so we can use the AddRowToDIB function above. */
   tAdam7.Class = IFLCL_GRAYA;

   tAdam7.cTotalScanLines = iADAM7CalculateNumberOfScanLines(&tAdam7);

   for (tAdam7.iScanLine = 0; tAdam7.iScanLine < tAdam7.cTotalScanLines; tAdam7.iScanLine++)
   {
      ADAM7AddRowToDIB((BYTE **)ppwDeinterlaced, (BYTE *)(ppwInterlaced[tAdam7.iScanLine]), &tAdam7);
   }

   return TRUE;
}

/*--------------------------------------------------------------------
            Local Function Definitions.
 --------------------------------------------------------------------*/

long iFindPass(pADAM7_STRUCT ptAdam7)
{
   BOOL fFound = FALSE;

   ptAdam7->iPass = 0;
   int iSubTotal = ptAdam7->cPassScanLines[ptAdam7->iPass];
   while (!fFound)
   {
      if (ptAdam7->iScanLine < iSubTotal)
      {
         fFound = TRUE;
         ptAdam7->iPassLine = ptAdam7->iScanLine -
                             (iSubTotal - ptAdam7->cPassScanLines[ptAdam7->iPass]);
      }
      else
      {
         ptAdam7->iPass += 1;
         iSubTotal += ptAdam7->cPassScanLines[ptAdam7->iPass];
      }
   }

   return ptAdam7->iPass;
}

long iFindImageLine(pADAM7_STRUCT ptAdam7)
{
   if (kgacPassStartHorzPosn[ptAdam7->iPass] >= ptAdam7->iImageWidth)
   {
      return (ptAdam7->iImageHeight + 1);
   }

   ptAdam7->iImageLine = kgacPassStartVertPosn[ptAdam7->iPass] +
                         ptAdam7->iPassLine * kgacPassVertIncrements[ptAdam7->iPass];
   return ptAdam7->iImageLine;
}
