/*
 * libtxc_dxtn
 * Version:  1.0
 *
 * Copyright (C) 2004  Roland Scheidegger   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "txc_dxtn.h"

/* weights used for error function, basically weights (unsquared 2/4/1) according to rgb->luminance conversion
   not sure if this really reflects visual perception */
#define REDWEIGHT 4
#define GREENWEIGHT 16
#define BLUEWEIGHT 1

#define ALPHACUT 127

static void fancybasecolorsearch( GLubyte *blkaddr, GLubyte srccolors[4][4][4], GLubyte *bestcolor[2],
                           GLint numxpixels, GLint numypixels, GLint type, GLboolean haveAlpha)
{
   /* use same luminance-weighted distance metric to determine encoding as for finding the base colors */

   /* TODO could also try to find a better encoding for the 3-color-encoding type, this really should be done
      if it's rgba_dxt1 and we have alpha in the block, currently even values which will be mapped to black
      due to their alpha value will influence the result */
   GLint i, j, colors, z;
   GLuint pixerror, pixerrorred, pixerrorgreen, pixerrorblue, pixerrorbest;
   GLint colordist, blockerrlin[2][3];
   GLubyte nrcolor[2];
   GLint pixerrorcolorbest[3];
   GLubyte enc = 0;
   GLubyte cv[4][4];
   GLubyte testcolor[2][3];

/*   fprintf(stderr, "color begin 0 r/g/b %d/%d/%d, 1 r/g/b %d/%d/%d\n",
      bestcolor[0][0], bestcolor[0][1], bestcolor[0][2], bestcolor[1][0], bestcolor[1][1], bestcolor[1][2]);*/
   if (((bestcolor[0][0] & 0xf8) << 8 | (bestcolor[0][1] & 0xfc) << 3 | bestcolor[0][2] >> 3) <
      ((bestcolor[1][0] & 0xf8) << 8 | (bestcolor[1][1] & 0xfc) << 3 | bestcolor[1][2] >> 3)) {
      testcolor[0][0] = bestcolor[0][0];
      testcolor[0][1] = bestcolor[0][1];
      testcolor[0][2] = bestcolor[0][2];
      testcolor[1][0] = bestcolor[1][0];
      testcolor[1][1] = bestcolor[1][1];
      testcolor[1][2] = bestcolor[1][2];
   }
   else {
      testcolor[1][0] = bestcolor[0][0];
      testcolor[1][1] = bestcolor[0][1];
      testcolor[1][2] = bestcolor[0][2];
      testcolor[0][0] = bestcolor[1][0];
      testcolor[0][1] = bestcolor[1][1];
      testcolor[0][2] = bestcolor[1][2];
   }

   for (i = 0; i < 3; i ++) {
      cv[0][i] = testcolor[0][i];
      cv[1][i] = testcolor[1][i];
      cv[2][i] = (testcolor[0][i] * 2 + testcolor[1][i]) / 3;
      cv[3][i] = (testcolor[0][i] + testcolor[1][i] * 2) / 3;
   }

   blockerrlin[0][0] = 0;
   blockerrlin[0][1] = 0;
   blockerrlin[0][2] = 0;
   blockerrlin[1][0] = 0;
   blockerrlin[1][1] = 0;
   blockerrlin[1][2] = 0;

   nrcolor[0] = 0;
   nrcolor[1] = 0;

   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         pixerrorbest = 0xffffffff;
         for (colors = 0; colors < 4; colors++) {
            colordist = srccolors[j][i][0] - (cv[colors][0]);
            pixerror = colordist * colordist * REDWEIGHT;
            pixerrorred = colordist;
            colordist = srccolors[j][i][1] - (cv[colors][1]);
            pixerror += colordist * colordist * GREENWEIGHT;
            pixerrorgreen = colordist;
            colordist = srccolors[j][i][2] - (cv[colors][2]);
            pixerror += colordist * colordist * BLUEWEIGHT;
            pixerrorblue = colordist;
            if (pixerror < pixerrorbest) {
               enc = colors;
               pixerrorbest = pixerror;
               pixerrorcolorbest[0] = pixerrorred;
               pixerrorcolorbest[1] = pixerrorgreen;
               pixerrorcolorbest[2] = pixerrorblue;
            }
         }
         if (enc == 0) {
            for (z = 0; z < 3; z++) {
               blockerrlin[0][z] += 3 * pixerrorcolorbest[z];
            }
            nrcolor[0] += 3;
         }
         else if (enc == 2) {
            for (z = 0; z < 3; z++) {
               blockerrlin[0][z] += 2 * pixerrorcolorbest[z];
            }
            nrcolor[0] += 2;
            for (z = 0; z < 3; z++) {
               blockerrlin[1][z] += 1 * pixerrorcolorbest[z];
            }
            nrcolor[1] += 1;
         }
         else if (enc == 3) {
            for (z = 0; z < 3; z++) {
               blockerrlin[0][z] += 1 * pixerrorcolorbest[z];
            }
            nrcolor[0] += 1;
            for (z = 0; z < 3; z++) {
               blockerrlin[1][z] += 2 * pixerrorcolorbest[z];
            }
            nrcolor[1] += 2;
         }
         else if (enc == 1) {
            for (z = 0; z < 3; z++) {
               blockerrlin[1][z] += 3 * pixerrorcolorbest[z];
            }
            nrcolor[1] += 3;
         }
      }
   }
   if (nrcolor[0] == 0) nrcolor[0] = 1;
   if (nrcolor[1] == 0) nrcolor[1] = 1;
   for (j = 0; j < 2; j++) {
      for (i = 0; i < 3; i++) {
	 GLint newvalue = testcolor[j][i] + blockerrlin[j][i] / nrcolor[j];
	 if (newvalue <= 0)
	    testcolor[j][i] = 0;
	 else if (newvalue >= 255)
	    testcolor[j][i] = 255;
	 else testcolor[j][i] = newvalue;
      }
   }

   if ((abs(testcolor[0][0] - testcolor[1][0]) < 8) &&
       (abs(testcolor[0][1] - testcolor[1][1]) < 4) &&
       (abs(testcolor[0][2] - testcolor[1][2]) < 8)) {
       /* both colors are so close they might get encoded as the same 16bit values */
      GLubyte coldiffred, coldiffgreen, coldiffblue, coldiffmax, factor, ind0, ind1;

      coldiffred = abs(testcolor[0][0] - testcolor[1][0]);
      coldiffgreen = 2 * abs(testcolor[0][1] - testcolor[1][1]);
      coldiffblue = abs(testcolor[0][2] - testcolor[1][2]);
      coldiffmax = coldiffred;
      if (coldiffmax < coldiffgreen) coldiffmax = coldiffgreen;
      if (coldiffmax < coldiffblue) coldiffmax = coldiffblue;
      if (coldiffmax > 0) {
         if (coldiffmax > 4) factor = 2;
         else if (coldiffmax > 2) factor = 3;
         else factor = 4;
         /* Won't do much if the color value is near 255... */
         /* argh so many ifs */
         if (testcolor[1][1] >= testcolor[0][1]) {
            ind1 = 1; ind0 = 0;
         }
         else {
            ind1 = 0; ind0 = 1;
         }
         if ((testcolor[ind1][1] + factor * coldiffgreen) <= 255)
            testcolor[ind1][1] += factor * coldiffgreen;
         else testcolor[ind1][1] = 255;
         if ((testcolor[ind1][0] - testcolor[ind0][1]) > 0) {
            if ((testcolor[ind1][0] + factor * coldiffred) <= 255)
               testcolor[ind1][0] += factor * coldiffred;
            else testcolor[ind1][0] = 255;
         }
         else {
            if ((testcolor[ind0][0] + factor * coldiffred) <= 255)
               testcolor[ind0][0] += factor * coldiffred;
            else testcolor[ind0][0] = 255;
         }
         if ((testcolor[ind1][2] - testcolor[ind0][2]) > 0) {
            if ((testcolor[ind1][2] + factor * coldiffblue) <= 255)
               testcolor[ind1][2] += factor * coldiffblue;
            else testcolor[ind1][2] = 255;
         }
         else {
            if ((testcolor[ind0][2] + factor * coldiffblue) <= 255)
               testcolor[ind0][2] += factor * coldiffblue;
            else testcolor[ind0][2] = 255;
         }
      }
   }

   if (((testcolor[0][0] & 0xf8) << 8 | (testcolor[0][1] & 0xfc) << 3 | testcolor[0][2] >> 3) <
      ((testcolor[1][0] & 0xf8) << 8 | (testcolor[1][1] & 0xfc) << 3 | testcolor[1][2]) >> 3) {
      for (i = 0; i < 3; i++) {
         bestcolor[0][i] = testcolor[0][i];
         bestcolor[1][i] = testcolor[1][i];
      }
   }
   else {
      for (i = 0; i < 3; i++) {
         bestcolor[0][i] = testcolor[1][i];
         bestcolor[1][i] = testcolor[0][i];
      }
   }

