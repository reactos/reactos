/******************************Module*Header*******************************\
* Module Name: sswindow.cxx
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "scrnsave.h"

#include "glscrnsv.h"
#include "ssintrnl.hxx"
#include "sswindow.hxx"
#include "ssutil.hxx"

static void (__stdcall *glAddSwapHintRect)(GLint, GLint, GLint, GLint);

// externs from ssinit.cxx
extern void *gDataPtr;
extern void (*gReshapeFunc)(int, int, void *);
extern void (*gRepaintFunc)( LPRECT, void *);
extern void (*gUpdateFunc)( void *);
extern void (*gInitFunc)( void *);
extern void (*gFinishFunc)( void *);
extern void (*gFloaterBounceFunc)( void *);

// forwards
static void GetWindowSize( HWND hwnd, ISIZE *pSize );
static void ss_QueryAddSwapHintRect();
static void DrawGdiDeltaRect( HDC hdc, HBRUSH hbr, RECT *pRect1, RECT *pRect2 );
static void DrawGLDeltaRect( GLRECT *pRect1, GLRECT *pRect2 );

/**************************************************************************\
* SSW constructor
*
\**************************************************************************/

SSW::SSW( PSSW psswParentArg, ISIZE *pSize, IPOINT2D *pPos, BOOL bMotion,
          SSCHILDSIZEPROC ChildSizeFuncArg )
{
    // Basic initialization

    Reset();

    // Initialization based on constructor parameters

    psswParent = psswParentArg;

    ChildSizeFunc = ChildSizeFuncArg;

    if( pSize )
        size = *pSize;
    if( pPos )
        pos = *pPos;
    else
        pos.x = pos.y = 0;

    if( bMotion && psswParent ) {
        // Allocate motion structure
        pMotion = (MOTION *)
                LocalAlloc( LMEM_ZEROINIT | LMEM_FIXED, sizeof(MOTION) );
        // If pMotion is NULL, then motion is disabled
    }

    // Call back to the client screen saver to determine size and motion
    // characteristics of child based on its parent size.
    GetChildInfo(); // this can set pos and size

    if( pMotion ) {
        if( pPos == NULL ) {
            // Set a random window pos
            pos.x = ss_iRand2( 0, (psswParent->size.width - size.width) );
            pos.y = ss_iRand2( 0, (psswParent->size.height - size.height) );

            // Set the motion parameters
            ResetMotion();
        }
        // Have to make sure parent has an hdc so it can draw background when
        // this child moves
        if( !psswParent->hdc )
            psswParent->hdc = GetDC( psswParent->hwnd );
    }

    if( psswParent ) {
        // Need to add this pssw to its parent's list
        psswParent->AddChild( this );
        // Default is to be subWindow of parent
        psswParent->iSubWindow++;  // increment reference count
    }
}

/**************************************************************************\
* SSW constructor
*
* Used when wrapping an SSW around an already existing window
* (as when drawing GL on dialog buttons)
\**************************************************************************/

SSW::SSW( PSSW psswParentArg, HWND hwndArg )
{
    Reset();

    psswParent = psswParentArg;
    if( psswParent )
        // Need to add this pssw to its parent's list
        psswParent->AddChild( this );

    hwnd = hwndArg;
    if( !hwnd ) {
        SS_ERROR( "SSW::SSW : NULL hwnd\n" );
        return;
    }

    bOwnWindow = FALSE;
    // Get the window size
    GetWindowSize( hwnd, &size );

    gpss->sswTable.Register( hwnd, this );
}

/**************************************************************************\
* Reset
*
* Reset parameters to default init state
\**************************************************************************/

void
SSW::Reset()
{
    // Basic initialization

    bOwnWindow = TRUE;
    iSubWindow = 0;
    bValidateBg = FALSE;
    wFlags = 0;
    hwnd = 0;
    hdc = 0;
    hrc = 0;
    pos.x = pos.y = 0;
    size.width = size.height = 0;
    psswParent =    NULL;
    psswSibling =   NULL;
    psswChildren =  NULL;
    bDoubleBuf =    FALSE;
    pStretch =      NULL;
    pMotion =       NULL;
    pGLc =          NULL;

    InitFunc =      NULL;
    UpdateFunc =    NULL;
    ReshapeFunc =   NULL;
    RepaintFunc =   NULL;
    FloaterBounceFunc = NULL;
    FinishFunc =    NULL;
    ChildSizeFunc = NULL;
    DataPtr =       NULL;
}

/**************************************************************************\
* SSW destructor
*
* This can be called when a window is closed, or by the ss client
*
\**************************************************************************/

SSW::~SSW()
{
    // If this window has any children, they will have to be terminated too

    if( psswChildren ) {
        PSSW psswChild = psswChildren;
        while( psswChild ) {
            // Delete first child in list
            if( psswChild->hwnd && bOwnWindow ) {
                // We created this window, we must destroy it
                DestroyWindow( psswChild->hwnd );
            } else {
                delete psswChild;
            }
            // Next child is now first child in list
            psswChild = psswChildren;
        }
    }

    if( psswParent )
        // Need to remove this pssw from its parent's list
        psswParent->RemoveChild( this );

    if( hwnd ) {
        // Remove from SSWTable
        gpss->sswTable.Remove( hwnd );
    } else {
        // subWindow
        if( psswParent ) {
            SS_ASSERT1( (psswParent->iSubWindow > 0),
                 "Invalid subWindow reference count for pssw=0x%x\n", this );
            psswParent->iSubWindow--;  // decrement subWindow reference count
        }
    }

    // Clean up GL

    if( hrc ) {
        // FinishFunc still needs gl
        if( FinishFunc )
            (*FinishFunc)( DataPtr );

        wglMakeCurrent( NULL, NULL );
        if( ! (wFlags & SS_HRC_PROXY_BIT) )
            wglDeleteContext( hrc );
    }

    // Clean up any bitmaps

    if( pStretch ) {
        SS_BITMAP *pssbm = &pStretch->ssbm;

        DeleteObject(SelectObject(pssbm->hdc, pssbm->hbmOld));
        DeleteDC(pssbm->hdc);
    }

    //  Release the dc
    if( hdc ) {
        HWND hwndForHdc = hwnd ? hwnd : psswParent ? psswParent->hwnd : NULL;
        ReleaseDC(hwndForHdc, hdc);
    }
}

