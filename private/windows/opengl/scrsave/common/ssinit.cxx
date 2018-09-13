/******************************Module*Header*******************************\
* Module Name: ssinit.cxx
*
* Main code for common screen saver functions.
*
* Created: 12-24-94 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#include <windows.h>
#include <commdlg.h>
#include <scrnsave.h>
#include <GL\gl.h>
#include "tk.h"
#include <math.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include "ssintrnl.hxx"

void     *gDataPtr = NULL; // data ptr used with callbacks

// function protos
void (*gReshapeFunc)(int, int, void *)      = NULL;
void (*gRepaintFunc)(LPRECT, void *)      = NULL;
void (*gUpdateFunc)( void *)           = NULL;
void (*gInitFunc)( void *)             = NULL;
void (*gFinishFunc)( void *)           = NULL;
void (*gFloaterBounceFunc)( void *)      = NULL;

// Debug stuff
#if DBG
#ifdef SS_DEBUG
long ssDebugMsg = 1;
long ssDebugLevel = SS_LEVEL_INFO;
#else
long ssDebugMsg = 0;
long ssDebugLevel = SS_LEVEL_ERROR;
#endif
#endif

// Callback functions:

/******************************Public*Routine******************************\
* ss_InitFunc
*
\**************************************************************************/

void 
ss_InitFunc(SSINITPROC Func)
{
    gInitFunc = Func;
}

/******************************Public*Routine******************************\
* ss_ReshapeFunc
*
\**************************************************************************/

void 
ss_ReshapeFunc(SSRESHAPEPROC Func)
{
    gReshapeFunc = Func;
    if( gpss->psswGL )
        gpss->psswGL->ReshapeFunc = gReshapeFunc;
}

/******************************Public*Routine******************************\
* ss_RepaintFunc
*
\**************************************************************************/

void 
ss_RepaintFunc(SSREPAINTPROC Func)
{
    gRepaintFunc = Func;
    if( gpss->psswGL )
        gpss->psswGL->RepaintFunc = gRepaintFunc;
}

/******************************Public*Routine******************************\
* ss_UpdateFunc
*
\**************************************************************************/

void 
ss_UpdateFunc(SSUPDATEPROC Func)
{
    gUpdateFunc = Func;
    if( gpss->psswGL )
        gpss->psswGL->UpdateFunc = gUpdateFunc;
}

/******************************Public*Routine******************************\
* ss_FinishFunc
*
\**************************************************************************/

void 
ss_FinishFunc(SSFINISHPROC Func)
{
    gFinishFunc = Func;
    if( gpss->psswGL )
        gpss->psswGL->FinishFunc = gFinishFunc;
}

/******************************Public*Routine******************************\
* ss_FloaterBounceFunc
*
\**************************************************************************/

void 
ss_FloaterBounceFunc(SSFLOATERBOUNCEPROC Func)
{
    gFloaterBounceFunc = Func;
    if( gpss->psswGL )
        gpss->psswGL->FloaterBounceFunc = gFloaterBounceFunc;
}

/******************************Public*Routine******************************\
* ss_DataPtr
*
* Sets data ptr to be sent with callbacks
*
\**************************************************************************/

void 
ss_DataPtr( void *data )
{
    gDataPtr = data;
    if( gpss->psswGL )
        gpss->psswGL->DataPtr = gDataPtr;
}

/******************************Public*Routine******************************\
* RandomWindowPos
*
* Sets a new random window position and direction.
*
\**************************************************************************/

void 
ss_RandomWindowPos()
{
    if( gpss->psswGL )
        gpss->psswGL->RandomWindowPos();
}

/******************************Public*Routine******************************\
* ss_SetWindowAspectRatio
*
* Resize the window to conform to the supplied aspect ratio.  We do this by
* maintaining the existing width, and adjusting the height.
*
* Window resize seems to be executed synchronously, so gl should be able to
* immediately validate its buffer dimensions (we count on it).
*
* Returns TRUE if new height is different from last, else FALSE.
\**************************************************************************/

BOOL 
ss_SetWindowAspectRatio( FLOAT aspect )
{
    if( gpss->psswGL )
        return gpss->psswGL->SetAspectRatio( aspect );
    return FALSE;
}

/******************************Public*Routine******************************\
* ss_GetScreenSize
*
* Returns size of screen saver window
*
\**************************************************************************/

void
ss_GetScreenSize( ISIZE *size )
{
    if( gpss->psswMain )
        *size = gpss->psswMain->size;
}

/******************************Public*Routine******************************\
* ss_GetHWND
*
* Return HWND of the main window
\**************************************************************************/

HWND 
ss_GetHWND()
{
    if( gpss->psswMain )
        return gpss->psswMain->hwnd;
    return NULL;
}

/******************************Public*Routine******************************\
* ss_GetGLHWND
*
* Return HWND of the GL window
\**************************************************************************/

HWND 
ss_GetGLHWND()
{
    if( gpss->psswGL )
        return gpss->psswGL->hwnd;
    return NULL;
}

/******************************Public*Routine******************************\
* ss_GetMainPSSW
*
* Return PSSW of top level window
\**************************************************************************/

PSSW
ss_GetMainPSSW()
{
    if( gpss->psswMain )
        return gpss->psswMain;
    return NULL;
}
