#include "stdafx.h"

namespace MSTSCLib
{
#include "mstsclib_h.h"
};

namespace MSTSCLib_Redist
{
// extremely ew, but actually the cleanest way to import the alternate UUIDs
#include "mstsclib_redist_i.c"
};

#include "rdesktop/rdesktop.h"
#include "rdesktop/proto.h"

namespace
{
#ifdef _MSC_VER
extern "C" char __ImageBase;
#endif

HMODULE GetCurrentModule()
{
	return reinterpret_cast<HMODULE>(&__ImageBase);
}

}

namespace
{

LONG g_moduleRefCount = 0;

void lockServer()
{
	InterlockedIncrement(&g_moduleRefCount);
}

void unlockServer()
{
	InterlockedDecrement(&g_moduleRefCount);
}

bool canUnloadServer()
{
	return g_moduleRefCount == 0;
}

}

namespace
{

void FreeLpsz(LPSTR lpsz)
{
	if(lpsz)
		delete[] lpsz;
}

LPSTR AllocLpsz(const CHAR * lpsz, size_t cb)
{
	LPSTR lpszNew = new CHAR[cb + 1];

	if(lpszNew == NULL)
		return NULL;

	CopyMemory(lpszNew, lpsz, cb);
	lpszNew[cb] = 0;

	return lpszNew;
}

LPSTR AllocLpsz(const WCHAR * lpwsz, int cchIn)
{
	int cch = WideCharToMultiByte(CP_ACP, 0, lpwsz, cchIn, NULL, 0, NULL, NULL);

	if(cch <= 0)
		return NULL;

	LPSTR lpsz = new CHAR[cch];

	if(lpsz == NULL)
		return NULL;

	cch = WideCharToMultiByte(CP_ACP, 0, lpwsz, cchIn, lpsz, cch, NULL, NULL);

	if(cch <= 0)
	{
		FreeLpsz(lpsz);
		return NULL;
	}

	return lpsz;
}

LPSTR BstrToLpsz(BSTR bstr)
{
	return AllocLpsz(bstr, SysStringLen(bstr));
}

BSTR LpszToBstr(LPSTR lpsz)
{
	int cch = MultiByteToWideChar(CP_ACP, 0, lpsz, -1, NULL, 0);

	if(cch <= 0)
		return NULL;

	BSTR bstr = SysAllocStringLen(NULL, cch);

	if(bstr == NULL)
		return NULL;

	cch = MultiByteToWideChar(CP_ACP, 0, lpsz, -1, bstr, cch);

	if(cch <= 0)
	{
		SysFreeString(bstr);
		return NULL;
	}

	return bstr;
}

}

namespace
{

template<class T, class U> T aligndown(const T& X, const U& align)
{
	return X & ~(T(align) - 1);
}

template<class T, class U> T alignup(const T& X, const U& align)
{
	return aligndown(X + (align - 1), align);
}

/* Convert between bitmap formats */
uint8 * win32_convert_scanlines(int width, int height, int bitcount, int fromalign, int toalign, const uint8 * data, uint8 ** buffer)
{
	// TBD: profile & optimize the most common cases
	assert(width > 0);
	assert(height);
	assert(bitcount && bitcount <= 32);
	assert(fromalign <= toalign);
	assert(data);
	assert(buffer);

	bool flipped = height < 0;

	if(flipped)
		height = - height;

	int bytesperrow = alignup(width * bitcount, 8) / 8;
	int fromstride = alignup(bytesperrow, fromalign);
	int tostride = alignup(bytesperrow, toalign);
	assert(fromstride <= tostride);

	int datasize = tostride * height;

	uint8 * dibits = new uint8[datasize];

	const uint8 * src = data;
	uint8 * dest = dibits;

	const int pad = tostride - fromstride;

	assert(pad < 4);
	__assume(pad < 4);

	if(flipped)
	{
		dest += (height - 1) * tostride;
		tostride = - tostride;
	}

	for(int i = 0; i < height; ++ i)
	{
		memcpy(dest, src, fromstride);
		memset(dest + fromstride, 0, pad);
		src += fromstride;
		dest += tostride;
	}

	*buffer = dibits;
	return dibits;
}

/* Creates bitmaps */
HBITMAP win32_create_dib(LONG width, LONG height, WORD bitcount, const BYTE * data)
{
	struct b_
	{
		BITMAPINFO bmi;
		RGBQUAD colormap[256 - ARRAYSIZE(RTL_FIELD_TYPE(BITMAPINFO, bmiColors))];
	}
	b;

	b.bmi.bmiHeader.biSize = sizeof(b.bmi.bmiHeader);
	b.bmi.bmiHeader.biWidth = width;
	b.bmi.bmiHeader.biHeight = height;
	b.bmi.bmiHeader.biPlanes = 1;
	b.bmi.bmiHeader.biBitCount = bitcount;
	b.bmi.bmiHeader.biCompression = BI_RGB;
	b.bmi.bmiHeader.biSizeImage = 0;
	b.bmi.bmiHeader.biXPelsPerMeter = 0;
	b.bmi.bmiHeader.biYPelsPerMeter = 0;

	if(bitcount > 8)
	{
		b.bmi.bmiHeader.biClrUsed = 0;
		b.bmi.bmiHeader.biClrImportant = 0;
	}
	else
	{
		b.bmi.bmiHeader.biClrUsed = 2 << bitcount;
		b.bmi.bmiHeader.biClrImportant = 2 << bitcount;

		// TODO: palette
	}

	// FIXME: beyond ugly
	HDC hdc = CreateCompatibleDC(NULL);

	if(hdc == NULL)
		return NULL;

	HBITMAP hbm = CreateDIBitmap(hdc, &b.bmi.bmiHeader, CBM_INIT, data, &b.bmi, DIB_RGB_COLORS);

	if(hbm == NULL)
		error("CreateDIBitmap %dx%dx%d failed\n", width, height, bitcount);

	DeleteDC(hdc);
	return hbm;
}

/* Creates brushes */
HBRUSH win32_create_brush(BRUSH * brush, COLORREF fgcolour)
{
	if(brush == NULL)
		return (HBRUSH)GetStockObject(NULL_BRUSH);

	switch(brush->style)
	{
	case BS_SOLID:
	case BS_NULL:
	case BS_HATCHED:
	case BS_PATTERN:
	case BS_PATTERN8X8:
		break;

	default:
		return NULL;
	}

	switch(brush->style)
	{
	case BS_SOLID:
		return CreateSolidBrush(fgcolour);

	case BS_HATCHED:
		return CreateHatchBrush(brush->pattern[0], fgcolour);

	case BS_NULL:
		return (HBRUSH)GetStockObject(NULL_BRUSH);

	case BS_PATTERN:
	case BS_PATTERN8X8:
		{
			uint16 pattern[8];

			for(size_t i = 0; i < 8; ++ i)
				pattern[i] = brush->pattern[i];

			HBITMAP hpattern = CreateBitmap(8, 8, 1, 1, pattern);
			HBRUSH hbr = CreatePatternBrush(hpattern);
			DeleteObject(hpattern);
			return hbr;
		}

	DEFAULT_UNREACHABLE;
	}
}
};

/*
	"sealed" can improve optimizations by asserting a class cannot be derived
	from, optimizing out accesses to the v-table from inside the class
*/
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define SEALED_ sealed
#else
#define SEALED_
#endif


/* Class that implements the RDP client GUI */
class RdpClientUI
{
public:
	// TODO: pass the client settings relevant to the GUI here
	HRESULT Initialize(HWND hwndParent)
	{
		// TODO: create the various windows
		// TODO: create display window thread
		// TODO: create input thread
		return E_FAIL;
	}

public:
	static BOOL Startup()
	{
		WNDCLASSEX wcexUI = { sizeof(wcexUI) };
		WNDCLASSEX wcexConsole = { sizeof(wcexConsole) };
		WNDCLASSEX wcexDisplay = { sizeof(wcexDisplay) };
		WNDCLASSEX wcexInput = { sizeof(wcexInput) };

		HBRUSH nullBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

		wcexUI.lpfnWndProc = NULL; // TODO
		wcexUI.hInstance = GetCurrentModule();
		wcexUI.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcexUI.hbrBackground = nullBrush;
		wcexUI.lpszClassName = TEXT("MissTosca_UI");

		wcexConsole.style = CS_VREDRAW | CS_HREDRAW;
		wcexConsole.lpfnWndProc = NULL; // TODO
		wcexConsole.hInstance = GetCurrentModule();
		wcexConsole.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcexConsole.hbrBackground = nullBrush;
		wcexConsole.lpszClassName = TEXT("MissTosca_Console");

		wcexDisplay.style = CS_VREDRAW | CS_HREDRAW;
		wcexDisplay.lpfnWndProc = NULL; // TODO
		wcexDisplay.hInstance = GetCurrentModule();
		wcexDisplay.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcexDisplay.hbrBackground = nullBrush;
		wcexDisplay.lpszClassName = TEXT("MissTosca_Display");

		wcexInput.style = CS_VREDRAW | CS_HREDRAW;
		wcexInput.lpfnWndProc = NULL; // TODO
		wcexInput.hInstance = GetCurrentModule();
		wcexInput.hCursor = NULL;
		wcexInput.hbrBackground = nullBrush;
		wcexInput.lpszClassName = TEXT("MissTosca_Input");

		return
			RegisterClassEx(&wcexUI) &&
			RegisterClassEx(&wcexConsole) &&
			RegisterClassEx(&wcexDisplay) &&
			RegisterClassEx(&wcexInput);
	}

	static void Shutdown()
	{
		// TODO
	}

	/*
		This is the main UI window. It's the direct child of the control
		window, it fills its whole extent and it contains the scrollbars.
		When activated, it will move keyboard focus to the input window
	*/
private:
	HWND m_uiWindow;
	LONG m_scrollHPos;
	LONG m_scrollVPos;

	LRESULT UIWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			// Keep the keyboard focus on the input window
		case WM_ACTIVATE:
			switch(LOWORD(wParam))
			{
			case WA_INACTIVE:
				break;

			case WA_ACTIVE:
			case WA_CLICKACTIVE:
				if(!HIWORD(wParam))
					SetFocus(m_inputWindow);
			}

			return 0;

			// Resized: rearrange children windows, adjust scrollbars
		case WM_SIZE:
			{
				if(IsIconic(m_uiWindow))
					break;

				RECT rcClient;
				GetWindowRect(m_uiWindow, &rcClient);

				if(m_smartSizing)
				{
					// we are not supposed to maintain aspect ratio. Container has to do that
					m_consoleX = 0;
					m_consoleY = 0;
					m_consoleWidth = rcClient.right;
					m_consoleHeight = rcClient.bottom;
				}
				else
				{
					// center horizontally, no horizontal scrollbar
					if(rcClient.right >= m_consoleWidth)
						m_consoleX = (m_consoleWidth - rcClient.right) / 2;

					// center vertically, no vertical scrollbar
					if(rcClient.bottom >= m_consoleHeight)
						m_consoleY = (m_consoleHeight - rcClient.right) / 2;
				}

				SCROLLINFO scroll = { sizeof(scroll), SIF_ALL, 0 };

				// update the horizontal scrollbar
				scroll.nMax = m_consoleWidth;
				scroll.nPage = rcClient.right;
				scroll.nPos = 0 - m_consoleX;
				SetScrollInfo(m_uiWindow, SB_HORZ, &scroll, TRUE);

				// update the vertical scrollbar
				scroll.nMax = m_consoleHeight;
				scroll.nPage = rcClient.bottom;
				scroll.nPos = 0 - m_consoleY;
				SetScrollInfo(m_uiWindow, SB_VERT, &scroll, TRUE);

				// move/resize the console window
				MoveWindow(m_consoleWindow, m_consoleX, m_consoleY, m_consoleWidth, m_consoleHeight, TRUE);
			}

			return 0;

		case WM_HSCROLL:
			{
				SCROLLINFO scroll = { sizeof(scroll), SIF_TRACKPOS };
				GetScrollInfo(m_uiWindow, SB_HORZ, &scroll);
				m_consoleX = - scroll.nTrackPos;
				MoveWindow(m_consoleWindow, m_consoleX, m_consoleY, m_consoleWidth, m_consoleHeight, TRUE);
			}

			return 0;

		case WM_VSCROLL:
			{
				SCROLLINFO scroll = { sizeof(scroll), SIF_TRACKPOS };
				GetScrollInfo(m_uiWindow, SB_VERT, &scroll);
				m_consoleY = - scroll.nTrackPos;
				MoveWindow(m_consoleWindow, m_consoleX, m_consoleY, m_consoleWidth, m_consoleHeight, TRUE);
			}

			return 0;

		default:
			break;
		}

		return DefWindowProc(m_uiWindow, uMsg, wParam, lParam);
	}

	/*
		This is the full-screen title bar. It's displayed at the top of the
		main UI window while in full-screen mode, and it contains two toolbars
		with the pin, minimize, restore and close buttons
	*/
	HWND m_fullScreenBarWindow;

	/*
		This is the console window. It has the same extent as the display on
		the remote computer, or it fills the UI window in smart resizing mode,
		and it contains the input and display windows
	*/
private:
	HWND m_consoleWindow;
	LONG m_consoleX;
	LONG m_consoleY;
	LONG m_consoleWidth;
	LONG m_consoleHeight;
	bool m_smartSizing;

	LRESULT ConsoleWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_SIZE:
			{
				RECT rcClient;
				GetClientRect(m_consoleWindow, &rcClient);

				MoveWindow(m_inputWindow, 0, 0, rcClient.right, rcClient.bottom, TRUE);
				MoveWindow(m_displayWindow, 0, 0, rcClient.right, rcClient.bottom, TRUE);
			}

			return 0;

		default:
			break;
		}

		return DefWindowProc(m_consoleWindow, uMsg, wParam, lParam);
	}

	/*
		This is the display window. It represents the virtual display of the
		remote computer. It completely fills its parent, the console window,
		and it runs in its own thread for performance reasons
	*/
private:
	HWND m_displayWindow;
	LONG m_displayBufferWidth;
	LONG m_displayBufferHeight;
	HDC m_displayBuffer;
	void * m_displayBufferRaw;
	int m_displayBufferSave;
	int m_displayBufferBitDepth;
	int m_displayBufferByteDepth;
	int m_displayBufferStride;
	RECT m_displayBufferClip;
	CRITICAL_SECTION m_displayBufferMutex;

	LRESULT DisplayWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_PRINTCLIENT:
			if(wParam == 0)
				break;

		case WM_PAINT:
			{
				HDC hdc = (HDC)wParam;

				EnterCriticalSection(&m_displayBufferMutex);

				if(hdc)
				{
					RECT rc;
					GetClientRect(m_displayWindow, &rc);
					BitBlt(hdc, 0, 0, rc.right, rc.bottom, m_displayBuffer, 0, 0, SRCCOPY);
				}
				else
				{
					PAINTSTRUCT ps;
					hdc = BeginPaint(m_displayWindow, &ps);

					if(!m_smartSizing)
					{
						BitBlt
						(
							hdc,
							ps.rcPaint.left,
							ps.rcPaint.top,
							ps.rcPaint.right - ps.rcPaint.left,
							ps.rcPaint.bottom - ps.rcPaint.top,
							m_displayBuffer,
							ps.rcPaint.left,
							ps.rcPaint.top,
							SRCCOPY
						);
					}
					else
					{
						// bleh. There has to be a better way
						SetStretchBltMode(hdc, HALFTONE);

						StretchBlt
						(
							hdc,
							0,
							0,
							m_consoleWidth,
							m_consoleHeight,
							m_displayBuffer,
							0,
							0,
							m_displayBufferWidth,
							m_displayBufferHeight,
							SRCCOPY
						);
					}

					EndPaint(m_displayWindow, &ps);
				}

				LeaveCriticalSection(&m_displayBufferMutex);
			}

			return 0;

		default:
			break;
		}

		return DefWindowProc(m_displayWindow, uMsg, wParam, lParam);
	}

	/* Screen repainting */
	void Display_RepaintRect(const RECT * lprc)
	{
		if(m_smartSizing)
			return Display_RepaintAll();

		RECT rcDamage;
		IntersectRect(&rcDamage, lprc, &m_displayBufferClip);
		InvalidateRect(m_displayWindow, &rcDamage, FALSE);
	}

	void Display_RepaintArea(int x, int y, int cx, int cy)
	{
		if(m_smartSizing)
			return Display_RepaintAll();

		RECT rcDamage;
		rcDamage.left = x;
		rcDamage.top = y;
		rcDamage.right = x + cx;
		rcDamage.bottom = y + cy;
		Display_RepaintRect(&rcDamage);
	}

	void Display_RepaintPolygon(POINT * point, int npoints, int linewidth)
	{
		if(m_smartSizing)
			return Display_RepaintAll();

		RECT rcDamage;

		rcDamage.left = MAXLONG;
		rcDamage.top = MAXLONG;
		rcDamage.right = 0;
		rcDamage.bottom = 0;

		for(int i = 0; i < npoints; ++ i)
		{
			if(point[i].x < rcDamage.left)
				rcDamage.left = point[i].x;

			if(point[i].y < rcDamage.top)
				rcDamage.top = point[i].y;

			if(point[i].x > rcDamage.right)
				rcDamage.right = point[i].x;

			if(point[i].y > rcDamage.bottom)
				rcDamage.bottom = point[i].y;
		}

		InflateRect(&rcDamage, linewidth, linewidth);
		Display_RepaintRect(&rcDamage);
	}

	void Display_RepaintAll()
	{
		InvalidateRgn(m_displayWindow, NULL, FALSE);
	}

