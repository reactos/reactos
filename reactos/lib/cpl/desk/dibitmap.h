
#ifndef __DIBITMAP_H__
#define __DIBITMAP_H__

#include <windows.h>

typedef struct
{
    BITMAPFILEHEADER *header;
    BITMAPINFO       *info;
    BYTE             *bits;
    
    int               width;
    int               height;

} DIBitmap;

extern DIBitmap *DibLoadImage(TCHAR *filename);
extern void DibFreeImage(DIBitmap *bitmap);

#endif /* __DIBITMAP_H__ */

