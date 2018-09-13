#error "@@@ This file is nuked"
/*
 *  _SIFT.H
 *
 *  Purpose:
 *      declarations for sift support api's
 *      also contains declarations for all sifted api's
 *
 *  Author:
 *      alexgo  3/24/95
 *
 */

#ifndef __SIFT_H__
#define __SIFT_H__

#if DBG==1

// clipboard functions
#define SSCloseClipboard() SiftCloseClipboard()
BOOL SiftCloseClipboard(void);

#define SSEmptyClipboard() SiftEmptyClipboard()
BOOL SiftEmptyClipboard(void);

#define SSOpenClipboard(a) SiftOpenClipboard(a)
BOOL SiftOpenClipboard(HWND hwnd);

#define SSGetClipboardData(a) SiftGetClipboardData(a)
HANDLE SiftGetClipboardData( UINT cf );

#define SSSetClipboardData(a,b) SiftSetClipboardData(a,b)
HANDLE SiftSetClipboardData( UINT cf, HANDLE hdata );


#else

// clipboard functions

#define SSOpenClipboard(a) OpenClipboard(a)
#define SSCloseClipboard() CloseClipboard()
#define SSEmptyClipboard() EmptyClipboard()
#define SSSetClipboardData(a,b) SetClipboardData(a,b)
#define SSGetClipboardData(a) GetClipboardData(a)

#endif

#endif // !__SIFT_H__
