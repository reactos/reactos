/******************************Module*Header*******************************\
* Module Name: ssimage.c
*
* Operations on .rgb files
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tk.h"
#include "sscommon.h"

#define IMAGIC      0x01da
#define IMAGIC_SWAP 0xda01

#define SWAP_SHORT_BYTES(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define SWAP_LONG_BYTES(x) (((((x) & 0xff) << 24) | (((x) & 0xff00) << 8)) | \
                            ((((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24)))

typedef struct _rawImageRec {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    unsigned long min, max;
    unsigned long wasteBytes;
    char name[80];
    unsigned long colorMap;
    HANDLE file;
    unsigned char *tmp, *tmpR, *tmpG, *tmpB;
    unsigned long rleEnd;
    unsigned long *rowStart;
    long *rowSize;
    // !!! Hack to stick in a pointer to the resource data - shouldn't be
    // a problem, since rgb files always have 512 byte header
    unsigned char *data;
} rawImageRec;

static void RawImageClose(rawImageRec *raw);

/**************************************************************************\
*
* Hacked form of tk_RGBImageLoad(), for reading a .rgb file from a resource
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
static rawImageRec *RawImageOpen( PVOID pv )
{
    rawImageRec *raw;
    unsigned long *rowStart, *rowSize, ulTmp;
    int x;
    DWORD dwBytesRead;

    raw = (rawImageRec *)malloc(sizeof(rawImageRec));
    if (raw == NULL) {
        return 0;
    }

    // Make a copy of the resource header, since we may be doing some byte
    // swapping, and resources are read-only
    *raw = *((rawImageRec *) pv);

    if (raw->imagic == IMAGIC_SWAP) {
        raw->type = SWAP_SHORT_BYTES(raw->type);
        raw->dim = SWAP_SHORT_BYTES(raw->dim);
        raw->sizeX = SWAP_SHORT_BYTES(raw->sizeX);
        raw->sizeY = SWAP_SHORT_BYTES(raw->sizeY);
        raw->sizeZ = SWAP_SHORT_BYTES(raw->sizeZ);
    }

    raw->tmp = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpR = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpG = (unsigned char *)malloc(raw->sizeX*256);
    raw->tmpB = (unsigned char *)malloc(raw->sizeX*256);
    if (raw->tmp == NULL || raw->tmpR == NULL || raw->tmpG == NULL ||
        raw->tmpB == NULL) {
        RawImageClose(raw);
        return 0;
    }

    if ((raw->type & 0xFF00) == 0x0100) {
        x = raw->sizeY * raw->sizeZ * sizeof(long);
        raw->rowStart = (unsigned long *)malloc(x);
        raw->rowSize = (long *)malloc(x);
        if (raw->rowStart == NULL || raw->rowSize == NULL) {
            RawImageClose(raw);
            return 0;
        }
//mf: not used (?)
        raw->rleEnd = 512 + (2 * x);

        //mf: hack to point to resource data
        raw->data = ((unsigned char *) pv);
        RtlCopyMemory( raw->rowStart, raw->data + 512, x );
        RtlCopyMemory( raw->rowSize, raw->data + 512 + x, x );

        if (raw->imagic == IMAGIC_SWAP) {
            x /= sizeof(long);
            rowStart = raw->rowStart;
            rowSize = (unsigned long *) raw->rowSize;
            while (x--) {
                ulTmp = *rowStart;
                *rowStart++ = SWAP_LONG_BYTES(ulTmp);
                ulTmp = *rowSize;
                *rowSize++ = SWAP_LONG_BYTES(ulTmp);
            }
        }
    }
    return raw;
}

static void RawImageClose(rawImageRec *raw)
{
    if( !raw )
        return;
    if( raw->tmp ) free(raw->tmp);
    if( raw->tmpR ) free(raw->tmpR);
    if( raw->tmpG ) free(raw->tmpG);
    if( raw->tmpB ) free(raw->tmpB);
    free(raw);
}

static void RawImageGetRow(rawImageRec *raw, unsigned char *buf, int y, int z)
{
    unsigned char *iPtr, *oPtr, pixel;
    int count;
    DWORD dwBytesRead;

    if ((raw->type & 0xFF00) == 0x0100) {
        RtlCopyMemory(raw->tmp, raw->data + raw->rowStart[y+z*raw->sizeY],
                 (unsigned int)raw->rowSize[y+z*raw->sizeY] );
        iPtr = raw->tmp;
        oPtr = buf;
        while (1) {
            pixel = *iPtr++;
            count = (int)(pixel & 0x7F);
            if (!count) {
                return;
            }
            if (pixel & 0x80) {
                while (count--) {
                    *oPtr++ = *iPtr++;
                }
            } else {
                pixel = *iPtr++;
                while (count--) {
                    *oPtr++ = pixel;
                }
            }
        }
    } else {
        iPtr = raw->data + 512 + (y*raw->sizeX)+(z*raw->sizeX*raw->sizeY);
        RtlCopyMemory( buf, iPtr, raw->sizeX );
    }
}

static void RawImageGetData(rawImageRec *raw, TEXTURE *ptex)
{
    unsigned char *ptr;
    int i, j;

    ptex->data = (unsigned char *)malloc((raw->sizeX+1)*(raw->sizeY+1)*4);
    if (ptex->data == NULL) {
        return;
    }

    ptr = ptex->data;
    for (i = 0; i < raw->sizeY; i++) {
        RawImageGetRow(raw, raw->tmpR, i, 0);
        RawImageGetRow(raw, raw->tmpG, i, 1);
        RawImageGetRow(raw, raw->tmpB, i, 2);
        for (j = 0; j < raw->sizeX; j++) {
            *ptr++ = *(raw->tmpR + j);
            *ptr++ = *(raw->tmpG + j);
            *ptr++ = *(raw->tmpB + j);
        }
    }
}

BOOL ss_RGBImageLoad( PVOID pv, TEXTURE *ptex )
{
    rawImageRec *raw;

    if( !(raw = RawImageOpen( pv )) )
        return FALSE;
    
    ptex->width = raw->sizeX;
    ptex->height = raw->sizeY;
    ptex->format = GL_RGB;
    ptex->components = 3;
    ptex->pal_size = 0;
    ptex->pal = NULL;
    RawImageGetData(raw, ptex);
    RawImageClose(raw);
    return TRUE;
}

/******************************Public*Routine******************************\
*
* bVerifyRGB
*
* Stripped down version of tkRGBImageLoadAW that verifies that an rgb
* file is valid and, if so, returns the bitmap dimensions.
*
* Returns:
*   TRUE if valid rgb file; otherwise, FALSE.
*
\**************************************************************************/

