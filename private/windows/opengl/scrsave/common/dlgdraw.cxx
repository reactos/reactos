/******************************Module*Header*******************************\
* Module Name: dlgdraw.c
*
* For gl drawing in dialog boxes
*
* Created: 12-06-95 -by- Marc Fortier [marcfo]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include <windows.h>
#include <commdlg.h>
#include <scrnsave.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "ssintrnl.hxx"
#include "dlgdraw.hxx"

// Define this if want each TEX_BUTTON to be a separate window.  This is
// necessary if the main dialog window has WS_CLIP_CHILDREN, but so far this
// doesn't seem to be the case.
//#define SS_MULTIWINDOW 1

static void CalcGLViewport( HWND hwndParent, HWND hwndChild, IPOINT2D *pOrigin, ISIZE *pSize );

//materials for varying intensities

enum{
    MAT_INTENSITY_LOW = 0,
    MAT_INTENSITY_MID,
    MAT_INTENSITY_HIGH,
    MAT_COUNT
};

MATERIAL gMat[MAT_COUNT] = {
    {{0.3f, 0.3f, 0.3f}, {0.6f, 0.6f, 0.6f}, {0.2f, 0.2f, 0.2f}, 0.3f },
    {{0.2f, 0.2f, 0.2f}, {0.8f, 0.8f, 0.8f}, {0.2f, 0.2f, 0.2f}, 0.3f },
    {{0.2f, 0.2f, 0.2f}, {1.0f, 1.0f, 1.0f}, {0.2f, 0.2f, 0.2f}, 0.3f }
};

float colorBlack[3] = {0.0f, 0.0f, 0.0f};

/**************************************************************************\
* SS_TEX_BUTTON constructor
*
* This allows drawing GL textures on a button
*
* For optimum performance, GL is configured on the main dialog window, and
* 'viewported' to the button.
* Defining SS_MULTIWINDOW results in the texture being drawn in the actual
* button window.
*
* Note: this only works for buttons on the main dialog window for now.
\**************************************************************************/

SS_TEX_BUTTON::SS_TEX_BUTTON( HWND hDlg, HWND hDlgBtn )
{
    PSSW psswParent = gpss->sswTable.PsswFromHwnd( hDlg );
    SS_ASSERT( psswParent, "SS_TEX_BUTTON constructor: NULL psswParent\n" );

    // The parent needs to have an hrc context, since we will be using it
    // for drawing.

    SS_GL_CONFIG GLc = { 0, 0, NULL };
    if( !psswParent->ConfigureForGL( &GLc ) ) {
        SS_WARNING( "SS_TEX_BUTTON constructor: ConfigureForGL failed\n" );
        return;
    }

#ifdef SS_MULTIWINDOW
    // Each button is a separate GL window, using its parents hrc
    pssw = new SSW( psswParent, hDlgBtn );

    SS_ASSERT( pssw, "SS_TEX_BUTTON constructor: pssw alloc failure\n" );

    // Configure the pssw for GL

    GLc.pfFlags = 0;
    GLc.hrc = psswParent->GetHRC();
    GLc.pStretch = NULL;

    if( ! pssw->ConfigureForGL( &GLc ) ) {
        SS_WARNING( "SS_TEX_BUTTON constructor: ConfigureForGL failed\n" );
        return;
    }
#else
    // Make the button a 'subwindow' of the parent
    pssw = NULL;

    // Calculate the viewport to draw to

    CalcGLViewport( hDlg, hDlgBtn, &origin, &size );
#endif

    // Init various GL stuff
    InitGL();

    pCurTex = NULL;
    bEnabled = TRUE;
}

/**************************************************************************\
* SS_TEX_BUTTON destructor
*
\**************************************************************************/

SS_TEX_BUTTON::~SS_TEX_BUTTON()
{
    if( pssw )
        delete pssw;
}

/**************************************************************************\
* InitGL
*
\**************************************************************************/

void
SS_TEX_BUTTON::InitGL()
{
    float ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float diffuse[] = {0.7f, 0.7f, 0.7f, 1.0f};
    float position[] = {0.0f, 0.0f, -150.0f, 1.0f};
    float lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
    MATERIAL *pMat;

    // lighting, for intensity levels

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHT0);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    glCullFace( GL_BACK );
    glEnable(GL_CULL_FACE);
    glFrontFace( GL_CW );
    glShadeModel( GL_FLAT );

    glColor3f( 1.0f, 1.0f, 1.0f );
    gluOrtho2D( -1.0, 1.0, -1.0, 1.0 );

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/**************************************************************************\
* SetTexture
*
* Set a current texture for the button
*
* Note this is a pointer to a texture, so any texture memory management is
* done by the caller.
\**************************************************************************/

void
SS_TEX_BUTTON::SetTexture( TEXTURE *pTex )
{
    pCurTex = pTex;
}

/**************************************************************************\
* Draw
*
\**************************************************************************/

void
SS_TEX_BUTTON::Draw( TEXTURE *pTex )
{
    if( pTex != NULL ) {
        glEnable(GL_TEXTURE_2D);

        ss_SetTexture( pTex ); // doesn't look at iPalRot yet
        // Set the texture palette if it exists
        if( pTex->pal && pTex->iPalRot )
            ss_SetTexturePalette( pTex, pTex->iPalRot );
    }
    // else white rectangle will be drawn

    if( bEnabled )
        intensity = DLG_INTENSITY_HIGH;
    else
        intensity = DLG_INTENSITY_LOW;

    switch( intensity ) {
        case DLG_INTENSITY_LOW:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_LIGHTING);
            glColor3f( 0.5f, 0.5f, 0.5f );
            break;
        case DLG_INTENSITY_MID:
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnable(GL_LIGHTING);
            glColor3f( 0.7f, 0.7f, 0.7f );
            break;
        case DLG_INTENSITY_HIGH:
        default:
            glColor3f( 1.0f, 1.0f, 1.0f );
            glDisable(GL_LIGHTING);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    }

    // Set the viewport
#ifdef SS_MULTIWINDOW
    glViewport( 0, 0, pssw->size.width, pssw->size.height );
#else
    glViewport( origin.x, origin.y, size.width, size.height );
#endif

    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 1.0f );
        glVertex2f( -1.0f, 1.0f );
        glTexCoord2f( 1.0f, 1.0f );
        glVertex2f(  1.0f,  1.0f );
        glTexCoord2f( 1.0f, 0.0f );
        glVertex2f(  1.0f, -1.0f );
        glTexCoord2f( 0.0f, 0.0f );
        glVertex2f(  -1.0f, -1.0f );
    glEnd();

    glDisable( GL_TEXTURE_2D);

    glFlush();
}

void
SS_TEX_BUTTON::Draw()
{
    Draw( pCurTex );
}

/**************************************************************************\
* CalcGLViewport
*
* Calculate viewport for the child window
*
\**************************************************************************/

static void 
CalcGLViewport( HWND hwndParent, HWND hwndChild, IPOINT2D *pOrigin, ISIZE *pSize )
{
    RECT childRect, parentRect;

    // Get size of the child window

    GetClientRect( hwndChild, &childRect );
    pSize->width = childRect.right;
    pSize->height = childRect.bottom;

    // Calc origin of the child window wrt its parents client area
    // Note that the y-coord must be inverted for GL

    // Map the child client rect to the parent client coords
    MapWindowPoints( hwndChild, hwndParent, (POINT *) &childRect, 2 );
    pOrigin->x = childRect.left;
    // invert y coord
    GetClientRect( hwndParent, &parentRect );
    pOrigin->y = parentRect.bottom - childRect.bottom;
}
