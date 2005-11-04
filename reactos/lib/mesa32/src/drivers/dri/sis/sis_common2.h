/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_common.h,v 1.5 2000/09/26 15:56:48 tsi Exp $ */

/*
 * Authors:
 *    Sung-Ching Lin <sclin@sis.com.tw>
 */

#ifndef _sis_common_h_
#define _sis_common_h_

#if 0
#define free(x)
#define calloc(x,y) sis_debug_malloc((x)*(y))
extern void *sis_debug_malloc(int x);
#endif

#if defined(SIS_DUMP)
#include "sis_debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct _Box
{
  short x1, y1, x2, y2;
}
BoxRec;
#define NullBox ((BoxPtr)0)
typedef struct _Box *BoxPtr;

/* BitBlt Commands */
#define CMD0_DD_ENABLE      0x06
#define CMD0_SRC_VIDEO      0x00
#define CMD0_SRC_CPU        0x10
#define CMD0_PAT_FG_COLOR   0x00
#define CMD1_DIR_X_DEC      0x00
#define CMD1_DIR_X_INC      0x01
#define CMD1_DIR_Y_DEC      0x00
#define CMD1_DIR_Y_INC      0x02
#define REG_SRC_ADDR        0x8200
#define REG_CMD0            0x823c

typedef struct
{
  GLshort wSrcPitch;
  GLshort wDestPitch;
}
_PITCH;
typedef struct
{
  GLshort wWidth;
  GLshort wHeight;
}
_DIM;
typedef struct
{
  GLshort wY;
  GLshort wX;
}
_POS;

typedef struct
{
  GLubyte cCmd0;
  GLubyte cRop;
  GLubyte cCmd1;
  GLubyte cReserved;
}
_CMD;

typedef struct
{
  GLshort wStatus0;
  GLbyte cStatus0_GLbyte3;
  GLbyte cStatus0_GLbyte4;
}
_CMDQUESTATUS;

typedef struct
{
  GLint dwSrcBaseAddr;
  GLint dwSrcPitch;
  _POS stdwSrcPos;
  _POS stdwDestPos;
  GLint dwDestBaseAddr;
  GLshort wDestPitch;
  GLshort wDestHeight;
  _DIM stdwDim;
  GLint dwFgRopColor;
  GLint dwBgRopColor;
  GLint dwSrcHiCKey;
  GLint dwSrcLoCKey;
  GLint dwMaskA;
  GLint dwMaskB;
  GLint dwClipA;
  GLint dwClipB;
  _CMD stdwCmd;
  _CMDQUESTATUS stdwCmdQueStatus;
}
ENGPACKET, *LPENGPACKET;

/* Hardware Info */
#include "sis_reg.h"

/* HW capability */
#define SIS_MAX_TEXTURE_SIZE 2048
#define SIS_MAX_TEXTURES 2

#define SIS_MAX_FRAME_LENGTH 3

GLint doFPtoFixedNoRound( GLfloat dwInValue, int nFraction );

#endif