BOOL 
bVerifyRGB(LPTSTR pszFileName, ISIZE *pSize )
{
    rawImageRec *raw;
    DWORD dwBytesRead;
    BOOL bRet = FALSE;

    raw = (rawImageRec *) 
          LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, sizeof(rawImageRec) );

    if (raw == NULL) {
        return FALSE;
    }

    raw->file = CreateFile((LPTSTR) pszFileName, GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, 0, 0);

    if (raw->file == INVALID_HANDLE_VALUE) {
        goto bVerifyRGB_cleanup;
    }

    ReadFile(raw->file, (LPVOID) raw, 12, &dwBytesRead, (LPOVERLAPPED) NULL);

    if( raw->imagic == IMAGIC_SWAP ) {
        raw->sizeX = SWAP_SHORT_BYTES(raw->sizeX);
        raw->sizeY = SWAP_SHORT_BYTES(raw->sizeY);
        bRet = TRUE;
    } else if( raw->imagic == IMAGIC)
        bRet = TRUE;

bVerifyRGB_cleanup:

    if( bRet && pSize ) {
        pSize->width = raw->sizeX;
        pSize->height = raw->sizeY;
    }
        
    if( raw->file != INVALID_HANDLE_VALUE )
        CloseHandle( raw->file );

    LocalFree( raw );

    return bRet;
}
