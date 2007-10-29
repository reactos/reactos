/*
 *  Copyright 2003 J Brown
 *  Copyright 2006 Andrey Korotaev <unC0Rr@inbox.ru>
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

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <tchar.h>
#include "resource.h"

#define APPNAME _T("Cylfrac")
#define wfactor 0.9
#define rotfactor 1.5
#define FPS 100
#define timerdelay 1000/FPS

POINT initpoint;
HDC dc;
HGLRC hrc;
DWORD oldticks;
MMRESULT TimerID;

DWORD lvls = 7;
int cylquality = 8;

HINSTANCE hInstance;

GLUquadricObj * cylinder;

float angle = 0;
float colorh = 0.0;
float rval, gval, bval;

BOOL fullscreen = FALSE;

float _RGB(float H, float M1, float M2)
{
	if(H < 0.0) H += 360.0;
		else if(H > 360.0) H -= 360.0;
	if(H < 60) return M1 + (M2 - M1) * H / 60.0;
	if((H >= 60 )&&(H < 180)) return M2;
	if((H >= 180)&&(H < 240)) return M1 + (M2 - M1)*(240 - H) / 60.0;
	return M1;
}

void HLStoRGB(float H, float L, float S,
              float* R, float* G, float* B)
{
    float M1, M2;
	if(S <= 0.5) M2 = S * (1 + L);
		else M2 = S * (1 - L) + L;
	M1 = 2 * S - M2;
	if (L == 0.0)
	{
		*R = S;
		*G = S;
		*B = S;
	} else {
		*R = _RGB(H + 120.0, M1, M2);
		*G = _RGB(H        , M1, M2);
		*B = _RGB(H - 120.0, M1, M2);
	}
}

void DrawCylinder(int n, float rota, float width)
{
	glPushMatrix();
	glColor3f(rval/n, gval/n, bval/n);
	glRotatef(rota, 0.0, 1.0, 0.0);
	gluCylinder(cylinder, width, width * wfactor, n * 0.5, cylquality, 1);
	glTranslatef(0.0, 0.0, -n * 0.5);
	gluCylinder(cylinder, width * wfactor, width, n * 0.5, cylquality, 1);
	if(n > 1)
	{
		float r = rota * rotfactor;
		glRotatef(90.0, 1.0, 0.0, 0.0);
		DrawCylinder(n - 1,  r, width * wfactor);
		glTranslatef(0.0, n, 0.0);
		DrawCylinder(n - 1, -r, width * wfactor);
	}
	glPopMatrix();
}

void DrawScene(HWND hwnd, HDC dc, int ticks)
{
	PAINTSTRUCT ps;
	dc = BeginPaint(hwnd, &ps);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glRotatef(ticks * 0.01, 0.0, 1.0, -0.5);
	angle += ticks * 0.01;
	colorh += ticks * 0.003;
	if (colorh > 360.0) colorh -= 360.0;
	HLStoRGB(colorh, 1.0, 0.7, &rval, &gval, &bval);
	DrawCylinder(lvls, angle, 0.2);
	SwapBuffers(dc);
	EndPaint(hwnd, &ps);
}

void CALLBACK TimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	InvalidateRect((HWND)dwUser, NULL, 0);
}

void MyPixelFormat(HDC dc)
{
	int npf;
	PIXELFORMATDESCRIPTOR pfd;

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;

	npf = ChoosePixelFormat(dc, &pfd);
	if(npf != 0)
		SetPixelFormat(dc, npf, &pfd);
}

void InitGL(HWND hwnd)
{
	GLfloat lightpos[4] = {2.0, 2.0, -2.0, 0.7};
	GLfloat ca = 1.0;
	dc = GetDC(hwnd);
	MyPixelFormat(dc);
	hrc = wglCreateContext(dc);
	wglMakeCurrent(dc, hrc);
	cylinder = gluNewQuadric();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0,  GL_POSITION, (GLfloat *)&lightpos);
	glLightfv(GL_LIGHT0,  GL_LINEAR_ATTENUATION, &ca);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_CREATE:
			GetCursorPos(&initpoint);
			InitGL(hwnd);
			oldticks = GetTickCount();
			TimerID = timeSetEvent (timerdelay, 0, TimeProc, (DWORD)hwnd, TIME_PERIODIC);
			break;
		case WM_PAINT:
		{
			DWORD ticks = oldticks;
			POINT currpoint;
			oldticks = GetTickCount();
			DrawScene(hwnd, dc, oldticks - ticks);
			if(fullscreen)
			{
				GetCursorPos(&currpoint);
				if(abs(currpoint.x - initpoint.x) + (abs(currpoint.y - initpoint.y)) > 10)
					PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
			break;
		}
		case WM_DESTROY:
			timeKillEvent(TimerID);
			gluDeleteQuadric(cylinder);
			wglMakeCurrent(0, 0);
			wglDeleteContext(hrc);
			ReleaseDC(hwnd, dc);
			DeleteDC(dc);
			PostQuitMessage(0);
			break;
		case WM_NOTIFY:
		case WM_SYSKEYDOWN:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case WM_SIZE:
		{
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			float fscale;
			glViewport(0, 0, width, height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			fscale = 0.8/(float)lvls;
			glScalef(fscale, fscale, fscale);
			break;
		}

	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
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

void ParseCommandLine(PSTR szCmdLine, int *chOption, HWND *hwndParent)
{
	int ch;

	if (!strlen(szCmdLine))
		return;

	ch = *szCmdLine++;

	if(ch == '-' || ch == '/')
		ch = *szCmdLine++;

	if(ch >= 'A' && ch <= 'Z')
		ch += 'a' - 'A';

	*chOption = ch;

	if (ch == 's' || ch == 'c')
		return;

	ch = *szCmdLine++;

	if(ch == ':')
		ch = *szCmdLine++;

	while(ch == ' ' || ch == '\t')
		ch = *szCmdLine++;

	if(isdigit(ch))
	{
		unsigned int i = atoi(szCmdLine - 1);
		*hwndParent = (HWND)i;
	}
	else
		*hwndParent = 0;
}

void Configure(void)
{
	TCHAR szTitle[256];
	TCHAR szText[256];

	LoadString(hInstance,
		   IDS_TITLE,
		   szTitle,
		   256);

	LoadString(hInstance,
		   IDS_TEXT,
		   szText,
		   256);

	MessageBox(0,
	           szText,
	           szTitle,
	           MB_OK | MB_ICONWARNING);
}

int WINAPI WinMain (HINSTANCE hInst,
                    HINSTANCE hPrev,
                    LPSTR lpCmdLine,
                    int iCmdShow)
{
	HWND	hwndParent = 0;
	int	chOption = 0;
	MSG	Message;

	hInstance = hInst;

	ParseCommandLine(lpCmdLine, &chOption, &hwndParent);

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