/*     fprintf(stderr, "color end 0 r/g/b %d/%d/%d, 1 r/g/b %d/%d/%d\n",
     bestcolor[0][0], bestcolor[0][1], bestcolor[0][2], bestcolor[1][0], bestcolor[1][1], bestcolor[1][2]);*/
}



static void storedxtencodedblock( GLubyte *blkaddr, GLubyte srccolors[4][4][4], GLubyte *bestcolor[2],
                           GLint numxpixels, GLint numypixels, GLuint type, GLboolean haveAlpha)
{
   /* use same luminance-weighted distance metric to determine encoding as for finding the base colors */

   GLint i, j, colors;
   GLuint testerror, testerror2, pixerror, pixerrorbest;
   GLint colordist;
   GLushort color0, color1, tempcolor;
   GLuint bits = 0, bits2 = 0;
   GLubyte *colorptr;
   GLubyte enc = 0;
   GLubyte cv[4][4];

   bestcolor[0][0] = bestcolor[0][0] & 0xf8;
   bestcolor[0][1] = bestcolor[0][1] & 0xfc;
   bestcolor[0][2] = bestcolor[0][2] & 0xf8;
   bestcolor[1][0] = bestcolor[1][0] & 0xf8;
   bestcolor[1][1] = bestcolor[1][1] & 0xfc;
   bestcolor[1][2] = bestcolor[1][2] & 0xf8;

   color0 = bestcolor[0][0] << 8 | bestcolor[0][1] << 3 | bestcolor[0][2] >> 3;
   color1 = bestcolor[1][0] << 8 | bestcolor[1][1] << 3 | bestcolor[1][2] >> 3;
   if (color0 < color1) {
      tempcolor = color0; color0 = color1; color1 = tempcolor;
      colorptr = bestcolor[0]; bestcolor[0] = bestcolor[1]; bestcolor[1] = colorptr;
   }


   for (i = 0; i < 3; i++) {
      cv[0][i] = bestcolor[0][i];
      cv[1][i] = bestcolor[1][i];
      cv[2][i] = (bestcolor[0][i] * 2 + bestcolor[1][i]) / 3;
      cv[3][i] = (bestcolor[0][i] + bestcolor[1][i] * 2) / 3;
   }

   testerror = 0;
   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         pixerrorbest = 0xffffffff;
         for (colors = 0; colors < 4; colors++) {
            colordist = srccolors[j][i][0] - cv[colors][0];
            pixerror = colordist * colordist * REDWEIGHT;
            colordist = srccolors[j][i][1] - cv[colors][1];
            pixerror += colordist * colordist * GREENWEIGHT;
            colordist = srccolors[j][i][2] - cv[colors][2];
            pixerror += colordist * colordist * BLUEWEIGHT;
            if (pixerror < pixerrorbest) {
               pixerrorbest = pixerror;
               enc = colors;
            }
         }
         testerror += pixerrorbest;
         bits |= enc << (2 * (j * 4 + i));
      }
   }
   /* some hw might disagree but actually decoding should always use 4-color encoding
      for non-dxt1 formats */
   if (type == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || type == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
      for (i = 0; i < 3; i++) {
         cv[2][i] = (bestcolor[0][i] + bestcolor[1][i]) / 2;
         /* this isn't used. Looks like the black color constant can only be used
            with RGB_DXT1 if I read the spec correctly (note though that the radeon gpu disagrees,
            it will decode 3 to black even with DXT3/5), and due to how the color searching works
            it won't get used even then */
         cv[3][i] = 0;
      }
      testerror2 = 0;
      for (j = 0; j < numypixels; j++) {
         for (i = 0; i < numxpixels; i++) {
            pixerrorbest = 0xffffffff;
            if ((type == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) && (srccolors[j][i][3] <= ALPHACUT)) {
               enc = 3;
               pixerrorbest = 0; /* don't calculate error */
            }
            else {
               /* we're calculating the same what we have done already for colors 0-1 above... */
               for (colors = 0; colors < 3; colors++) {
                  colordist = srccolors[j][i][0] - cv[colors][0];
                  pixerror = colordist * colordist * REDWEIGHT;
                  colordist = srccolors[j][i][1] - cv[colors][1];
                  pixerror += colordist * colordist * GREENWEIGHT;
                  colordist = srccolors[j][i][2] - cv[colors][2];
                  pixerror += colordist * colordist * BLUEWEIGHT;
                  if (pixerror < pixerrorbest) {
                     pixerrorbest = pixerror;
                     /* need to exchange colors later */
                     if (colors > 1) enc = colors;
                     else enc = colors ^ 1;
                  }
               }
            }
            testerror2 += pixerrorbest;
            bits2 |= enc << (2 * (j * 4 + i));
         }
      }
   } else {
      testerror2 = 0xffffffff;
   }

   /* finally we're finished, write back colors and bits */
   if ((testerror > testerror2) || (haveAlpha)) {
      *blkaddr++ = color1 & 0xff;
      *blkaddr++ = color1 >> 8;
      *blkaddr++ = color0 & 0xff;
      *blkaddr++ = color0 >> 8;
      *blkaddr++ = bits2 & 0xff;
      *blkaddr++ = ( bits2 >> 8) & 0xff;
      *blkaddr++ = ( bits2 >> 16) & 0xff;
      *blkaddr = bits2 >> 24;
   }
   else {
      *blkaddr++ = color0 & 0xff;
      *blkaddr++ = color0 >> 8;
      *blkaddr++ = color1 & 0xff;
      *blkaddr++ = color1 >> 8;
      *blkaddr++ = bits & 0xff;
      *blkaddr++ = ( bits >> 8) & 0xff;
      *blkaddr++ = ( bits >> 16) & 0xff;
      *blkaddr = bits >> 24;
   }
}

