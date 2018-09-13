/******************************Module*Header*******************************\
* Module Name: sswindow.hxx
*
* 
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __sswindow_hxx__
#define __sswindow_hxx__

#include "sscommon.h"

// SSW flags
#define SS_HRC_PROXY_BIT       (1 << 0)

/**************************************************************************\
* SSW 
*
\**************************************************************************/

// GL config struct

typedef struct {
    int     pfFlags; // pixel format flags
    HGLRC   hrc;
    STRETCH_INFO *pStretch;
} SS_GL_CONFIG;

class SSW {
public:
    int     wFlags;  // various window flags
    HWND    hwnd;    // NULL indicates logical sub-window
    BOOL    bOwnWindow;  // TRUE if we created the window, otherwise system 
                         // window
    int     iSubWindow;  // Reference count for sub-window children
    BOOL    bValidateBg;
    SSW     *psswParent;
    SSW     *psswSibling;
    SSW     *psswChildren;
    HDC     hdc;
    HGLRC   hrc; // Can be for this window or a bitmap in pStretch
    ISIZE   size;
    IPOINT2D pos;  // pos'n relative to parent's origin
    BOOL    bDoubleBuf;
    SSINITPROC      InitFunc;
    SSUPDATEPROC    UpdateFunc;
    SSRESHAPEPROC   ReshapeFunc;
    SSREPAINTPROC   RepaintFunc;
    SSFLOATERBOUNCEPROC FloaterBounceFunc;
    SSFINISHPROC    FinishFunc;
    SSCHILDSIZEPROC ChildSizeFunc;
    MOTION  *pMotion; // if this window moves
    STRETCH_INFO *pStretch;
    void *DataPtr;
    WNDPROC wndProc;
    SS_GL_CONFIG *pGLc;   // GL configuration

    SSW( SSW *psswParent, ISIZE *pSize, IPOINT2D *pPos, BOOL bMotion,
            SSCHILDSIZEPROC ChildSizeFuncArg );
    SSW( SSW *psswParentArg, HWND hwndArg );
    ~SSW();
    BOOL    CreateSSWindow( HINSTANCE hMainInstance, UINT uStyle, UINT uExStyle,
                          LPCTSTR pszWindowTitle, WNDPROC wndProcArg,
                          LPCTSTR pszClassName, HWND hwndParentOverride );
    void    Reset();    // Set to default init state

    void    SetInitFunc(SSINITPROC);
    void    SetReshapeFunc(SSRESHAPEPROC);
    void    SetRepaintFunc(SSREPAINTPROC);
    void    SetUpdateFunc(SSUPDATEPROC);
    void    SetFinishFunc(SSFINISHPROC);
    void    SetFloaterBounceFunc(SSFLOATERBOUNCEPROC);
    void    SetCallbackData( void *pData ) { DataPtr = pData; };

    BOOL    ConfigureForGL( SS_GL_CONFIG *pGLcArg );
    BOOL    ConfigureForGL();
    BOOL    ConfigureForGdi();
    void    MakeCurrent();
    void    InitGL();  // Initialize GL
    HGLRC   GetHRC() { return hrc; };
    void    GdiClear();
    void    Start();  // Start drawing
    void    Resize( int width, int height ); // called on WM_RESIZE
    void    Repaint( BOOL bCheckUpdateRect ); // called on WM_REPAINT
    void    Reshape(); // Call back to ss to reshape its GL draw area
    void    UpdateWindow();
    void    GetChildInfo();
    BOOL    SetAspectRatio( float fAspect );
    void    RandomWindowPos();
    void    GetSize( ISIZE *pSize ) { *pSize = size; };

private:
    GLRECT  lastRect;   // for optimizing HW sub-window clears
    void    AddChild( SSW *psswChild );
    BOOL    RemoveChild( SSW *psswChild );
    HGLRC   hrcSetupGL();// pixelformat, createcontext, etc.
    BOOL    NeedStretchedWindow();
    void    SwapSSBuffers();
    void    SwapStretchBuffers();
    void    ResizeStretch();
    BOOL    bValidateChildPos();
    void    ValidateChildSize();
    void    ResetMotion();
    void    SetSSWindowPos( int flags );
    void    SetSSWindowPos();
    void    MoveSSWindow( BOOL bRedrawBg );
//mf: can't have function ptrs to member functions!
//    void    (*UpdateDoubleBufWinFunc)();
    void    UpdateDoubleBufWin(); // normal double buffer update
    void    UpdateDoubleBufSubWin();    // sub-win case
    BOOL    CalcNextWindowPos();
    void    GetSSWindowRect( LPRECT lpRect );
    int     GLPosY();  // Convert window pos.y from gdi to GL
};

typedef SSW*    PSSW;

#endif // __sswindow_hxx__
