/*
 *  Copyright 2000 Jeff Molofee http://nehe.gamedev.net/ (Original code)
 *  Copyright 2006 Eric Kohl
 *  Copyright 2007 Marc Piulachs (marc.piulachs@codexchange.net) - minor modifications , converted to screensaver
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "3dtext.h"

#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <scrnsave.h>
#include <math.h>
#include <GL/glu.h>

#include "resource.h"

static HGLRC hRC;       // Permanent Rendering Context
static HDC hDC;         // Private GDI Device Context

GLuint base;            // Base Display List For The Font Set
GLfloat rot;            // Used To Rotate The Text
GLfloat extentX = 0.0f;
GLfloat extentY = 0.0f;

#define APPNAME _T("3DText")

HINSTANCE hInstance;
BOOL fullscreen = FALSE;
UINT uTimerID;                                          // SetTimer Actual ID
#define APP_TIMER             1                         // Graphics Update Timer ID
#define APP_TIMER_INTERVAL    (USER_TIMER_MINIMUM * 5)  // Graphics Update Interval

// Build Our Bitmap Font
GLvoid BuildFont(GLvoid)
{
    // Address Buffer For Font Storage
    GLYPHMETRICSFLOAT gmf[256];
    // Windows Font Handle
    HFONT font;
    size_t i;
    TCHAR c;
    GLfloat cellOriginX = 0.0f;
    GLfloat stringOriginX;
    GLfloat stringExtentX = 0.0f;
    GLfloat stringExtentY = 0.0f;

    // Storage For 256 Characters
    base = glGenLists(256);

    font = CreateFont(-12,
                      0,                            // Width Of Font
                      0,                            // Angle Of Escapement
                      0,                            // Orientation Angle
                      FW_BOLD,                      // Font Weight
                      FALSE,                        // Italic
                      FALSE,                        // Underline
                      FALSE,                        // Strikeout
                      DEFAULT_CHARSET,              // Character Set Identifier
                      OUT_TT_PRECIS,                // Output Precision
                      CLIP_DEFAULT_PRECIS,          // Clipping Precision
                      ANTIALIASED_QUALITY,          // Output Quality
                      FF_DONTCARE|DEFAULT_PITCH,    // Family And Pitch
                      _T("Tahoma"));                // Font Name

    // Selects The Font We Created
    SelectObject(hDC, font);

    wglUseFontOutlines(hDC,                     // Select The Current DC
                       0,                       // Starting Character
                       255,                     // Number Of Display Lists To Build
                       base,                    // Starting Display Lists
                       0.0f,                    // Deviation From The True Outlines
                       0.2f,                    // Font Thickness In The Z Direction
                       WGL_FONT_POLYGONS,       // Use Polygons, Not Lines
                       gmf);                    // Address Of Buffer To Receive Data

    // Calculate the string extent
    for (i = 0; i < _tcslen(m_Text); i++)
    {
        c = m_Text[i];

        stringOriginX = cellOriginX + gmf[c].gmfptGlyphOrigin.x;

        stringExtentX = stringOriginX + gmf[c].gmfBlackBoxX;
        if (gmf[c].gmfBlackBoxY > stringExtentY)
            stringExtentY = gmf[c].gmfBlackBoxY;

        cellOriginX = cellOriginX + gmf[c].gmfCellIncX;
    }

    extentX = stringExtentX;
    extentY = stringExtentY;
}

// Delete The Font
GLvoid KillFont(GLvoid)
{
    // Delete all 256 characters
    glDeleteLists(base, 256);
}

// Custom GL "Print" Routine
GLvoid glPrint(LPTSTR text)
{
    // If there's no text, do nothing
    if (text == NULL)
        return;

    // Pushes The Display List Bits
    glPushAttrib(GL_LIST_BIT);

    // Sets The Base Character to 32
    glListBase(base);

    // Draws The Display List Text
    glCallLists(_tcslen(text),
#ifdef UNICODE
                GL_UNSIGNED_SHORT,
#else
                GL_UNSIGNED_BYTE,
#endif
                text);

    // Pops The Display List Bits
    glPopAttrib();
}

// Will Be Called Right After The GL Window Is Created
GLvoid InitGL(GLsizei Width, GLsizei Height)
{
    // Clear The Background Color To Black
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enables Clearing Of The Depth Buffer
    glClearDepth(1.0);

    // The Type Of Depth Test To Do
    glDepthFunc(GL_LESS);

    // Enables Depth Testing
    glEnable(GL_DEPTH_TEST);

    // Enables Smooth Color Shading
    glShadeModel(GL_SMOOTH);

    // Select The Projection Matrix
    glMatrixMode(GL_PROJECTION);

    // Reset The Projection Matrix
    glLoadIdentity();

    // Calculate The Aspect Ratio Of The Window
    gluPerspective(45.0f, (GLfloat)Width / (GLfloat)Height, 0.1f, 100.0f);

    // Select The Modelview Matrix
    glMatrixMode(GL_MODELVIEW);

    // Build The Font
    BuildFont();

    // Enable Default Light (Quick And Dirty)
    glEnable(GL_LIGHT0);

    // Enable Lighting
    glEnable(GL_LIGHTING);

    // Enable Coloring Of Material
    glEnable(GL_COLOR_MATERIAL);
}

// Handles Window Resizing
GLvoid ReSizeGLScene(GLsizei Width, GLsizei Height)
{
    // Is Window Too Small (Divide By Zero Error)
    if (Height == 0)
    {
        // If So Make It One Pixel Tall
        Height = 1;
    }

    // Reset The Current Viewport And Perspective Transformation
    glViewport(0, 0, Width, Height);

    // Select The Projection Matrix
    glMatrixMode(GL_PROJECTION);

    // Reset The Projection Matrix
    glLoadIdentity();

    // Calculate The Aspect Ratio Of The Window
    gluPerspective(45.0f, (GLfloat)Width / (GLfloat)Height, 0.1f, 100.0f);

    // Select The Modelview Matrix
    glMatrixMode(GL_MODELVIEW);
}

// Handles Rendering
GLvoid DrawGLScene(GLvoid)
{
    // Save ticks count of previous frame here
    static DWORD dwTicks = 0;

    // Clear The Screen And The Depth Buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset The View
    glLoadIdentity();

    // Move One Unit Into The Screen
    glTranslatef(0.0f, 0.0f, -10.0f);

    // Rotate On The X Axis
    glRotatef(rot, 1.0f, 0.0f, 0.0f);

    // Rotate On The Y Axis
    glRotatef(rot * 1.2f, 0.0f, 1.0f, 0.0f);

    // Rotate On The Z Axis
    glRotatef(rot * 1.4f, 0.0f, 0.0f, 1.0f);

    // Move to the Left and Down before drawing
    glTranslatef(-(extentX / 2.0f),
                 -(extentY / 2.0f),
                 0.0f);

    // Pulsing Colors Based On The Rotation
    glColor3f((1.0f * (GLfloat)(cos(rot / 20.0f))),
              (1.0f * (GLfloat)(sin(rot / 25.0f))),
              (1.0f - 0.5f * (GLfloat)(cos(rot / 17.0f))));

    // Print GL Text To The Screen
    glPrint(m_Text);

    // Make The Text Blue
    glColor3f(0.0f, 0.0f, 1.0f);

    // Increase The Rotation Variable
    if(dwTicks)
        rot += (GetTickCount() - dwTicks) / 20.0f;
    dwTicks = GetTickCount();
}

LRESULT CALLBACK
ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT Screen;                            // Used Later On To Get The Size Of The Window
    GLuint PixelFormat;                     // Pixel Format Storage
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

    switch (message)
    {
        case WM_CREATE:
            LoadSettings();

            // Gets A Device Context For The Window
            hDC = GetDC(hWnd);

            // Finds The Closest Match To The Pixel Format We Set Above
            PixelFormat = ChoosePixelFormat(hDC, &pfd);

            // No Matching Pixel Format?
            if (!PixelFormat)
            {
                MessageBox(0, _TEXT("Can't Find A Suitable PixelFormat."), _TEXT("Error"),MB_OK | MB_ICONERROR);

                // This Sends A 'Message' Telling The Program To Quit
                PostQuitMessage(0);
                break;
            }

            // Can We Set The Pixel Mode?
            if (!SetPixelFormat(hDC, PixelFormat, &pfd))
            {
                MessageBox(0, _TEXT("Can't Set The PixelFormat."), _TEXT("Error"), MB_OK | MB_ICONERROR);

                // This Sends A 'Message' Telling The Program To Quit
                PostQuitMessage(0);
                break;
            }

            // Grab A Rendering Context
            hRC = wglCreateContext(hDC);

            // Did We Get One?
            if (!hRC)
            {
                MessageBox(0, _TEXT("Can't Create A GL Rendering Context."), _TEXT("Error"), MB_OK | MB_ICONERROR);

                // This Sends A 'Message' Telling The Program To Quit
                PostQuitMessage(0);
                break;
            }

            // Can We Make The RC Active?
            if (!wglMakeCurrent(hDC, hRC))
            {
                MessageBox(0, _TEXT("Can't Activate GLRC."), _TEXT("Error"), MB_OK | MB_ICONERROR);

                // This Sends A 'Message' Telling The Program To Quit
                PostQuitMessage(0);
                break;
            }

            // Grab Screen Info For The Current Window
            GetClientRect(hWnd, &Screen);

            // Initialize The GL Screen Using Screen Info
            InitGL(Screen.right, Screen.bottom);

            // Create Graphics update timer
            uTimerID = SetTimer(hWnd, APP_TIMER, APP_TIMER_INTERVAL, NULL);
            break;

        case WM_DESTROY:
            // Disable Fullscreen Mode
            ChangeDisplaySettings(NULL, 0);

            // Delete the Update Timer
            KillTimer(hWnd, uTimerID);

            // Deletes The Font Display List
            KillFont();

            // Make The DC Current
            wglMakeCurrent(hDC, NULL);

            // Kill The RC
            wglDeleteContext(hRC);

            // Free The DC
            ReleaseDC(hWnd, hDC);
            break;

        case WM_PAINT:
            DrawGLScene();
            SwapBuffers(hDC);
            // Mark this window as updated, so the OS won't ask us to update it again.
            ValidateRect(hWnd, NULL);
            break;

        case WM_SIZE: // Resizing The Screen
            // Resize To The New Window Size
            ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_TIMER:
            // Used to update graphic based on timer udpate interval
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        default:
            // Pass Windows Messages to the default screensaver window procedure
            return DefScreenSaverProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

//
// Dialogbox procedure for Configuration window
//
BOOL CALLBACK ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            LoadSettings();
            SetDlgItemText(hDlg, IDC_MESSAGE_TEXT, m_Text);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    GetDlgItemText(hDlg, IDC_MESSAGE_TEXT, m_Text, MAX_PATH);
                    SaveSettings();

                    /* Fall through */

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
            }
            return FALSE;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}