/**************************************************************************\
* AddChild
*
* Add the supplied child SSW to this SSW.
\**************************************************************************/

void
SSW::AddChild( PSSW psswChild )
{
    if( !psswChildren ) {
        psswChildren = psswChild;
        return;
    }

    // Else travel along the sibling chain of psswChildren and deposit
    // psswChild at the end

    PSSW pssw = psswChildren;
    while( pssw->psswSibling )
        pssw = pssw->psswSibling;
    pssw->psswSibling = psswChild;
}


/**************************************************************************\
* RemoveChild
*
* Remove this child from the parent's list
*
* Whoever calls this needs to update SSW_TABLE too...
\**************************************************************************/

BOOL
SSW::RemoveChild( PSSW psswChild )
{
    if( !psswChildren ) {
        // Something wrong - this window has no children
        SS_ERROR( "SSW::RemoveChild : no children\n" );
        return FALSE;
    }

    PSSW psswPrev;
    PSSW pssw = psswChildren;

    while( pssw != NULL ) {
        if( pssw == psswChild ) {
            // found it !
            if( psswChild == psswChildren )
                // The child being removed is the first in the list
                psswChildren = psswChild->psswSibling;
            else
                psswPrev->psswSibling = pssw->psswSibling;
            return TRUE;
        }
        // Move up the pointers
        psswPrev = pssw;
        pssw = psswPrev->psswSibling;
    }

    SS_ERROR( "SSW::RemoveChild : child not found\n" );
    return FALSE;
}

/**************************************************************************\
* GetWindowSize
*
\**************************************************************************/

static void
GetWindowSize( HWND hwnd, ISIZE *pSize )
{
    RECT clientRect;

    GetClientRect( hwnd, &clientRect );

    pSize->width = clientRect.right - clientRect.left + 1;
    pSize->height = clientRect.bottom - clientRect.top + 1;
}

/**************************************************************************\
* CreateSSWindow
*
* Create OpenGL floater window.  This window floats on top of the screen
* saver window, bouncing off each of the screen edges.
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Floater motion characteristics now defined by caller
*  Aug. 14, 95 : [marcfo]
*    - Position the window offscreen initially, to workaround a win95 bug
*      with a corrupted initial clip rect.
*
\**************************************************************************/

BOOL
SSW::CreateSSWindow(HINSTANCE hMainInstance,  UINT uStyle, UINT uExStyle ,
                  LPCTSTR pszWindowTitle, WNDPROC wndProcArg, LPCTSTR pszClassName, HWND hwndParentOverride )
{
    IPOINT2D startPos;
    HWND hwndParent;

    if( hwndParentOverride )
        hwndParent = hwndParentOverride;
    else
        hwndParent =  psswParent ? psswParent->hwnd : NULL;


    wndProc = wndProcArg;

    if( !pMotion )
        startPos = pos;
    else {
        // Initialize start position off screen to work around win95 screen
        // validation bug
        startPos.x = pos.x - psswParent->size.width;
        startPos.y = pos.y - psswParent->size.height;
    }

    hwnd = CreateWindowEx(
                                 uExStyle,
                                 pszClassName,
                                 pszWindowTitle,
                                 uStyle,
                                 startPos.x,
                                 startPos.y,
                                 size.width,     // width
                                 size.height,    // height
                                 hwndParent,
                                 NULL,               // menu
                                 hMainInstance,
                                 (LPVOID) this
                                );

    if (!hwnd) {
        //mf: could still continue here by using sub-windows
        SS_WARNING( "SSW::CreateSSWindow : CreateWindowEx failure\n" );
        return FALSE;
    }

    // This window is on its own now
    if( psswParent ) {
        SS_ASSERT1( (psswParent->iSubWindow > 0),
                 "Invalid subWindow reference count for pssw=0x%x\n", this );
        psswParent->iSubWindow--;  // decrement subWindow reference count
    }

    ShowWindow(hwnd, SW_SHOW);

    return TRUE;
}

/**************************************************************************\
* GetChildInfo
*
* Call the window's ChildSizeFunc
\**************************************************************************/

void
SSW::GetChildInfo( )
{
    if( !ChildSizeFunc )
        return;

    CHILD_INFO childInfo;

    // Call the client's SizeFunc to get required info

    (*ChildSizeFunc)( &psswParent->size, &childInfo );

    // Pull required values into pssw and validate them

    size = childInfo.size;
    ValidateChildSize();

    if( !pMotion ) {
        pos = childInfo.pos;
        bValidateChildPos();
    } else {
        pMotion->posInc = childInfo.motionInfo.posInc;
        pMotion->posIncVary = childInfo.motionInfo.posIncVary;
        pMotion->posIncCur = pMotion->posInc;
    }
}


/**************************************************************************\
* ConfigureForGdi
*
* Creates an hdc for the window
*
\**************************************************************************/

