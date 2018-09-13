/******************************Module*Header*******************************\
* Module Name: sscommon.hxx
*
* Defines and externals for screen saver common shell
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __clear_hxx__
#define __clear_hxx__

#include "sscommon.h"


class SS_DIGITAL_DISSOLVE_CLEAR {
public:
    SS_DIGITAL_DISSOLVE_CLEAR();
    ~SS_DIGITAL_DISSOLVE_CLEAR();
//    int Calibrate( int width, int height, int startSize, int fClearTime );
    int CalibrateClear( int width, int height, float fClearTime );
    BOOL Clear( int width, int height, int size );
    BOOL Clear( int width, int height );
private:
    BOOL *rectBuf;
    int rectBufSize;
    int rectSize;
    BOOL ValidateBufSize( int nRects );
    int RectangleCount( int width, int height, int size );
};

extern void DrawGdiRect( HDC hdc, HBRUSH hbr, RECT *pRect );
extern void ss_GdiRectWipeClear( HWND, int, int);

#endif // __clear_hxx__
