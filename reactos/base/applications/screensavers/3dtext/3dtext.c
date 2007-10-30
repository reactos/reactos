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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <windows.h>	// Header File For Windows
#include <math.h>		// Header File For Windows Math Library
#include <GL/gl.h>		// Header File For The OpenGL32 Library
#include <GL/glu.h>		// Header File For The GLu32 Library
#include <tchar.h>

#include "resource.h"
#include "3dtext.h"

static	HGLRC hRC;		// Permanent Rendering Context
static	HDC hDC;		// Private GDI Device Context

GLuint	base;			// Base Display List For The Font Set
GLfloat	rot;			// Used To Rotate The Text

#define APPNAME _T("3DText")

HINSTANCE hInstance;
BOOL fullscreen = FALSE;

GLvoid BuildFont(GLvoid)								// Build Our Bitmap Font
{
	GLYPHMETRICSFLOAT	gmf[256];						// Address Buffer For Font Storage
	HFONT	font;										// Windows Font ID

	base = glGenLists(256);								// Storage For 256 Characters

	font = CreateFont(	-12,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_BOLD,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						DEFAULT_CHARSET,				// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						_T("Tahoma"));				// Font Name

	SelectObject(hDC, font);							// Selects The Font We Created

	wglUseFontOutlines(	hDC,							// Select The Current DC
						0,								// Starting Character
						255,							// Number Of Display Lists To Build
						base,							// Starting Display Lists
						0.0f,							// Deviation From The True Outlines
						0.2f,							// Font Thickness In The Z Direction
						WGL_FONT_POLYGONS,				// Use Polygons, Not Lines
						gmf);							// Address Of Buffer To Recieve Data
}

GLvoid KillFont(GLvoid)									// Delete The Font
{
  glDeleteLists(base, 256);								// Delete All 256 Characters
}

GLvoid glPrint(LPTSTR text)								// Custom GL "Print" Routine
{
  if (text == NULL)										// If There's No Text
    return;												// Do Nothing

  glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
  glListBase(base);										// Sets The Base Character to 32

  glCallLists(_tcslen(text),
#ifdef UNICODE 
      GL_UNSIGNED_SHORT
#else
      GL_UNSIGNED_BYTE
#endif
          , text); // Draws The Display List Text

  glPopAttrib();										// Pops The Display List Bits
}

GLvoid InitGL(GLsizei Width, GLsizei Height)	// Will Be Called Right After The GL Window Is Created
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);		// Clear The Background Color To Black
	glClearDepth(1.0);							// Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LESS);						// The Type Of Depth Test To Do
	glEnable(GL_DEPTH_TEST);					// Enables Depth Testing
	glShadeModel(GL_SMOOTH);					// Enables Smooth Color Shading

	glMatrixMode(GL_PROJECTION);				// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);					// Select The Modelview Matrix

	BuildFont();								// Build The Font
	glEnable(GL_LIGHT0);						// Enable Default Light (Quick And Dirty)
	glEnable(GL_LIGHTING);						// Enable Lighting
	glEnable(GL_COLOR_MATERIAL);				// Enable Coloring Of Material
}

GLvoid ReSizeGLScene(GLsizei Width, GLsizei Height)	// Handles Window Resizing
{
	if (Height==0)									// Is Window Too Small (Divide By Zero Error)
		Height=1;									// If So Make It One Pixel Tall

	// Reset The Current Viewport And Perspective Transformation
	glViewport(0, 0, Width, Height);

	glMatrixMode(GL_PROJECTION);					// Select The Projection Matrix
	glLoadIdentity();								// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);
	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
}

GLvoid DrawGLScene(GLvoid)									// Handles Rendering
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer
	glLoadIdentity();										// Reset The View
	glTranslatef(0.0f,0.0f,-10.0f);							// Move One Unit Into The Screen
	glRotatef(rot,1.0f,0.0f,0.0f);							// Rotate On The X Axis
	glRotatef(rot*1.2f,0.0f,1.0f,0.0f);						// Rotate On The Y Axis
	glRotatef(rot*1.4f,0.0f,0.0f,1.0f);						// Rotate On The Z Axis
	glTranslatef(-3.5f,0.0f,0.0f);							// Move To The Left Before Drawing
	// Pulsing Colors Based On The Rotation
	glColor3f(
		(1.0f*(cos(rot/20.0f))),
		(1.0f*(sin(rot/25.0f))),
		(1.0f-0.5f*(cos(rot/17.0f))));
	glPrint(m_Text);							// Print GL Text To The Screen
	glColor3f(0.0f,0.0f,1.0f);								// Make The Text Blue
	rot+=0.1f;												// Increase The Rotation Variable
}

