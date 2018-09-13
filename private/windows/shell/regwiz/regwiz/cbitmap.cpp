/*********************************************************************
Registration Wizard
Class: CBitmap

--- This class subclasses a Window control to create a control that
displays a bitmap

11/16/94 - Tracy Ferrier
04/15/97 - Modified to take care of crashing in Memphis as the default destoy was not handled 
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#include <Windows.h>
#include <stdio.h>
#include "cbitmap.h"
#include "Resource.h"
#include "assert.h"

static HBITMAP BitmapFromDib (
    LPVOID         pDIB,
    HPALETTE   hpal, WORD wPalSize);


LRESULT PASCAL BitmapWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

CBitmap::CBitmap(HINSTANCE hInstance, HWND hwndDlg,int idDlgCtl, int idBitmap)
/*********************************************************************
Constructor for our CBitmap class.  
**********************************************************************/
{
	m_hInstance = hInstance;
	m_nIdBitmap = idBitmap;
	m_hPal = NULL;
	m_hBitmap =  GetBmp(); //LoadBitmap(hInstance,MAKEINTRESOURCE(m_nIdBitmap));
	assert(m_hBitmap != NULL);

	HWND hwndCtl = GetDlgItem(hwndDlg,idDlgCtl);
	m_lpfnOrigWndProc = (FARPROC) GetWindowLongPtr(hwndCtl,GWLP_WNDPROC);
	assert(m_lpfnOrigWndProc != NULL);
	m_isActivePal = TRUE;
	SetWindowLongPtr(hwndCtl,GWLP_WNDPROC,(LONG_PTR) BitmapWndProc);
	SetWindowLongPtr(hwndCtl,GWLP_USERDATA,(LONG_PTR) this);
}


CBitmap::~CBitmap()
/*********************************************************************
Destructor for our CBitmap class
**********************************************************************/
{

	
	if (m_hBitmap) 
		DeleteObject(m_hBitmap);
	if( m_hPal ) 
		DeleteObject(m_hPal);

}

/* *****************************************************************
Create a 256 Color Bitmap
********************************************************************/

HBITMAP CBitmap::GetBmp()
{
       RECT rect;
       HDC  hDC;
       BOOL bRet;
 
       // detect this display is 256 colors or not
       hDC = GetDC(NULL);
       bRet = (GetDeviceCaps(hDC, BITSPIXEL) != 8);
       ReleaseDC(NULL, hDC);
       if (bRet) 
	   {                             
		   // the display is not 256 colors, let Windows handle it
          return LoadBitmap(m_hInstance,MAKEINTRESOURCE(m_nIdBitmap));
       }
 

       LPBITMAPINFO lpBmpInfo;               // bitmap informaiton
       int i;
       HRSRC hRsrc;
	   HANDLE hDib;
	   HBITMAP hBMP;
	   HPALETTE hPal;
       struct {
			   WORD            palVersion;
		       WORD            palNumEntries;
			   PALETTEENTRY    PalEntry[256];
	   } MyPal;
               
       hRsrc = FindResource(m_hInstance, MAKEINTRESOURCE(m_nIdBitmap),RT_BITMAP);
       if (!hRsrc)
         return NULL;
 
       hDib = LoadResource(m_hInstance, hRsrc);
       if (!hDib)
         return NULL;
 
       if (!(lpBmpInfo = (LPBITMAPINFO) LockResource(hDib)))
               return NULL;
                               
       MyPal.palVersion = 0x300;
       MyPal.palNumEntries = 1 << lpBmpInfo->bmiHeader.biBitCount;
 
       for (i = 0; i < MyPal.palNumEntries; i++) 
	   {
         MyPal.PalEntry[i].peRed   = lpBmpInfo->bmiColors[i].rgbRed;
         MyPal.PalEntry[i].peGreen = lpBmpInfo->bmiColors[i].rgbGreen;
         MyPal.PalEntry[i].peBlue  = lpBmpInfo->bmiColors[i].rgbBlue;
         MyPal.PalEntry[i].peFlags = 0;
       }
       m_hPal = CreatePalette((LPLOGPALETTE)&MyPal);

       if (m_hPal == NULL) 
	   {        // create palette fail, let window handle the bitmap
          return LoadBitmap(m_hInstance,MAKEINTRESOURCE(m_nIdBitmap));          
       }
       
       hBMP = BitmapFromDib(hDib,m_hPal,MyPal.palNumEntries);
       UnlockResource(hDib);
	   if( hBMP == NULL ) {
		   DeleteObject(m_hPal);
		   m_hPal = NULL;
		   hBMP = LoadBitmap(m_hInstance,MAKEINTRESOURCE(m_nIdBitmap));
       }
	   //DeleteObject(hPal);
	   return hBMP;
}


