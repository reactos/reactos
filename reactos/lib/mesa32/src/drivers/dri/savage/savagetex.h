/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef SAVAGETEX_INC
#define SAVAGETEX_INC

#include "mtypes.h"

#include "savagecontext.h"
#include "texmem.h"

#define SAVAGE_TEX_MAXLEVELS 12

/** \brief Texture tiling information */
typedef struct savage_tileinfo_t {
    GLuint width, height;       /**< tile width and height */
    GLuint wInSub, hInSub;      /**< tile width and height in subtiles */
    GLuint subWidth, subHeight; /**< subtile width and height */
    GLuint tinyOffset[2];       /**< internal offsets size 1 and 2 images */
} savageTileInfo, *savageTileInfoPtr;

typedef struct {
    GLuint offset;
    GLuint nTiles;
    GLuint *dirtyTiles;		/* bit vector of dirty tiles (still unused) */
} savageTexImage;

typedef struct {
    driTextureObject base;

    GLubyte *bufAddr;

    GLuint age;
    savageTexImage image[SAVAGE_TEX_MAXLEVELS];
    GLuint dirtySubImages;

    struct {
	GLuint sWrapMode, tWrapMode;
	GLuint minFilter, magFilter;
	GLuint physAddr;
    } setup;

    GLuint hwFormat;
    GLuint texelBytes;
    const savageTileInfo *tileInfo;
} savageTexObj, *savageTexObjPtr;

#define SAVAGE_NO_PALETTE        0x0
#define SAVAGE_USE_PALETTE       0x1
#define SAVAGE_UPDATE_PALETTE    0x2
#define SAVAGE_FALLBACK_PALETTE  0x4
#define __HWEnvCombineSingleUnitScale(imesa, flag0, flag1, TexBlendCtrl)
#define __HWParseTexEnvCombine(imesa, flag0, TexCtrl, TexBlendCtrl)


void savageUpdateTextureState( GLcontext *ctx );
void savageDDInitTextureFuncs( struct dd_function_table *functions );

void savageDestroyTexObj( savageContextPtr imesa, savageTexObjPtr t );

#endif