LRESULT CALLBACK WndProc(	HWND	hWnd,
							UINT	message,
							WPARAM	wParam,
							LPARAM	lParam)
{
	static POINT ptLast;
	static POINT ptCursor;
	static BOOL  fFirstTime = TRUE;
	RECT	Screen;							// Used Later On To Get The Size Of The Window
	GLuint	PixelFormat;					// Pixel Format Storage
	static	PIXELFORMATDESCRIPTOR pfd=		// Pixel Format Descriptor
	{
		sizeof(PIXELFORMATDESCRIPTOR),		// Size Of This Pixel Format Descriptor
		1,									// Version Number (?)
		PFD_DRAW_TO_WINDOW |				// Format Must Support Window
		PFD_SUPPORT_OPENGL |				// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,					// Must Support Double Buffering
		PFD_TYPE_RGBA,						// Request An RGBA Format
		16,									// Select A 16Bit Color Depth
		0, 0, 0, 0, 0, 0,					// Color Bits Ignored (?)
		0,									// No Alpha Buffer
		0,									// Shift Bit Ignored (?)
		0,									// No Accumulation Buffer
		0, 0, 0, 0,							// Accumulation Bits Ignored (?)
		16,									// 16Bit Z-Buffer (Depth Buffer)  
		0,									// No Stencil Buffer
		0,									// No Auxiliary Buffer (?)
		PFD_MAIN_PLANE,						// Main Drawing Layer
		0,									// Reserved (?)
		0, 0, 0								// Layer Masks Ignored (?)
	};

	switch (message)						// Tells Windows We Want To Check The Message
	{
		case WM_CREATE:										// Window Creation
			hDC = GetDC(hWnd);								// Gets A Device Context For The Window
			PixelFormat = ChoosePixelFormat(hDC, &pfd);		// Finds The Closest Match To The Pixel Format We Set Above

			if (!PixelFormat)								// No Matching Pixel Format?
			{
				MessageBox(0,_TEXT("Can't Find A Suitable PixelFormat."),_TEXT("Error"),MB_OK|MB_ICONERROR);
				PostQuitMessage(0);			// This Sends A 'Message' Telling The Program To Quit
				break;						// Prevents The Rest Of The Code From Running
			}

			if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Can We Set The Pixel Mode?
			{
				MessageBox(0,_TEXT("Can't Set The PixelFormat."),_TEXT("Error"),MB_OK|MB_ICONERROR);
				PostQuitMessage(0);			// This Sends A 'Message' Telling The Program To Quit
				break;						// Prevents The Rest Of The Code From Running
			}

			hRC = wglCreateContext(hDC);					// Grab A Rendering Context
			if(!hRC)										// Did We Get One?
			{
				MessageBox(0,_TEXT("Can't Create A GL Rendering Context."),_TEXT("Error"),MB_OK|MB_ICONERROR);
				PostQuitMessage(0);			// This Sends A 'Message' Telling The Program To Quit
				break;						// Prevents The Rest Of The Code From Running
			}

			if(!wglMakeCurrent(hDC, hRC))					// Can We Make The RC Active?
			{
				MessageBox(0,_TEXT("Can't Activate GLRC."),_TEXT("Error"),MB_OK|MB_ICONERROR);
				PostQuitMessage(0);			// This Sends A 'Message' Telling The Program To Quit
				break;						// Prevents The Rest Of The Code From Running
			}

			GetClientRect(hWnd, &Screen);					// Grab Screen Info For The Current Window
			InitGL(Screen.right, Screen.bottom);			// Initialize The GL Screen Using Screen Info
			break;

		case WM_DESTROY:									// Windows Being Destroyed
		case WM_CLOSE:										// Windows Being Closed
			{
				ChangeDisplaySettings(NULL, 0);					// Disable Fullscreen Mode
				KillFont();										// Deletes The Font Display List

				wglMakeCurrent(hDC,NULL);						// Make The DC Current
				wglDeleteContext(hRC);							// Kill The RC
				ReleaseDC(hWnd,hDC);							// Free The DC

				PostQuitMessage(0);								// Quit The Program
			}
			break;
		case WM_PAINT:
		{
			DrawGLScene();
			SwapBuffers(hDC);
			break;
		}
		case WM_NOTIFY:
		case WM_SYSKEYDOWN:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			{
				// If we've got a parent then we must be a preview
				if(GetParent(hWnd) != 0)
					return 0;

				if(fFirstTime)
				{
					GetCursorPos(&ptLast);
					fFirstTime = FALSE;
				}

				GetCursorPos(&ptCursor);

				// if the mouse has moved more than 3 pixels then exit
				if(abs(ptCursor.x - ptLast.x) >= 3 || abs(ptCursor.y - ptLast.y) >= 3)
					PostMessage(hWnd, WM_CLOSE, 0, 0);

				ptLast = ptCursor;

				return 0;
			}
		case WM_SIZE:										// Resizing The Screen
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));	// Resize To The New Window Size
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));	// Pass Windows Messages
	}
