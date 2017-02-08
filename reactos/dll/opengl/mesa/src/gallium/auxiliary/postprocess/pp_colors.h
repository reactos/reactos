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

#ifndef PP_COLORS_H
#define PP_COLORS_H

static const char nored[] = "FRAG\n"
   "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR\n"
   "DCL SAMP[0]\n"
   "DCL TEMP[0]\n"
   "IMM FLT32 {    0.0000,     0.0000,     0.0000,     0.0000}\n"
   "  0: TEX TEMP[0], IN[0].xyyy, SAMP[0], 2D\n"
   "  1: MOV TEMP[0].x, IMM[0].xxxx\n"
   "  2: MOV OUT[0], TEMP[0]\n"
   "  3: END\n";


static const char nogreen[] = "FRAG\n"
   "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR\n"
   "DCL SAMP[0]\n"
   "DCL TEMP[0]\n"
   "IMM FLT32 {    0.0000,     0.0000,     0.0000,     0.0000}\n"
   "  0: TEX TEMP[0], IN[0].xyyy, SAMP[0], 2D\n"
   "  1: MOV TEMP[0].y, IMM[0].xxxx\n"
   "  2: MOV OUT[0], TEMP[0]\n"
   "  3: END\n";


static const char noblue[] = "FRAG\n"
   "PROPERTY FS_COLOR0_WRITES_ALL_CBUFS 1\n"
   "DCL IN[0], GENERIC[0], PERSPECTIVE\n"
   "DCL OUT[0], COLOR\n"
   "DCL SAMP[0]\n"
   "DCL TEMP[0]\n"
   "IMM FLT32 {    0.0000,     0.0000,     0.0000,     0.0000}\n"
   "  0: TEX TEMP[0], IN[0].xyyy, SAMP[0], 2D\n"
   "  1: MOV TEMP[0].z, IMM[0].xxxx\n"
   "  2: MOV OUT[0], TEMP[0]\n"
   "  3: END\n";

#endif