static void encodedxtcolorblockfaster( GLubyte *blkaddr, GLubyte srccolors[4][4][4],
                         GLint numxpixels, GLint numypixels, GLuint type )
{
/* simplistic approach. We need two base colors, simply use the "highest" and the "lowest" color
   present in the picture as base colors */

   /* define lowest and highest color as shortest and longest vector to 0/0/0, though the
      vectors are weighted similar to their importance in rgb-luminance conversion
      doesn't work too well though...
      This seems to be a rather difficult problem */

   GLubyte *bestcolor[2];
   GLubyte basecolors[2][3];
   GLubyte i, j;
   GLuint lowcv, highcv, testcv;
   GLboolean haveAlpha = GL_FALSE;

   lowcv = highcv = srccolors[0][0][0] * srccolors[0][0][0] * REDWEIGHT +
                          srccolors[0][0][1] * srccolors[0][0][1] * GREENWEIGHT +
                          srccolors[0][0][2] * srccolors[0][0][2] * BLUEWEIGHT;
   bestcolor[0] = bestcolor[1] = srccolors[0][0];
   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         /* don't use this as a base color if the pixel will get black/transparent anyway */
         if ((type != GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) || (srccolors[j][i][3] > ALPHACUT)) {
            testcv = srccolors[j][i][0] * srccolors[j][i][0] * REDWEIGHT +
                     srccolors[j][i][1] * srccolors[j][i][1] * GREENWEIGHT +
                     srccolors[j][i][2] * srccolors[j][i][2] * BLUEWEIGHT;
            if (testcv > highcv) {
               highcv = testcv;
               bestcolor[1] = srccolors[j][i];
            }
            else if (testcv < lowcv) {
               lowcv = testcv;
               bestcolor[0] = srccolors[j][i];
            }
         }
         else haveAlpha = GL_TRUE;
      }
   }
   /* make sure the original color values won't get touched... */
   for (j = 0; j < 2; j++) {
      for (i = 0; i < 3; i++) {
         basecolors[j][i] = bestcolor[j][i];
      }
   }
   bestcolor[0] = basecolors[0];
   bestcolor[1] = basecolors[1];

   /* try to find better base colors */
   fancybasecolorsearch(blkaddr, srccolors, bestcolor, numxpixels, numypixels, type, haveAlpha);
   /* find the best encoding for these colors, and store the result */
   storedxtencodedblock(blkaddr, srccolors, bestcolor, numxpixels, numypixels, type, haveAlpha);
}