return (0);
}

void InitSaver(HWND hwndParent)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style            = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc      = WndProc;
	wc.lpszClassName    = APPNAME;
	RegisterClass(&wc);

	if (hwndParent != 0)
	{
		RECT rect;
		GetClientRect(hwndParent, &rect);
		CreateWindow(APPNAME, APPNAME,
		             WS_VISIBLE | WS_CHILD,
		             0, 0,
		             rect.right,
		             rect.bottom,
		             hwndParent, 0,
		             hInstance, NULL);
		fullscreen = FALSE;
	} else {
		HWND hwnd;
		hwnd = CreateWindow(APPNAME, APPNAME,
		                    WS_VISIBLE | WS_POPUP | WS_EX_TOPMOST,
		                    0, 0,
		                    GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		                    HWND_DESKTOP, 0,
		                    hInstance, NULL);
		ShowWindow(hwnd, SW_SHOWMAXIMIZED);
		ShowCursor(FALSE);
		fullscreen = TRUE;
	}
}

//
//	Look for any options Windows has passed to us:
//
//	-a <hwnd>		(set password)
//  -s				(screensave)
//  -p <hwnd>		(preview)
//  -c <hwnd>		(configure)
//
VOID ParseCommandLine(LPTSTR szCmdLine, UCHAR *chOption, HWND *hwndParent)
{
	TCHAR ch = *szCmdLine++;

	if(ch == _T('-') || ch == _T('/'))
		ch = *szCmdLine++;

	if(ch >= _T('A') && ch <= _T('Z'))
		ch += _T('a') - _T('A');		//convert to lower case

	*chOption = ch;
	ch = *szCmdLine++;

	if(ch == _T(':'))
		ch = *szCmdLine++;

	while(ch == _T(' ') || ch == _T('\t'))
		ch = *szCmdLine++;

	if(isdigit(ch))
	{
		unsigned int i = _ttoi(szCmdLine - 1);
		*hwndParent = (HWND)i;
	}
	else
		*hwndParent = NULL;
}

//
//	Dialogbox procedure for Configuration window
//
BOOL CALLBACK ConfigDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
        case WM_INITDIALOG:
			SetDlgItemText(hwnd, IDC_MESSAGE_TEXT, m_Text);
			return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					GetDlgItemText(hwnd, IDC_MESSAGE_TEXT, m_Text, MAX_PATH);
					SaveSettings();
                    EndDialog(hwnd, IDOK);
                break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                break;
            }
			return FALSE;
		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;
        default:
            return FALSE;
	}

	return TRUE;
}

void Configure(void)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG), NULL , (DLGPROC)ConfigDlgProc);	
}

int CALLBACK _tWinMain (HINSTANCE hInst,
                      HINSTANCE hPrev,
                      LPTSTR lpCmdLine,
                      int iCmdShow)
{
	HWND	hwndParent = 0;
	UCHAR	chOption;
	MSG	Message;

	hInstance = hInst;

	ParseCommandLine(lpCmdLine, &chOption, &hwndParent);

	LoadSettings();

	switch(chOption)
	{
		case 's':
			InitSaver(0);
			break;

		case 'p':
			InitSaver(hwndParent);
			break;

		case 'c':
		default:
			Configure();
			return 0;
	}

	while (GetMessage(&Message, 0, 0, 0))
		DispatchMessage(&Message);

	return Message.wParam;
}

