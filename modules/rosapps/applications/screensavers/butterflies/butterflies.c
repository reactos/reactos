#include <windows.h>
#include <scrnsave.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "resource.h"

HINSTANCE		hInstance;			// Holds The Instance Of The Application

GLuint texture[3];	                //stores texture objects and display list

LPCTSTR registryPath = _T("Software\\Microsoft\\ScreenSavers\\Butterflies");
BOOL dRotate;


struct object						// Create A Structure Called Object
{
	int   tex;						// Integer Used To Select Our Texture
	float x;						// X Position
	float y;						// Y Position
	float z;						// Z Position
	float yi;						// Y Increase Speed (Fall Speed)
	float spinz;					// Z Axis Spin
	float spinzi;					// Z Axis Spin Speed
	float flap;						// Flapping Triangles :)
	float fi;						// Flap Direction (Increase Value)
};

struct object obj[50];
//object obj[50];						// Create 50 Objects Using The Object Structure

void SetDefaults()
{
	dRotate = TRUE;
}

void ReadRegistry(){
	LONG result;
	HKEY skey;
	DWORD valtype, valsize, val;

	SetDefaults();

	result = RegOpenKeyEx(HKEY_CURRENT_USER, registryPath, 0, KEY_READ, &skey);
	if(result != ERROR_SUCCESS)
		return;

	valsize=sizeof(val);

	result = RegQueryValueEx(skey, _T("Rotate"), 0, &valtype, (LPBYTE)&val, &valsize);
	if(result == ERROR_SUCCESS)
		dRotate = val;
	RegCloseKey(skey);
}

void WriteRegistry(){
    LONG result;
	HKEY skey;
	DWORD val, disp;

	result = RegCreateKeyEx(HKEY_CURRENT_USER, registryPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &skey, &disp);
	if(result != ERROR_SUCCESS)
		return;

	val = dRotate;
	RegSetValueEx(skey, _T("Rotate"), 0, REG_DWORD, (CONST BYTE*)&val, sizeof(val));

	RegCloseKey(skey);
}

void SetObject(int loop)										// Sets The Initial Value Of Each Object (Random)
{
	obj[loop].tex=rand()%3;										// Texture Can Be One Of 3 Textures
	obj[loop].x=rand()%34-17.0f;								// Random x Value From -17.0f To 17.0f
	obj[loop].y=18.0f;											// Set y Position To 18 (Off Top Of Screen)
	obj[loop].z=-((rand()%30000/1000.0f)+10.0f);				// z Is A Random Value From -10.0f To -40.0f
	obj[loop].spinzi=(rand()%10000)/5000.0f-1.0f;				// spinzi Is A Random Value From -1.0f To 1.0f
	obj[loop].flap=0.0f;										// flap Starts Off At 0.0f;
	obj[loop].fi=0.05f+(rand()%100)/1000.0f;					// fi Is A Random Value From 0.05f To 0.15f
	obj[loop].yi=0.001f+(rand()%1000)/10000.0f;					// yi Is A Random Value From 0.001f To 0.101f
}

void LoadGLTextures()											// Creates Textures From Bitmaps In The Resource File
{
	HBITMAP hBMP;												// Handle Of The Bitmap
	BITMAP	BMP;												// Bitmap Structure
    int loop;

	// The ID Of The 3 Bitmap Images We Want To Load From The Resource File
	byte	Texture[]={	IDB_BUTTERFLY1,	IDB_BUTTERFLY2,	IDB_BUTTERFLY3 };

	glGenTextures(sizeof(Texture), &texture[0]);				// Generate 3 Textures (sizeof(Texture)=3 ID's)
	for (loop=0; loop<sizeof(Texture); loop++)				// Loop Through All The ID's (Bitmap Images)
	{
		hBMP=(HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(Texture[loop]), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		if (hBMP)												// Does The Bitmap Exist?
		{														// If So...
			GetObject(hBMP,sizeof(BMP), &BMP);					// Get The Object
																// hBMP: Handle To Graphics Object
																// sizeof(BMP): Size Of Buffer For Object Information
																// Buffer For Object Information
			glPixelStorei(GL_UNPACK_ALIGNMENT,4);				// Pixel Storage Mode (Word Alignment / 4 Bytes)
			glBindTexture(GL_TEXTURE_2D, texture[loop]);		// Bind Our Texture
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); // Mipmap Linear Filtering

			// Generate Mipmapped Texture (3 Bytes, Width, Height And Data From The BMP)
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, BMP.bmWidth, BMP.bmHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, BMP.bmBits);
		}
	}
}

HGLRC InitOGLWindow(HWND hWnd)
{
	HDC hDC = GetDC(hWnd);
	HGLRC hRC = 0;
	PIXELFORMATDESCRIPTOR pfd;
	int nFormat;

	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;

	nFormat = ChoosePixelFormat(hDC, &pfd);
	DescribePixelFormat(hDC, nFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	SetPixelFormat(hDC, nFormat, &pfd);

	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);

	ReleaseDC(hWnd, hDC);

	return hRC;
}

