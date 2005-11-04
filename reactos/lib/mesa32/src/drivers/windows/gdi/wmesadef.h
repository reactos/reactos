
#include "context.h"

typedef struct _dibSection{
    HDC		hDC;
    HANDLE	hFileMap;
    BOOL	fFlushed;
    LPVOID	base;
}WMDIBSECTION, *PWMDIBSECTION;

typedef struct wmesa_context{
    GLcontext           *gl_ctx;	/* The core GL/Mesa context */
    GLvisual            *gl_visual;	/* Describes the buffers */
    GLframebuffer       *gl_buffer;	/* Depth, stencil, accum, etc buffers*/
    
    HWND		Window;
    HDC                 hDC;
    COLORREF		clearColorRef;
    HPEN                clearPen;
    HBRUSH              clearBrush;
    GLuint		width;
    GLuint		height;
    GLuint		ScanWidth;
    GLboolean		db_flag;
    WMDIBSECTION	dib;
    BITMAPINFO          bmi;
    HBITMAP             hbmDIB;
    HBITMAP             hOldBitmap;
    PBYTE               pbPixels;
    BYTE		cColorBits;
    int			pixelformat;
}  *PWMC;


