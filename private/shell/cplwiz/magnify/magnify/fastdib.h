/******************************Module*Header*******************************\
* Module Name: fastdib.h
*
* CreateCompatibleDIB definitions.
*
* Created: 02-Feb-1996 19:30:45
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _FASTDIB_H_
#define _FASTDIB_H_

HBITMAP APIENTRY CreateCompatibleDIB(HDC hdc, HPALETTE hpal, ULONG ulWidth, ULONG ulHeight, PVOID *ppvBits);
BOOL APIENTRY UpdateDIBColorTable(HDC hdcMem, HDC hdc, HPALETTE hpal);
BOOL APIENTRY GetCompatibleDIBInfo(HBITMAP hbm, PVOID *ppvBase, LONG *plStride);
BOOL APIENTRY GetDIBTranslationVector(HDC hdcMem, HPALETTE hpal, BYTE *pbVector);

#endif //_FASTDIB_H_