LRESULT PASCAL CBitmap::CtlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
/*********************************************************************
**********************************************************************/
{
	switch (message)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HPALETTE hpalT;
			HDC hdc = BeginPaint(hwnd,&ps);
			HDC hMemDC = CreateCompatibleDC(hdc);
			SelectObject(hMemDC,m_hBitmap);
			RECT wndRect;
			GetClientRect(hwnd,&wndRect);
		    if (m_hPal){
				hpalT = SelectPalette(hdc,m_hPal,FALSE);
				RealizePalette(hdc);     
			}


			BitBlt(hdc,0,0,wndRect.right - wndRect.left,wndRect.bottom - wndRect.top,hMemDC,0,0,SRCCOPY);
			if( m_hPal ) 
				SelectPalette(hdc,hpalT,FALSE);

			DeleteDC(hMemDC);
			EndPaint(hwnd,&ps);
#ifdef _WIN95
			return CallWindowProc(m_lpfnOrigWndProc,hwnd,message,wParam,lParam);
#else
			return CallWindowProc((WNDPROC) m_lpfnOrigWndProc,hwnd,message,wParam,lParam);
#endif
			

			break;
		}
		case WM_QUERYNEWPALETTE :
				if(m_hPal && !m_isActivePal) 
					InvalidateRect(hwnd,NULL,FALSE);
				return 0;//CallWindowProc(m_lpfnOrigWndProc,hwnd,message,wParam,lParam);


		case WM_PALETTECHANGED :
			if( (HWND)wParam != hwnd ) {
				if(m_hPal ) {
					m_isActivePal = FALSE;
					InvalidateRect(hwnd,NULL,FALSE);
				}
			}
			else m_isActivePal = TRUE;
			return 0; //CallWindowProc(m_lpfnOrigWndProc,hwnd,message,wParam,lParam);

		case WM_DESTROY:
			SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR) m_lpfnOrigWndProc);

		default:
#ifdef _WIN95
			return CallWindowProc(m_lpfnOrigWndProc,hwnd,message,wParam,lParam);
#else
			return CallWindowProc((WNDPROC) m_lpfnOrigWndProc,hwnd,message,wParam,lParam);
#endif

			

			break;
	}
	return 0;
}


LRESULT PASCAL BitmapWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
/*********************************************************************
**********************************************************************/
{
	CBitmap* pclBitMap = (CBitmap*) GetWindowLongPtr(hwnd,GWLP_USERDATA);
	LRESULT lret;
		
	switch (message)
	{
		case WM_DESTROY:

			//return 
			lret = pclBitMap->CtlWndProc(hwnd,message,wParam,lParam);
			delete pclBitMap;
			return lret;
			// fall through
		default:
			lret = pclBitMap->CtlWndProc(hwnd,message,wParam,lParam);
			return lret ;
			break;
	}
}


/****************************************************************************
 *                                                                          *
 *  FUNCTION   : BitmapFromDib(LPVOID hdib, HPALETTE hpal, WORD palSize)                  *
 *                                                                          *
 *  PURPOSE    : Will create a DDB (Device Dependent Bitmap) given a global *
 *               handle to a memory block in CF_DIB format                  *
 *                                                                          *
 *  RETURNS    : A handle to the DDB.                                       *
 *                                                                          *
 ****************************************************************************/

static HBITMAP BitmapFromDib (
    LPVOID         pDIB,
    HPALETTE   hpal, WORD wPalSize)
{
    LPBITMAPINFOHEADER  lpbi;
    HPALETTE            hpalT;
    HDC                 hdc;
    HBITMAP             hbm;

   

    if (!pDIB || wPalSize == 16 )
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)pDIB; // lock resource


    hdc = GetDC(NULL);

    if (hpal){
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);     
    }

    hbm = CreateDIBitmap(hdc,
                (LPBITMAPINFOHEADER)lpbi,
                (LONG)CBM_INIT,
                (LPSTR)lpbi + lpbi->biSize + wPalSize*sizeof(PALETTEENTRY),
                (LPBITMAPINFO)lpbi,
                DIB_RGB_COLORS );

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    ReleaseDC(NULL,hdc);

    return hbm;
}





 
 