public:
	void Display_SetClip(int x, int y, int cx, int cy)
	{
		m_displayBufferClip.left = x;
		m_displayBufferClip.top = y;
		m_displayBufferClip.right = x + cx + 1;
		m_displayBufferClip.bottom = y + cy + 1;

		HRGN hrgn = CreateRectRgnIndirect(&m_displayBufferClip);
		SelectClipRgn(m_displayBuffer, hrgn);
		DeleteObject(hrgn);
	}

	void Display_ResetClip()
	{
		m_displayBufferClip.left = 0;
		m_displayBufferClip.top = 0;
		m_displayBufferClip.right = m_displayBufferWidth;
		m_displayBufferClip.bottom = m_displayBufferHeight;
		SelectClipRgn(m_displayBuffer, NULL);
	}

	void Display_PaintBitmap(int x, int y, int cx, int cy, int width, int height, uint8 * data)
	{
		GdiFlush();

		int fromstride = alignup(width * m_displayBufferByteDepth, 4);
		int sizex = cx * m_displayBufferByteDepth;

		const uint8 * src = data;

		uint8 * dst =
			(uint8 *)m_displayBufferRaw +
			(m_displayBufferHeight - y - cy) * m_displayBufferStride +
			x * m_displayBufferByteDepth;

		for(int i = 0; i < cy; ++ i)
		{
			memcpy(dst, src, sizex);
			src += fromstride;
			dst += m_displayBufferStride;
		}

		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_DestBlt(uint8 opcode, int x, int y, int cx, int cy)
	{
		int dcsave = SaveDC(m_displayBuffer);
		SelectObject(m_displayBuffer, GetStockObject(BLACK_BRUSH));
		PatBlt(m_displayBuffer, x, y, cx, cy, MAKELONG(0, opcode));
		RestoreDC(m_displayBuffer, dcsave);
		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_PatBlt(uint8 opcode, int x, int y, int cx, int cy, BRUSH * brush, int bgcolour, int fgcolour)
	{
		HBRUSH hbr = win32_create_brush(brush, fgcolour);

		int dcsave = SaveDC(m_displayBuffer);

		SetBkColor(m_displayBuffer, bgcolour);
		SetTextColor(m_displayBuffer, fgcolour);
		SetBrushOrgEx(m_displayBuffer, brush->xorigin, brush->yorigin, NULL);
		SelectObject(m_displayBuffer, hbr);

		PatBlt(m_displayBuffer, x, y, cx, cy, MAKELONG(0, opcode));

		RestoreDC(m_displayBuffer, dcsave);

		DeleteObject(hbr);

		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_ScreenBlt(uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
	{
		BitBlt(m_displayBuffer, x, y, cx, cy, m_displayBuffer, srcx, srcy, MAKELONG(0, opcode));
		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_MemBlt(uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, int srcx, int srcy)
	{
		HDC hdcSrc = CreateCompatibleDC(m_displayBuffer);
		HGDIOBJ hOld = SelectObject(hdcSrc, src);

		BitBlt(m_displayBuffer, x, y, cx, cy, hdcSrc, srcx, srcy, MAKELONG(0, opcode));

		SelectObject(hdcSrc, hOld);
		DeleteDC(hdcSrc);

		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_TriBlt(uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, int srcx, int srcy, BRUSH * brush, int bgcolour, int fgcolour)
	{
		// TODO
		HDC hdcSrc = CreateCompatibleDC(m_displayBuffer);
		HGDIOBJ hOld = SelectObject(hdcSrc, src);

		//SELECT_BRUSH(brush, bgcolour, fgcolour);

		BitBlt(m_displayBuffer, x, y, cx, cy, hdcSrc, srcx, srcy, MAKELONG(0, opcode));

		//RESET_BRUSH();

		SelectObject(hdcSrc, hOld);
		DeleteDC(hdcSrc);

		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_Line(uint8 opcode, int startx, int starty, int endx, int endy, PEN * pen)
	{
		HPEN hpen = CreatePen(pen->style, pen->width, pen->colour);

		int dcsave = SaveDC(m_displayBuffer);

		SetROP2(m_displayBuffer, opcode);
		SelectObject(m_displayBuffer, hpen);
		MoveToEx(m_displayBuffer, startx, starty, NULL);

		LineTo(m_displayBuffer, endx, endy);

		RestoreDC(m_displayBuffer, dcsave);

		DeleteObject(hpen);

		RECT rcDamage;

		if(startx < endx)
		{
			rcDamage.left = startx;
			rcDamage.right = endx;
		}
		else
		{
			rcDamage.left = endx;
			rcDamage.right = startx;
		}

		if(starty < endy)
		{
			rcDamage.top = starty;
			rcDamage.bottom = endy;
		}
		else
		{
			rcDamage.top = endy;
			rcDamage.bottom = starty;
		}

		InflateRect(&rcDamage, pen->width, pen->width);
		Display_RepaintRect(&rcDamage);
	}

	void Display_Rect(int x, int y, int cx, int cy, int colour)
	{
		HBRUSH hbr = CreateSolidBrush(colour);

		int dcsave = SaveDC(m_displayBuffer);

		SelectObject(m_displayBuffer, hbr);
		SelectObject(m_displayBuffer, GetStockObject(NULL_PEN));

		Rectangle(m_displayBuffer, x, y, x + cx + 1, y + cy + 1);

		RestoreDC(m_displayBuffer, dcsave);

		DeleteObject(hbr);

		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_Polygon(uint8 opcode, uint8 fillmode, POINT * point, int npoints, BRUSH * brush, int bgcolour, int fgcolour)
	{
		HBRUSH hbr = win32_create_brush(brush, fgcolour);

		int dcsave = SaveDC(m_displayBuffer);

		SetBkColor(m_displayBuffer, bgcolour);
		SetTextColor(m_displayBuffer, fgcolour);
		SetPolyFillMode(m_displayBuffer, fillmode);
		SelectObject(m_displayBuffer, hbr);

		Polygon(m_displayBuffer, point, npoints);

		RestoreDC(m_displayBuffer, dcsave);

		Display_RepaintPolygon(point, npoints, 0);
	}

	void Display_Polyline(uint8 opcode, POINT * points, int npoints, PEN * pen)
	{
		POINT last = points[0];

		for(int i = 1; i < npoints; ++ i)
		{
			points[i].x += last.x;
			points[i].y += last.y;
			last = points[i];
		}

		HPEN hpen = CreatePen(pen->style, pen->width, pen->colour);

		int dcsave = SaveDC(m_displayBuffer);

		SetROP2(m_displayBuffer, opcode);
		SelectObject(m_displayBuffer, hpen);

		Polyline(m_displayBuffer, points, npoints);

		RestoreDC(m_displayBuffer, dcsave);

		DeleteObject(hpen);

		Display_RepaintPolygon(points, npoints, pen->width);
	}

	void Display_Ellypse(uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, BRUSH * brush, int bgcolour, int fgcolour)
	{
		// TODO

		Display_RepaintArea(x, y, cx, cy);
	}

	// TBD: optimize text drawing
	void Display_DrawGlyph(int mixmode, int x, int y, int cx, int cy, HGLYPH glyph, int srcx, int srcy, int bgcolour, int fgcolour)
	{
		HBITMAP hbmGlyph = (HBITMAP)glyph;
		HDC hdcGlyph = CreateCompatibleDC(m_displayBuffer);
		HGDIOBJ hOld = SelectObject(hdcGlyph, hbmGlyph);

		int dcsave = SaveDC(m_displayBuffer);

		switch(mixmode)
		{
		case MIX_TRANSPARENT:
			{
				/*
					ROP is DSPDxax:
					 - where the glyph (S) is white, D is set to the foreground color (P)
					 - where the glyph (S) is black, D is left untouched

					This paints a transparent glyph in the specified color
				*/
				HBRUSH hbr = CreateSolidBrush(fgcolour);
				SelectObject(m_displayBuffer, hbr);
				BitBlt(m_displayBuffer, x, y, cx, cy, hdcGlyph, srcx, srcy, MAKELONG(0, 0xe2));
				DeleteObject(hbr);
			}

			break;

		case MIX_OPAQUE:
			{
				/* Curiously, glyphs are inverted (white-on-black) */
				SetBkColor(m_displayBuffer, fgcolour);
				SetTextColor(m_displayBuffer, bgcolour);
				BitBlt(m_displayBuffer, x, y, cx, cy, hdcGlyph, srcx, srcy, SRCCOPY);
			}

			break;
		}

		RestoreDC(m_displayBuffer, dcsave);

		SelectObject(hdcGlyph, hOld);
		DeleteDC(hdcGlyph);

		Display_RepaintArea(x, y, cx, cy);
	}

	void Display_DoGlyph(uint8 font, uint8 flags, int mixmode, int& x, int& y, int bgcolour, int fgcolour, const uint8 * ttext, int& idx)
	{
		FONTGLYPH * glyph;

		glyph = cache_get_font(/*This*/NULL, font, ttext[idx]);

		if(!(flags & TEXT2_IMPLICIT_X))
		{
			int xyoffset = ttext[++ idx];

			if((xyoffset & 0x80))
			{
				if (flags & TEXT2_VERTICAL)
					y += ttext[idx + 1] | (ttext[idx + 2] << 8);
				else
					x += ttext[idx + 1] | (ttext[idx + 2] << 8);

			  idx += 2;
			}
			else
			{
				if (flags & TEXT2_VERTICAL)
					y += xyoffset;
				else
					x += xyoffset;
			}
		}

		if(glyph)
		{
			Display_DrawGlyph
			(
				mixmode,
				x + (short)glyph->offset,
				y + (short)glyph->baseline,
				glyph->width,
				glyph->height,
				glyph->pixmap,
				0,
				0,
				bgcolour,
				fgcolour
			);

			if(flags & TEXT2_IMPLICIT_X)
				x += glyph->width;
		}
	}

	void Display_DrawText
	(
		uint8 font,
		uint8 flags,
		uint8 opcode,
		int mixmode,
		int x,
		int y,
		int clipx,
		int clipy,
		int clipcx,
		int clipcy,
		int boxx,
		int boxy,
		int boxcx,
		int boxcy,
		BRUSH * brush,
		int bgcolour,
		int fgcolour,
		uint8 * text,
		uint8 length
	)
	{
		int i, j;
		DATABLOB *entry;

		HBRUSH hbr = CreateSolidBrush(bgcolour);
		HGDIOBJ holdbrush = SelectObject(m_displayBuffer, hbr);
		HGDIOBJ holdpen = SelectObject(m_displayBuffer, GetStockObject(NULL_PEN));

		if (boxcx > 1)
			Rectangle(m_displayBuffer, boxx, boxy, boxx + boxcx + 1, boxy + boxcy + 1);
		else if (mixmode == MIX_OPAQUE)
			Rectangle(m_displayBuffer, clipx, clipy, clipx + clipcx + 1, clipy + clipcy + 1);

		SelectObject(m_displayBuffer, holdpen);
		SelectObject(m_displayBuffer, holdbrush);

		DeleteObject(hbr);

		if(boxcx > 1)
			Display_RepaintArea(boxx, boxy, boxcx, boxcy);
		else
			Display_RepaintArea(clipx, clipy, clipcx, clipcy);

		/* Paint text, character by character */
		for (i = 0; i < length;)
		{
			switch (text[i])
			{
				case 0xff:
					/* At least two bytes needs to follow */
					if (i + 3 > length)
					{
						warning("Skipping short 0xff command:");
						for (j = 0; j < length; j++)
							fprintf(stderr, "%02x ", text[j]);
						fprintf(stderr, "\n");
						i = length = 0;
						break;
					}
					cache_put_text(NULL /* TODO */, text[i + 1], text, text[i + 2]);
					i += 3;
					length -= i;
					/* this will move pointer from start to first character after FF command */
					text = &(text[i]);
					i = 0;
					break;

				case 0xfe:
					/* At least one byte needs to follow */
					if (i + 2 > length)
					{
						warning("Skipping short 0xfe command:");
						for (j = 0; j < length; j++)
							fprintf(stderr, "%02x ", text[j]);
						fprintf(stderr, "\n");
						i = length = 0;
						break;
					}
					entry = cache_get_text(/*This*/NULL, text[i + 1]);
					if (entry->data != NULL)
					{
						if ((((uint8 *) (entry->data))[1] == 0)
							&& (!(flags & TEXT2_IMPLICIT_X)) && (i + 2 < length))
						{
							if (flags & TEXT2_VERTICAL)
								y += text[i + 2];
							else
								x += text[i + 2];
						}
						for (j = 0; j < entry->size; j++)
							Display_DoGlyph(font, flags, mixmode, x, y, bgcolour, fgcolour, ((uint8 *) (entry->data)), j);
					}
					if (i + 2 < length)
						i += 3;
					else
						i += 2;
					length -= i;
					/* this will move pointer from start to first character after FE command */
					text = &(text[i]);
					i = 0;
					break;

				default:
					Display_DoGlyph(font, flags, mixmode, x, y, bgcolour, fgcolour, text, i);
					i++;
					break;
			}
		}
	}

	void Display_SaveDesktop(uint32 offset, int x, int y, int cx, int cy)
	{
		GdiFlush();

		uint8 * data =
			(uint8 *)m_displayBufferRaw +
			x * m_displayBufferByteDepth +
			(m_displayBufferHeight - y - cy) * m_displayBufferStride;

		cache_put_desktop
		(
			/*This*/NULL,
			offset * m_displayBufferByteDepth,
			cx,
			cy,
			m_displayBufferStride,
			m_displayBufferByteDepth,
			data
		);
	}

	void Display_RestoreDesktop(uint32 offset, int x, int y, int cx, int cy)
	{
		int fromstride = cx * m_displayBufferByteDepth;

		const uint8 * src = cache_get_desktop(/*This*/NULL, offset, cx, cy, m_displayBufferByteDepth);

		uint8 * dst =
			(uint8 *)m_displayBufferRaw +
			x * m_displayBufferByteDepth +
			(m_displayBufferHeight - y - cy) * m_displayBufferStride;

		GdiFlush();

		for(int i = 0; i < cy; ++ i)
		{
			memcpy(dst, src, fromstride);
			src += fromstride;
			dst += m_displayBufferStride;
		}

		Display_RepaintArea(x, y, cx, cy);
	}


	void Display_BeginUpdate()
	{
		EnterCriticalSection(&m_displayBufferMutex);
		m_displayBufferSave = SaveDC(m_displayBuffer);
	}

	void Display_EndUpdate()
	{
		RestoreDC(m_displayBuffer, m_displayBufferSave);
		LeaveCriticalSection(&m_displayBufferMutex);
	}

	/*
		This is the input window. It receives the keyboard and mouse input from
		the user, and it's the only window that can receive the keyboard focus.
		It completely fills its parent, the console window, and it runs in its
		own thread for performance reasons and because of technical reasons
		involving keyboard hooks in full-screen mode
	*/
	HWND m_inputWindow;
	HCURSOR m_inputCursor;

	LRESULT InputWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

			/* Keyboard stuff */
			// TODO: we need a good way to post output cross-thread
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_SCANCODE, RDP_KEYPRESS | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0), LOBYTE(HIWORD(lparam)), 0);
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_SCANCODE, RDP_KEYRELEASE | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0), LOBYTE(HIWORD(lparam)), 0);
			break;

			/* Mouse stuff */
			// Cursor shape
		case WM_SETCURSOR:
			if(LOWORD(lParam) == HTCLIENT)
			{
				SetCursor(m_inputCursor);
				return TRUE;
			}

			break;

			// Movement
		case WM_MOUSEMOVE:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, LOWORD(lparam), HIWORD(lparam));
			break;

			// Buttons
			// TODO: X buttons
		case WM_LBUTTONDOWN:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
			break;

		case WM_RBUTTONDOWN:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
			break;

		case WM_MBUTTONDOWN:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
			break;

		case WM_LBUTTONUP:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1, LOWORD(lparam), HIWORD(lparam));
			break;

		case WM_RBUTTONUP:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2, LOWORD(lparam), HIWORD(lparam));
			break;

		case WM_MBUTTONUP:
			//rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3, LOWORD(lparam), HIWORD(lparam));
			break;

			// Wheel
		case WM_MOUSEWHEEL:
			//mstsc_mousewheel(This, (SHORT)HIWORD(wparam), lparam);
			break;
		}

		return DefWindowProc(m_inputWindow, uMsg, wParam, lParam);
	}

public:
};

#pragma warning(push)
#pragma warning(disable: 4584)