static void writedxt5encodedalphablock( GLubyte *blkaddr, GLubyte alphabase1, GLubyte alphabase2,
                         GLubyte alphaenc[16])
{
   *blkaddr++ = alphabase1;
   *blkaddr++ = alphabase2;
   *blkaddr++ = alphaenc[0] | (alphaenc[1] << 3) | ((alphaenc[2] & 3) << 6);
   *blkaddr++ = (alphaenc[2] >> 2) | (alphaenc[3] << 1) | (alphaenc[4] << 4) | ((alphaenc[5] & 1) << 7);
   *blkaddr++ = (alphaenc[5] >> 1) | (alphaenc[6] << 2) | (alphaenc[7] << 5);
   *blkaddr++ = alphaenc[8] | (alphaenc[9] << 3) | ((alphaenc[10] & 3) << 6);
   *blkaddr++ = (alphaenc[10] >> 2) | (alphaenc[11] << 1) | (alphaenc[12] << 4) | ((alphaenc[13] & 1) << 7);
   *blkaddr++ = (alphaenc[13] >> 1) | (alphaenc[14] << 2) | (alphaenc[15] << 5);
}

static void encodedxt5alpha(GLubyte *blkaddr, GLubyte srccolors[4][4][4],
                            GLint numxpixels, GLint numypixels)
{
   GLubyte alphabase[2], alphause[2];
   GLshort alphatest[2];
   GLuint alphablockerror1, alphablockerror2, alphablockerror3;
   GLubyte i, j, aindex, acutValues[7];
   GLubyte alphaenc1[16], alphaenc2[16], alphaenc3[16];
   GLboolean alphaabsmin = GL_FALSE;
   GLboolean alphaabsmax = GL_FALSE;
   GLshort alphadist;

   /* find lowest and highest alpha value in block, alphabase[0] lowest, alphabase[1] highest */
   alphabase[0] = 0xff; alphabase[1] = 0x0;
   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         if (srccolors[j][i][3] == 0)
            alphaabsmin = GL_TRUE;
         else if (srccolors[j][i][3] == 255)
            alphaabsmax = GL_TRUE;
         else {
            if (srccolors[j][i][3] > alphabase[1])
               alphabase[1] = srccolors[j][i][3];
            if (srccolors[j][i][3] < alphabase[0])
               alphabase[0] = srccolors[j][i][3];
         }
      }
   }


   if ((alphabase[0] > alphabase[1]) && !(alphaabsmin && alphaabsmax)) { /* one color, either max or min */
      /* shortcut here since it is a very common case (and also avoids later problems) */
      /* || (alphabase[0] == alphabase[1] && !alphaabsmin && !alphaabsmax) */
      /* could also test for alpha0 == alpha1 (and not min/max), but probably not common, so don't bother */

      *blkaddr++ = srccolors[0][0][3];
      blkaddr++;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
      *blkaddr++ = 0;
/*      fprintf(stderr, "enc0 used\n");*/
      return;
   }

   /* find best encoding for alpha0 > alpha1 */
   /* it's possible this encoding is better even if both alphaabsmin and alphaabsmax are true */
   alphablockerror1 = 0x0;
   alphablockerror2 = 0xffffffff;
   alphablockerror3 = 0xffffffff;
   if (alphaabsmin) alphause[0] = 0;
   else alphause[0] = alphabase[0];
   if (alphaabsmax) alphause[1] = 255;
   else alphause[1] = alphabase[1];
   /* calculate the 7 cut values, just the middle between 2 of the computed alpha values */
   for (aindex = 0; aindex < 7; aindex++) {
      /* don't forget here is always rounded down */
      acutValues[aindex] = (alphause[0] * (2*aindex + 1) + alphause[1] * (14 - (2*aindex + 1))) / 14;
   }

   for (j = 0; j < numypixels; j++) {
      for (i = 0; i < numxpixels; i++) {
         /* maybe it's overkill to have the most complicated calculation just for the error
            calculation which we only need to figure out if encoding1 or encoding2 is better... */
         if (srccolors[j][i][3] > acutValues[0]) {
            alphaenc1[4*j + i] = 0;
            alphadist = srccolors[j][i][3] - alphause[1];
         }
         else if (srccolors[j][i][3] > acutValues[1]) {
            alphaenc1[4*j + i] = 2;
            alphadist = srccolors[j][i][3] - (alphause[1] * 6 + alphause[0] * 1) / 7;
         }
         else if (srccolors[j][i][3] > acutValues[2]) {
            alphaenc1[4*j + i] = 3;
            alphadist = srccolors[j][i][3] - (alphause[1] * 5 + alphause[0] * 2) / 7;
         }
         else if (srccolors[j][i][3] > acutValues[3]) {
            alphaenc1[4*j + i] = 4;
            alphadist = srccolors[j][i][3] - (alphause[1] * 4 + alphause[0] * 3) / 7;
         }
         else if (srccolors[j][i][3] > acutValues[4]) {
            alphaenc1[4*j + i] = 5;
            alphadist = srccolors[j][i][3] - (alphause[1] * 3 + alphause[0] * 4) / 7;
         }
         else if (srccolors[j][i][3] > acutValues[5]) {
            alphaenc1[4*j + i] = 6;
            alphadist = srccolors[j][i][3] - (alphause[1] * 2 + alphause[0] * 5) / 7;
         }
         else if (srccolors[j][i][3] > acutValues[6]) {
            alphaenc1[4*j + i] = 7;
            alphadist = srccolors[j][i][3] - (alphause[1] * 1 + alphause[0] * 6) / 7;
         }
         else {
            alphaenc1[4*j + i] = 1;
            alphadist = srccolors[j][i][3] - alphause[0];
         }
         alphablockerror1 += alphadist * alphadist;
      }
   }
