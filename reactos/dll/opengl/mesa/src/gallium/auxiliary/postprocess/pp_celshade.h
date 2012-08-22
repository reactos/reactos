/**************************************************************************
 *
 * Copyright 2011 Lauri Kasanen
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef CELSHADE_H
#define CELSHADE_H

static const char celshade[] = "FRAG\n"
   "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR\n"
   "DCL SAMP[0]\n"
   "DCL TEMP[0..4]\n"
   "IMM FLT32 {    0.2126,     0.7152,     0.0722,     4.0000}\n"
   "IMM FLT32 {    0.5000,     2.0000,     1.0000,    -0.1250}\n"
   "IMM FLT32 {    0.2500,     0.1000,     0.1250,     3.0000}\n"
   "  0: TEX TEMP[0], IN[0].xyyy, SAMP[0], 2D\n"
   "  1: DP3 TEMP[1].x, TEMP[0].xyzz, IMM[0]\n"
   "  2: MUL TEMP[3].x, TEMP[1].xxxx, IMM[0].wwww\n"
   "  3: ROUND TEMP[2].x, TEMP[3].xxxx\n"
   "  4: MUL TEMP[3].x, TEMP[2].xxxx, IMM[2].xxxx\n"
   "  5: MOV TEMP[2].x, TEMP[3].xxxx\n"
   "  6: ADD TEMP[4].x, TEMP[1].xxxx, -TEMP[3].xxxx\n"
   "  7: SGT TEMP[1].w, TEMP[4].xxxx, IMM[2].yyyy\n"
   "  8: IF TEMP[1].wwww :19\n"
   "  9:   ADD TEMP[4].y, TEMP[3].xxxx, IMM[2].yyyy\n"
   " 10:   ADD TEMP[1].z, TEMP[1].xxxx, -TEMP[4].yyyy\n"
   " 11:   ADD TEMP[1].y, TEMP[3].xxxx, IMM[2].zzzz\n"
   " 12:   ADD TEMP[2].x, TEMP[1].yyyy, -TEMP[4].yyyy\n"
   " 13:   RCP TEMP[4].y, TEMP[2].xxxx\n"
   " 14:   MUL TEMP[2].x, TEMP[1].zzzz, TEMP[4].yyyy\n"
   " 15:   MAD TEMP[1].y, -IMM[1].yyyy, TEMP[2].xxxx, IMM[2].wwww\n"
   " 16:   MUL TEMP[1].z, TEMP[2].xxxx, TEMP[1].yyyy\n"
   " 17:   MUL TEMP[1].y, TEMP[2].xxxx, TEMP[1].zzzz\n"
   " 18:   MAD TEMP[2].x, TEMP[1].yyyy, IMM[2].zzzz, TEMP[3].xxxx\n"
   " 19: ENDIF\n"
   " 20: SLT TEMP[3].x, TEMP[4].xxxx, -IMM[2].yyyy\n"
   " 21: IF TEMP[3].xxxx :34\n"
   " 22:   ADD TEMP[3].x, TEMP[2].xxxx, -IMM[2].zzzz\n"
   " 23:   ADD TEMP[4].x, TEMP[1].xxxx, -TEMP[3].xxxx\n"
   " 24:   ADD TEMP[1].x, TEMP[2].xxxx, -IMM[2].yyyy\n"
   " 25:   ADD TEMP[4].y, TEMP[1].xxxx, -TEMP[3].xxxx\n"
   " 26:   RCP TEMP[3].x, TEMP[4].yyyy\n"
   " 27:   MUL TEMP[1].x, TEMP[4].xxxx, TEMP[3].xxxx\n"
   " 28:   MAD TEMP[4].x, -IMM[1].yyyy, TEMP[1].xxxx, IMM[2].wwww\n"
   " 29:   MUL TEMP[3].x, TEMP[1].xxxx, TEMP[4].xxxx\n"
   " 30:   MUL TEMP[4].x, TEMP[1].xxxx, TEMP[3].xxxx\n"
   " 31:   ADD TEMP[3].x, IMM[1].zzzz, -TEMP[4].xxxx\n"
   " 32:   MAD TEMP[1].x, TEMP[3].xxxx, -IMM[2].zzzz, TEMP[2].xxxx\n"
   " 33:   MOV TEMP[2].x, TEMP[1].xxxx\n"
   " 34: ENDIF\n"
   " 35: MAD TEMP[1].x, TEMP[2].xxxx, IMM[1].yyyy, IMM[2].yyyy\n"
   " 36: MUL OUT[0], TEMP[0], TEMP[1].xxxx\n"
   " 37: END\n";

#endif