BOOL
SSW::ConfigureForGdi()
{
    if( hdc )
        // already configured
        return TRUE;

    // Figure window to get hdc from
    HWND hwndForHdc = hwnd ? hwnd : psswParent ? psswParent->hwnd : NULL;

    if( !hwndForHdc || !(hdc = GetDC(hwndForHdc)) ) {
        SS_WARNING( "SSW::ConfigureForGdi failed\n" );
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************\
* ConfigureForGL
*
* Creates a GL rendering context for the specified window
*
\**************************************************************************/

BOOL
SSW::ConfigureForGL( SS_GL_CONFIG *pGLcArg )
{
    pGLc = pGLcArg;
    return ConfigureForGL();
}

BOOL
SSW::ConfigureForGL()
{
    if( hrc )
        // Already configured...
        return TRUE;

    if( ConfigureForGdi() &&
        (hrc = hrcSetupGL()) )
        return TRUE;

    SS_WARNING( "SSW::ConfigureForGL failed\n" );
    return FALSE;
}

/**************************************************************************\
* hrcSetupGL
*
* Setup OpenGL.
*
\**************************************************************************/

#define NULL_RC ((HGLRC) 0)

HGLRC
SSW::hrcSetupGL()
{
    if( !pGLc )
        return NULL_RC;

//mf: This routine does not yet fully support logical sub-windows...

    HGLRC hrc;
    HDC hgldc;
    int pfFlags = pGLc->pfFlags;
    PIXELFORMATDESCRIPTOR pfd = {0};

    pStretch = pGLc->pStretch;
    if( pStretch ) {
        if( NeedStretchedWindow() ) {
            // Only need single buffered pixel format
            pfFlags &= ~SS_DOUBLEBUF_BIT;
            pfFlags |= SS_BITMAP_BIT; // yup, BOTH window and bitmap need this
        } else
            // Turn off stretching
            pStretch = NULL;
    }

    // If preview mode or config mode, don't allow pixel formats that need
    // the system palette, as this will create much ugliness.
    if( ss_fPreviewMode() || ss_fConfigMode() )
        pfFlags |= SS_NO_SYSTEM_PALETTE_BIT;

    // If config mode, force a non-accelerated pixel format, as WNDOBJ's
    // seem to have problems with MCD, ICD.  Do the same thing if a
    // monitor configuration is detected, since only the generic implementation
    // will work properly in this case on all displays.

    if( ss_fConfigMode() || (GetSystemMetrics(SM_CMONITORS) > 1) )
        pfFlags |= SS_GENERIC_UNACCELERATED_BIT;

    bDoubleBuf = SS_HAS_DOUBLEBUF( pfFlags );

    if( !SSU_SetupPixelFormat( hdc, pfFlags, &pfd ) )
        return NULL_RC;

    // Update pGLc->pfFlags based on pfd returned
    // !!! mf: klugey, fix after SUR
    // (for now, the only ones we care about are the generic/accelerated flags)
    if(  (pfd.dwFlags & (PFD_GENERIC_FORMAT|PFD_GENERIC_ACCELERATED))
		 == PFD_GENERIC_FORMAT )
        pGLc->pfFlags |= SS_GENERIC_UNACCELERATED_BIT;

    if( SSU_bNeedPalette( &pfd ) ) {
        // Note: even if bStretch, need to set up palette here so they match
        if( !gpss->pssPal ) {
            SS_PAL *pssPal;
            BOOL bTakeOverPalette = ss_fFullScreenMode() ? TRUE : FALSE;

            // The global palette has not been created yet - do it
            // SS_PAL creation requires pixel format descriptor for color bit
            // information, etc. (the pfd is cached in SS_PAL, since for
            // palette purposes it is the same for all windows)
            pssPal = new SS_PAL( hdc, &pfd, bTakeOverPalette );
            if( !pssPal )
                return NULL_RC;
            // Set approppriate palette manage proc
            if( ss_fFullScreenMode() )
                pssPal->paletteManageProc = FullScreenPaletteManageProc;
            else
                // use regular palette manager proc
                pssPal->paletteManageProc = PaletteManageProc;
            gpss->pssPal = pssPal;
        }
        // Realize the global palette in this window
        //mf: assume we're realizing in foreground
        HWND hwndPal = hwnd ? hwnd : psswParent ? psswParent->hwnd : NULL;
        if( hwndPal )
            gpss->pssPal->Realize( hwndPal, hdc, FALSE );
    }

    if( pStretch ) {
        // Stretch blt mode: For every frame, we'll be doing a StretchBlt
        // from a DIB to the screen.  Need to set up a compatible memdc.
        SS_BITMAP *pssbm;

        pssbm = &pStretch->ssbm;

        pssbm->hdc = CreateCompatibleDC(hdc);
        if( !pssbm->hdc )
            return NULL_RC;
        ResizeStretch(); // this creates the DIB Section
        pfFlags = 0;
        pfFlags |= SS_BITMAP_BIT;
        if( !SSU_SetupPixelFormat( pssbm->hdc, pfFlags, &pfd ) ) {
            return NULL_RC;
        }
        //mf: this ppfd's palette bits must match the window's !!
        // If window needs palette, so does bitmap...
        if( gpss->pssPal ) {
            SS_PAL *pssPal = gpss->pssPal;
            extern void ssw_UpdateDIBColorTable( HDC, HDC );

            ssw_UpdateDIBColorTable( pssbm->hdc, hdc );
        }
        hgldc = pssbm->hdc;
    } else {
        hgldc = hdc;
    }

    if( pGLc->hrc ) {
        // Use the supplied hrc
        hrc = pGLc->hrc;
        // Set flag so we don't delete this borrowed hrc when the SSW terminates
        wFlags |= SS_HRC_PROXY_BIT;
    } else
        // Create a new hrc
        hrc = wglCreateContext(hgldc);

    if( !hrc || !wglMakeCurrent(hgldc, hrc) ) {
        SS_WARNING( "SSW::hrcSetupGL : hrc context failure\n" );
        return NULL_RC;
    }

    if( !hwnd && (bDoubleBuf || pStretch) ) {

        // enable scissoring
        glEnable( GL_SCISSOR_TEST );

        if( !(pGLc->pfFlags & SS_GENERIC_UNACCELERATED_BIT) ) {
            // MCD or ICD, possible hardware implementation - we maintain
            // a lastRect to handle SwapBuffer issues
            lastRect.x = lastRect.y = lastRect.width = lastRect.height = 0;
        }
    }

    SS_DBGLEVEL2( SS_LEVEL_INFO,
        "SSW::hrcSetupGL: wglMakeCurrent( hrc=0x%x, hwnd=0x%x )\n", hrc, hwnd );

//mf: Note that these queries are based on a single gl window screen saver.  In
// a more complicated scenario, these capabilities could be queried on a
// per-window basis (since support could vary with pixel formats).

    // Query the GL version - sets support for any new (e.g. 1.1) functionality
    ss_QueryGLVersion();

    // Query paletted texture extension
    ss_QueryPalettedTextureEXT();

    // Query the AddSwapHintRect WIN extension
    ss_QueryAddSwapHintRect();

    // Pull in any Func's that were already defined (for compatibility with
    // old mechanism)

    InitFunc = gInitFunc;
    UpdateFunc = gUpdateFunc;
    ReshapeFunc = gReshapeFunc;
    RepaintFunc = gRepaintFunc;
    FloaterBounceFunc = gFloaterBounceFunc;
    FinishFunc = gFinishFunc;
    DataPtr = gDataPtr;

    return hrc;
}

/**************************************************************************\
* MakeCurrent
*
* Call wglMakeCurrent for this window's hrc.  Note: an ss client may have
* more than one hrc (e.g. pipes), in which case it is the client's
* responsibility to make current.
\**************************************************************************/

void
SSW::MakeCurrent()
{
    if( ! wglMakeCurrent( hdc, hrc ) ) {
        SS_WARNING( "SSW::MakeCurrent : wglMakeCurrent failure\n" );
    }
}

/**************************************************************************\
* InitGL
*
* Call the window's GL Init Func
*
* Priority is raised to expedite any initialization (e.g. loading and
* processing textures can take a while.
*
* A Reshape msg is sent to the client ss, as this is required for setting
* glViewport, etc.
\**************************************************************************/

void
SSW::InitGL()
{
    PSSW psswChild = psswChildren;

    // Configure the window for GL if pGLc non-NULL
    if( pGLc && (! ConfigureForGL()) ) {
        // This is fatal for this window - if it is the main window,
        // the ss will terminate
        if( hwnd )
            PostMessage( hwnd, WM_CLOSE, 0, 0l );
        return;
    }

    // If window configured for GL, hrc will have been set...

    // Call the InitFunc
    if( hrc && InitFunc ) {
        DWORD oldPriority;

        // Bump up priority during initialization phase
        oldPriority = GetPriorityClass( GetCurrentProcess() );
        SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

        SS_DBGLEVEL1( SS_LEVEL_INFO,
                "SSW::InitGL: Calling client GLInit for 0x%x\n", hwnd );

        (*InitFunc)( DataPtr );

        // restore original priority
        SetPriorityClass( GetCurrentProcess(), oldPriority );
    }

    /* Send another Reshape, since initial one triggered by window
     * creation would have been received before GL init'd
     */
    Reshape();

    // Next, init any child windows.  This has to be done after the parent
    // window initialization.

    while( psswChild ) {
        if( psswChild->hwnd )
            SendMessage( psswChild->hwnd, SS_WM_INITGL, 0, 0 );
        else
            // Must be a logical sub-window
            psswChild->InitGL();
        psswChild = psswChild->psswSibling;
    }
}

/**************************************************************************\
* ResizeStretch
*
* Resize the compatible bitmap for stretch blt mode
*
* There are 2 sizing modes.  If bRatioMode, then we set the bitmap size
* by dividing the window dimensions by the supplied ratios.  In this case,
* the base* values set a lower limit for the bitmap dimensions.  Otherwise, the
* base* values determine the bitmap dimensions.
*
* Feb. 12, 96 : [marcfo]
*
\**************************************************************************/

void
SSW::ResizeStretch()
{
    RECT rc;
    HBITMAP hbmNew;
    PVOID pvBits;
    int width, height;
    int cwidth, cheight; // client area size
    SS_BITMAP *pssbm = &pStretch->ssbm;

    cwidth = size.width;
    cheight = size.height;

    if( pStretch->bRatioMode ) {
        width = (int) ( (float)cwidth / pStretch->widthRatio );
        height = (int) ( (float)cheight / pStretch->heightRatio );
        if( width < pStretch->baseWidth )
            width = pStretch->baseWidth;
        if( height < pStretch->baseHeight )
            height = pStretch->baseHeight ;
    } else {
        width = pStretch->baseWidth;
        height = pStretch->baseHeight;
    }

    // Limit width, height to window dimensions
    if( width > cwidth )
        width = cwidth;
    if( height > cheight )
        height = cheight;

    // If same size, get out
    if( (width == pssbm->size.width) && (height == pssbm->size.height) )
        return;

    pssbm->size.width = width;
    pssbm->size.height = height;

#if 1
    // Use system palette
    hbmNew = SSDIB_CreateCompatibleDIB(hdc, NULL, width, height, &pvBits);
#else
    // Use log palette
    hbmNew = SSDIB_CreateCompatibleDIB(hdc,
                                    gpss->pssPal ? gpss->pssPal->hPal : NULL,
                                    width, height, &pvBits);
#endif
    if (hbmNew)
    {
        if (pssbm->hbm != (HBITMAP) 0)
        {
            SelectObject( pssbm->hdc, pssbm->hbmOld );
            DeleteObject( pssbm->hbm );
        }

        pssbm->hbm = hbmNew;
        pssbm->hbmOld = (HBITMAP) SelectObject( pssbm->hdc, pssbm->hbm );
    }
}

/**************************************************************************\
* Resize
*
* Resize wrapper
*
* Called in response to WM_SIZE.
*
\**************************************************************************/

void
SSW::Resize( int width, int height )
{
    size.width  = width;
    size.height = height;

    if( pStretch ) {
        // May have to resize associated bitmap
        ResizeStretch();
    }

    if( psswChildren ) {
        // May need to resize any children
        PSSW pssw = psswChildren;
        while( pssw ) {
            // Get new size/motion for the floater
            pssw->GetChildInfo();
            pssw->SetSSWindowPos();
            if( !pssw->hwnd ) {
                // Handle sub-window case
                // Need to call Reshape, since win32 system won't send WM_SIZE.
                pssw->Reshape();
            }
            pssw = pssw->psswSibling;
        }
    }

    Reshape();
}

/**************************************************************************\
* Repaint
*
* Repaint wrapper
*
* Called in response to WM_PAINT.
*
\**************************************************************************/

#define NULL_UPDATE_RECT( pRect ) \
     (  ((pRect)->left == 0) && \
        ((pRect)->right == 0) && \
        ((pRect)->top == 0) && \
        ((pRect)->bottom == 0) )

void
SSW::Repaint( BOOL bCheckUpdateRect )
{
    if( !hwnd )
        return;

    RECT rect, *pRect = NULL;

    if( bCheckUpdateRect ) {
        GetUpdateRect( hwnd, &rect, FALSE );
        // mf: Above supposed to return NULL if rect is all 0's,
        // but this doesn't happen
        if( NULL_UPDATE_RECT( &rect ) )
            return;
        pRect = &rect;
    }

    if( RepaintFunc )
        (*RepaintFunc)( pRect, DataPtr );
}

/**************************************************************************\
* NeedStretchedWindow
*
* Check if stretch mode is necessary
*
\**************************************************************************/

BOOL
SSW::NeedStretchedWindow()
{
    if( (pStretch->baseWidth >= size.width) &&
        (pStretch->baseHeight >= size.height) ) {
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************\
* SetSSWindowPos
*
* Set new size and position for an SSW
*
\**************************************************************************/

void
SSW::SetSSWindowPos()
{
    SetSSWindowPos( 0 );
}

void
SSW::SetSSWindowPos( int flags )
{
    if( hwnd ) {
        SetWindowPos(hwnd, 0, pos.x, pos.y,
                 size.width, size.height,
                 SWP_NOCOPYBITS | SWP_NOZORDER | SWP_NOACTIVATE | flags );
    // Note: If flags does not specify SWP_NOREDRAW, this generates a WM_PAINT
    // msg for the entire window region
    } else if( psswParent && !pStretch ) {
        // Set viewport for new position
        // (mf: if pStretch, shouldn't need to do anything, as viewport should
        // have been already set to the bitmap)
        int posy = GLPosY();
        glViewport( pos.x, posy, size.width, size.height );
        glScissor( pos.x, posy, size.width, size.height );
        glAddSwapHintRect( pos.x, posy, size.width, size.height );
    }
}

/******************************Public*Routine******************************\
* SetAspectRatio
*
* Resize a child window to conform to the supplied aspect ratio.  We do this by
* maintaining the existing width, and adjusting the height.
*
* Window resize seems to be executed synchronously, so gl should be able to
* immediately validate its buffer dimensions (we count on it).
*
* Returns TRUE if new height is different from last, else FALSE.
\**************************************************************************/

BOOL
SSW::SetAspectRatio( FLOAT fAspect )
{
    if( !psswParent )
        return FALSE;

    int oldHeight;
    UINT uFlags = 0;

    // Check for zero fAspect
    if( fAspect == 0.0f ) fAspect = 1.0f;

    oldHeight = size.height;
    // Set the new height, based on desired aspect ratio
    size.height = (int) ((FLOAT) size.width / fAspect);
    // make sure new height not TOO big!
    ValidateChildSize();

    if( size.height == oldHeight )
        // no change
        return FALSE;

    // make sure current position still valid when height changes
    if( !bValidateChildPos() ) {
        // current position OK, don't need to move it
        uFlags |= SWP_NOMOVE;
    }
    SetSSWindowPos( uFlags );

    // ! remember not to call the client's ReshapeFunc here, as SetAspectRatio
    // may be called by the client in response to a window resize, resulting in
    // an infinite loop
    //mf: but if there is no size change (above) we return, avoiding the
    // infinite loop
    if( !hwnd )
        // Need to call Reshape, since win32 system won't send WM_SIZE.  Need
        // to fix to avoid possible infinite loops
        Reshape();

    return TRUE;
}

/**************************************************************************\
* CalcNextWindowPos
*
* Calculate the next position for a moving window, using the pMotion data.
* If the new position would cause the window to bounce off the
* edge of its parent, return TRUE.
*
\**************************************************************************/

BOOL
SSW::CalcNextWindowPos()
{
    POINT2D *fpos = &pMotion->pos;
    IPOINT2D *ipos = &pos;
    POINT2D *posInc = &pMotion->posInc;
    POINT2D *posIncV = &pMotion->posIncVary;
    POINT2D *posIncCur = &pMotion->posIncCur;
    BOOL bounce = FALSE;

    if( !psswParent )
        return FALSE;

    // Compute the next window position.

    fpos->x += posIncCur->x;
    fpos->y += posIncCur->y;
    ipos->x = (int) fpos->x;
    ipos->y = (int) fpos->y;

    if ( (ipos->x + size.width) > psswParent->size.width) {
        // Right hit
        ipos->x = psswParent->size.width - size.width;
        fpos->x = (float) ipos->x;
        posIncCur->x =
            - ss_fRand( posInc->x - posIncV->x, posInc->x + posIncV->x );
        if( posIncCur->x > -0.5f ) posIncCur->x = -0.5f;
        bounce = TRUE;
    } else if (ipos->x < 0) {
        // Left hit
        ipos->x = 0;
        fpos->x = 0.0f;
        posIncCur->x =
            ss_fRand( posInc->x - posIncV->x, posInc->x + posIncV->x );
        if( posIncCur->x < 0.5f ) posIncCur->x = 0.5f;
        bounce = TRUE;
    }

    if ( (ipos->y + size.height) > psswParent->size.height) {
        // Bottom hit
        ipos->y = psswParent->size.height - size.height;
        fpos->y = (float) (ipos->y);
        posIncCur->y =
            - ss_fRand( posInc->y - posIncV->y, posInc->y + posIncV->y );
        if( posIncCur->y > -0.5f ) posIncCur->y = -0.5f;
        bounce = TRUE;
    } else if (ipos->y < 0) {
        // Top hit
        ipos->y = 0;
        fpos->y = 0.0f;
        posIncCur->y =
            ss_fRand( posInc->y - posIncV->y, posInc->y + posIncV->y );
        if( posIncCur->y < 0.5f ) posIncCur->y = 0.5f;
        bounce = TRUE;
    }
    return bounce;
}


/**************************************************************************\
* MoveSSWindow
*
* This is the function that moves the OpenGL window around, causing it to
* bounce around.  Each time the window is moved, the contents of the
* window are updated from the hidden (or back) buffer by SwapBuffers().
*
* The bRedrawBg flag determines whether the area that was covered by the old
* position should be updated by the parent window.
*
\**************************************************************************/

void
SSW::MoveSSWindow( BOOL bRedrawBg )
{
    BOOL bounce;
    int flags = SWP_NOSIZE;

    // Synchronize with OpenGL.  Flush OpenGL commands and wait for completion.

    glFinish();

    // Move the window

    bounce = CalcNextWindowPos();

    if( bounce && FloaterBounceFunc )
        // The window bounced off one of the sides
        // ! This function should *not* be used for rendering - for
        // informational purposes only (.e.g. changing the spin of a
        // rotating object).
        (*FloaterBounceFunc)( DataPtr );

    if( !bRedrawBg )
        flags |= SWP_NOREDRAW;
    SetSSWindowPos( flags );
}


/**************************************************************************\
* UpdateWindow
*
* Update the window
*
* Ccurrently this assumes all windows are being animated (i.e. not showing
*   a static image)
*
* Things *must* happen in the order defined here, so they work on generic as
* well as hardware implementations.
* Note: Move must happen after SwapBuf, and will cause some encroaching on
* the current display, as the parent window repaints after the move.  Therefore
* apps must take care to leave an empty border around their rendered image,
* equal to the maximum window move delta.
*
\**************************************************************************/

void
SSW::UpdateWindow()
{
    // update any children first

    PSSW pssw = psswChildren;
    while( pssw ) {
        pssw->UpdateWindow();
        pssw = pssw->psswSibling;
    }

//mf: semi-kluge
    // If this window is a subWindow in a non-generic implementation, the
    // background of the parent may be invalid (this may eventually be
    // useful for regular windows, which is why we don't && !hwnd)
    if( psswParent &&
        psswParent->bValidateBg &&
        pGLc &&
        !(pGLc->pfFlags & SS_GENERIC_UNACCELERATED_BIT) )
    {
        // Clear the entire parent window
        // mf: I think this is only needed for double-buffered schemes, since
        // the windowing system should repaint the front buffer in this case.
        glDisable( GL_SCISSOR_TEST );
        glClear( GL_COLOR_BUFFER_BIT );
        glEnable( GL_SCISSOR_TEST );
        psswParent->bValidateBg = FALSE;
    }

    if( !UpdateFunc )
        return;

    // bDoubleBuf and pStretch should be mutually exclusive...

    if( bDoubleBuf || pStretch ) {
        UpdateDoubleBufWin();
    } else {
//mf: ? where's the clearing here ?  (true, no one uses this path...)
        if( pMotion )
            MoveSSWindow( TRUE );
        (*UpdateFunc)( DataPtr );
    }
}

/**************************************************************************\
* UpdateDoubleBufWin
*
* This is used when moving a double buffered window around.  It will
* work for all configurations.
*
\**************************************************************************/

void
SSW::UpdateDoubleBufWin()
{
    if( !hwnd ) {
        UpdateDoubleBufSubWin();
        return;
    }

    RECT updateRect;

    // Move the window

    if( pMotion ) {
        // Save child update rect before move
        GetSSWindowRect( &updateRect );
        // Move window, without repainting exposed area
        MoveSSWindow( FALSE );
    }

    // Update the back buffer

    (*UpdateFunc)( DataPtr );

    if( pMotion ) {
        // (pMotion will be NULL if this window has no parent)
        if( hwnd ) {
            // Paint the exposed area with bg brush (the current image will
            // be partially erased momentarily, until the SwapBuffers() call
            // comes through)
            // (This rect should be clipped to our new window position...)
            DrawGdiRect( psswParent->hdc, gpss->hbrBg, &updateRect );
        } else {
//mf: currently this path not possible, since if !hwnd, we use one of the
// UpdateDoubleBufSubWin* functions
            SS_WARNING( "SSW::UpdateDoubleBufWin: no hwnd\n" );
            // sub-window case : need to do our own clipping
            RECT newRect;
            GetSSWindowRect( &newRect );
            DrawGdiDeltaRect( psswParent->hdc, gpss->hbrBg, &updateRect, &newRect );
        }
    }

    // Swap to the new window position
    SwapSSBuffers();
}


/**************************************************************************\
* UpdateDoubleBufSubWin
*
* Used for generic double buffered gl sub-windows.
*
\**************************************************************************/

void
SSW::UpdateDoubleBufSubWin()
{
    GLRECT curRect, newRect;

    // AddSwapHintRect for current position
    glAddSwapHintRect( pos.x, GLPosY(), size.width, size.height );

    if( pMotion ) {

        // Save current rect
        curRect.x = pos.x;
        curRect.y = GLPosY();
        curRect.width = size.width;
        curRect.height = size.height;

        // Move window, without repainting exposed area
        MoveSSWindow( FALSE );

        // Get new rect
        newRect.x = pos.x;
        newRect.y = GLPosY();
        newRect.width = size.width;
        newRect.height = size.height;

        DrawGLDeltaRect( &curRect, &newRect );

        // Have to consider previous rect for ICD or MCD
        if( !(pGLc->pfFlags & SS_GENERIC_UNACCELERATED_BIT) ) {
            DrawGLDeltaRect( &lastRect, &newRect );
            lastRect = curRect;
        }

        // Reset scissor to new rect (this *was* set by MoveSSWindow, but
        // DrawGLDeltaRect sets scissor to do its clearing
        glScissor( newRect.x, newRect.y, newRect.width, newRect.height );
    }

    // Update the back buffer

    (*UpdateFunc)( DataPtr );

    // Swap to the new window position
    SwapSSBuffers();
}


/******************************Public*Routine******************************\
* RandomWindowPos
*
* Sets a new random window position and motion
*
\**************************************************************************/

void
SSW::RandomWindowPos()
{
    if( psswParent ) {
        if( !hwnd ) {
            // sub-window : manually clear old window rect
            if( bDoubleBuf ) {
                glClear( GL_COLOR_BUFFER_BIT );
            } else {
                RECT oldRect;
                GetSSWindowRect( &oldRect );
                DrawGdiRect( psswParent->hdc, gpss->hbrBg, &oldRect );
            }
        }

        // Calc and set new position
        pos.x = ss_iRand2( 0, (psswParent->size.width - size.width) );
        pos.y = ss_iRand2( 0, (psswParent->size.height - size.height) );
        SetSSWindowPos( SWP_NOSIZE );

        // Reset motion
        if( pMotion )
            ResetMotion();
    }
}

/**************************************************************************\
* ResetMotion
*
* Calculate a random position and motion vector for the floater window
* Note that a floating point position is maintained for DDA window movement
*
\**************************************************************************/

void
SSW::ResetMotion()
{
    if( !psswParent || !pMotion )
        // Only child windows can be reset
        return;

    // Set floating point pos also, for DDA
    pMotion->pos.x = (float) pos.x;
    pMotion->pos.y = (float) pos.y;

    // also reset the window motion directions

    if( ss_iRand(2) )  // 0 or 1
        pMotion->posIncCur.x = - pMotion->posIncCur.x;
    if( ss_iRand(2) )
        pMotion->posIncCur.y = - pMotion->posIncCur.y;
}

/**************************************************************************\
* ValidateChildSize
*
* Make sure it's not bigger than its parent
*
\**************************************************************************/

void
SSW::ValidateChildSize()
{
    if( !psswParent )
        return;

    SS_CLAMP_TO_RANGE2( size.width, 0, psswParent->size.width );
    SS_CLAMP_TO_RANGE2( size.height, 0, psswParent->size.height );
}

/**************************************************************************\
* bValidateChildPos
*
* Make sure that with the current window position, none of the floating
* window extends beyond the parent window.
*
\**************************************************************************/

BOOL
SSW::bValidateChildPos()
{
    BOOL bRet = FALSE;

    if( !psswParent )
        return FALSE;

    if ( (pos.x + size.width) > psswParent->size.width) {
        pos.x = psswParent->size.width - size.width;
        bRet = TRUE;
    }

    if ( (pos.y + size.height) > psswParent->size.height) {
        pos.y = psswParent->size.height - size.height;
        bRet = TRUE;
    }
    return bRet;
}

/**************************************************************************\
* GetSSWindowRect
*
* Return window position and size in supplied RECT structure
*
* mf: this rect is relative to the parent
\**************************************************************************/

void
SSW::GetSSWindowRect( LPRECT lpRect )
{
    lpRect->left = pos.x;
    lpRect->top = pos.y;
    lpRect->right = pos.x + size.width;
    lpRect->bottom = pos.y + size.height;
}

/**************************************************************************\
* GLPosY
*
* Return y-coord of window position in GL coordinates (a win32 window position
* (starts from top left, while GL starts from bottom left)
*
\**************************************************************************/

int
SSW::GLPosY()
{
    if( !psswParent )
        return 0;

    return psswParent->size.height - size.height - pos.y;
}

/**************************************************************************\
* SwapStretchBuffers
*
* Swaps from the stretch buffer to the GL window, using StretchBlt
*
\**************************************************************************/

void
SSW::SwapStretchBuffers()
{
    SS_BITMAP *pssbm = &pStretch->ssbm;

    if( (size.width == pssbm->size.width) &&
        (size.height == pssbm->size.height) ) // buffers same size
    {
        BitBlt(hdc, 0, 0, size.width, size.height,
               pssbm->hdc, 0, 0, SRCCOPY);
    }
    else
    {
        StretchBlt(hdc, 0, 0, size.width, size.height,
                   pssbm->hdc, 0, 0, pssbm->size.width, pssbm->size.height,
                   SRCCOPY);
    }
    GdiFlush();
}

/**************************************************************************\
* SwapBuffers
*
* Wrapper for SwapBuffers / SwapStretchBuffers
*
\**************************************************************************/

void
SSW::SwapSSBuffers()
{
    if( pStretch )
        SwapStretchBuffers();
    else if( bDoubleBuf ) {
        SwapBuffers( hdc );
    }
}

/**************************************************************************\
* Reshape
*
* Reshape wrapper

* Sends reshape msg to screen saver
* This is the size of the surface that gl renders onto, which can be a bitmap.
*
\**************************************************************************/

void
SSW::Reshape()
{
    // Point to size of window, or bitmap if it has one
    ISIZE *pSize = &size;
    if( pStretch )
        pSize = &pStretch->ssbm.size;

    // If the window has an hrc, set default viewport

    if( hrc ) {
        if( hwnd )
            glViewport( 0, 0, pSize->width, pSize->height );
        else if ( psswParent ) {
            // sub-window (only 1 level of sub-windowing supported)
//mf: klugey ? - should take into account non-GL and single buffer...
#if 1
            // clear entire window to black
            glDisable( GL_SCISSOR_TEST );
            glClear( GL_COLOR_BUFFER_BIT );
            glEnable( GL_SCISSOR_TEST );
#endif
            // Convert win32 y-coord to GL
            glViewport( pos.x, GLPosY(), pSize->width, pSize->height );
        }
    }

    if( ReshapeFunc ) {
        (*ReshapeFunc)( pSize->width, pSize->height, DataPtr );
    }
}

/******************************Public*Routine******************************\
* GdiClear
*
* Clears window using Gdi FillRect
\**************************************************************************/

void
SSW::GdiClear()
{
    if( !hdc )
        return;

    RECT rect;

//mf: this should use GetClientRect
    GetSSWindowRect( &rect );

    FillRect( hdc, &rect, gpss->hbrBg );
    GdiFlush();
}


/******************************Public*Routine******************************\
* MyAddSwapHintRect
*
\**************************************************************************/

static void _stdcall
MyAddSwapHintRect(GLint xs, GLint ys, GLint xe, GLint ye)
{
    return;
}

/******************************Public*Routine******************************\
* QueryAddSwapHintRectWIN
*
\**************************************************************************/

static void
ss_QueryAddSwapHintRect()
{
    glAddSwapHintRect = (PFNGLADDSWAPHINTRECTWINPROC)
        wglGetProcAddress("glAddSwapHintRectWIN");
    if (glAddSwapHintRect == NULL) {
        glAddSwapHintRect = MyAddSwapHintRect;
    }
}

/******************************Public*Routine******************************\
* DrawGdiDeltaRect
*
* Draw the exposed area by transition from rect1 to rect2
\**************************************************************************/

static void
DrawGdiDeltaRect( HDC hdc, HBRUSH hbr, RECT *pRect1, RECT *pRect2 )
{
    if( (pRect1 == NULL) || (pRect2 == NULL) ) {
        SS_WARNING( "DrawGdiDeltaRect : one or both rects are NULL\n" );
        return;
    }

    // Draw 2 rects

    RECT rect;

    // Rect exposed in x-direction:

    rect.top = pRect1->top;
    rect.bottom = pRect1->bottom;
    if( pRect2->left > pRect1->left ) {
        // moving right
        rect.left = pRect1->left;
        rect.right = pRect2->left;
    } else {
        // moving left
        rect.left = pRect2->right;
        rect.right = pRect1->right;
    }
    FillRect( hdc, &rect, hbr );

    // Rect exposed in y-direction:

    rect.left = pRect1->left;
    rect.right = pRect1->right;
    if( pRect2->bottom > pRect1->bottom ) {
        // moving down
        rect.top = pRect1->top;
        rect.bottom = pRect2->top;
    } else {
        // moving up
        rect.top = pRect2->bottom;
        rect.bottom = pRect1->bottom;
    }
    FillRect( hdc, &rect, hbr );

    GdiFlush();
}


/******************************Public*Routine******************************\
* DrawGLDeltaRect
*
* Draw the exposed area by transition from rect1 to rect2
\**************************************************************************/

static void
DrawGLDeltaRect( GLRECT *pRect1, GLRECT *pRect2 )
{
    if( (pRect1 == NULL) || (pRect2 == NULL) ) {
        SS_WARNING( "DrawGLDeltaRect : one or both rects are NULL\n" );
        return;
    }

    // Draw 2 rects :

//mf: !!! this assumes rect1 and rect2 have same dimensions !

    GLRECT rect;

    // Rect exposed in x-direction:

    rect.height = pRect1->height;
    rect.y = pRect1->y;

    if( pRect2->x > pRect1->x ) {
        // moving right
        rect.width = pRect2->x - pRect1->x;
        rect.x = pRect1->x;
    } else {
        // moving left
        rect.width = pRect1->x - pRect2->x;
        rect.x = pRect2->x + pRect2->width;
    }

    glScissor( rect.x, rect.y, rect.width, rect.height );
    glClear( GL_COLOR_BUFFER_BIT );

    // Rect exposed in y-direction:

    rect.width = pRect1->width;
    rect.x = pRect1->x;

    if( pRect2->y > pRect1->y ) {
        // moving up
        rect.height = pRect2->y - pRect1->y;
        rect.y = pRect1->y;
    } else {
        // moving down
        rect.height = pRect1->y - pRect2->y;
        rect.y = pRect2->y + pRect2->height;
    }

    glScissor( rect.x, rect.y, rect.width, rect.height );
    glClear( GL_COLOR_BUFFER_BIT );
}