/*      for (i = 0; i < 16; i++) {
         fprintf(stderr, "%d ", alphaenc1[i]);
      }
      fprintf(stderr, "cutVals ");
      for (i = 0; i < 8; i++) {
         fprintf(stderr, "%d ", acutValues[i]);
      }
      fprintf(stderr, "srcVals ");
      for (j = 0; j < numypixels; j++)
         for (i = 0; i < numxpixels; i++) {
            fprintf(stderr, "%d ", srccolors[j][i][3]);
         }

      fprintf(stderr, "\n");
   }*/
   /* it's not very likely this encoding is better if both alphaabsmin and alphaabsmax
      are false but try it anyway */
   if (alphablockerror1 >= 32) {

      /* don't bother if encoding is already very good, this condition should also imply
      we have valid alphabase colors which we absolutely need (alphabase[0] <= alphabase[1]) */
      alphablockerror2 = 0;
      for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
         acutValues[aindex] = (alphabase[0] * (10 - (2*aindex + 1)) + alphabase[1] * (2*aindex + 1)) / 10;
      }
      for (j = 0; j < numypixels; j++) {
         for (i = 0; i < numxpixels; i++) {
             /* maybe it's overkill to have the most complicated calculation just for the error
               calculation which we only need to figure out if encoding1 or encoding2 is better... */
            if (srccolors[j][i][3] == 0) {
               alphaenc2[4*j + i] = 6;
               alphadist = 0;
            }
            else if (srccolors[j][i][3] == 255) {
               alphaenc2[4*j + i] = 7;
               alphadist = 0;
            }
            else if (srccolors[j][i][3] <= acutValues[0]) {
               alphaenc2[4*j + i] = 0;
               alphadist = srccolors[j][i][3] - alphabase[0];
            }
            else if (srccolors[j][i][3] <= acutValues[1]) {
               alphaenc2[4*j + i] = 2;
               alphadist = srccolors[j][i][3] - (alphabase[0] * 4 + alphabase[1] * 1) / 5;
            }
            else if (srccolors[j][i][3] <= acutValues[2]) {
               alphaenc2[4*j + i] = 3;
               alphadist = srccolors[j][i][3] - (alphabase[0] * 3 + alphabase[1] * 2) / 5;
            }
            else if (srccolors[j][i][3] <= acutValues[3]) {
               alphaenc2[4*j + i] = 4;
               alphadist = srccolors[j][i][3] - (alphabase[0] * 2 + alphabase[1] * 3) / 5;
            }
            else if (srccolors[j][i][3] <= acutValues[4]) {
               alphaenc2[4*j + i] = 5;
               alphadist = srccolors[j][i][3] - (alphabase[0] * 1 + alphabase[1] * 4) / 5;
            }
            else {
               alphaenc2[4*j + i] = 1;
               alphadist = srccolors[j][i][3] - alphabase[1];
            }
            alphablockerror2 += alphadist * alphadist;
         }
      }


      /* skip this if the error is already very small
         this encoding is MUCH better on average than #2 though, but expensive! */
      if ((alphablockerror2 > 96) && (alphablockerror1 > 96)) {
         GLshort blockerrlin1 = 0;
         GLshort blockerrlin2 = 0;
         GLubyte nralphainrangelow = 0;
         GLubyte nralphainrangehigh = 0;
         alphatest[0] = 0xff;
         alphatest[1] = 0x0;
         /* if we have large range it's likely there are values close to 0/255, try to map them to 0/255 */
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
               if ((srccolors[j][i][3] > alphatest[1]) && (srccolors[j][i][3] < (255 -(alphabase[1] - alphabase[0]) / 28)))
                  alphatest[1] = srccolors[j][i][3];
               if ((srccolors[j][i][3] < alphatest[0]) && (srccolors[j][i][3] > (alphabase[1] - alphabase[0]) / 28))
                  alphatest[0] = srccolors[j][i][3];
            }
         }
          /* shouldn't happen too often, don't really care about those degenerated cases */
          if (alphatest[1] <= alphatest[0]) {
             alphatest[0] = 1;
             alphatest[1] = 254;
/*             fprintf(stderr, "only 1 or 0 colors for encoding!\n");*/
         }
         for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
            acutValues[aindex] = (alphatest[0] * (10 - (2*aindex + 1)) + alphatest[1] * (2*aindex + 1)) / 10;
         }

         /* find the "average" difference between the alpha values and the next encoded value.
            This is then used to calculate new base values.
            Should there be some weighting, i.e. those values closer to alphatest[x] have more weight,
            since they will see more improvement, and also because the values in the middle are somewhat
            likely to get no improvement at all (because the base values might move in different directions)?
            OTOH it would mean the values in the middle are even less likely to get an improvement
         */
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
               if (srccolors[j][i][3] <= alphatest[0] / 2) {
               }
               else if (srccolors[j][i][3] > ((255 + alphatest[1]) / 2)) {
               }
               else if (srccolors[j][i][3] <= acutValues[0]) {
                  blockerrlin1 += (srccolors[j][i][3] - alphatest[0]);
                  nralphainrangelow += 1;
               }
               else if (srccolors[j][i][3] <= acutValues[1]) {
                  blockerrlin1 += (srccolors[j][i][3] - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                  blockerrlin2 += (srccolors[j][i][3] - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i][3] <= acutValues[2]) {
                  blockerrlin1 += (srccolors[j][i][3] - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                  blockerrlin2 += (srccolors[j][i][3] - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i][3] <= acutValues[3]) {
                  blockerrlin1 += (srccolors[j][i][3] - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                  blockerrlin2 += (srccolors[j][i][3] - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
               }
               else if (srccolors[j][i][3] <= acutValues[4]) {
                  blockerrlin1 += (srccolors[j][i][3] - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                  blockerrlin2 += (srccolors[j][i][3] - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                  nralphainrangelow += 1;
                  nralphainrangehigh += 1;
                  }
               else {
                  blockerrlin2 += (srccolors[j][i][3] - alphatest[1]);
                  nralphainrangehigh += 1;
               }
            }
         }
         /* shouldn't happen often, needed to avoid div by zero */
         if (nralphainrangelow == 0) nralphainrangelow = 1;
         if (nralphainrangehigh == 0) nralphainrangehigh = 1;
         alphatest[0] = alphatest[0] + (blockerrlin1 / nralphainrangelow);
/*         fprintf(stderr, "block err lin low %d, nr %d\n", blockerrlin1, nralphainrangelow);
         fprintf(stderr, "block err lin high %d, nr %d\n", blockerrlin2, nralphainrangehigh);*/
         /* again shouldn't really happen often... */
         if (alphatest[0] < 0) {
            alphatest[0] = 0;
/*            fprintf(stderr, "adj alpha base val to 0\n");*/
         }
         alphatest[1] = alphatest[1] + (blockerrlin2 / nralphainrangehigh);
         if (alphatest[1] > 255) {
            alphatest[1] = 255;
/*            fprintf(stderr, "adj alpha base val to 255\n");*/
         }

         alphablockerror3 = 0;
         for (aindex = 0; aindex < 5; aindex++) {
         /* don't forget here is always rounded down */
            acutValues[aindex] = (alphatest[0] * (10 - (2*aindex + 1)) + alphatest[1] * (2*aindex + 1)) / 10;
         }
         for (j = 0; j < numypixels; j++) {
            for (i = 0; i < numxpixels; i++) {
                /* maybe it's overkill to have the most complicated calculation just for the error
                  calculation which we only need to figure out if encoding1 or encoding2 is better... */
               if (srccolors[j][i][3] <= alphatest[0] / 2) {
                  alphaenc3[4*j + i] = 6;
                  alphadist = srccolors[j][i][3];
               }
               else if (srccolors[j][i][3] > ((255 + alphatest[1]) / 2)) {
                  alphaenc3[4*j + i] = 7;
                  alphadist = 255 - srccolors[j][i][3];
               }
               else if (srccolors[j][i][3] <= acutValues[0]) {
                  alphaenc3[4*j + i] = 0;
                  alphadist = srccolors[j][i][3] - alphatest[0];
               }
               else if (srccolors[j][i][3] <= acutValues[1]) {
                 alphaenc3[4*j + i] = 2;
                 alphadist = srccolors[j][i][3] - (alphatest[0] * 4 + alphatest[1] * 1) / 5;
               }
               else if (srccolors[j][i][3] <= acutValues[2]) {
                  alphaenc3[4*j + i] = 3;
                  alphadist = srccolors[j][i][3] - (alphatest[0] * 3 + alphatest[1] * 2) / 5;
               }
               else if (srccolors[j][i][3] <= acutValues[3]) {
                  alphaenc3[4*j + i] = 4;
                  alphadist = srccolors[j][i][3] - (alphatest[0] * 2 + alphatest[1] * 3) / 5;
               }
               else if (srccolors[j][i][3] <= acutValues[4]) {
                  alphaenc3[4*j + i] = 5;
                  alphadist = srccolors[j][i][3] - (alphatest[0] * 1 + alphatest[1] * 4) / 5;
               }
               else {
                  alphaenc3[4*j + i] = 1;
                  alphadist = srccolors[j][i][3] - alphatest[1];
               }
               alphablockerror3 += alphadist * alphadist;
            }
         }
      }
   }
  /* write the alpha values and encoding back. */
   if ((alphablockerror1 <= alphablockerror2) && (alphablockerror1 <= alphablockerror3)) {
/*      if (alphablockerror1 > 96) fprintf(stderr, "enc1 used, error %d\n", alphablockerror1);*/
      writedxt5encodedalphablock( blkaddr, alphause[1], alphause[0], alphaenc1 );
   }
   else if (alphablockerror2 <= alphablockerror3) {
/*      if (alphablockerror2 > 96) fprintf(stderr, "enc2 used, error %d\n", alphablockerror2);*/
      writedxt5encodedalphablock( blkaddr, alphabase[0], alphabase[1], alphaenc2 );
   }
   else {
/*      fprintf(stderr, "enc3 used, error %d\n", alphablockerror3);*/
      writedxt5encodedalphablock( blkaddr, (GLubyte)alphatest[0], (GLubyte)alphatest[1], alphaenc3 );
   }
}

static void extractsrccolors( GLubyte srcpixels[4][4][4], const GLchan *srcaddr,
                         GLint srcRowStride, GLint numxpixels, GLint numypixels, GLint comps)
{
   GLubyte i, j, c;
   const GLchan *curaddr;
   for (j = 0; j < numypixels; j++) {
      curaddr = srcaddr + j * srcRowStride * comps;
      for (i = 0; i < numxpixels; i++) {
         for (c = 0; c < comps; c++) {
            srcpixels[j][i][c] = *curaddr++ / (CHAN_MAX / 255);
         }
      }
   }
}


void tx_compress_dxtn(GLint srccomps, GLint width, GLint height, const GLubyte *srcPixData,
                     GLenum destFormat, GLubyte *dest, GLint dstRowStride)
{
      GLubyte *blkaddr = dest;
      GLubyte srcpixels[4][4][4];
      const GLchan *srcaddr = srcPixData;
      GLint numxpixels, numypixels;
      GLint i, j;
      GLint dstRowDiff;

   switch (destFormat) {
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
      /* hmm we used to get called without dstRowStride... */
      dstRowDiff = dstRowStride >= (width * 2) ? dstRowStride - (((width + 3) & ~3) * 2) : 0;
/*      fprintf(stderr, "dxt1 tex width %d tex height %d dstRowStride %d\n",
              width, height, dstRowStride); */
      for (j = 0; j < height; j += 4) {
         if (height > j + 3) numypixels = 4;
         else numypixels = height - j;
         srcaddr = srcPixData + j * width * srccomps;
         for (i = 0; i < width; i += 4) {
            if (width > i + 3) numxpixels = 4;
            else numxpixels = width - i;
            extractsrccolors(srcpixels, srcaddr, width, numxpixels, numypixels, srccomps);
            encodedxtcolorblockfaster(blkaddr, srcpixels, numxpixels, numypixels, destFormat);
            srcaddr += srccomps * numxpixels;
            blkaddr += 8;
         }
         blkaddr += dstRowDiff;
      }
      break;
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
      dstRowDiff = dstRowStride >= (width * 4) ? dstRowStride - (((width + 3) & ~3) * 4) : 0;
/*      fprintf(stderr, "dxt3 tex width %d tex height %d dstRowStride %d\n",
              width, height, dstRowStride); */
      for (j = 0; j < height; j += 4) {
         if (height > j + 3) numypixels = 4;
         else numypixels = height - j;
         srcaddr = srcPixData + j * width * srccomps;
         for (i = 0; i < width; i += 4) {
            if (width > i + 3) numxpixels = 4;
            else numxpixels = width - i;
            extractsrccolors(srcpixels, srcaddr, width, numxpixels, numypixels, srccomps);
            *blkaddr++ = (srcpixels[0][0][3] >> 4) | (srcpixels[0][1][3] & 0xf0);
            *blkaddr++ = (srcpixels[0][2][3] >> 4) | (srcpixels[0][3][3] & 0xf0);
            *blkaddr++ = (srcpixels[1][0][3] >> 4) | (srcpixels[1][1][3] & 0xf0);
            *blkaddr++ = (srcpixels[1][2][3] >> 4) | (srcpixels[1][3][3] & 0xf0);
            *blkaddr++ = (srcpixels[2][0][3] >> 4) | (srcpixels[2][1][3] & 0xf0);
            *blkaddr++ = (srcpixels[2][2][3] >> 4) | (srcpixels[2][3][3] & 0xf0);
            *blkaddr++ = (srcpixels[3][0][3] >> 4) | (srcpixels[3][1][3] & 0xf0);
            *blkaddr++ = (srcpixels[3][2][3] >> 4) | (srcpixels[3][3][3] & 0xf0);
            encodedxtcolorblockfaster(blkaddr, srcpixels, numxpixels, numypixels, destFormat);
            srcaddr += srccomps * numxpixels;
            blkaddr += 8;
         }
         blkaddr += dstRowDiff;
      }
      break;
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
      dstRowDiff = dstRowStride >= (width * 4) ? dstRowStride - (((width + 3) & ~3) * 4) : 0;
/*      fprintf(stderr, "dxt5 tex width %d tex height %d dstRowStride %d\n",
              width, height, dstRowStride); */
      for (j = 0; j < height; j += 4) {
         if (height > j + 3) numypixels = 4;
         else numypixels = height - j;
         srcaddr = srcPixData + j * width * srccomps;
         for (i = 0; i < width; i += 4) {
            if (width > i + 3) numxpixels = 4;
            else numxpixels = width - i;
            extractsrccolors(srcpixels, srcaddr, width, numxpixels, numypixels, srccomps);
            encodedxt5alpha(blkaddr, srcpixels, numxpixels, numypixels);
            encodedxtcolorblockfaster(blkaddr + 8, srcpixels, numxpixels, numypixels, destFormat);
            srcaddr += srccomps * numxpixels;
            blkaddr += 16;
         }
         blkaddr += dstRowDiff;
      }
      break;
   default:
      /* fprintf(stderr, "libdxtn: Bad dstFormat %d in tx_compress_dxtn\n", destFormat); */
      return;
   }
}
