/*
 * Star field screensaver
 *
 * Copyright 2011 Carlo Bramini
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <scrnsave.h>

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "resource.h"
#include "settings.h"

#define FAR_PLANE       -80.0f
#define NEAR_PLANE      3.0f
#define GAP             0.0f
#define FIELD_WIDTH     50.f
#define FIELD_HEIGHT    45.f
#define FIELD_DEPTH     (NEAR_PLANE - FAR_PLANE + GAP)

#define STAR_RED        0.f
#define STAR_GREEN      0.f
#define STAR_BLUE       0.10f
#define STAR_TAIL       0.9f

typedef struct {
    float x1;
    float y1;
    float x2;
    float y2;
    float z;
} VERTEX;

static VERTEX  Vertex[MAX_STARS];
static HGLRC   hRC;        // Permanent Rendering Context
static HDC     hDC;        // Private GDI Device Context
static float   fAngle;
static GLuint  glStarTex;

// Light position
static GLfloat g_light_position[4] = {
    0.0f, 0.0f, 3.0f, 1.0f
};

static PIXELFORMATDESCRIPTOR pfd=       // Pixel Format Descriptor
{
    sizeof(PIXELFORMATDESCRIPTOR),      // Size Of This Pixel Format Descriptor
    1,                                  // Version Number (?)
    PFD_DRAW_TO_WINDOW |                // Format Must Support Window
    PFD_SUPPORT_OPENGL |                // Format Must Support OpenGL
    PFD_DOUBLEBUFFER,                   // Must Support Double Buffering
    PFD_TYPE_RGBA,                      // Request An RGBA Format
    16,                                 // Select A 16Bit Color Depth
    0, 0, 0, 0, 0, 0,                   // Color Bits Ignored (?)
    0,                                  // No Alpha Buffer
    0,                                  // Shift Bit Ignored (?)
    0,                                  // No Accumulation Buffer
    0, 0, 0, 0,                         // Accumulation Bits Ignored (?)
    16,                                 // 16Bit Z-Buffer (Depth Buffer)
    0,                                  // No Stencil Buffer
    0,                                  // No Auxiliary Buffer (?)
    PFD_MAIN_PLANE,                     // Main Drawing Layer
    0,                                  // Reserved (?)
    0, 0, 0                             // Layer Masks Ignored (?)
};

static HBITMAP CreateStarBitmap(HWND hWnd, HDC hDC)
{
    BITMAPINFO bi;
    LPBYTE     lpBits;
    LPBYTE    *lppBits;
    HBITMAP    hTextBmp, hFileBmp;
    HDC        hTextDC, hFileDC;
    HGDIOBJ    hOldText, hOldFile;
    UINT       i;
    DWORD     *Ptr32;
    BITMAP     bm;
    HINSTANCE  hInstance;

    // Get instance for loading the texture
    hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

    // Load the texture
    hFileBmp = (HBITMAP)
              LoadImage(
                    hInstance,
                    MAKEINTRESOURCE(IDB_STAR),
                    IMAGE_BITMAP,
                    0, 0,
                    LR_CREATEDIBSECTION | LR_DEFAULTSIZE
              );

    // Get texture specs
    GetObject(hFileBmp, sizeof(BITMAP), &bm);

    // Allocate new 32 bit texture
    ZeroMemory(&bi, sizeof(bi));

    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = bm.bmWidth;
    bi.bmiHeader.biHeight      = -bm.bmHeight;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    // Makes GCC happy ;-|
    lppBits = &lpBits;

    hTextBmp = CreateDIBSection(hDC,
                                (BITMAPINFO*)&bi,
                                DIB_RGB_COLORS,
                                (void**)lppBits,
                                NULL,
                                0);

    // Save new texture specs
//    GetObject(hTextBmp, sizeof(BITMAP), &bmStarTex);
//    bmStarTex.bmBits = lpBits;

    // Copy 24 bit texture in 32 texture
    hTextDC = CreateCompatibleDC(hDC);
    hFileDC = CreateCompatibleDC(hDC);

    hOldText = SelectObject(hTextDC, hTextBmp);
    hOldFile = SelectObject(hFileDC, hFileBmp);

    BitBlt(hTextDC, 0, 0, bm.bmWidth, bm.bmHeight, hFileDC, 0, 0, SRCCOPY);

    SelectObject(hTextDC, hOldText);
    SelectObject(hFileDC, hOldFile);

    DeleteDC(hTextDC);
    DeleteDC(hFileDC);

    // Delete 24 bit texture
    DeleteObject(hFileBmp);

    GetObject(hTextBmp, sizeof(BITMAP), &bm);

    // Apply ALPHA channel to new texture
    for (Ptr32=(DWORD *)lpBits, i=0; i < (UINT)(bm.bmWidth * bm.bmHeight); i++)
    {
        DWORD Color = Ptr32[i] & 0x00FFFFFF;
        DWORD Alpha = Color & 0xFF;

        Color |= Alpha << 24;

        Ptr32[i] = Color;
    }

    return hTextBmp;
}

static void InitGL(HBITMAP hStarTex)
{
    BITMAP       bm;
    unsigned int i;
    float        xp, yp, zp;

    // set the GL clear color - use when the color buffer is cleared
    glClearColor(STAR_RED, STAR_GREEN, STAR_BLUE, STAR_TAIL);

    if (Settings.bSmoothShading)
        // set the shading model to 'smooth'
        glShadeModel( GL_SMOOTH );
    else
        // set the shading model to 'flat'
        glShadeModel( GL_FLAT );

    // set GL to render front of polygons
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // enable depth test
    glDisable( GL_DEPTH_TEST );

    // enable lighting
    glEnable( GL_LIGHTING );
    // enable lighting for front
    glLightModeli( GL_FRONT, GL_TRUE );
    // material have diffuse and ambient lighting
    glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
    // enable color
    glEnable( GL_COLOR_MATERIAL );
    // enable light 0
    glEnable( GL_LIGHT0 );

    // set light attenuation
    glLightf( GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.01f); //0.01f );
    glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.01f); //0.2f );
    glLightf( GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.005f); //0.001f );

    // clear the color buffer once
    glClear( GL_COLOR_BUFFER_BIT );

    // randomly generate
    srand( time( NULL ) );

    // Initialize *ALL* stars vertexes (not just programmed ones).
    for (i = 0; i < MAX_STARS; i++)
    {
        xp = ( (float) rand() / RAND_MAX - .5f ) * FIELD_WIDTH;
        yp = ( (float) rand() / RAND_MAX - .5f ) * FIELD_HEIGHT;
        zp = ( (float) rand() / RAND_MAX       ) * FIELD_DEPTH + FAR_PLANE;

        Vertex[i].x1 = -1.f + xp;
        Vertex[i].y1 = -1.f + yp;
        Vertex[i].x2 =  1.f + xp;
        Vertex[i].y2 =  1.f + yp;
        Vertex[i].z  = zp;
    }

    glGenTextures(1, &glStarTex);           // Create One Texture

    // Create Linear Filtered Texture
    glBindTexture(GL_TEXTURE_2D, glStarTex);

    if (Settings.bEnableFiltering)
    {
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    }

    // Get Texture properties
    GetObject(hStarTex, sizeof(BITMAP), &bm);

    // Create texture as a mipmap
#if 0
    glTexImage2D(GL_TEXTURE_2D, 0, 4, bm.bmWidth, bm.bmHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.bmBits);
#else
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, bm.bmWidth, bm.bmHeight, GL_RGBA, GL_UNSIGNED_BYTE, bm.bmBits);
#endif

    // Disable Texture Mapping (background smoothing)
    glDisable(GL_TEXTURE_2D);

    if (Settings.bFinePerspective)
        // Really Fast Perspective Calculations
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    else
        // Really Nice Perspective Calculations
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // enable blending
    glEnable( GL_BLEND );
}

static void
render(void)
{
    unsigned int i;
    float        fSpin;
    float        fSpeed;
    float        xp, yp;

    // Initialize current speed
    fSpeed = (float)Settings.uiSpeed / 100.f;

    glEnable(GL_TEXTURE_2D);                // Enable Texture Mapping

    glBlendFunc(GL_SRC_ALPHA,GL_ONE);           // Set The Blending Function For Translucency

    switch (Settings.uiRotation) {
    case ROTATION_LINEAR:
        fAngle += fSpeed;
        glRotatef( fAngle, 0.0f, 0.0f, 1.0f );
        break;

    case ROTATION_PERIODIC:
        fAngle += fSpeed / 75.f;
        fSpin = (float)(50. * cos(fAngle));
        glRotatef( fSpin, 0.0f, 0.0f, 1.0f );
        break;
    }

    glColor3ub(255, 255, 255);

    glBegin(GL_QUADS);              // Begin Drawing The Textured Quad

    // Draw the stars
    for (i = 0; i < Settings.uiNumStars; i++)
    {
        glTexCoord2f(0.0f, 0.0f); glVertex3f(Vertex[i].x1, Vertex[i].y1, Vertex[i].z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(Vertex[i].x2, Vertex[i].y1, Vertex[i].z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(Vertex[i].x2, Vertex[i].y2, Vertex[i].z);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(Vertex[i].x1, Vertex[i].y2, Vertex[i].z);

        // increment z
        Vertex[i].z += fSpeed;

        // check to see if passed view
        if( Vertex[i].z > NEAR_PLANE + GAP ||
            Vertex[i].z < FAR_PLANE )
        {
            xp = ( (float) rand() / RAND_MAX - .5f ) * FIELD_WIDTH;
            yp = ( (float) rand() / RAND_MAX - .5f ) * FIELD_HEIGHT;

            Vertex[i].x1 = -1.f + xp;
            Vertex[i].y1 = -1.f + yp;
            Vertex[i].x2 =  1.f + xp;
            Vertex[i].y2 =  1.f + yp;
            Vertex[i].z  = FAR_PLANE;
        }
    }

    glEnd();                    // Done Drawing The Textured Quad

    glDisable(GL_TEXTURE_2D);               // Enable Texture Mapping
}

static LRESULT CALLBACK
OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    GLuint  PixelFormat;
    HBITMAP hStarTex;

    LoadSettings();

    // Gets A Device Context For The Window
    hDC = GetDC(hWnd);

    // Finds The Closest Match To The Pixel Format We Set Above
    PixelFormat = ChoosePixelFormat(hDC, &pfd);

    // No Matching Pixel Format?
    if (!PixelFormat)
    {
        MessageBox(0, _T("Can't Find A Suitable PixelFormat."), _T("Error"),MB_OK | MB_ICONERROR);

        // This Sends A 'Message' Telling The Program To Quit
        PostQuitMessage(0);
        return 0;
    }

    // Can We Set The Pixel Mode?
    if (!SetPixelFormat(hDC, PixelFormat, &pfd))
    {
        MessageBox(0, _TEXT("Can't Set The PixelFormat."), _T("Error"), MB_OK | MB_ICONERROR);

        // This Sends A 'Message' Telling The Program To Quit
        PostQuitMessage(0);
        return 0;
    }

    // Grab A Rendering Context
    hRC = wglCreateContext(hDC);

    // Did We Get One?
    if (!hRC)
    {
        MessageBox(0, _T("Can't Create A GL Rendering Context."), _T("Error"), MB_OK | MB_ICONERROR);

        // This Sends A 'Message' Telling The Program To Quit
        PostQuitMessage(0);
        return 0;
    }

    // Can We Make The RC Active?
    if (!wglMakeCurrent(hDC, hRC))
    {
        MessageBox(0, _T("Can't Activate GLRC."), _TEXT("Error"), MB_OK | MB_ICONERROR);

        // This Sends A 'Message' Telling The Program To Quit
        PostQuitMessage(0);
        return 0;
    }

    // Load star texture
    hStarTex = CreateStarBitmap(hWnd, hDC);

    // Initialize The GL Screen Using Screen Info
    InitGL(hStarTex);

    // Delete GDI object for texture
    DeleteObject(hStarTex);

    // Update screen every 10ms
    SetTimer(hWnd, 1, 10, NULL);

    // Initialize spinning angle
    fAngle = 0.f;

    return 0L;
}

static LRESULT CALLBACK
OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // Delete update timer
    KillTimer(hWnd, 1);

    // Disable Fullscreen Mode
    ChangeDisplaySettings(NULL, 0);

    // Make The DC Current
    wglMakeCurrent(hDC, NULL);

    // Kill The RC
    wglDeleteContext(hRC);

    // Free The DC
    ReleaseDC(hWnd, hDC);

#ifdef _DEBUG_SSTARS
    PostQuitMessage(0);
#endif

    return 0L;
}

static LRESULT CALLBACK
OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (Settings.bDoBlending)
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        // disable lighting
        glDisable( GL_LIGHTING );

        // blend in a polygon
        glColor4f(STAR_RED, STAR_GREEN, STAR_BLUE, STAR_TAIL);

        // Restore both model view and projection to identity
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        // Blur the background
        glBegin( GL_QUADS );
            glVertex3f( -1.0f, -1.0f, -1.0f );
            glVertex3f( -1.0f,  1.0f, -1.0f );
            glVertex3f(  1.0f,  1.0f, -1.0f );
            glVertex3f(  1.0f, -1.0f, -1.0f );
        glEnd();

        // Recover previous matrix
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        // enable lighting
        glEnable( GL_LIGHTING );
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // save the current matrix state, so transformation will
    // not persist across displayFunc calls, since we
    // will do a glPopMatrix() at the end of this function
    glPushMatrix();

    // render the scene
    render();

    // restore the matrix state
    glPopMatrix();

    // flush the buffer
    glFlush();

    // swap the double buffer
    SwapBuffers(hDC);

    // Clear redraw event
    ValidateRect(hWnd, NULL);

    return 0L;
}

static LRESULT CALLBACK
OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    GLsizei w = LOWORD(lParam);
    GLsizei h = HIWORD(lParam);

    // map the view port to the entire client area
    glViewport(0, 0, w, h);

    // set the matrix mode to projection matrix
    glMatrixMode(GL_PROJECTION);

    // load the identity matrix
    glLoadIdentity();

    // set the perspective matrix
    gluPerspective( 64.0, (GLdouble) w / (GLdouble)h, .1, 300.0 );

    // set the matrix mode to the modelview matrix
    glMatrixMode(GL_MODELVIEW);

    // load the identity matrix into the modelview matrix
    glLoadIdentity();

    // set the 'camera'
    gluLookAt( 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 2.0, 3.0 );

    // set the position of the light
    glLightfv( GL_LIGHT0, GL_POSITION, g_light_position );

    return 0L;
}

LRESULT CALLBACK
ScreenSaverProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        return OnCreate(hWnd, wParam, lParam);

    case WM_TIMER:
        InvalidateRect(hWnd, NULL, FALSE);
        return 0L;

    case WM_DESTROY:
        return OnDestroy(hWnd, wParam, lParam);

    case WM_PAINT:
        return OnPaint(hWnd, wParam, lParam);

    case WM_SIZE: // Resizing The Screen
        return OnSize(hWnd, wParam, lParam);
    }

#ifdef _DEBUG_SSTARS
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
#else
    return DefScreenSaverProc(hWnd, uMsg, wParam, lParam);
#endif
}

#ifdef _DEBUG_SSTARS

ATOM InitApp(HINSTANCE hInstance, LPCTSTR szClassName)
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(wc));

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ScreenSaverProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szClassName;

    return RegisterClass(&wc);
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND  hWnd;
    TCHAR szClass[] = _T("CLASS");
    TCHAR szTitle[] = _T("TITLE");

    InitApp(hInstance, szClass);

    hWnd = CreateWindow(
            szClass,
            szTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            192*3, //CW_USEDEFAULT,
            108*3, //CW_USEDEFAULT,
            NULL,
            NULL,
            hInstance,
            NULL);

    if (hWnd)
    {
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
    }

    return hWnd;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    MSG     msg;
    HWND    hWnd;
    HWND    hDlg;

    RegisterDialogClasses(hInstance);

    hWnd = InitInstance(hInstance, nShowCmd);

    hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(DLG_SCRNSAVECONFIGURE), NULL, ScreenSaverConfigureDialog);
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    for (;;)
    {
        if (GetMessage(&msg, NULL, 0, 0) <= 0)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
#endif