void InitOpenGL(GLsizei width, GLsizei height)
{
    int loop;

	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective (45.0f, (GLfloat)(width)/(GLfloat)(height),1.0f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();
		// Start Of User Initialization
	LoadGLTextures();									// Load The Textures From Our Resource File

	glClearColor (0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth (1.0f);								// Depth Buffer Setup
	glDepthFunc (GL_LEQUAL);							// The Type Of Depth Testing (Less Or Equal)
	glDisable(GL_DEPTH_TEST);							// Disable Depth Testing
	glShadeModel (GL_SMOOTH);							// Select Smooth Shading
	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Set Perspective Calculations To Most Accurate
	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glBlendFunc(GL_ONE,GL_SRC_ALPHA);					// Set Blending Mode (Cheap / Quick)
	glEnable(GL_BLEND);


	for (loop=0; loop<50; loop++)					// Loop To Initialize 50 Objects
	{
		SetObject(loop);										// Call SetObject To Assign New Random Values
	}

}

void Display()
{
    int loop;
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear Screen And Depth Buffer

	for (loop=0; loop<50; loop++)							// Loop Of 50 (Draw 50 Objects)
	{
		glLoadIdentity ();										// Reset The Modelview Matrix
		glBindTexture(GL_TEXTURE_2D, texture[obj[loop].tex]);	// Bind Our Texture
		glTranslatef(obj[loop].x,obj[loop].y,obj[loop].z);		// Position The Object
		glRotatef(45.0f,1.0f,0.0f,0.0f);						// Rotate On The X-Axis
		if (dRotate)
		{
			glRotatef((obj[loop].spinz),0.0f,0.0f,1.0f);			// Spin On The Z-Axis
		}
		glBegin(GL_TRIANGLES);									// Begin Drawing Triangles
			// First Triangle														    _____
			glTexCoord2f(1.0f,1.0f); glVertex3f( 1.0f, 1.0f, 0.0f);				//	(2)|    / (1)
			glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f, 1.0f, obj[loop].flap);	//	   |  /
			glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0f, 0.0f);				//	(3)|/

			// Second Triangle
			glTexCoord2f(1.0f,1.0f); glVertex3f( 1.0f, 1.0f, 0.0f);				//	       /|(1)
			glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0f, 0.0f);				//	     /  |
			glTexCoord2f(1.0f,0.0f); glVertex3f( 1.0f,-1.0f, obj[loop].flap);	//	(2)/____|(3)

		glEnd();												// Done Drawing Triangles

		obj[loop].y-=obj[loop].yi;								// Move Object Down The Screen
		obj[loop].spinz+=obj[loop].spinzi;						// Increase Z Rotation By spinzi
		obj[loop].flap+=obj[loop].fi;							// Increase flap Value By fi

		if (obj[loop].y<-18.0f)									// Is Object Off The Screen?
		{
			SetObject(loop);									// If So, Reassign New Values
		}

		if ((obj[loop].flap>1.0f) || (obj[loop].flap<-1.0f))	// Time To Change Flap Direction?
		{
			obj[loop].fi=-obj[loop].fi;							// Change Direction By Making fi = -fi
		}
	}

	Sleep(15);													// Create A Short Delay (15 Milliseconds)

	glFlush ();

}

INT_PTR CALLBACK AboutProc(HWND hdlg, UINT msg, WPARAM wpm, LPARAM lpm){

	switch(msg){
	case WM_CTLCOLORSTATIC:
		if(((HWND)lpm == GetDlgItem(hdlg, WEBPAGE1)) || ((HWND)lpm == GetDlgItem(hdlg, WEBPAGE2)))
		{
			SetTextColor((HDC)wpm, RGB(0,0,255));
			SetBkColor((HDC)wpm, (COLORREF)GetSysColor(COLOR_3DFACE));
			return (INT_PTR)GetSysColorBrush(COLOR_3DFACE);
		}
		break;
    case WM_COMMAND:
		switch(LOWORD(wpm)){
		case IDOK:
			EndDialog(hdlg, LOWORD(wpm));
			break;
		case WEBPAGE1:
			ShellExecute(NULL, _T("open"), _T("http://nehe.gamedev.net"), NULL, NULL, SW_SHOWNORMAL);
			break;
		case WEBPAGE2:
			ShellExecute(NULL, _T("open"), _T("http://www.thaputer.com"), NULL, NULL, SW_SHOWNORMAL);
			break;
		}
	}
	return FALSE;
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message,
					 WPARAM wParam, LPARAM lParam)
{
	static HGLRC hRC;
	static DWORD timer = 1;
	HDC hDC;
    RECT WindowRect;
	int width;
	int height;

	switch (message)
	{
	case WM_CREATE:
		ReadRegistry();
		hRC = InitOGLWindow(hWnd);
		GetClientRect (hWnd, &WindowRect);
		width = WindowRect.right - WindowRect.left;
		height = WindowRect.bottom - WindowRect.top;
		InitOpenGL(width,height);
		SetTimer(hWnd, timer, 5, NULL);
		break;
	case WM_TIMER:
		hDC = GetDC(hWnd);
		Display();
		SwapBuffers(hDC);
		ReleaseDC(hWnd, hDC);
		break;
	case WM_DESTROY:
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hRC);
		break;
	}

	return DefScreenSaverProc(hWnd, message, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message,
								WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
        case WM_INITDIALOG:
	        ReadRegistry();
	        CheckDlgButton(hDlg, ROTATE, dRotate);
	        return TRUE;
	    case WM_COMMAND:
		    switch (LOWORD(wParam))
		    {
		        case IDOK:
			        dRotate = (IsDlgButtonChecked(hDlg, ROTATE) == BST_CHECKED);
			        WriteRegistry();
			        EndDialog(hDlg, TRUE);
			        return TRUE;
		        case IDCANCEL:
			        EndDialog(hDlg, TRUE);
			        break;
		        case IDABOUT:
			        DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_ABOUT), hDlg, AboutProc);
                    break;
		    }
	}

	return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}