/* The ActiveX control */
class RdpClient SEALED_:
	/* COM basics */
	public IUnknown,
	public IDispatch,

	/* ActiveX stuff */
	public IConnectionPointContainer,
	public IDataObject,
	public IObjectSafety,
	public IOleControl,
	public IOleInPlaceActiveObject,
	public IOleInPlaceObject,
	public IOleObject,
	public IOleWindow,
	public IPersist,
	public IPersistPropertyBag,
	public IPersistStorage,
	public IPersistStreamInit,
	public IProvideClassInfo,
	public IProvideClassInfo2,
	public IQuickActivate,
	public IViewObject,
	public IViewObject2,

	// NOTE: the original has a vestigial, non-functional implementation of this, which we omit
	// ISpecifyPropertyPages

	// Hidden interfaces, not available through QueryInterface
	public IConnectionPoint,

	/* RDP client interface */
	public MSTSCLib::IMsRdpClient4,
	public MSTSCLib::IMsRdpClientNonScriptable2

	// NOTE: implemented by inner classes due to requiring distinct IDispatch implementations
	// IMsRdpClientAdvancedSettings4
	// IMsRdpClientSecuredSettings
{
private:
	/* An endless amount of COM glue */
	// Reference counting
	LONG m_refCount;

#ifdef _DEBUG
	DWORD m_apartmentThreadId;

	bool InsideApartment() const
	{
		return GetCurrentThreadId() == m_apartmentThreadId;
	}
#endif

	// Aggregation support
	IUnknown * m_punkOuter;

	class RdpClientInner: public IUnknown
	{
	private:
		RdpClient * Outer()
		{
			return InnerToOuter(this);
		}

	public:
		virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
		{
			return Outer()->queryInterface(riid, ppvObject);
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
		{
			return Outer()->addRef();
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::Release()
		{
			return Outer()->release();
		}

	}
	m_inner;

	// Persistence support
	CLSID m_classId;

	// Late binding support
	unsigned m_typeLibIndex;
	ITypeLib * m_typeLib;
	ITypeInfo * m_dispTypeInfo;

	// Event sinks
	size_t m_EventSinksCount;

	union
	{
		MSTSCLib::IMsTscAxEvents * m_EventSinksStatic[1];
		MSTSCLib::IMsTscAxEvents ** m_EventSinks;
	};

	// OLE control glue
	HWND m_controlWindow;
	IOleClientSite * m_clientSite;
	IOleInPlaceSite * m_inPlaceSite;
	IOleAdviseHolder * m_adviseHolder;
	LONG m_freezeEvents;
	bool m_uiActive;

	// UrlMon security
	DWORD m_SafetyOptions;

	bool IsSafeForScripting() const
	{
		return m_SafetyOptions & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
	}

	/* Glue to interface to rdesktop-core */
	RdpClientUI * m_clientUI;
	RDPCLIENT m_protocolState;
	HANDLE m_protocolThread;
	HANDLE m_protocolThreadWaitingReconnection;
	bool m_reconnectAborted;
	bool m_actuallyConnected;
	bool m_loggedIn;

	/* Properties */
	// Storage fields
	// NOTE: keep sorted by alignment (pointers and handles, integers, enumerations, booleans)
	BSTR m_Domain;
	BSTR m_UserName;
	BSTR m_DisconnectedText;
	BSTR m_ConnectingText;
	BSTR m_FullScreenTitle;
	BSTR m_StartProgram;
	BSTR m_WorkDir;
	BSTR m_ConnectedStatusText;
	BSTR m_ClearTextPassword; // FIXME! dangerous, shouldn't store in cleartext!
	BSTR m_RdpdrLocalPrintingDocName;
	BSTR m_RdpdrClipCleanTempDirString;
	BSTR m_RdpdrClipPasteInfoString;
	BSTR m_KeyboardLayoutString;
	LPSTR m_Server;
	LPSTR m_LoadBalanceInfo;
	// TODO: plugin DLLs
	HWND m_UIParentWindowHandle;
	long m_DesktopWidth;
	long m_DesktopHeight;
	long m_StartConnected;
	long m_ColorDepth;
	long m_KeyboardHookMode;
	long m_AudioRedirectionMode;
	long m_TransportType;
	long m_SasSequence;
	long m_RDPPort;
	long m_HotKeyFullScreen;
	long m_HotKeyAltEsc;
	long m_HotKeyAltShiftTab;
	long m_HotKeyAltSpace;
	long m_HotKeyAltTab;
	long m_HotKeyCtrlAltDel;
	long m_HotKeyCtrlEsc;
	long m_orderDrawThresold;
	long m_BitmapCacheSize;
	long m_BitmapVirtualCacheSize;
	long m_NumBitmapCaches;
	long m_brushSupportLevel;
	long m_minInputSendInterval;
	long m_InputEventsAtOnce;
	long m_maxEventCount;
	long m_keepAliveInternal;
	long m_shutdownTimeout;
	long m_overallConnectionTimeout;
	long m_singleConnectionTimeout;
	long m_MinutesToIdleTimeout;
	long m_BitmapVirtualCache16BppSize;
	long m_BitmapVirtualCache24BppSize;
	long m_PerformanceFlags;
	long m_MaxReconnectAttempts;
	unsigned int m_AuthenticationLevel;

	MSTSCLib::ExtendedDisconnectReasonCode m_ExtendedDisconnectReason;

	bool m_Connected;
	bool m_Compress;
	bool m_BitmapPersistence;
	bool m_allowBackgroundInput;
	bool m_ContainerHandledFullScreen;
	bool m_DisableRdpdr;
	bool m_SecuredSettingsEnabled;
	bool m_FullScreen;
	bool m_AcceleratorPassthrough;
	bool m_ShadowBitmap;
	bool m_EncryptionEnabled;
	bool m_DedicatedTerminal;
	bool m_DisableCtrlAltDel;
	bool m_EnableWindowsKey;
	bool m_DoubleClickDetect;
	bool m_MaximizeShell;
	bool m_ScaleBitmapCachesByBpp;
	bool m_CachePersistenceActive;
	bool m_ConnectToServerConsole;
	bool m_SmartSizing; // FIXME: this can be set while the control is connected
	bool m_DisplayConnectionBar;
	bool m_PinConnectionBar;
	bool m_GrabFocusOnConnect;
	bool m_RedirectDrives;
	bool m_RedirectPrinters;
	bool m_RedirectPorts;
	bool m_RedirectSmartCards;
	bool m_NotifyTSPublicKey;
	bool m_CanAutoReconnect;
	bool m_EnableAutoReconnect;
	bool m_ConnectionBarShowMinimizeButton;
	bool m_ConnectionBarShowRestoreButton;

	// Generic getters/setters
	HRESULT GetProperty(BSTR& prop, BSTR * retVal) const
	{
		assert(InsideApartment());

		if(retVal == NULL)
			return E_POINTER;

		*retVal = SysAllocStringLen(prop, SysStringLen(prop));

		if(*retVal == NULL)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	HRESULT GetProperty(LPSTR& prop, BSTR * retVal) const
	{
		assert(InsideApartment());

		if(retVal == NULL)
			return E_POINTER;

		*retVal = LpszToBstr(prop);

		if(*retVal == NULL)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	HRESULT SetProperty(BSTR& prop, BSTR newValue)
	{
		assert(InsideApartment());

		if(m_Connected)
			return E_FAIL;

		SysFreeString(prop);

		UINT len = SysStringLen(newValue);

		if(len)
		{
			// no embedded NULs, please
			if(len != lstrlenW(newValue))
				return E_INVALIDARG;

			prop = SysAllocStringLen(newValue, len);

			if(prop == NULL)
				return E_OUTOFMEMORY;
		}
		else
			prop = NULL;

		return S_OK;
	}

	HRESULT ReplaceProperty(BSTR& prop, BSTR newValue)
	{
		assert(InsideApartment());
		assert((prop == NULL && newValue == NULL) || prop != newValue);

		SysFreeString(prop);
		prop = newValue;
		return S_OK;
	}

	HRESULT SetProperty(LPSTR& prop, BSTR newValue)
	{
		assert(InsideApartment());

		if(m_Connected)
			return E_FAIL;

		delete[] prop;

		if(SysStringLen(newValue))
		{
			prop = BstrToLpsz(newValue);

			if(prop == NULL)
				return E_OUTOFMEMORY;
		}
		else
			prop = NULL;

		return S_OK;
	}

	HRESULT ReplaceProperty(LPSTR& prop, LPSTR newValue)
	{
		assert(InsideApartment());
		assert((prop == NULL && newValue == NULL) || prop != newValue);

		if(prop)
			delete[] prop;

		prop = newValue;
		return S_OK;
	}

	template<class Type> HRESULT SetProperty(bool& prop, const Type& newValue)
	{
		assert(InsideApartment());

		if(m_Connected)
			return E_FAIL;

		prop = !!newValue;
		return S_OK;
	}

	template<class Type> HRESULT SetProperty(Type& prop, const Type& newValue)
	{
		assert(InsideApartment());

		if(m_Connected)
			return E_FAIL;

		prop = newValue;
		return S_OK;
	}

	template<class Type> HRESULT GetProperty(const bool& prop, Type * retVal) const
	{
		assert(InsideApartment());

		if(retVal == NULL)
			return E_POINTER;

		*retVal = prop ? VARIANT_TRUE : VARIANT_FALSE;
		return S_OK;
	}

	template<class Type> HRESULT GetProperty(const Type& prop, Type * retVal) const
	{
		assert(InsideApartment());

		if(retVal == NULL)
			return E_POINTER;

		*retVal = prop;
		return S_OK;
	}

	/* Events */
	MSTSCLib::IMsTscAxEvents ** GetSinks() const
	{
		if(m_EventSinksCount > 1)
			return m_EventSinks;
		else
			return const_cast<MSTSCLib::IMsTscAxEvents **>(m_EventSinksStatic);
	}

	// Event freezing
	void UnfreezeEvents()
	{
		// Just in case
	}

	// Generic event riser & helpers
	void InvokeSinks(DISPID eventId, VARIANTARG rgvarg[], unsigned int cArgs, VARIANTARG * retval)
	{
		assert(InsideApartment());

		DISPPARAMS params;

		params.rgvarg = rgvarg;
		params.rgdispidNamedArgs = NULL;
		params.cArgs = cArgs;
		params.cNamedArgs = 0;

		MSTSCLib::IMsTscAxEvents ** sinks = GetSinks();

		for(size_t i = 0; i < m_EventSinksCount; ++ i)
			sinks[i]->Invoke(eventId, IID_NULL, 0, DISPATCH_METHOD, &params, retval, NULL, NULL);
	}

	typedef void (RdpClient::* AsyncEventCallback)
	(
		DISPID eventId,
		VARIANTARG * rgvarg,
		unsigned int cArgs,
		VARIANTARG * retVal
	);

	void CleanupEventArgumentsCallback
	(
		DISPID eventId,
		VARIANTARG * rgvarg,
		unsigned int cArgs,
		VARIANTARG * retVal
	)
	{
		assert((rgvarg == NULL) == (cArgs == 0));

		for(unsigned int i = 0; i < cArgs; ++ i)
			VariantClear(&rgvarg[i]);

		if(retVal)
			VariantClear(retVal);
	}

	// synchronous call from inside the apartment that owns the object
	void FireEventInsideApartment
	(
		DISPID eventId,
		VARIANTARG * rgvarg = NULL,
		unsigned int cArgs = 0,
		VARIANTARG * retval = NULL,
		AsyncEventCallback callback = NULL
	)
	{
		assert(InsideApartment());

		if(retval == NULL && callback)
		{
			VARIANTARG localRetval = { };
			retval = &localRetval;
		}

		InvokeSinks(eventId, rgvarg, cArgs, retval);

		if(callback)
			(this->*callback)(eventId, rgvarg, cArgs, retval);
	}

	struct EventArguments
	{
		DISPID eventId;
		VARIANTARG * rgvarg;
		unsigned int cArgs;
		VARIANTARG * retval;
		AsyncEventCallback callback;
	};

	struct RedirectArguments
	{
		uint32 flags;
		uint32 server_len;
		wchar_t * server;
		uint32 cookie_len;
		char * cookie;
		uint32 username_len;
		wchar_t * username;
		uint32 domain_len;
		wchar_t * domain;
		uint32 password_len;
		wchar_t * password;
	};

	enum
	{
		RDPC_WM_ = WM_USER,
		RDPC_WM_SYNC_EVENT,
		RDPC_WM_ASYNC_EVENT,
		RDPC_WM_DISCONNECT,
		RDPC_WM_REQUEST_CLOSE,
		RDPC_WM_REDIRECT,
	};

	static VOID CALLBACK DisconnectAPC(ULONG_PTR)
	{
		// no need to do anything. The interruption will be enough
	}

	bool HandleEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
	{
		result = 0;

		switch(uMsg)
		{
			/* Regular event to be dispatched to the container's sink */
		case RDPC_WM_SYNC_EVENT:
			assert(InSendMessage());

		case RDPC_WM_ASYNC_EVENT:
			{
				const EventArguments * eventArgs = reinterpret_cast<EventArguments *>(lParam);
				assert(eventArgs);

				FireEventInsideApartment
				(
					eventArgs->eventId,
					eventArgs->rgvarg,
					eventArgs->cArgs,
					eventArgs->retval,
					eventArgs->callback
				);

				if(uMsg == RDPC_WM_ASYNC_EVENT)
					delete eventArgs;
			}

			break;

			/* The protocol thread is about to die: prepare for disconnection */
		case RDPC_WM_DISCONNECT:
			{
				assert(m_Connected);
				assert(InsideApartment());
				assert(InSendMessage());

				// Unblock the protocol thread and wait for it to terminate
				ReplyMessage(0);
				JoinProtocolThread();

				// Finish disconnecting
				PerformDisconnect(static_cast<long>(wParam));
			}

			break;

		case RDPC_WM_REDIRECT:
			{
				assert(InSendMessage());
				assert(lParam);
				assert(m_Connected);
				assert(m_protocolState.redirect);

				RedirectArguments * redirectArgs = reinterpret_cast<RedirectArguments *>(lParam);

				// BUGBUG: this is extremely messy and more prone to out-of-memory than it should be
				LPSTR lpszNewServer = NULL;
				LPSTR lpszNewCookie = NULL;
				BSTR strNewUsername = NULL;
				BSTR strNewDomain = NULL;
				BSTR strNewPassword = NULL;
				HRESULT hr = S_OK;

				for(;;)
				{
					// Allocate the new properties
					hr = E_OUTOFMEMORY;

					// FIXME: convert the hostname to Punycode, not the ANSI codepage
					lpszNewServer = AllocLpsz(redirectArgs->server, redirectArgs->server_len / sizeof(OLECHAR));

					if(lpszNewServer == NULL && redirectArgs->server_len)
						break;

					lpszNewCookie = AllocLpsz(redirectArgs->cookie, redirectArgs->cookie_len);

					if(lpszNewCookie == NULL && redirectArgs->cookie_len)
						break;

					strNewUsername = SysAllocStringLen(redirectArgs->username, redirectArgs->username_len / sizeof(OLECHAR));

					if(strNewUsername == NULL && redirectArgs->username_len)
						break;

					strNewDomain = SysAllocStringLen(redirectArgs->domain, redirectArgs->domain_len / sizeof(OLECHAR));

					if(strNewDomain == NULL && redirectArgs->domain_len)
						break;

					strNewPassword = SysAllocStringLen(redirectArgs->password, redirectArgs->password_len / sizeof(OLECHAR));

					if(strNewPassword == NULL && redirectArgs->password_len)
						break;

					hr = S_OK;
					break;
				}

				// Success
				if(SUCCEEDED(hr))
				{
					// set the new properties
					ReplaceProperty(m_Server, lpszNewServer);
					ReplaceProperty(m_LoadBalanceInfo, lpszNewCookie);
					ReplaceProperty(m_UserName, strNewUsername);
					ReplaceProperty(m_Domain, strNewDomain);
					ReplaceProperty(m_ClearTextPassword, strNewPassword);
				}
				// Failure
				else
				{
					// free the buffers
					FreeLpsz(lpszNewServer);
					FreeLpsz(lpszNewCookie);
					SysFreeString(strNewUsername);
					SysFreeString(strNewDomain);
					SysFreeString(strNewPassword);

					// signal the error
					m_protocolState.disconnect_reason = 262;
					m_protocolState.redirect = False;
					result = -1;
				}
			}

			break;

			// BUGBUG: this could potentially disconnect an unrelated connection established later...
		case RDPC_WM_REQUEST_CLOSE:
			{
				assert(!InSendMessage());

				if(m_Connected)
				{
					// Ask confirmation to the container in case we are logged in
					if(m_loggedIn && !FireConfirmClose())
						break;

					// For reentrancy (OnConfirmClose could deviously call Disconnect)
					if(m_protocolThread == NULL)
						break;

					// Terminate the protocol thread. It will fire the Disconnected event on exit
					TerminateProtocolThread();
				}
			}

			break;

		default:
			return false;
		}

		// If the calling thread is blocked, unblock it ASAP
		if(InSendMessage())
			ReplyMessage(result);

		return true;
	}

	// synchronous call from outside the apartment
	void FireEventOutsideApartment
	(
		DISPID eventId,
		VARIANTARG * rgvarg = NULL,
		unsigned int cArgs = 0,
		VARIANTARG * retval = NULL,
		AsyncEventCallback callback = NULL
	)
	{
		assert(!InsideApartment());
		EventArguments syncEvent = { eventId, rgvarg, cArgs, retval, callback };
		SendMessage(m_controlWindow, RDPC_WM_SYNC_EVENT, 0, reinterpret_cast<LPARAM>(&syncEvent));
	}

	// asynchronous call from outside the apartment
	HRESULT FireEventOutsideApartmentAsync
	(
		DISPID eventId,
		VARIANTARG * rgvarg = NULL,
		unsigned int cArgs = 0,
		VARIANTARG * retval = NULL,
		AsyncEventCallback callback = NULL
	)
	{
		assert(!InsideApartment());

		EventArguments * asyncEvent = new EventArguments();

		if(asyncEvent == NULL)
			return E_OUTOFMEMORY;

		asyncEvent->eventId = eventId;
		asyncEvent->rgvarg = rgvarg;
		asyncEvent->cArgs = cArgs;
		asyncEvent->retval = NULL;

		if(!PostMessage(m_controlWindow, RDPC_WM_ASYNC_EVENT, 0, reinterpret_cast<LPARAM>(asyncEvent)))
		{
			delete asyncEvent;
			return HRESULT_FROM_WIN32(GetLastError());
		}

		return S_OK;
	}

	// Specific events
	void FireConnecting()
	{
		// Source: protocol
		FireEventOutsideApartment(1);
	}

	void FireConnected()
	{
		// Source: protocol
		FireEventOutsideApartment(2);
	}

	void FireLoginComplete()
	{
		// Source: protocol
		FireEventOutsideApartment(3);
	}

	void FireDisconnected(long reason)
	{
		// Source: protocol. Special handling
		SendMessage(m_controlWindow, RDPC_WM_DISCONNECT, reason, 0);
	}

	void FireEnterFullScreenMode()
	{
		// Source: UI window
		FireEventInsideApartment(5);
	}

	void FireLeaveFullScreenMode()
	{
		// Source: UI window
		FireEventInsideApartment(6);
	}

	HRESULT FireChannelReceivedData(char (& chanName)[CHANNEL_NAME_LEN + 1], void * chanData, unsigned int chanDataSize)
	{
		// BUGBUG: what to do when we run out of memory?

		OLECHAR wchanName[ARRAYSIZE(chanName)];
		std::copy(chanName + 0, chanName + ARRAYSIZE(chanName), wchanName);

		BSTR bstrChanName = SysAllocString(wchanName);

		if(bstrChanName == NULL)
			return E_OUTOFMEMORY;

		BSTR bstrChanData = SysAllocStringByteLen(NULL, chanDataSize);

		if(bstrChanData == NULL)
		{
			SysFreeString(bstrChanName);
			return E_OUTOFMEMORY;
		}

		CopyMemory(bstrChanData, chanData, chanDataSize);

		VARIANTARG args[2] = { };

		args[1].vt = VT_BSTR;
		args[1].bstrVal = bstrChanName;

		args[0].vt = VT_BSTR;
		args[0].bstrVal = bstrChanData;

		// Source: protocol
		HRESULT hr = FireEventOutsideApartmentAsync(7, args, ARRAYSIZE(args), NULL, &RdpClient::CleanupEventArgumentsCallback);

		if(FAILED(hr))
			CleanupEventArgumentsCallback(7, args, ARRAYSIZE(args), NULL);

		return hr;
	}

	void FireRequestGoFullScreen()
	{
		// Source: UI window
		FireEventInsideApartment(8);
	}

	void FireRequestLeaveFullScreen()
	{
		// Source: UI window
		FireEventInsideApartment(9);
	}

	void FireFatalError(long errorCode)
	{
		VARIANTARG arg = { };

		arg.vt = VT_I4;
		arg.lVal = errorCode;

		// Source: protocol
		FireEventOutsideApartment(10, &arg, 1);
	}

	void FireFatalErrorFromApartment(long errorCode)
	{
		VARIANTARG arg = { };

		arg.vt = VT_I4;
		arg.lVal = errorCode;

		// Source: control
		FireEventInsideApartment(10, &arg, 1);
	}

	void FireWarning(long warningCode)
	{
		VARIANTARG arg = { };

		arg.vt = VT_I4;
		arg.lVal = warningCode;

		// Source: protocol
		FireEventOutsideApartment(11, &arg, 1);
	}

	void FireRemoteDesktopSizeChange(long width, long height)
	{
		VARIANTARG args[2] = { };

		args[1].vt = VT_I4;
		args[1].lVal = width;

		args[0].vt = VT_I4;
		args[0].lVal = height;

		// Source: UI window
		FireEventInsideApartment(12, args, ARRAYSIZE(args));
	}

	void FireIdleTimeoutNotification()
	{
		// Source: input thread
		FireEventOutsideApartment(13);
	}

	void FireRequestContainerMinimize()
	{
		// Source: UI window
		FireEventInsideApartment(14);
	}

	bool FireConfirmClose()
	{
		VARIANTARG retval = { };
		VARIANT_BOOL allowClose = VARIANT_TRUE;

		retval.vt = VT_BYREF | VT_BOOL;
		retval.pboolVal = &allowClose;

		// Source: control
		FireEventInsideApartment(15, NULL, 0, &retval);

		return allowClose != VARIANT_FALSE;
	}

    HRESULT FireReceivedTSPublicKey(void * publicKey, unsigned int publicKeyLength)
	{
		assert(m_Connected);

		if(!m_NotifyTSPublicKey)
			return S_OK;

		BSTR bstrPublicKey = SysAllocStringByteLen(NULL, publicKeyLength);

		if(bstrPublicKey == NULL)
			return E_OUTOFMEMORY;

		CopyMemory(bstrPublicKey, publicKey, publicKeyLength);

		VARIANT_BOOL continueLogon = VARIANT_TRUE;
		VARIANTARG arg = { };
		VARIANTARG retval = { };

		arg.vt = VT_BSTR;
		arg.bstrVal = bstrPublicKey;

		retval.vt = VT_BYREF | VT_BOOL;
		retval.pboolVal = &continueLogon;

		// Source: protocol
		FireEventOutsideApartment(16, &arg, 1, &retval);

		return continueLogon ? S_OK : S_FALSE;
	}

	LONG FireAutoReconnecting(long disconnectReason, long attemptCount)
	{
		LONG continueStatus = MSTSCLib::autoReconnectContinueAutomatic;
		VARIANTARG args[2] = { };
		VARIANTARG retval = { };

		args[1].vt = VT_I4;
		args[1].lVal = disconnectReason;

		args[0].vt = VT_I4;
		args[0].lVal = attemptCount;

		retval.vt = VT_BYREF | VT_I4;
		retval.plVal = &continueStatus;

		// Source: protocol
		FireEventOutsideApartment(17, args, ARRAYSIZE(args), &retval);

		return continueStatus;
	}

    void FireAuthenticationWarningDisplayed()
	{
		// Source: protocol
		FireEventOutsideApartment(18);
	}

    void FireAuthenticationWarningDismissed()
	{
		// Source: protocol
		FireEventOutsideApartment(19);
	}

	/* Actual IUnknown implementation */
	HRESULT queryInterface(REFIID riid, void ** ppvObject)
	{
		IUnknown * pvObject = NULL;

		using namespace MSTSCLib;

		if(riid == IID_IUnknown)
			pvObject = static_cast<IUnknown *>(&m_inner);
		else if(riid == IID_IConnectionPointContainer)
			pvObject = static_cast<IConnectionPointContainer *>(this);
		else if(riid == IID_IDataObject)
			pvObject = static_cast<IDataObject *>(this);
		else if(riid == IID_IObjectSafety)
			pvObject = static_cast<IObjectSafety *>(this);
		else if(riid == IID_IOleControl)
			pvObject = static_cast<IOleControl *>(this);
		else if(riid == IID_IOleInPlaceActiveObject)
			pvObject = static_cast<IOleInPlaceActiveObject *>(this);
		else if(riid == IID_IOleInPlaceObject)
			pvObject = static_cast<IOleInPlaceObject *>(this);
		else if(riid == IID_IOleObject)
			pvObject = static_cast<IOleObject *>(this);
		else if(riid == IID_IOleWindow)
			pvObject = static_cast<IOleWindow *>(this);
		else if(riid == IID_IPersist)
			pvObject = static_cast<IPersist *>(this);
		else if(riid == IID_IPersistPropertyBag)
			pvObject = static_cast<IPersistPropertyBag *>(this);
		else if(riid == IID_IPersistStorage)
			pvObject = static_cast<IPersistStorage *>(this);
		else if(riid == IID_IPersistStreamInit)
			pvObject = static_cast<IPersistStreamInit *>(this);
		else if(riid == IID_IQuickActivate)
			pvObject = static_cast<IQuickActivate *>(this);
		else if(riid == IID_IViewObject)
			pvObject = static_cast<IViewObject *>(this);
		else if(riid == IID_IViewObject2)
			pvObject = static_cast<IViewObject2 *>(this);
		else if(riid == IID_IMsTscAx || riid == MSTSCLib_Redist::IID_IMsTscAx)
			pvObject = static_cast<IMsTscAx *>(this);
		else if(riid == IID_IMsRdpClient)
			pvObject = static_cast<IMsRdpClient *>(this);
		else if(riid == IID_IMsRdpClient2)
			pvObject = static_cast<IMsRdpClient2 *>(this);
		else if(riid == IID_IMsRdpClient3)
			pvObject = static_cast<IMsRdpClient3 *>(this);
		else if(riid == IID_IMsRdpClient4)
			pvObject = static_cast<IMsRdpClient4 *>(this);
		else if(riid == IID_IMsTscNonScriptable)
			pvObject = static_cast<IMsTscNonScriptable *>(this);
		else if(riid == IID_IMsRdpClientNonScriptable)
			pvObject = static_cast<IMsRdpClientNonScriptable *>(this);
		else if(riid == IID_IMsRdpClientNonScriptable2)
			pvObject = static_cast<IMsRdpClientNonScriptable2 *>(this);

		*ppvObject = pvObject;

		if(pvObject)
		{
			pvObject->AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	ULONG addRef()
	{
		return InterlockedIncrement(&m_refCount);
	}

	ULONG release()
	{
		LONG n = InterlockedDecrement(&m_refCount);

		if(n == 0)
			delete this;

		return n;
	}

	/* Constructor */
	RdpClient(REFCLSID classId, unsigned libIndex, IUnknown * punkOuter):
		// COM/OLE internals
		m_refCount(0),
		m_punkOuter(punkOuter),
		m_classId(classId),
		m_typeLibIndex(libIndex),
		m_typeLib(),
		m_dispTypeInfo(),
		m_controlWindow(NULL),
		m_clientSite(),
		m_inPlaceSite(),
		m_adviseHolder(),
		m_freezeEvents(0),
		m_uiActive(false),
		m_SafetyOptions(),

#ifdef _DEBUG
		m_apartmentThreadId(GetCurrentThreadId()),
#endif

		// rdesktop-core interface
		m_protocolState(),
		m_protocolThread(),

		// Properties
		m_Server(),
		m_Domain(),
		m_UserName(),
		m_DisconnectedText(),
		m_ConnectingText(),
		m_FullScreenTitle(),
		m_StartProgram(),
		m_WorkDir(),
		m_LoadBalanceInfo(),
		m_ConnectedStatusText(),
		m_ClearTextPassword(),
		m_RdpdrLocalPrintingDocName(),
		m_RdpdrClipCleanTempDirString(),
		m_RdpdrClipPasteInfoString(),
		m_UIParentWindowHandle(),
		m_DesktopWidth(),
		m_DesktopHeight(),
		m_StartConnected(),
		m_ColorDepth(16),
		m_KeyboardHookMode(2),
		m_AudioRedirectionMode(0),
		m_TransportType(1), // BUGBUG: ??? what's this ???
		m_SasSequence(0xAA03), // BUGBUG: ??? what's this ???
		m_RDPPort(3389),
		m_HotKeyFullScreen(VK_CANCEL),
		m_HotKeyAltEsc(VK_INSERT),
		m_HotKeyAltShiftTab(VK_NEXT),
		m_HotKeyAltSpace(VK_DELETE),
		m_HotKeyAltTab(VK_PRIOR),
		m_HotKeyCtrlAltDel(VK_END),
		m_HotKeyCtrlEsc(VK_HOME),
		m_orderDrawThresold(0),
		m_BitmapCacheSize(1500),
		m_BitmapVirtualCacheSize(10),
		m_brushSupportLevel(),
		m_minInputSendInterval(),
		m_InputEventsAtOnce(),
		m_maxEventCount(),
		m_keepAliveInternal(0),
		m_shutdownTimeout(10),
		m_overallConnectionTimeout(120),
		m_singleConnectionTimeout(30),
		m_MinutesToIdleTimeout(0),
		m_BitmapVirtualCache16BppSize(20),
		m_BitmapVirtualCache24BppSize(30),
		m_PerformanceFlags(),
		m_MaxReconnectAttempts(20),
		m_AuthenticationLevel(0),
		m_ExtendedDisconnectReason(MSTSCLib::exDiscReasonNoInfo),
		m_Connected(false),
		m_Compress(true),
		m_BitmapPersistence(true),
		m_allowBackgroundInput(false),
		m_ContainerHandledFullScreen(false),
		m_DisableRdpdr(false),
		m_SecuredSettingsEnabled(true),
		m_FullScreen(false),
		m_AcceleratorPassthrough(true),
		m_ShadowBitmap(true),
		m_EncryptionEnabled(true),
		m_DedicatedTerminal(false),
		m_DisableCtrlAltDel(true),
		m_EnableWindowsKey(true),
		m_DoubleClickDetect(false),
		m_MaximizeShell(true),
		m_ScaleBitmapCachesByBpp(false),
		m_CachePersistenceActive(false),
		m_ConnectToServerConsole(false),
		m_SmartSizing(false),
		m_DisplayConnectionBar(true),
		m_PinConnectionBar(true),
		m_GrabFocusOnConnect(true),
		m_RedirectDrives(false),
		m_RedirectPrinters(false),
		m_RedirectPorts(false),
		m_RedirectSmartCards(false),
		m_NotifyTSPublicKey(false),
		m_CanAutoReconnect(false),
		m_EnableAutoReconnect(true),
		m_ConnectionBarShowMinimizeButton(true),
		m_ConnectionBarShowRestoreButton(true)
	{
		if(m_punkOuter == NULL)
			m_punkOuter = &m_inner;
	}

	/* Destructor */
	~RdpClient()
	{
		assert(m_refCount == 0);

		if(m_Connected)
		{
			// Terminate the protocol thread
			TerminateProtocolThread();

			// Dispatch the RDPC_WM_DISCONNECT message sent by the dying thread
			MSG msg;
			PeekMessage(&msg, m_controlWindow, 0, 0, PM_NOREMOVE);

			assert(!m_Connected);
		}

		DestroyControlWindow();

		if(m_typeLib)
			m_typeLib->Release();

		if(m_dispTypeInfo)
			m_dispTypeInfo->Release();

		MSTSCLib::IMsTscAxEvents ** sinks = GetSinks();

		for(size_t i = 0; i < m_EventSinksCount; ++ i)
			sinks[i]->Release();

		if(m_EventSinksCount > 1)
			delete[] m_EventSinks;

		if(m_clientSite)
			m_clientSite->Release();

		if(m_inPlaceSite)
			m_inPlaceSite->Release();

		if(m_adviseHolder)
			m_adviseHolder->Release();

		SysFreeString(m_Domain);
		SysFreeString(m_UserName);
		SysFreeString(m_DisconnectedText);
		SysFreeString(m_DisconnectedText);
		SysFreeString(m_FullScreenTitle);
		SysFreeString(m_StartProgram);
		SysFreeString(m_WorkDir);
		SysFreeString(m_ConnectedStatusText);
		SysFreeString(m_ClearTextPassword);
		SysFreeString(m_RdpdrLocalPrintingDocName);
		SysFreeString(m_RdpdrClipCleanTempDirString);
		SysFreeString(m_RdpdrClipPasteInfoString);

		FreeLpsz(m_LoadBalanceInfo);
		FreeLpsz(m_Server);

		unlockServer();
	}

	/* Advanced settings wrapper */
	friend class AdvancedSettings;

	class AdvancedSettings SEALED_: public MSTSCLib::IMsRdpClientAdvancedSettings4
	{
	private:
		RdpClient * Outer()
		{
			return InnerToOuter(this);
		}

		const RdpClient * Outer() const
		{
			return InnerToOuter(this);
		}

		/* IDispatch type information */
		ITypeInfo * m_dispTypeInfo;

		HRESULT LoadDispTypeInfo()
		{
			if(m_dispTypeInfo)
				return S_OK;

			HRESULT hr = Outer()->LoadTypeLibrary();

			if(FAILED(hr))
				return hr;

			assert(MSTSCLib::IID_IMsRdpClientAdvancedSettings4 == MSTSCLib_Redist::IID_IMsRdpClientAdvancedSettings4);

			hr = Outer()->m_typeLib->GetTypeInfoOfGuid(MSTSCLib::IID_IMsRdpClientAdvancedSettings4, &m_dispTypeInfo);

			if(FAILED(hr))
				return hr;

			assert(m_dispTypeInfo);
			return S_OK;
		}

		HRESULT AcquireDispTypeInfo(ITypeInfo ** ppTI)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			m_dispTypeInfo->AddRef();
			*ppTI = m_dispTypeInfo;
			return S_OK;
		}

	public:
		~AdvancedSettings()
		{
			if(m_dispTypeInfo)
				m_dispTypeInfo->Release();
		}

		/* IUnknown */
		virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
		{
			using namespace MSTSCLib;

			if
			(
				riid == IID_IUnknown ||
				riid == IID_IDispatch ||
				riid == IID_IMsTscAdvancedSettings ||
				riid == IID_IMsRdpClientAdvancedSettings ||
				riid == IID_IMsRdpClientAdvancedSettings2 ||
				riid == IID_IMsRdpClientAdvancedSettings3 ||
				riid == IID_IMsRdpClientAdvancedSettings4
			)
			{
				*ppvObject = this;
				Outer()->addRef();
				return S_OK;
			}
			else
			{
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
		{
			return Outer()->addRef();
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::Release()
		{
			return Outer()->release();
		}

		/* IDispatch */
		virtual STDMETHODIMP IDispatch::GetTypeInfoCount(UINT * pctinfo)
		{
			*pctinfo = 1;
			return S_OK;
		}

		virtual STDMETHODIMP IDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
		{
			if(iTInfo != 0)
				return DISP_E_BADINDEX;

			return AcquireDispTypeInfo(ppTInfo);
		}

		virtual STDMETHODIMP IDispatch::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			return m_dispTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
		}

		virtual STDMETHODIMP IDispatch::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
		{
			return m_dispTypeInfo->Invoke
			(
				static_cast<MSTSCLib::IMsRdpClientAdvancedSettings4 *>(this),
				dispIdMember,
				wFlags,
				pDispParams,
				pVarResult,
				pExcepInfo,
				puArgErr
			);
		}

		/* IMsTscAdvancedSettings */
		virtual STDMETHODIMP IMsTscAdvancedSettings::put_Compress(long pcompress)
		{
			return Outer()->SetProperty(Outer()->m_Compress, pcompress);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_Compress(long * pcompress) const
		{
			return Outer()->GetProperty(Outer()->m_Compress, pcompress);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_BitmapPeristence(long pbitmapPeristence)
		{
			return Outer()->SetProperty(Outer()->m_BitmapPersistence, pbitmapPeristence);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_BitmapPeristence(long * pbitmapPeristence) const
		{
			return Outer()->GetProperty(Outer()->m_BitmapPersistence, pbitmapPeristence);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_allowBackgroundInput(long pallowBackgroundInput)
		{
			if(Outer()->IsSafeForScripting())
				return S_FALSE;

			return Outer()->SetProperty(Outer()->m_allowBackgroundInput, pallowBackgroundInput);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_allowBackgroundInput(long * pallowBackgroundInput) const
		{
			return Outer()->GetProperty(Outer()->m_allowBackgroundInput, pallowBackgroundInput);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_KeyBoardLayoutStr(BSTR rhs)
		{
			return Outer()->SetProperty(Outer()->m_KeyboardLayoutString, rhs);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_PluginDlls(BSTR rhs)
		{
			// TODO: split rhs into an array

			// Control marked safe for scripting: only allow filenames
			if(Outer()->IsSafeForScripting())
			{
				// TODO: validate entries
				// TODO: replace each entry with a full path based on the Virtual Channel DLL path
			}

			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_IconFile(BSTR rhs)
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_IconIndex(long rhs)
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_ContainerHandledFullScreen(long pContainerHandledFullScreen)
		{
			if(Outer()->IsSafeForScripting())
				return S_FALSE;

			return Outer()->SetProperty(Outer()->m_ContainerHandledFullScreen, pContainerHandledFullScreen);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_ContainerHandledFullScreen(long * pContainerHandledFullScreen) const
		{
			return Outer()->GetProperty(Outer()->m_ContainerHandledFullScreen, pContainerHandledFullScreen);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_DisableRdpdr(long pDisableRdpdr)
		{
			return Outer()->SetProperty(Outer()->m_DisableRdpdr, pDisableRdpdr);
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_DisableRdpdr(long * pDisableRdpdr) const
		{
			return Outer()->GetProperty(Outer()->m_DisableRdpdr, pDisableRdpdr);
		}

		/* IMsRdpClientAdvancedSettings */
		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_SmoothScroll(long psmoothScroll)
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_SmoothScroll(long * psmoothScroll) const
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_AcceleratorPassthrough(long pacceleratorPassthrough)
		{
			return Outer()->SetProperty(Outer()->m_AcceleratorPassthrough, pacceleratorPassthrough);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_AcceleratorPassthrough(long * pacceleratorPassthrough) const
		{
			return Outer()->GetProperty(Outer()->m_AcceleratorPassthrough, pacceleratorPassthrough);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ShadowBitmap(long pshadowBitmap)
		{
			return Outer()->SetProperty(Outer()->m_ShadowBitmap, pshadowBitmap);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_ShadowBitmap(long * pshadowBitmap) const
		{
			return Outer()->GetProperty(Outer()->m_ShadowBitmap, pshadowBitmap);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_TransportType(long ptransportType)
		{
			// Reserved
			return Outer()->SetProperty(Outer()->m_TransportType, ptransportType);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_TransportType(long * ptransportType) const
		{
			// Reserved
			return Outer()->GetProperty(Outer()->m_TransportType, ptransportType);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_SasSequence(long psasSequence)
		{
			// Reserved
			return Outer()->SetProperty(Outer()->m_SasSequence, psasSequence);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_SasSequence(long * psasSequence) const
		{
			// Reserved
			return Outer()->GetProperty(Outer()->m_SasSequence, psasSequence);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_EncryptionEnabled(long pencryptionEnabled)
		{
			return Outer()->SetProperty(Outer()->m_EncryptionEnabled, pencryptionEnabled);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_EncryptionEnabled(long * pencryptionEnabled) const
		{
			return Outer()->GetProperty(Outer()->m_EncryptionEnabled, pencryptionEnabled);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DedicatedTerminal(long pdedicatedTerminal)
		{
			return Outer()->SetProperty(Outer()->m_DedicatedTerminal, pdedicatedTerminal);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DedicatedTerminal(long * pdedicatedTerminal) const
		{
			return Outer()->GetProperty(Outer()->m_DedicatedTerminal, pdedicatedTerminal);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RDPPort(long prdpPort)
		{
			if(prdpPort == 0 || prdpPort > 65535)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_RDPPort, prdpPort);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RDPPort(long * prdpPort) const
		{
			return Outer()->GetProperty(Outer()->m_RDPPort, prdpPort);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_EnableMouse(long penableMouse)
		{
			return S_FALSE; // TBD? implement?
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_EnableMouse(long * penableMouse) const
		{
			return S_FALSE; // TBD? implement?
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DisableCtrlAltDel(long pdisableCtrlAltDel)
		{
			return Outer()->SetProperty(Outer()->m_DisableCtrlAltDel, pdisableCtrlAltDel);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DisableCtrlAltDel(long * pdisableCtrlAltDel) const
		{
			return Outer()->GetProperty(Outer()->m_DisableCtrlAltDel, pdisableCtrlAltDel);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_EnableWindowsKey(long penableWindowsKey)
		{
			return Outer()->SetProperty(Outer()->m_EnableWindowsKey, penableWindowsKey);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_EnableWindowsKey(long * penableWindowsKey) const
		{
			return Outer()->GetProperty(Outer()->m_EnableWindowsKey, penableWindowsKey);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DoubleClickDetect(long pdoubleClickDetect)
		{
			return Outer()->SetProperty(Outer()->m_DoubleClickDetect, pdoubleClickDetect);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DoubleClickDetect(long * pdoubleClickDetect) const
		{
			return Outer()->GetProperty(Outer()->m_DoubleClickDetect, pdoubleClickDetect);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_MaximizeShell(long pmaximizeShell)
		{
			return Outer()->SetProperty(Outer()->m_MaximizeShell, pmaximizeShell);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_MaximizeShell(long * pmaximizeShell) const
		{
			return Outer()->GetProperty(Outer()->m_MaximizeShell, pmaximizeShell);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyFullScreen(long photKeyFullScreen)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyFullScreen, photKeyFullScreen);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyFullScreen(long * photKeyFullScreen) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyFullScreen, photKeyFullScreen);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyCtrlEsc(long photKeyCtrlEsc)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyCtrlEsc, photKeyCtrlEsc);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyCtrlEsc(long * photKeyCtrlEsc) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyCtrlEsc, photKeyCtrlEsc);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltEsc(long photKeyAltEsc)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyAltEsc, photKeyAltEsc);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltEsc(long * photKeyAltEsc) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyAltEsc, photKeyAltEsc);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltTab(long photKeyAltTab)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyAltTab, photKeyAltTab);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltTab(long * photKeyAltTab) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyAltTab, photKeyAltTab);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltShiftTab(long photKeyAltShiftTab)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyAltShiftTab, photKeyAltShiftTab);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltShiftTab(long * photKeyAltShiftTab) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyAltShiftTab, photKeyAltShiftTab);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltSpace(long photKeyAltSpace)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyAltSpace, photKeyAltSpace);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltSpace(long * photKeyAltSpace) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyAltSpace, photKeyAltSpace);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyCtrlAltDel(long photKeyCtrlAltDel)
		{
			return Outer()->SetProperty(Outer()->m_HotKeyCtrlAltDel, photKeyCtrlAltDel);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyCtrlAltDel(long * photKeyCtrlAltDel) const
		{
			return Outer()->GetProperty(Outer()->m_HotKeyCtrlAltDel, photKeyCtrlAltDel);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_orderDrawThreshold(long porderDrawThreshold)
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_orderDrawThreshold(long * porderDrawThreshold) const
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapCacheSize(long pbitmapCacheSize)
		{
			// NOTE: the upper bound of "32" for a field with a default value of 1500 seems to be a bug
			if(pbitmapCacheSize < 0 || pbitmapCacheSize > 32)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_BitmapCacheSize, pbitmapCacheSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapCacheSize(long * pbitmapCacheSize) const
		{
			return Outer()->GetProperty(Outer()->m_BitmapCacheSize, pbitmapCacheSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapVirtualCacheSize(long pbitmapVirtualCacheSize)
		{
			if(pbitmapVirtualCacheSize < 0 || pbitmapVirtualCacheSize > 32)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_BitmapVirtualCacheSize, pbitmapVirtualCacheSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapVirtualCacheSize(long * pbitmapVirtualCacheSize) const
		{
			return Outer()->GetProperty(Outer()->m_BitmapVirtualCacheSize, pbitmapVirtualCacheSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ScaleBitmapCachesByBPP(long pbScale)
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_ScaleBitmapCachesByBPP(long * pbScale) const
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_NumBitmapCaches(long pnumBitmapCaches)
		{
			return Outer()->SetProperty(Outer()->m_NumBitmapCaches, pnumBitmapCaches);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_NumBitmapCaches(long * pnumBitmapCaches) const
		{
			return Outer()->GetProperty(Outer()->m_NumBitmapCaches, pnumBitmapCaches);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_CachePersistenceActive(long pcachePersistenceActive)
		{
			return Outer()->SetProperty(Outer()->m_CachePersistenceActive, pcachePersistenceActive);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_CachePersistenceActive(long * pcachePersistenceActive) const
		{
			return Outer()->GetProperty(Outer()->m_CachePersistenceActive, pcachePersistenceActive);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_PersistCacheDirectory(BSTR rhs)
		{
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_brushSupportLevel(long pbrushSupportLevel)
		{
			return Outer()->SetProperty(Outer()->m_brushSupportLevel, pbrushSupportLevel);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_brushSupportLevel(long * pbrushSupportLevel) const
		{
			return Outer()->GetProperty(Outer()->m_brushSupportLevel, pbrushSupportLevel);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_minInputSendInterval(long pminInputSendInterval)
		{
			// TODO
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_minInputSendInterval(long * pminInputSendInterval) const
		{
			// TODO
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_InputEventsAtOnce(long pinputEventsAtOnce)
		{
			// TODO
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_InputEventsAtOnce(long * pinputEventsAtOnce) const
		{
			// TODO
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_maxEventCount(long pmaxEventCount)
		{
			// TODO
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_maxEventCount(long * pmaxEventCount) const
		{
			// TODO
			return S_FALSE;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_keepAliveInterval(long pkeepAliveInterval)
		{
			if(pkeepAliveInterval && pkeepAliveInterval < 10)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_keepAliveInternal, pkeepAliveInterval);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_keepAliveInterval(long * pkeepAliveInterval) const
		{
			return Outer()->GetProperty(Outer()->m_keepAliveInternal, pkeepAliveInterval);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_shutdownTimeout(long pshutdownTimeout)
		{
			if(pshutdownTimeout >= 600)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_shutdownTimeout, pshutdownTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_shutdownTimeout(long * pshutdownTimeout) const
		{
			return Outer()->GetProperty(Outer()->m_shutdownTimeout, pshutdownTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_overallConnectionTimeout(long poverallConnectionTimeout)
		{
			if(poverallConnectionTimeout < 0 || poverallConnectionTimeout >= 600)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_overallConnectionTimeout, poverallConnectionTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_overallConnectionTimeout(long * poverallConnectionTimeout) const
		{
			return Outer()->GetProperty(Outer()->m_overallConnectionTimeout, poverallConnectionTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_singleConnectionTimeout(long psingleConnectionTimeout)
		{
			if(psingleConnectionTimeout >= 600)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_singleConnectionTimeout, psingleConnectionTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_singleConnectionTimeout(long * psingleConnectionTimeout) const
		{
			return Outer()->GetProperty(Outer()->m_singleConnectionTimeout, psingleConnectionTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_KeyboardType(long pkeyboardType)
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_KeyboardType(long * pkeyboardType) const
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_KeyboardSubType(long pkeyboardSubType)
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_KeyboardSubType(long * pkeyboardSubType) const
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_KeyboardFunctionKey(long pkeyboardFunctionKey)
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_KeyboardFunctionKey(long * pkeyboardFunctionKey) const
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_WinceFixedPalette(long pwinceFixedPalette)
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_WinceFixedPalette(long * pwinceFixedPalette) const
		{
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ConnectToServerConsole(VARIANT_BOOL pConnectToConsole)
		{
			return Outer()->SetProperty(Outer()->m_ConnectToServerConsole, pConnectToConsole);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_ConnectToServerConsole(VARIANT_BOOL * pConnectToConsole) const
		{
			return Outer()->GetProperty(Outer()->m_ConnectToServerConsole, pConnectToConsole);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapPersistence(long pbitmapPersistence)
		{
			return put_BitmapPeristence(pbitmapPersistence);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapPersistence(long * pbitmapPersistence) const
		{
			return get_BitmapPeristence(pbitmapPersistence);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_MinutesToIdleTimeout(long pminutesToIdleTimeout)
		{
			if(pminutesToIdleTimeout > 240)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_MinutesToIdleTimeout, pminutesToIdleTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_MinutesToIdleTimeout(long * pminutesToIdleTimeout) const
		{
			return Outer()->GetProperty(Outer()->m_MinutesToIdleTimeout, pminutesToIdleTimeout);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_SmartSizing(VARIANT_BOOL pfSmartSizing)
		{
			return Outer()->SetProperty(Outer()->m_SmartSizing, pfSmartSizing);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_SmartSizing(VARIANT_BOOL * pfSmartSizing) const
		{
			return Outer()->GetProperty(Outer()->m_SmartSizing, pfSmartSizing);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RdpdrLocalPrintingDocName(BSTR pLocalPrintingDocName)
		{
			return Outer()->SetProperty(Outer()->m_RdpdrLocalPrintingDocName, pLocalPrintingDocName);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RdpdrLocalPrintingDocName(BSTR * pLocalPrintingDocName) const
		{
			return Outer()->GetProperty(Outer()->m_RdpdrLocalPrintingDocName, pLocalPrintingDocName);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RdpdrClipCleanTempDirString(BSTR clipCleanTempDirString)
		{
			return Outer()->SetProperty(Outer()->m_RdpdrClipCleanTempDirString, clipCleanTempDirString);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RdpdrClipCleanTempDirString(BSTR * clipCleanTempDirString) const
		{
			return Outer()->GetProperty(Outer()->m_RdpdrClipCleanTempDirString, clipCleanTempDirString);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RdpdrClipPasteInfoString(BSTR clipPasteInfoString)
		{
			return Outer()->SetProperty(Outer()->m_RdpdrClipPasteInfoString, clipPasteInfoString);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RdpdrClipPasteInfoString(BSTR * clipPasteInfoString) const
		{
			return Outer()->GetProperty(Outer()->m_RdpdrClipPasteInfoString, clipPasteInfoString);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ClearTextPassword(BSTR rhs)
		{
			return Outer()->put_ClearTextPassword(rhs);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DisplayConnectionBar(VARIANT_BOOL pDisplayConnectionBar)
		{
			if(!pDisplayConnectionBar && Outer()->IsSafeForScripting())
				return E_FAIL;

			return Outer()->SetProperty(Outer()->m_DisplayConnectionBar, pDisplayConnectionBar);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DisplayConnectionBar(VARIANT_BOOL * pDisplayConnectionBar) const
		{
			return Outer()->GetProperty(Outer()->m_DisplayConnectionBar, pDisplayConnectionBar);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_PinConnectionBar(VARIANT_BOOL pPinConnectionBar)
		{
			if(Outer()->IsSafeForScripting())
				return E_NOTIMPL;

			return Outer()->SetProperty(Outer()->m_PinConnectionBar, pPinConnectionBar);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_PinConnectionBar(VARIANT_BOOL * pPinConnectionBar) const
		{
			return Outer()->GetProperty(Outer()->m_PinConnectionBar, pPinConnectionBar);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_GrabFocusOnConnect(VARIANT_BOOL pfGrabFocusOnConnect)
		{
			return Outer()->SetProperty(Outer()->m_GrabFocusOnConnect, pfGrabFocusOnConnect);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_GrabFocusOnConnect(VARIANT_BOOL * pfGrabFocusOnConnect) const
		{
			return Outer()->GetProperty(Outer()->m_GrabFocusOnConnect, pfGrabFocusOnConnect);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_LoadBalanceInfo(BSTR pLBInfo)
		{
			return Outer()->SetProperty(Outer()->m_LoadBalanceInfo, pLBInfo);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_LoadBalanceInfo(BSTR * pLBInfo) const
		{
			return Outer()->GetProperty(Outer()->m_LoadBalanceInfo, pLBInfo);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectDrives(VARIANT_BOOL pRedirectDrives)
		{
			return Outer()->SetProperty(Outer()->m_RedirectDrives, pRedirectDrives);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectDrives(VARIANT_BOOL * pRedirectDrives) const
		{
			return Outer()->GetProperty(Outer()->m_RedirectDrives, pRedirectDrives);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectPrinters(VARIANT_BOOL pRedirectPrinters)
		{
			return Outer()->SetProperty(Outer()->m_RedirectPrinters, pRedirectPrinters);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectPrinters(VARIANT_BOOL * pRedirectPrinters) const
		{
			return Outer()->GetProperty(Outer()->m_RedirectPrinters, pRedirectPrinters);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectPorts(VARIANT_BOOL pRedirectPorts)
		{
			return Outer()->SetProperty(Outer()->m_RedirectPorts, pRedirectPorts);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectPorts(VARIANT_BOOL * pRedirectPorts) const
		{
			return Outer()->GetProperty(Outer()->m_RedirectPorts, pRedirectPorts);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectSmartCards(VARIANT_BOOL pRedirectSmartCards)
		{
			return Outer()->SetProperty(Outer()->m_RedirectSmartCards, pRedirectSmartCards);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectSmartCards(VARIANT_BOOL * pRedirectSmartCards) const
		{
			return Outer()->GetProperty(Outer()->m_RedirectSmartCards, pRedirectSmartCards);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapVirtualCache16BppSize(long pBitmapVirtualCache16BppSize)
		{
			if(pBitmapVirtualCache16BppSize < 0 || pBitmapVirtualCache16BppSize > 32)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_BitmapVirtualCache16BppSize, pBitmapVirtualCache16BppSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapVirtualCache16BppSize(long * pBitmapVirtualCache16BppSize) const
		{
			return Outer()->GetProperty(Outer()->m_BitmapVirtualCache16BppSize, pBitmapVirtualCache16BppSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapVirtualCache24BppSize(long pBitmapVirtualCache24BppSize)
		{
			if(pBitmapVirtualCache24BppSize < 0 || pBitmapVirtualCache24BppSize > 32)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_BitmapVirtualCache24BppSize, pBitmapVirtualCache24BppSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapVirtualCache24BppSize(long * pBitmapVirtualCache24BppSize) const
		{
			return Outer()->GetProperty(Outer()->m_BitmapVirtualCache24BppSize, pBitmapVirtualCache24BppSize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_PerformanceFlags(long pDisableList)
		{
			return Outer()->SetProperty(Outer()->m_PerformanceFlags, pDisableList);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_PerformanceFlags(long * pDisableList) const
		{
			return Outer()->GetProperty(Outer()->m_PerformanceFlags, pDisableList);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ConnectWithEndpoint(VARIANT * rhs)
		{
			// TBD? the Microsoft client implements this, but what does it mean?
			return E_NOTIMPL;
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_NotifyTSPublicKey(VARIANT_BOOL pfNotify)
		{
			return Outer()->SetProperty(Outer()->m_NotifyTSPublicKey, pfNotify);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_NotifyTSPublicKey(VARIANT_BOOL * pfNotify) const
		{
			return Outer()->GetProperty(Outer()->m_NotifyTSPublicKey, pfNotify);
		}

		/* IMsRdpClientAdvancedSettings2 */
		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::get_CanAutoReconnect(VARIANT_BOOL * pfCanAutoReconnect) const
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::put_EnableAutoReconnect(VARIANT_BOOL pfEnableAutoReconnect)
		{
			return Outer()->SetProperty(Outer()->m_EnableAutoReconnect, pfEnableAutoReconnect);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::get_EnableAutoReconnect(VARIANT_BOOL * pfEnableAutoReconnect) const
		{
			return Outer()->GetProperty(Outer()->m_EnableAutoReconnect, pfEnableAutoReconnect);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::put_MaxReconnectAttempts(long pMaxReconnectAttempts)
		{
			if(pMaxReconnectAttempts < 0 || pMaxReconnectAttempts > 200)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_MaxReconnectAttempts, pMaxReconnectAttempts);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::get_MaxReconnectAttempts(long * pMaxReconnectAttempts) const
		{
			return Outer()->GetProperty(Outer()->m_MaxReconnectAttempts, pMaxReconnectAttempts);
		}

		/* IMsRdpClientAdvancedSettings3 */
		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::put_ConnectionBarShowMinimizeButton(VARIANT_BOOL pfShowMinimize)
		{
			return Outer()->SetProperty(Outer()->m_ConnectionBarShowMinimizeButton, pfShowMinimize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::get_ConnectionBarShowMinimizeButton(VARIANT_BOOL * pfShowMinimize) const
		{
			return Outer()->GetProperty(Outer()->m_ConnectionBarShowMinimizeButton, pfShowMinimize);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::put_ConnectionBarShowRestoreButton(VARIANT_BOOL pfShowRestore)
		{
			return Outer()->SetProperty(Outer()->m_ConnectionBarShowRestoreButton, pfShowRestore);
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::get_ConnectionBarShowRestoreButton(VARIANT_BOOL * pfShowRestore) const
		{
			return Outer()->GetProperty(Outer()->m_ConnectionBarShowRestoreButton, pfShowRestore);
		}

		/* IMsRdpClientAdvancedSettings4 */
        virtual STDMETHODIMP IMsRdpClientAdvancedSettings4::put_AuthenticationLevel(unsigned int puiAuthLevel)
		{
			// TODO: this isn't implemented in rdesktop yet...
			return Outer()->SetProperty(Outer()->m_AuthenticationLevel, puiAuthLevel);
		}

        virtual STDMETHODIMP IMsRdpClientAdvancedSettings4::get_AuthenticationLevel(unsigned int * puiAuthLevel) const
		{
			return Outer()->GetProperty(Outer()->m_AuthenticationLevel, puiAuthLevel);
		}
	}
	m_advancedSettings;

	template<class Interface> HRESULT GetAdvancedSettings(Interface ** ppAdvSettings)
	{
		addRef();
		*ppAdvSettings = &m_advancedSettings;
		return S_OK;
	}

	/* Secured settings wrapper */
	friend class SecuredSettings;

	class SecuredSettings SEALED_: public MSTSCLib::IMsRdpClientSecuredSettings
	{
	private:
		RdpClient * Outer()
		{
			return InnerToOuter(this);
		}

		const RdpClient * Outer() const
		{
			return InnerToOuter(this);
		}

		/* IDispatch type information */
		ITypeInfo * m_dispTypeInfo;

		HRESULT LoadDispTypeInfo()
		{
			if(m_dispTypeInfo)
				return S_OK;

			HRESULT hr = Outer()->LoadTypeLibrary();

			if(FAILED(hr))
				return hr;

			assert(MSTSCLib::IID_IMsRdpClientSecuredSettings == MSTSCLib_Redist::IID_IMsRdpClientSecuredSettings);

			hr = Outer()->m_typeLib->GetTypeInfoOfGuid(MSTSCLib::IID_IMsRdpClientSecuredSettings, &m_dispTypeInfo);

			if(FAILED(hr))
				return hr;

			assert(m_dispTypeInfo);
			return S_OK;
		}

		HRESULT AcquireDispTypeInfo(ITypeInfo ** ppTI)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			m_dispTypeInfo->AddRef();
			*ppTI = m_dispTypeInfo;
			return S_OK;
		}

	public:
		~SecuredSettings()
		{
			if(m_dispTypeInfo)
				m_dispTypeInfo->Release();
		}

		/* IUnknown */
		virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
		{
			using namespace MSTSCLib;

			if
			(
				riid == IID_IUnknown ||
				riid == IID_IDispatch ||
				riid == IID_IMsTscSecuredSettings ||
				riid == IID_IMsRdpClientSecuredSettings
			)
			{
				*ppvObject = this;
				Outer()->addRef();
				return S_OK;
			}
			else
			{
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
		{
			return Outer()->addRef();
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::Release()
		{
			return Outer()->release();
		}

		/* IDispatch */
		virtual STDMETHODIMP IDispatch::GetTypeInfoCount(UINT * pctinfo)
		{
			*pctinfo = 1;
			return S_OK;
		}

		virtual STDMETHODIMP IDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
		{
			if(iTInfo != 0)
				return DISP_E_BADINDEX;

			return AcquireDispTypeInfo(ppTInfo);
		}

		virtual STDMETHODIMP IDispatch::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			return m_dispTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
		}

		virtual STDMETHODIMP IDispatch::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
		{
			return m_dispTypeInfo->Invoke
			(
				static_cast<MSTSCLib::IMsRdpClientSecuredSettings *>(this),
				dispIdMember,
				wFlags,
				pDispParams,
				pVarResult,
				pExcepInfo,
				puArgErr
			);
		}

		/* IMsTscSecuredSettings */
		virtual STDMETHODIMP IMsTscSecuredSettings::put_StartProgram(BSTR pStartProgram)
		{
			return Outer()->SetProperty(Outer()->m_StartProgram, pStartProgram);
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::get_StartProgram(BSTR * pStartProgram) const
		{
			return Outer()->GetProperty(Outer()->m_StartProgram, pStartProgram);
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::put_WorkDir(BSTR pWorkDir)
		{
			return Outer()->SetProperty(Outer()->m_WorkDir, pWorkDir);
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::get_WorkDir(BSTR * pWorkDir) const
		{
			return Outer()->GetProperty(Outer()->m_WorkDir, pWorkDir);
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::put_FullScreen(long pfFullScreen)
		{
			return Outer()->put_FullScreen(!!pfFullScreen);
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::get_FullScreen(long * pfFullScreen) const
		{
			return Outer()->GetProperty(Outer()->m_FullScreen, pfFullScreen);
		}

		/* IMsRdpClientSecuredSettings */
		virtual STDMETHODIMP IMsRdpClientSecuredSettings::put_KeyboardHookMode(long pkeyboardHookMode)
		{
			if(pkeyboardHookMode < 0 || pkeyboardHookMode > 2)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_KeyboardHookMode, pkeyboardHookMode);
		}

		virtual STDMETHODIMP IMsRdpClientSecuredSettings::get_KeyboardHookMode(long * pkeyboardHookMode) const
		{
			return Outer()->GetProperty(Outer()->m_KeyboardHookMode, pkeyboardHookMode);
		}

		virtual STDMETHODIMP IMsRdpClientSecuredSettings::put_AudioRedirectionMode(long pAudioRedirectionMode)
		{
			if(pAudioRedirectionMode < 0 || pAudioRedirectionMode > 2)
				return E_INVALIDARG;

			return Outer()->SetProperty(Outer()->m_AudioRedirectionMode, pAudioRedirectionMode);
		}

		virtual STDMETHODIMP IMsRdpClientSecuredSettings::get_AudioRedirectionMode(long * pAudioRedirectionMode) const
		{
			return Outer()->GetProperty(Outer()->m_AudioRedirectionMode, pAudioRedirectionMode);
		}
	}
	m_securedSettings;

	template<class Interface> HRESULT GetSecuredSettings(Interface ** ppSecuredSettings)
	{
		if(!m_SecuredSettingsEnabled)
			return E_FAIL;

		addRef();
		*ppSecuredSettings = &m_securedSettings;
		return S_OK;
	}

	/* Type library loading */
	HRESULT LoadTypeLibrary()
	{
		if(m_typeLib)
			return S_OK;

		// Get the DLL name of the ActiveX control
		WCHAR szPath[MAX_PATH + 1];
		DWORD cchPathLen = GetModuleFileNameW(GetCurrentModule(), szPath, ARRAYSIZE(szPath) - 1);

		if(cchPathLen == 0)
			return HRESULT_FROM_WIN32(GetLastError());

		if(cchPathLen > ((ARRAYSIZE(szPath) - 1) - 2))
			return E_FAIL;

		// Append the resource id of the type library
		assert(m_typeLibIndex < 10);

		szPath[cchPathLen + 0] = L'\\';
		szPath[cchPathLen + 1] = static_cast<WCHAR>(L'0' + m_typeLibIndex);
		szPath[cchPathLen + 2] = 0;

		// Load the type library
		HRESULT hr = LoadTypeLibEx(szPath, REGKIND_NONE, &m_typeLib);

		if(FAILED(hr))
			return hr;

		assert(m_typeLib);
		return S_OK;
	}

	/* IDispatch type information */
	HRESULT LoadDispTypeInfo()
	{
		if(m_dispTypeInfo)
			return S_OK;

		HRESULT hr = LoadTypeLibrary();

		if(FAILED(hr))
			return hr;

		assert(MSTSCLib::IID_IMsRdpClient4 == MSTSCLib_Redist::IID_IMsRdpClient4);

		hr = m_typeLib->GetTypeInfoOfGuid(MSTSCLib::IID_IMsRdpClient4, &m_dispTypeInfo);

		if(FAILED(hr))
			return hr;

		assert(m_dispTypeInfo);
		return S_OK;
	}

	HRESULT AcquireDispTypeInfo(ITypeInfo ** ppTI)
	{
		HRESULT hr = LoadDispTypeInfo();

		if(FAILED(hr))
			return hr;

		m_dispTypeInfo->AddRef();
		*ppTI = m_dispTypeInfo;
		return S_OK;
	}

public:
	/* Helpers for our various embedded children */
	static RdpClient * InnerToOuter(RdpClientInner * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_inner);
	}

	static RdpClient * InnerToOuter(AdvancedSettings * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_advancedSettings);
	}

	static RdpClient * InnerToOuter(SecuredSettings * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_securedSettings);
	}

	static RdpClient * InnerToOuter(RDPCLIENT * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_protocolState);
	}

	static const RdpClient * InnerToOuter(const RdpClientInner * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_inner);
	}

	static const RdpClient * InnerToOuter(const AdvancedSettings * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_advancedSettings);
	}

	static const RdpClient * InnerToOuter(const SecuredSettings * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_securedSettings);
	}

	static const RdpClient * InnerToOuter(const RDPCLIENT * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_protocolState);
	}

	RdpClientUI * GetUI() const
	{
		assert(m_clientUI);
		return m_clientUI;
	}

	/* Glue for rdesktop-core */
public:
	static bool OnPublicKey(RDPCLIENT * This, unsigned char * key, unsigned int key_size)
	{
		return InnerToOuter(This)->OnPublicKey(key, key_size);
	}

	static void OnLogon(RDPCLIENT * This)
	{
		return InnerToOuter(This)->OnLogon();
	}

	static bool OnRedirect
	(
		RDPCLIENT * This,
		uint32 flags,
		uint32 server_len,
		wchar_t * server,
		uint32 cookie_len,
		char * cookie,
		uint32 username_len,
		wchar_t * username,
		uint32 domain_len,
		wchar_t * domain,
		uint32 password_len,
		wchar_t * password
	)
	{
		return InnerToOuter(This)->OnRedirect
		(
			flags,
			server_len,
			server,
			cookie_len,
			cookie,
			username_len,
			username,
			domain_len,
			domain,
			password_len,
			password
		);
	}

private:
	bool OnPublicKey(unsigned char * key, unsigned int key_size)
	{
		HRESULT hr = FireReceivedTSPublicKey(key, key_size);

		if(FAILED(hr))
		{
			m_protocolState.disconnect_reason = 262;
			return false;
		}

		return hr == S_OK;
	}

	void OnLogon()
	{
		m_loggedIn = true;
		FireLoginComplete();
	}

	bool OnRedirect
	(
		uint32 flags,
		uint32 server_len,
		wchar_t * server,
		uint32 cookie_len,
		char * cookie,
		uint32 username_len,
		wchar_t * username,
		uint32 domain_len,
		wchar_t * domain,
		uint32 password_len,
		wchar_t * password
	)
	{
		assert(m_Connected);
		assert(!InsideApartment());
		assert(IsWindow(m_controlWindow));

		RedirectArguments redirectArgs =
		{
			flags,
			server_len,
			server,
			cookie_len,
			cookie,
			username_len,
			username,
			domain_len,
			domain,
			password_len,
			password
		};

		return SendMessage(m_controlWindow, RDPC_WM_REDIRECT, 0, reinterpret_cast<LPARAM>(&redirectArgs)) == 0;
	}

private:
	static DWORD WINAPI ProtocolLoopThreadProc(LPVOID lpParam)
	{
		static_cast<RdpClient *>(lpParam)->ProtocolLoop();
		return 0;
	}

	static VOID CALLBACK ConnectionTimerAPC(LPVOID, DWORD, DWORD)
	{
	}

	// FIXME: various potential inconsistencies due to lack of detailed documentation of expected semantics
	void ProtocolLoop()
	{
		HANDLE waitingReconnection = NULL;

		// Retrieve the local hostname to be passed to the server
		WCHAR hostname[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD hostnameLen = ARRAYSIZE(hostname);

		if(!GetComputerNameW(hostname, &hostnameLen))
			hostname[0] = 0;

		// Set some connection flags
		uint32 flags = RDP_LOGON_NORMAL;

		if(m_Compress)
			flags |= RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2;

		if(m_AudioRedirectionMode == 1)
			flags |= RDP_LOGON_LEAVE_AUDIO;

		if(m_ClearTextPassword)
			flags |= RDP_LOGON_AUTO;

		// Notify the container that the connection process is beginning now
		FireConnecting();

		// Set the overall connection timer, if a timeout is set
		// BUGBUG: the timeout semantics are ambiguous and have been most probably misinterpreted
		HANDLE overallConnectionTimer = NULL;
		LARGE_INTEGER overallTimeout;

		if(m_overallConnectionTimeout)
		{
			overallTimeout.QuadPart = - ((m_overallConnectionTimeout * 1000 * 1000 * 1000) / 100);

			overallConnectionTimer = CreateWaitableTimer(NULL, FALSE, NULL);

			if(overallConnectionTimer == NULL)
				goto l_Disconnect;
		}

		if(overallConnectionTimer)
			SetWaitableTimer(overallConnectionTimer, &overallTimeout, 0, ConnectionTimerAPC, NULL, FALSE);

		// Initial connection
		BOOL disconnected = rdp_connect
		(
			&m_protocolState,
			m_Server,
			flags,
			m_UserName,
			m_Domain,
			m_ClearTextPassword,
			m_StartProgram,
			m_WorkDir,
			hostname,
			m_LoadBalanceInfo
		);

		if(overallConnectionTimer)
			CancelWaitableTimer(overallConnectionTimer);

		if(disconnected)
			goto l_Disconnect;

		// TODO: set the disconnect reason for every instance in which we abort the loop
		for(;;)
		{
			BOOL deactivated = False;
			uint32 extendedDisconnectReason = 0;

			m_actuallyConnected = true;

			// Notify the container of the successful connection
			FireConnected();

			// Main protocol loop
			m_loggedIn = false;
			rdp_main_loop(&m_protocolState, &deactivated, &extendedDisconnectReason);
			rdp_disconnect(&m_protocolState);

			m_actuallyConnected = false;

			// Redirection
			// BUGBUG: redirection is very messy and probably this implementation is not "canonical"
			if(m_protocolState.redirect)
			{
				m_protocolState.redirect = False;
				rdp_reset_state(&m_protocolState);

				// TODO: reset connection parameters
				// This has to be done in the main thread, so use SendMessage on the control window

				flags |= RDP_LOGON_AUTO;

				// retry
				continue;
			}

			// Disconnection
			m_ExtendedDisconnectReason = static_cast<MSTSCLib::ExtendedDisconnectReasonCode>(extendedDisconnectReason);

			// Clean disconnection
			if(deactivated)
				break;

			BOOL success;

			long autoReconnections = 0;
			long totalReconnections = 0;

			// Reconnection
			// BUGBUG: reconnection semantics may not be entirely accurate
			do
			{
				++ totalReconnections;

				// ask the container whether we should reconnect
				long reconnectMode = FireAutoReconnecting(m_protocolState.disconnect_reason, totalReconnections);

				// do not reconnect
				if(reconnectMode == MSTSCLib::autoReconnectContinueStop)
					goto l_Disconnect;

				// the container will reconnect or abort manually
				if(reconnectMode == MSTSCLib::autoReconnectContinueManual)
				{
					assert(!m_reconnectAborted);
					assert(m_protocolThreadWaitingReconnection == NULL);

					if(waitingReconnection == NULL)
					{
						waitingReconnection = CreateEvent(NULL, TRUE, FALSE, NULL);

						if(waitingReconnection == NULL)
							// TODO: fatal error
							goto l_Disconnect;
					}

					m_protocolThreadWaitingReconnection = waitingReconnection;

					WaitForSingleObject(waitingReconnection, INFINITE);

					m_protocolThreadWaitingReconnection = NULL;

					if(m_reconnectAborted)
					{
						// FIXME? do we set the disconnection status here?
						goto l_Disconnect;
					}
				}
				// reconnect automatically
				else
				{
					// automatic reconnection is disabled
					if(m_EnableAutoReconnect)
						break;

					// too many consecutive automatic reconnections
					if(autoReconnections == m_MaxReconnectAttempts)
						break;

					++ autoReconnections;
				}

				if(overallConnectionTimer)
					SetWaitableTimer(overallConnectionTimer, &overallTimeout, 0, ConnectionTimerAPC, NULL, FALSE);

				// Reconnection
				success = rdp_reconnect
				(
					&m_protocolState,
					m_Server,
					flags,
					m_UserName,
					m_Domain,
					m_ClearTextPassword,
					m_StartProgram,
					m_WorkDir,
					hostname,
					m_LoadBalanceInfo
				);

				if(overallConnectionTimer)
					CancelWaitableTimer(overallConnectionTimer);
			}
			while(!success);
		}

l_Disconnect:
		// Disconnected
		FireDisconnected(m_protocolState.disconnect_reason);

		if(overallConnectionTimer)
			CloseHandle(overallConnectionTimer);
	}

	void JoinProtocolThread()
	{
		assert(m_protocolThread);
		WaitForSingleObject(m_protocolThread, INFINITE);
		CloseHandle(m_protocolThread);
		m_protocolThread = NULL;
	}

	void TerminateProtocolThread()
	{
		assert(m_protocolThread);

		// wake it up if it's waiting for a manual reconnection
		if(m_protocolThreadWaitingReconnection)
		{
			assert(!m_reconnectAborted);
			m_reconnectAborted = true;
			SetEvent(m_protocolThreadWaitingReconnection);
		}
		// otherwise, attempt to interrupt any current blocking operation
		else
		{
			// shutdown(m_protocolState.tcp.sock, SD_BOTH); // TBD: maybe in the future?
			QueueUserAPC(DisconnectAPC, m_protocolThread, 0);
		}

		assert(m_protocolThreadWaitingReconnection == NULL);
	}

	void PerformDisconnect(long reason)
	{
		assert(InsideApartment());
		assert(m_Connected);

		// TODO: notify virtual channels

		// TODO: do any other disconnection work here...

		// Put the control in the disconnected state
		m_Connected = false;
		m_loggedIn = false;

		// Notify the container
		VARIANTARG arg = { };

		arg.vt = VT_I4;
		arg.lVal = reason;

		FireEventInsideApartment(4, &arg, 1);
	}

public:
	/* Startup initialization */
	static BOOL Startup()
	{
		if(!RdpClientUI::Startup())
			return FALSE;

		WNDCLASSEX wcex = { sizeof(wcex) };

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = ControlWindowProc;
		wcex.hInstance = GetCurrentModule();
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszClassName = TEXT("MissTosca_Control");

		return RegisterClassEx(&wcex);
	}

	static void Shutdown()
	{
		UnregisterClass(TEXT("MissTosca_Control"), GetCurrentModule());
	}

	/* Class factory */
	static HRESULT CreateInstance(REFCLSID rclsid, unsigned libIndex, IUnknown * punkOuter, REFIID riid, void ** ppObj)
	{
		RdpClient * obj = new RdpClient(rclsid, libIndex, punkOuter);

		if(obj == NULL)
			return E_OUTOFMEMORY;

		HRESULT hr = obj->m_inner.QueryInterface(riid, ppObj);

		if(FAILED(hr))
		{
			delete obj;
			return hr;
		}

		assert(obj->m_refCount == 1);
		assert(*ppObj != NULL);

		return S_OK;
	}

private:
	/* Connection point enumerator */
	class CEnumConnectionPoints: public IEnumConnectionPoints
	{
	private:
		LONG m_refCount;
		IConnectionPoint * m_cp;
		bool m_done;

	public:
		CEnumConnectionPoints(IConnectionPoint * cp): m_refCount(1), m_cp(cp), m_done(false)
		{
			assert(m_cp);
			m_cp->AddRef();
		}

		CEnumConnectionPoints(const CEnumConnectionPoints& ecp): m_refCount(1), m_cp(ecp.m_cp), m_done(ecp.m_done)
		{
			assert(m_cp);
			m_cp->AddRef();
		}

		~CEnumConnectionPoints()
		{
			assert(m_cp);
			m_cp->Release();
		}

		virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject)
		{
			if(riid == IID_IUnknown || riid == IID_IEnumConnectionPoints)
			{
				*ppvObject = this;
				return S_OK;
			}
			else
			{
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
		}

		virtual STDMETHODIMP_(ULONG) AddRef()
		{
			return InterlockedIncrement(&m_refCount);
		}

		virtual STDMETHODIMP_(ULONG) Release()
		{
			LONG n = InterlockedDecrement(&m_refCount);

			if(n == 0)
				delete this;

			return n;
		}

		virtual STDMETHODIMP Next(ULONG cConnections, LPCONNECTIONPOINT * ppCP, ULONG * pcFetched)
		{
			if(cConnections == 0 || m_done)
				return S_FALSE;

			m_done = true;
			m_cp->AddRef();
			*ppCP = m_cp;
			*pcFetched = 1;

			return S_OK;
		}

		virtual STDMETHODIMP Skip(ULONG cConnections)
		{
			if(cConnections == 0)
				return S_OK;

			if(cConnections == 1 && !m_done)
			{
				m_done = true;
				return S_OK;
			}

			assert(cConnections > 1 || m_done);

			return S_FALSE;
		}

		virtual STDMETHODIMP Reset()
		{
			m_done = false;
			return S_OK;
		}

		virtual STDMETHODIMP Clone(IEnumConnectionPoints ** ppEnum)
		{
			if(ppEnum == NULL)
				return E_POINTER;

			*ppEnum = new CEnumConnectionPoints(*this);

			if(*ppEnum == NULL)
				return E_OUTOFMEMORY;

			return S_OK;
		}
	};

	/* Pay no attention, ActiveX glue... */
	LRESULT ControlWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_SIZE:
			{
				// TODO: resize UI
			}

			return 0;

		case WM_PAINT:
			{
				LPCWSTR text = NULL;

				if(!m_Connected)
					text = m_DisconnectedText;
				else if(m_actuallyConnected)
					text = m_ConnectedStatusText;
				else
					text = m_ConnectingText;

				RECT clientRect;
				GetClientRect(m_controlWindow, &clientRect);

				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(m_controlWindow, &ps);

				SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
				SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
				SelectObject(hdc, GetStockObject(SYSTEM_FONT));

				RECT textRect = clientRect;

				DrawTextW
				(
					hdc,
					text,
					-1,
					&textRect,
					DT_CENTER | DT_EDITCONTROL | DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT
				);

				if(textRect.right > clientRect.right)
					textRect.right = clientRect.right;

				if(textRect.bottom > clientRect.bottom)
					textRect.bottom = clientRect.bottom;

				textRect.left = (clientRect.right - textRect.right) / 2;
				textRect.right += textRect.left;
				textRect.top = (clientRect.bottom - textRect.bottom) / 2;
				textRect.bottom += textRect.top;

				DrawTextW
				(
					hdc,
					text,
					-1,
					&textRect,
					DT_CENTER | DT_EDITCONTROL | DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK
				);

				EndPaint(m_controlWindow, &ps);
			}

			return 0;

		default:
			{
				LRESULT result;

				if(HandleEvent(uMsg, wParam, lParam, result))
					return result;
			}

			break;
		}

		return DefWindowProc(m_controlWindow, uMsg, wParam, lParam);
	}

	static LRESULT CALLBACK ControlWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg == WM_CREATE)
		{
			SetWindowLongPtr
			(
				hwnd,
				GWLP_USERDATA,
				(LONG_PTR)reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams
			);
		}

		RdpClient * Self = reinterpret_cast<RdpClient *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		assert(Self);

		return Self->ControlWindowProc(uMsg, wParam, lParam);
	}

	HRESULT CreateControlWindow(HWND hwndParent)
	{
		m_controlWindow = CreateWindow
		(
			TEXT("MissTosca_Control"),
			NULL,
			WS_CHILD | WS_CLIPCHILDREN,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			hwndParent,
			NULL,
			GetCurrentModule(),
			this
		);

		if(m_controlWindow == NULL)
			return HRESULT_FROM_WIN32(GetLastError());

		m_UIParentWindowHandle = m_controlWindow;
		return S_OK;
	}

	HRESULT DestroyControlWindow()
	{
		if(m_controlWindow == NULL)
			return S_FALSE;

		HWND controlWindow = NULL;
		std::swap(controlWindow, m_controlWindow);
		DestroyWindow(controlWindow);
		return S_OK;
	}

	HRESULT Activate(LONG iVerb, IOleClientSite * pActiveSite, HWND hwndParent, LPCRECT lprcPosRect)
	{
		if(pActiveSite == NULL)
			pActiveSite = m_clientSite;

		if(pActiveSite == NULL)
			return E_FAIL;

		// TODO: store this until we are closed or deactivated
		IOleInPlaceSite * site;

		HRESULT hr = pActiveSite->QueryInterface(&site);

		if(FAILED(hr))
			return hr;

		IOleInPlaceFrame * frame = NULL;
		IOleInPlaceUIWindow * uiWindow = NULL;

		for(;;)
		{
			hr = site->CanInPlaceActivate();

			if(hr == S_FALSE)
				hr = E_FAIL;

			if(FAILED(hr))
				break;

			site->OnInPlaceActivate();

			if(hwndParent == NULL)
			{
				hr = site->GetWindow(&hwndParent);

				if(FAILED(hr))
					break;
			}

			RECT rcPos;
			RECT rcClip;
			OLEINPLACEFRAMEINFO frameInfo = { sizeof(frameInfo) };

			site->GetWindowContext(&frame, &uiWindow, &rcPos, &rcClip, &frameInfo);

			if(lprcPosRect == NULL)
				lprcPosRect = &rcPos;

			if(m_controlWindow)
				ShowWindow(m_controlWindow, SW_SHOW);
			else
			{
				hr = CreateControlWindow(hwndParent);

				if(FAILED(hr))
					break;
			}

			SetObjectRects(lprcPosRect, &rcClip);

			// UI activation
			if((iVerb == OLEIVERB_PRIMARY || iVerb == OLEIVERB_UIACTIVATE) && !m_uiActive)
			{
				m_uiActive = true;

				hr = site->OnUIActivate();

				if(FAILED(hr))
					break;

				SetWindowPos
				(
					m_controlWindow,
					NULL,
					lprcPosRect->left,
					lprcPosRect->top,
					lprcPosRect->right - lprcPosRect->left,
					lprcPosRect->bottom - lprcPosRect->top,
					SWP_SHOWWINDOW
				);

				if(frame)
				{
					frame->SetActiveObject(this, NULL);
					frame->SetBorderSpace(NULL);
				}

				if(uiWindow)
				{
					uiWindow->SetActiveObject(this, NULL);
					uiWindow->SetBorderSpace(NULL);
				}
			}

			break;
		}

		if(uiWindow)
			uiWindow->Release();

		if(frame)
			frame->Release();

		site->Release();

		if(SUCCEEDED(hr))
			pActiveSite->ShowObject();

		return hr;
	}

public:
	/* IUnknown */
	/*
		NOTE: this is the delegating implementation, to support aggregation. The actual
		implementation is RdpClientInner, above
	*/
	virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
	{
		return m_punkOuter->QueryInterface(riid, ppvObject);
	}

	virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
	{
		return m_punkOuter->AddRef();
	}

	virtual STDMETHODIMP_(ULONG) IUnknown::Release()
	{
		return m_punkOuter->Release();
	}

	/* IDispatch */
	virtual STDMETHODIMP IDispatch::GetTypeInfoCount(UINT * pctinfo)
	{
		*pctinfo = 1;
		return S_OK;
	}

	virtual STDMETHODIMP IDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
	{
		if(iTInfo != 0)
			return DISP_E_BADINDEX;

		return AcquireDispTypeInfo(ppTInfo);
	}

	virtual STDMETHODIMP IDispatch::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
	{
		HRESULT hr = LoadDispTypeInfo();

		if(FAILED(hr))
			return hr;

		return m_dispTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
	}

	virtual STDMETHODIMP IDispatch::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
	{
		HRESULT hr = LoadDispTypeInfo();

		if(FAILED(hr))
			return hr;

		return m_dispTypeInfo->Invoke
		(
			static_cast<MSTSCLib::IMsRdpClient4 *>(this),
			dispIdMember,
			wFlags,
			pDispParams,
			pVarResult,
			pExcepInfo,
			puArgErr
		);
	}

	/* IConnectionPoint */
	virtual STDMETHODIMP GetConnectionInterface(IID * pIID)
	{
		if(pIID == NULL)
			return E_POINTER;

		*pIID = MSTSCLib::DIID_IMsTscAxEvents;
		return S_OK;
	}

	virtual STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer ** ppCPC)
	{
		if(ppCPC == NULL)
			return E_POINTER;

		addRef();
		*ppCPC = this;
		return S_OK;
	}

	virtual STDMETHODIMP Advise(IUnknown * pUnkSink, DWORD * pdwCookie)
	{
		MSTSCLib::IMsTscAxEvents * sink;

		if(FAILED(pUnkSink->QueryInterface(&sink)))
			return CONNECT_E_CANNOTCONNECT;

		MSTSCLib::IMsTscAxEvents ** sinks = GetSinks();
		DWORD cookie = 0;

		if(m_EventSinksCount)
		{
			bool found = false;

			for(size_t i = 0; i < m_EventSinksCount; ++ i)
			{
				found = (sinks[i] == NULL);

				if(found)
				{
					cookie = static_cast<DWORD>(i);
					break;
				}
			}

			if(!found)
			{
				MSTSCLib::IMsTscAxEvents ** newSinks = new MSTSCLib::IMsTscAxEvents *[m_EventSinksCount + 1];

				if(newSinks == NULL)
				{
					sink->Release();
					return E_OUTOFMEMORY;
				}

				std::copy(sinks, sinks + m_EventSinksCount, newSinks);

				m_EventSinks = newSinks;
				sinks = newSinks;

				cookie = static_cast<DWORD>(m_EventSinksCount);
			}
		}

		sinks[cookie] = sink;
		*pdwCookie = cookie;

		return S_OK;
	}

	virtual STDMETHODIMP Unadvise(DWORD dwCookie)
	{
		MSTSCLib::IMsTscAxEvents ** sinks = GetSinks();

		if(dwCookie >= m_EventSinksCount || sinks[dwCookie] == NULL)
			return CONNECT_E_NOCONNECTION;

		sinks[dwCookie]->Release();
		sinks[dwCookie] = NULL;

		// BUGBUG: the array currently grows forever. Trim it whenever possible

		return S_OK;
	}

	virtual STDMETHODIMP EnumConnections(IEnumConnections ** ppEnum)
	{
		// I see no real value in this
		return E_NOTIMPL;
	}

	/* IConnectionPointContainer */
	virtual STDMETHODIMP IConnectionPointContainer::EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
	{
		*ppEnum = new CEnumConnectionPoints(this);

		if(*ppEnum == NULL)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	virtual STDMETHODIMP IConnectionPointContainer::FindConnectionPoint(REFIID riid, IConnectionPoint ** ppCP)
	{
		if(riid != MSTSCLib::DIID_IMsTscAxEvents)
			return CONNECT_E_NOCONNECTION;

		addRef();
		*ppCP = this;

		return S_OK;
	}

	/* IDataObject */ // 0/9
	virtual STDMETHODIMP IDataObject::GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::GetDataHere(FORMATETC * pformatetc, STGMEDIUM * pmedium)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::QueryGetData(FORMATETC * pformatetc)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::GetCanonicalFormatEtc(FORMATETC * pformatectIn, FORMATETC * pformatetcOut)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::SetData(FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::DAdvise(FORMATETC * pformatetc, DWORD advf, IAdviseSink * pAdvSink, DWORD * pdwConnection)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::DUnadvise(DWORD dwConnection)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
	{
		return E_NOTIMPL;
	}

	/* IObjectSafety */
	virtual STDMETHODIMP IObjectSafety::GetInterfaceSafetyOptions(REFIID riid, DWORD * pdwSupportedOptions, DWORD * pdwEnabledOptions)
	{
		if(pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
			return E_POINTER;

		if(riid != IID_IDispatch)
			return E_NOINTERFACE;

		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		*pdwEnabledOptions = m_SafetyOptions;
		return S_OK;
	}

	virtual STDMETHODIMP IObjectSafety::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
	{
		if(riid != IID_IDispatch)
			return E_NOINTERFACE;

		m_SafetyOptions = dwEnabledOptions & (dwOptionSetMask & INTERFACESAFE_FOR_UNTRUSTED_CALLER);
		return S_OK;
	}

	/* IOleControl */ // 3/4
	virtual STDMETHODIMP IOleControl::GetControlInfo(CONTROLINFO * pCI)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleControl::OnMnemonic(MSG * pMsg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleControl::OnAmbientPropertyChange(DISPID dispID)
	{
		return S_OK;
	}

	virtual STDMETHODIMP IOleControl::FreezeEvents(BOOL bFreeze)
	{
		if(bFreeze)
			InterlockedIncrement(&m_freezeEvents);
		else if(InterlockedDecrement(&m_freezeEvents) == 0)
			UnfreezeEvents();

		return S_OK;
	}

	/* IOleInPlaceActiveObject */ // 3/5
	virtual STDMETHODIMP IOleInPlaceActiveObject::TranslateAccelerator(LPMSG lpmsg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::OnFrameWindowActivate(BOOL fActivate)
	{
		// TODO
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::OnDocWindowActivate(BOOL fActivate)
	{
		// TODO
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fFrameWindow)
	{
		return S_OK;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::EnableModeless(BOOL fEnable)
	{
		return S_OK;
	}

	/* IOleInPlaceObject */ // 1/4
	virtual STDMETHODIMP IOleInPlaceObject::InPlaceDeactivate()
	{
		// TODO: UIDeactivate, destroy window, inplacesite->OnInPlaceDeactivate
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceObject::UIDeactivate()
	{
		// TODO
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceObject::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
	{
		if(m_controlWindow == NULL)
			return E_FAIL;

		MoveWindow
		(
			m_controlWindow,
			lprcPosRect->left,
			lprcPosRect->top,
			lprcPosRect->right - lprcPosRect->left,
			lprcPosRect->bottom - lprcPosRect->top,
			TRUE
		);

		SetWindowRgn(m_controlWindow, CreateRectRgnIndirect(lprcClipRect), TRUE);

		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceObject::ReactivateAndUndo()
	{
		return E_NOTIMPL;
	}

	/* IOleObject */ // 18/21
	virtual STDMETHODIMP IOleObject::SetClientSite(IOleClientSite * pClientSite)
	{
		if(m_clientSite)
			m_clientSite->Release();

		m_clientSite = pClientSite;

		if(m_clientSite)
			m_clientSite->AddRef();

		return S_OK;
	}

	virtual STDMETHODIMP IOleObject::GetClientSite(IOleClientSite ** ppClientSite)
	{
		if(ppClientSite == NULL)
			return E_POINTER;

		if(m_clientSite)
			m_clientSite->AddRef();

		*ppClientSite = m_clientSite;
		return S_OK;
	}

	virtual STDMETHODIMP IOleObject::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
	{
		return S_OK;
	}

	virtual STDMETHODIMP IOleObject::Close(DWORD dwSaveOption)
	{
		// TODO: deactivate, destroy window, release in-place site, release advise sink
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IOleObject::SetMoniker(DWORD dwWhichMoniker, IMoniker * pmk)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::InitFromData(IDataObject * pDataObject, BOOL fCreation, DWORD dwReserved)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetClipboardData(DWORD dwReserved, IDataObject ** ppDataObject)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite * pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
	{
		HRESULT hr;

		switch(iVerb)
		{
		case OLEIVERB_PRIMARY:
		case OLEIVERB_SHOW:
		case OLEIVERB_UIACTIVATE:
		case OLEIVERB_INPLACEACTIVATE:
			hr = S_OK;
			break;

		default:
			if(iVerb > 0)
				hr = OLEOBJ_S_INVALIDVERB;
			else
				hr = E_NOTIMPL;
		}

		if(FAILED(hr))
			return hr;

		HRESULT hrActivate = Activate(iVerb, pActiveSite, hwndParent, lprcPosRect);

		if(FAILED(hrActivate))
			hr = hrActivate;

		return hr;
	}

	virtual STDMETHODIMP IOleObject::EnumVerbs(IEnumOLEVERB ** ppEnumOleVerb)
	{
		return OleRegEnumVerbs(m_classId, ppEnumOleVerb);
	}

	virtual STDMETHODIMP IOleObject::Update()
	{
		return S_OK;
	}

	virtual STDMETHODIMP IOleObject::IsUpToDate()
	{
		return S_OK;
	}

	virtual STDMETHODIMP IOleObject::GetUserClassID(CLSID * pClsid)
	{
		*pClsid = m_classId;
		return S_OK;
	}

	virtual STDMETHODIMP IOleObject::GetUserType(DWORD dwFormOfType, LPOLESTR * pszUserType)
	{
		return OleRegGetUserType(m_classId, dwFormOfType, pszUserType);
	}

	virtual STDMETHODIMP IOleObject::SetExtent(DWORD dwDrawAspect, SIZEL * psizel)
	{
		// TODO: resize
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetExtent(DWORD dwDrawAspect, SIZEL * psizel)
	{
		// TODO: return size
		return E_NOTIMPL;
	}

	HRESULT NeedAdviseHolder()
	{
		if(m_adviseHolder)
			return S_OK;

		return CreateOleAdviseHolder(&m_adviseHolder);
	}

	virtual STDMETHODIMP IOleObject::Advise(IAdviseSink * pAdvSink, DWORD * pdwConnection)
	{
		HRESULT hr = NeedAdviseHolder();

		if(FAILED(hr))
			return hr;

		return m_adviseHolder->Advise(pAdvSink, pdwConnection);
	}

	virtual STDMETHODIMP IOleObject::Unadvise(DWORD dwConnection)
	{
		HRESULT hr = NeedAdviseHolder();

		if(FAILED(hr))
			return hr;

		return m_adviseHolder->Unadvise(dwConnection);
	}

	virtual STDMETHODIMP IOleObject::EnumAdvise(IEnumSTATDATA ** ppenumAdvise)
	{
		HRESULT hr = NeedAdviseHolder();

		if(FAILED(hr))
			return hr;

		return m_adviseHolder->EnumAdvise(ppenumAdvise);
	}

	virtual STDMETHODIMP IOleObject::GetMiscStatus(DWORD dwAspect, DWORD * pdwStatus)
	{
		return OleRegGetMiscStatus(m_classId, dwAspect, pdwStatus);
	}

	virtual STDMETHODIMP IOleObject::SetColorScheme(LOGPALETTE * pLogpal)
	{
		return E_NOTIMPL;
	}

	/* IOleWindow */
	virtual STDMETHODIMP IOleWindow::GetWindow(HWND * phwnd)
	{
		if(phwnd == NULL)
			return E_POINTER;

		if(m_controlWindow == NULL)
			return E_FAIL;

		*phwnd = m_controlWindow;
		return S_OK;
	}

	virtual STDMETHODIMP IOleWindow::ContextSensitiveHelp(BOOL fEnterMode)
	{
		return E_NOTIMPL;
	}

	/* IPersist */
	virtual STDMETHODIMP IPersist::GetClassID(CLSID * pClassID)
	{
		*pClassID = m_classId;
		return S_OK;
	}

	/* IPersistPropertyBag */ // 0/3
	virtual STDMETHODIMP IPersistPropertyBag::InitNew()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistPropertyBag::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistPropertyBag::Save(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
	{
		return E_NOTIMPL;
	}

	/* IPersistStorage */ // 0/6
	virtual STDMETHODIMP IPersistStorage::IsDirty()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::InitNew(IStorage * pStg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::Load(IStorage * pStg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::Save(IStorage * pStgSave, BOOL fSameAsLoad)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::SaveCompleted(IStorage * pStgNew)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::HandsOffStorage()
	{
		return E_NOTIMPL;
	}

	/* IPersistStreamInit */ // 0/5
	virtual STDMETHODIMP IPersistStreamInit::IsDirty()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::Load(LPSTREAM pStm)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::Save(LPSTREAM pStm, BOOL fClearDirty)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::GetSizeMax(ULARGE_INTEGER * pCbSize)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::InitNew()
	{
		return E_NOTIMPL;
	}

	/* IProvideClassInfo */
	virtual STDMETHODIMP IProvideClassInfo::GetClassInfo(ITypeInfo ** ppTI)
	{
		HRESULT hr = LoadTypeLibrary();

		if(FAILED(hr))
			return hr;

		return m_typeLib->GetTypeInfoOfGuid(m_classId, ppTI);
	}

	/* IProvideClassInfo2 */
	virtual STDMETHODIMP IProvideClassInfo2::GetGUID(DWORD dwGuidKind, GUID * pGUID)
	{
		if(dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
			return E_INVALIDARG;

		*pGUID = MSTSCLib::DIID_IMsTscAxEvents;
		return S_OK;
	}

	/* IQuickActivate */
	virtual STDMETHODIMP IQuickActivate::QuickActivate(QACONTAINER * pQaContainer, QACONTROL * pQaControl)
	{
		if(pQaContainer == NULL || pQaControl == NULL)
			return E_POINTER;

		if(pQaContainer->cbSize < sizeof(*pQaContainer) || pQaControl->cbSize < sizeof(*pQaControl))
			return E_INVALIDARG;

		ULONG cb = pQaControl->cbSize;
		ZeroMemory(pQaControl, cb);
		pQaControl->cbSize = cb;

		SetClientSite(pQaContainer->pClientSite);

		if(pQaContainer->pAdviseSink)
			SetAdvise(DVASPECT_CONTENT, 0, pQaContainer->pAdviseSink);

		if(pQaContainer->pUnkEventSink)
			Advise(pQaContainer->pUnkEventSink, &pQaControl->dwEventCookie);

		GetMiscStatus(DVASPECT_CONTENT, &pQaControl->dwMiscStatus);

		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IQuickActivate::SetContentExtent(LPSIZEL pSizel)
	{
		return SetExtent(DVASPECT_CONTENT, pSizel);
	}

	virtual STDMETHODIMP IQuickActivate::GetContentExtent(LPSIZEL pSizel)
	{
		return GetExtent(DVASPECT_CONTENT, pSizel);
	}

	/* IViewObject */ // 3/6
	virtual STDMETHODIMP IViewObject::Draw(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds, BOOL (STDMETHODCALLTYPE * pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IViewObject::GetColorSet(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LOGPALETTE ** ppColorSet)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::Freeze(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DWORD * pdwFreeze)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::Unfreeze(DWORD dwFreeze)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::SetAdvise(DWORD aspects, DWORD advf, IAdviseSink * pAdvSink)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IViewObject::GetAdvise(DWORD * pAspects, DWORD * pAdvf, IAdviseSink ** ppAdvSink)
	{
		return E_NOTIMPL; // TODO
	}

	/* IViewObject2 */ // 0/1
	virtual STDMETHODIMP IViewObject2::GetExtent(DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE * ptd, LPSIZEL lpsizel)
	{
		return E_NOTIMPL; // TODO
	}

	/* IMsTscAx */ // 23/30
	virtual STDMETHODIMP IMsTscAx::put_Server(BSTR pServer)
	{
		// FIXME: convert the hostname to Punycode, not the ANSI codepage
		return SetProperty(m_Server, pServer);
	}

	virtual STDMETHODIMP IMsTscAx::get_Server(BSTR * pServer) const
	{
		return GetProperty(m_Server, pServer);
	}

	virtual STDMETHODIMP IMsTscAx::put_Domain(BSTR pDomain)
	{
		return SetProperty(m_Domain, pDomain);
	}

	virtual STDMETHODIMP IMsTscAx::get_Domain(BSTR * pDomain) const
	{
		return GetProperty(m_Domain, pDomain);
	}

	virtual STDMETHODIMP IMsTscAx::put_UserName(BSTR pUserName)
	{
		return SetProperty(m_UserName, pUserName);
	}

	virtual STDMETHODIMP IMsTscAx::get_UserName(BSTR * pUserName) const
	{
		return GetProperty(m_UserName, pUserName);
	}

	virtual STDMETHODIMP IMsTscAx::put_DisconnectedText(BSTR pDisconnectedText)
	{
		return SetProperty(m_DisconnectedText, pDisconnectedText);
	}

	virtual STDMETHODIMP IMsTscAx::get_DisconnectedText(BSTR * pDisconnectedText) const
	{
		return GetProperty(m_DisconnectedText, pDisconnectedText);
	}

	virtual STDMETHODIMP IMsTscAx::put_ConnectingText(BSTR pConnectingText)
	{
		return SetProperty(m_ConnectingText, pConnectingText);
	}

	virtual STDMETHODIMP IMsTscAx::get_ConnectingText(BSTR * pConnectingText) const
	{
		return GetProperty(m_ConnectingText, pConnectingText);
	}

	virtual STDMETHODIMP IMsTscAx::get_Connected(short * pIsConnected) const
	{
		return GetProperty(m_Connected, pIsConnected);
	}

	virtual STDMETHODIMP IMsTscAx::put_DesktopWidth(long pVal)
	{
		if(pVal < 200 || pVal > 1600)
			return E_INVALIDARG;

		return SetProperty(m_DesktopWidth, pVal);
	}

	virtual STDMETHODIMP IMsTscAx::get_DesktopWidth(long * pVal) const
	{
		return GetProperty(m_DesktopWidth, pVal);
	}

	virtual STDMETHODIMP IMsTscAx::put_DesktopHeight(long pVal)
	{
		if(pVal < 200 || pVal > 1200)
			return E_INVALIDARG;

		return SetProperty(m_DesktopHeight, pVal);
	}

	virtual STDMETHODIMP IMsTscAx::get_DesktopHeight(long * pVal) const
	{
		return GetProperty(m_DesktopHeight, pVal);
	}

	virtual STDMETHODIMP IMsTscAx::put_StartConnected(long pfStartConnected)
	{
		return SetProperty(m_StartConnected, pfStartConnected);
	}

	virtual STDMETHODIMP IMsTscAx::get_StartConnected(long * pfStartConnected) const
	{
		return GetProperty(m_StartConnected, pfStartConnected);
	}

	virtual STDMETHODIMP IMsTscAx::get_HorizontalScrollBarVisible(long * pfHScrollVisible) const
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::get_VerticalScrollBarVisible(long * pfVScrollVisible) const
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::put_FullScreenTitle(BSTR rhs)
	{
		// TODO
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscAx::get_CipherStrength(long * pCipherStrength) const
	{
		if(pCipherStrength == NULL)
			return E_INVALIDARG;

		*pCipherStrength = 128; // BUGBUG: a later version may change this. Use a compile-time constant
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::get_Version(BSTR * pVersion) const
	{
		if(pVersion == NULL)
			return E_INVALIDARG;

		BSTR version = SysAllocString(L"5.2.3790.1830"); // BUGBUG: don't use hardcoded string

		if(version == NULL)
			return E_OUTOFMEMORY;

		*pVersion = version;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::get_SecuredSettingsEnabled(long * pSecuredSettingsEnabled) const
	{
		// TODO: initialize m_SecuredSettingsEnabled as soon as we have an OLE client site
		return GetProperty(m_SecuredSettingsEnabled, pSecuredSettingsEnabled);
	}

	virtual STDMETHODIMP IMsTscAx::get_SecuredSettings(MSTSCLib::IMsTscSecuredSettings ** ppSecuredSettings) const
	{
		return GetSecuredSettings(ppSecuredSettings);
	}

	virtual STDMETHODIMP IMsTscAx::get_AdvancedSettings(MSTSCLib::IMsTscAdvancedSettings ** ppAdvSettings) const
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	virtual STDMETHODIMP IMsTscAx::get_Debugger(MSTSCLib::IMsTscDebug ** ppDebugger) const
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscAx::Connect()
	{
		if(m_Connected)
		{
			// Protocol thread waiting for a manual reconnection: wake it up
			if(m_protocolThreadWaitingReconnection)
			{
				SetEvent(m_protocolThreadWaitingReconnection);
				return S_OK;
			}

			return E_FAIL;
		}

		m_Connected = true;

		HRESULT hr;

		if(m_controlWindow == NULL)
		{
			hr = CreateControlWindow(NULL);

			if(FAILED(hr))
				return hr;
		}

		for(;;)
		{
			// TODO: initialize plugin DLLs/channels

			m_clientUI = new RdpClientUI();

			if(m_clientUI == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			m_clientUI->Initialize(m_controlWindow);

			m_protocolState.licence_username = BstrToLpsz(m_UserName);

			if(m_protocolState.licence_username == NULL)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			DWORD dwSize = ARRAYSIZE(m_protocolState.licence_hostname);

			if(!GetComputerNameA(m_protocolState.licence_hostname, &dwSize))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				break;
			}

			// Keyboard layout
			// BUGBUG: not too sure about the semantics
			long keyboardLayout = -1;
			WCHAR * endPtr = NULL;

			if(m_KeyboardLayoutString)
				keyboardLayout = wcstol(m_KeyboardLayoutString, &endPtr, 0);

			// no keyboard layout specified or invalid keyboard layout: use current keyboard layout
			if(endPtr == NULL || *endPtr == 0 || keyboardLayout == -1)
				keyboardLayout = PtrToLong(GetKeyboardLayout(0)); // FIXME? use LOWORD()?

			m_protocolState.keylayout = keyboardLayout;

			// in case of failure, assume English (US)
			if(m_protocolState.keylayout == 0)
				m_protocolState.keylayout = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

			// Physical keyboard information
			m_protocolState.keyboard_type = GetKeyboardType(0);
			m_protocolState.keyboard_subtype = GetKeyboardType(1);
			m_protocolState.keyboard_functionkeys = GetKeyboardType(2);

			// in case of failure, assume an IBM Enhanced keyboard with 12 function keys
			if(m_protocolState.keyboard_type == 0 || m_protocolState.keyboard_functionkeys == 0)
			{
				m_protocolState.keyboard_type = 4;
				m_protocolState.keyboard_subtype = 0;
				m_protocolState.keyboard_functionkeys = 12;
			}

			// More initialization
			m_protocolState.width = m_DesktopWidth;
			m_protocolState.height = m_DesktopHeight;
			m_protocolState.server_depth = m_ColorDepth;
			m_protocolState.bitmap_compression = m_Compress ? TRUE : FALSE;
			m_protocolState.bitmap_cache = True; // TODO
			m_protocolState.bitmap_cache_persist_enable = False; // TODO
			m_protocolState.bitmap_cache_precache = True; // FIXME?
			m_protocolState.encryption = m_EncryptionEnabled ? TRUE : FALSE; // TBD: detect automatically
			m_protocolState.packet_encryption = m_EncryptionEnabled ? TRUE : FALSE;
			m_protocolState.desktop_save = True; // FIXME? tie to bitmap cache setting?
			m_protocolState.polygon_ellipse_orders = True;
			m_protocolState.use_rdp5 = True; // TBD: detect automatically
			m_protocolState.console_session = m_ConnectToServerConsole ? TRUE : FALSE;
			m_protocolState.rdp5_performanceflags = m_PerformanceFlags;
			m_protocolState.tcp_port_rdp = m_RDPPort;
			m_protocolState.rdp.current_status = 1;

			// TODO: cache tuning based on the provided parameters

			m_protocolState.cache.bmpcache_lru[0] = -1;
			m_protocolState.cache.bmpcache_lru[1] = -1;
			m_protocolState.cache.bmpcache_lru[2] = -1;
			m_protocolState.cache.bmpcache_mru[0] = -1;
			m_protocolState.cache.bmpcache_mru[1] = -1;
			m_protocolState.cache.bmpcache_mru[2] = -1;

			DWORD dwIgnore;
			m_protocolThread = CreateThread(NULL, 0, ProtocolLoopThreadProc, this, 0, &dwIgnore);

			hr = S_OK;
			break;
		}

		if(FAILED(hr))
			m_Connected = false;

		return hr;
	}

	virtual STDMETHODIMP IMsTscAx::Disconnect()
	{
		if(!m_Connected)
			return E_FAIL;

		// Terminate the protocol thread. On exit, it will fire the Disconnected event
		TerminateProtocolThread();
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::CreateVirtualChannels(BSTR newVal)
	{
		UINT strLength = SysStringLen(newVal);

		if(strLength < 1 || strLength > 300)
			return E_INVALIDARG;

		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::SendOnVirtualChannel(BSTR chanName, BSTR ChanData)
	{
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClient */ // 6/10
	virtual STDMETHODIMP IMsRdpClient::put_ColorDepth(long pcolorDepth)
	{
		switch(pcolorDepth)
		{
		case 8:
		case 15:
		case 16:
		case 24:
		case 32:
			break;

		default:
			return E_INVALIDARG;
		}

		return SetProperty(m_ColorDepth, pcolorDepth);
	}

	virtual STDMETHODIMP IMsRdpClient::get_ColorDepth(long * pcolorDepth) const
	{
		return GetProperty(m_ColorDepth, pcolorDepth);
	}

	virtual STDMETHODIMP IMsRdpClient::get_AdvancedSettings2(MSTSCLib::IMsRdpClientAdvancedSettings ** ppAdvSettings) const
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	virtual STDMETHODIMP IMsRdpClient::get_SecuredSettings2(MSTSCLib::IMsRdpClientSecuredSettings ** ppSecuredSettings) const
	{
		return GetSecuredSettings(ppSecuredSettings);
	}

	virtual STDMETHODIMP IMsRdpClient::get_ExtendedDisconnectReason(MSTSCLib::ExtendedDisconnectReasonCode * pExtendedDisconnectReason) const
	{
		return GetProperty(m_ExtendedDisconnectReason, pExtendedDisconnectReason);
	}

	virtual STDMETHODIMP IMsRdpClient::put_FullScreen(VARIANT_BOOL pfFullScreen)
	{
		if(!m_Connected)
			return E_FAIL;

		if(pfFullScreen && !m_SecuredSettingsEnabled)
			return E_FAIL;

		// TODO
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsRdpClient::get_FullScreen(VARIANT_BOOL * pfFullScreen) const
	{
		return GetProperty(m_FullScreen, pfFullScreen);
	}

	virtual STDMETHODIMP IMsRdpClient::SetVirtualChannelOptions(BSTR chanName, long chanOptions)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::GetVirtualChannelOptions(BSTR chanName, long * pChanOptions)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::RequestClose(MSTSCLib::ControlCloseStatus * pCloseStatus)
	{
		if(pCloseStatus == NULL)
			return E_POINTER;

		if(!m_Connected)
		{
			*pCloseStatus = MSTSCLib::controlCloseCanProceed;
			return S_OK;
		}

		*pCloseStatus = MSTSCLib::controlCloseWaitForEvents;

		if(!PostMessage(m_controlWindow, RDPC_WM_REQUEST_CLOSE, 0, 0))
			return HRESULT_FROM_WIN32(GetLastError());

		return S_OK;
	}

	/* IMsRdpClient2 */
	virtual STDMETHODIMP IMsRdpClient2::get_AdvancedSettings3(MSTSCLib::IMsRdpClientAdvancedSettings2 ** ppAdvSettings) const
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	virtual STDMETHODIMP IMsRdpClient2::put_ConnectedStatusText(BSTR pConnectedStatusText)
	{
		return SetProperty(m_ConnectedStatusText, pConnectedStatusText);
	}

	virtual STDMETHODIMP IMsRdpClient2::get_ConnectedStatusText(BSTR * pConnectedStatusText) const
	{
		return GetProperty(m_ConnectedStatusText, pConnectedStatusText);
	}

	/* IMsRdpClient3 */
	virtual STDMETHODIMP IMsRdpClient3::get_AdvancedSettings4(MSTSCLib::IMsRdpClientAdvancedSettings3 ** ppAdvSettings) const
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	/* IMsRdpClient4 */
    virtual STDMETHODIMP IMsRdpClient4::get_AdvancedSettings5(MSTSCLib::IMsRdpClientAdvancedSettings4 ** ppAdvSettings5) const
	{
		return GetAdvancedSettings(ppAdvSettings5);
	}

	/* IMsTscNonScriptable */
	virtual STDMETHODIMP IMsTscNonScriptable::put_ClearTextPassword(BSTR rhs)
	{
		return SetProperty(m_ClearTextPassword, rhs);
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_PortablePassword(BSTR pPortablePass)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_PortablePassword(BSTR * pPortablePass) const
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_PortableSalt(BSTR pPortableSalt)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_PortableSalt(BSTR * pPortableSalt) const
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_BinaryPassword(BSTR pBinaryPassword)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_BinaryPassword(BSTR * pBinaryPassword) const
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_BinarySalt(BSTR pSalt)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_BinarySalt(BSTR * pSalt) const
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::ResetPassword()
	{
		return SetProperty(m_ClearTextPassword, NULL);
	}

	/* IMsRdpClientNonScriptable */ // 0/2
	virtual STDMETHODIMP IMsRdpClientNonScriptable::NotifyRedirectDeviceChange(MSTSCLib::UINT_PTR wParam, MSTSCLib::LONG_PTR lParam)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClientNonScriptable::SendKeys(long numKeys, VARIANT_BOOL * pbArrayKeyUp, long * plKeyData)
	{
		// NOTE: the keys must be sent in a single, atomic sequence
		// TODO: acquire the write lock
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClientNonScriptable2 */
	virtual STDMETHODIMP IMsRdpClientNonScriptable2::put_UIParentWindowHandle(HWND phwndUIParentWindowHandle)
	{
		return SetProperty(m_UIParentWindowHandle, phwndUIParentWindowHandle);
	}

	virtual STDMETHODIMP IMsRdpClientNonScriptable2::get_UIParentWindowHandle(HWND * phwndUIParentWindowHandle) const
	{
		return GetProperty(m_UIParentWindowHandle, phwndUIParentWindowHandle);
	}
};

#pragma warning(pop)

/* More glue to interface to the rdesktop code */
extern "C"
{

/* Orders */
/* support routines */
void ui_begin_update(RDPCLIENT * This)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_BeginUpdate();
}

void ui_end_update(RDPCLIENT * This)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_EndUpdate();
}

void ui_set_clip(RDPCLIENT * This, int x, int y, int cx, int cy)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_SetClip(x, y, cx, cy);
}

void ui_reset_clip(RDPCLIENT * This)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_ResetClip();
}

/* blits */
void ui_destblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_DestBlt(opcode, x, y, cx, cy);
}

void ui_memblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, int srcx, int srcy)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_MemBlt(opcode, x, y, cx, cy, src, srcx, srcy);
}

void ui_patblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, BRUSH * brush, int bgcolour, int fgcolour)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_PatBlt(opcode, x, y, cx, cy, brush, bgcolour, fgcolour);
}

void ui_screenblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_ScreenBlt(opcode, x, y, cx, cy, srcx, srcy);
}

void ui_triblt(RDPCLIENT * This, uint8 opcode, int x, int y, int cx, int cy, HBITMAP src, int srcx, int srcy, BRUSH * brush, int bgcolour, int fgcolour)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_TriBlt(opcode, x, y, cx, cy, src, srcx, srcy, brush, bgcolour, fgcolour);
}

void ui_paint_bitmap(RDPCLIENT * This, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_PaintBitmap(x, y, cx, cy, width, height, data);
}

/* shapes */
void ui_ellipse(RDPCLIENT * This, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, BRUSH * brush, int bgcolour, int fgcolour)
{
	// TODO
//	RdpClient::InnerToOuter(This)->GetUI()->Display_Ellipse(opcode, fillmode, x, y, cx, cy, brush, bgcolour, fgcolour);
}

void ui_line(RDPCLIENT * This, uint8 opcode, int startx, int starty, int endx, int endy, PEN * pen)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_Line(opcode, startx, starty, endx, endy, pen);
}

void ui_polygon(RDPCLIENT * This, uint8 opcode, uint8 fillmode, POINT * point, int npoints, BRUSH * brush, int bgcolour, int fgcolour)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_Polygon(opcode, fillmode, point, npoints, brush, bgcolour, fgcolour);
}

void ui_polyline(RDPCLIENT * This, uint8 opcode, POINT * points, int npoints, PEN * pen)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_Polyline(opcode, points, npoints, pen);
}

void ui_rect(RDPCLIENT * This, int x, int y, int cx, int cy, int colour)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_Rect(x, y, cx, cy, colour);
}

/* text */
void ui_draw_text
(
	RDPCLIENT * This,
	uint8 font,
	uint8 flags,
	uint8 opcode,
	int mixmode,
	int x,
	int y,
	int clipx,
	int clipy,
	int clipcx,
	int clipcy,
	int boxx,
	int boxy,
	int boxcx,
	int boxcy,
	BRUSH * brush,
	int bgcolour,
	int fgcolour,
	uint8 * text,
	uint8 length
)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_DrawText
	(
		font,
		flags,
		opcode,
		mixmode,
		x,
		y,
		clipx,
		clipy,
		clipcx,
		clipcy,
		boxx,
		boxy,
		boxcx,
		boxcy,
		brush,
		bgcolour,
		fgcolour,
		text,
		length
	);
}

/* desktop save/restore */
void ui_desktop_save(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_SaveDesktop(offset, x, y, cx, cy);
}

void ui_desktop_restore(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
{
	RdpClient::InnerToOuter(This)->GetUI()->Display_RestoreDesktop(offset, x, y, cx, cy);
}

/* Resources */
/* bitmaps */
HBITMAP ui_create_bitmap(RDPCLIENT * This, int width, int height, uint8 * data)
{
	return win32_create_dib(width, height, This->server_depth, data);
}

void ui_destroy_bitmap(RDPCLIENT *, HBITMAP bmp)
{
	DeleteObject(bmp);
}

/* palettes */
HCOLOURMAP ui_create_colourmap(RDPCLIENT *, COLOURMAP * colours)
{
	return NULL;
}

void ui_set_colourmap(RDPCLIENT * This, HCOLOURMAP map)
{
	// TODO
}

/* cursors */
HCURSOR ui_create_cursor(RDPCLIENT * This, unsigned int x, unsigned int y, int width, int height, uint8 * andmask, uint8 * xormask)
{
	uint8 * andbuf = NULL;
	uint8 * xorbuf = NULL;

	uint8 * andbits = win32_convert_scanlines(width, - height, 1, 2, 4, andmask, &andbuf);
	uint8 * xorbits = win32_convert_scanlines(width, height, 24, 2, 4, xormask, &xorbuf);

	HBITMAP hbmMask = CreateBitmap(width, height, 1, 1, andbits);
	HBITMAP hbmColor = win32_create_dib(width, height, 24, xorbits);

	ICONINFO iconinfo;
	iconinfo.fIcon = FALSE;
	iconinfo.xHotspot = x;
	iconinfo.yHotspot = y;
	iconinfo.hbmMask = hbmMask;
	iconinfo.hbmColor = hbmColor;

	HICON icon = CreateIconIndirect(&iconinfo);

	if(icon == NULL)
		error("CreateIconIndirect %dx%d failed\n", width, height);

	if(andbuf)
		delete[] andbuf;

	if(xorbuf)
		delete[] xorbuf;

	DeleteObject(hbmMask);
	DeleteObject(hbmColor);

	return icon;
}

void ui_destroy_cursor(RDPCLIENT *, HCURSOR cursor)
{
	DestroyIcon(cursor);
}

/* glyphs */
HGLYPH ui_create_glyph(RDPCLIENT * This, int width, int height, const uint8 * data)
{
	uint8 * databuf = NULL;
	uint8 * databits = win32_convert_scanlines(width, height, 1, 1, 2, data, &databuf);

	HBITMAP hbm = CreateBitmap(width, height, 1, 1, databits);

	if(databuf)
		delete[] databuf;

	const uint8 * p = data;
	int stride = alignup(alignup(width, 8) / 8, 1);

#ifdef _DEBUG
	printf("glyph %p\n", hbm);

	for(int i = 0; i < height; ++ i, p += stride)
	{
		for(int j = 0; j < width; ++ j)
		{
			int B = p[j / 8];
			int b = 8 - j % 8 - 1;

			if(B & (1 << b))
				fputs("##", stdout);
			else
				fputs("..", stdout);
		}

		fputc('\n', stdout);
	}

	fputc('\n', stdout);
#endif

	return hbm;
}

void ui_destroy_glyph(RDPCLIENT *, HGLYPH glyph)
{
	DeleteObject(glyph);
}

/* Input window */
void ui_move_pointer(RDPCLIENT * This, int x, int y)
{
	// TODO
}

void ui_set_cursor(RDPCLIENT * This, HCURSOR cursor)
{
	// TODO
}

void ui_set_null_cursor(RDPCLIENT * This)
{
	// TODO
}

/* Miscellaneous */
void ui_resize_window(RDPCLIENT * This)
{
}

void ui_bell(RDPCLIENT *)
{
	MessageBeep(0);
}

int ui_select(RDPCLIENT * This, SOCKET rdp_socket)
{
	return SleepEx(0, TRUE) == WAIT_IO_COMPLETION;
}

/* Events */
BOOL event_pubkey(RDPCLIENT * This, unsigned char * key, unsigned int key_size)
{
	if(!RdpClient::OnPublicKey(This, key, key_size))
		return FALSE;
	else
		return TRUE;
}

void event_logon(RDPCLIENT * This)
{
	RdpClient::OnLogon(This);
}

BOOL event_redirect(RDPCLIENT * This, uint32 flags, uint32 server_len, wchar_t * server, uint32 cookie_len, char * cookie, uint32 username_len, wchar_t * username, uint32 domain_len, wchar_t * domain, uint32 password_len, wchar_t * password)
{
	if
	(
		!RdpClient::OnRedirect
		(
			This,
			flags,
			server_len,
			server,
			cookie_len,
			cookie,
			username_len,
			username,
			domain_len,
			domain,
			password_len,
			password
		)
	)
		return FALSE;
	else
		return TRUE;
}

}

class ClassFactory: public IClassFactory
{
private:
	LONG m_refCount;
	CLSID m_classId;
	unsigned m_libIndex;

public:
	ClassFactory(REFCLSID rclsid, unsigned libIndex):
		m_refCount(1),
		m_classId(rclsid)
	{
		lockServer();
	}

	~ClassFactory()
	{
		unlockServer();
	}

	virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject)
	{
		if(riid == IID_IUnknown || riid == IID_IClassFactory)
		{
			*ppvObject = this;
			return S_OK;
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	virtual STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_refCount);
	}

	virtual STDMETHODIMP_(ULONG) Release()
	{
		LONG n = InterlockedDecrement(&m_refCount);

		if(n == 0)
			delete this;

		return n;
	}

	virtual STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)
	{
		if(pUnkOuter && riid != IID_IUnknown)
			return CLASS_E_NOAGGREGATION;

		return RdpClient::CreateInstance(m_classId, m_libIndex, pUnkOuter, riid, ppvObject);
	}

	virtual STDMETHODIMP LockServer(BOOL fLock)
	{
		if(fLock)
			lockServer();
		else
			unlockServer();

		return S_OK;
	}
};

extern "C"
{

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv)
{
	unsigned libindex;

	if
	(
		rclsid == MSTSCLib::CLSID_MsTscAx ||
		rclsid == MSTSCLib::CLSID_MsRdpClient ||
		rclsid == MSTSCLib::CLSID_MsRdpClient2 ||
		rclsid == MSTSCLib::CLSID_MsRdpClient3 ||
		rclsid == MSTSCLib::CLSID_MsRdpClient4
	)
		libindex = 1;
	else if
	(
		rclsid == MSTSCLib_Redist::CLSID_MsTscAx ||
		rclsid == MSTSCLib_Redist::CLSID_MsRdpClient ||
		rclsid == MSTSCLib_Redist::CLSID_MsRdpClient2 ||
		rclsid == MSTSCLib_Redist::CLSID_MsRdpClient3 // ||
		// rclsid != MSTSCLib::CLSID_MsRdpClient4
	)
		libindex = 2;
	else
		return CLASS_E_CLASSNOTAVAILABLE;

	ClassFactory * p = new ClassFactory(rclsid, libindex);

	if(p == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = p->QueryInterface(riid, ppv);

	p->Release();

	if(FAILED(hr))
		return hr;

	return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
	return canUnloadServer() ? S_OK : S_FALSE;
}

STDAPI_(ULONG) DllGetTscCtlVer(void)
{
	// BUGBUG: don't hardcode this
	return 0x05020ECE; // 5.2.3790
}

DWORD WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	assert(hInstance == GetCurrentModule());

	switch(fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hInstance);

			if(!RdpClient::Startup())
				return FALSE;
		}

		break;

	case DLL_PROCESS_DETACH:
		{
			// Process is terminating, no need to clean up
			if(lpvReserved)
				break;

			RdpClient::Shutdown();
		}

		break;
	}

	return TRUE;
}

}

// EOF
