/****************************************************************************
 *
 *   capdib.h
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992-1994 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

void SetDefaultCaptureFormat (LPBITMAPINFOHEADER lpbih);
DWORD AllocNewGlobalBitmapInfo (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbi);
DWORD AllocNewBitSpace (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih);
DWORD DibInit (LPCAPSTREAM lpcs);
void DibFini (LPCAPSTREAM lpcs);
DWORD SendDriverFormat (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih, DWORD dwInfoHeaderSize);
DWORD SetFormatFromDIB (LPCAPSTREAM lpcs, LPBITMAPINFOHEADER lpbih);
LPBITMAPINFO DibGetCurrentFormat (LPCAPSTREAM lpcs);
DWORD DibGetNewFormatFromDriver (LPCAPSTREAM lpcs);
BOOL DibNewFormatFromApp (LPCAPSTREAM lpcs, LPBITMAPINFO lpbi, UINT dwSize);
DWORD DibNewPalette (LPCAPSTREAM lpcs, HPALETTE hPalNew);
void DibPaint(LPCAPSTREAM lpcs, HDC hdc);
HANDLE CreatePackedDib (LPBITMAPINFO lpBitsInfo, LPBYTE lpSrcBits, HPALETTE hPalette);
BOOL FAR PASCAL dibIsWritable (LPBITMAPINFO lpBitsInfo);
BOOL FAR PASCAL dibWrite(LPCAPSTREAM lpcs, HMMIO hmmio);
BOOL FAR PASCAL fileSaveDIB(LPCAPSTREAM lpcs, LPTSTR lpszFileName);

