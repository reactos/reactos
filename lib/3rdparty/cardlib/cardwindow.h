#ifndef CARDBOARD_INCLUDED
#define CARDBOARD_INCLUDED

#define MAXBUTTONS		32
#define MAXCARDSTACKS	32
#define MAXDROPZONES	8

#include "dropzone.h"
#include "cardlib.h"

class CardRegion;
class CardButton;

LRESULT CALLBACK CardWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

class CardWindow
{
	friend class CardRegion;
	friend class CardButton;

	friend void RegisterCardWindow();

public:

	CardWindow();
	~CardWindow();

	//
	//	Basic windowing support
	//
	BOOL Create(HWND hwndParent, DWORD dwExStyle, DWORD dwStyle, int x, int y, int width, int height);
	BOOL Destroy();

	operator HWND() { return m_hWnd; }

	CardButton *CreateButton (int id, TCHAR *szText, UINT uStyle, bool fVisible, int x, int y, int width, int height);
	CardRegion *CreateRegion (int id, bool fVisible, int x, int y, int xoffset, int yoffset);

	CardButton *CardButtonFromId(int id);
	CardRegion *CardRegionFromId(int id);

	bool DeleteButton(CardButton *pButton);
	bool DeleteRegion(CardRegion *pRegion);
	bool DeleteAll();

	void	 SetBackColor(COLORREF cr);
	COLORREF GetBackColor();
	void	 SetBackCardIdx(UINT uBackIdx);
	UINT	 GetBackCardIdx();
	void	 SetBackImage(HBITMAP hBitmap);

	void EmptyStacks(void);
	void Redraw(void);
	void Update(void);

	bool DistributeStacks(int nIdFrom, int nNumStacks, UINT xJustify, int xSpacing, int nStartX);
	void SetResizeProc(pResizeWndProc proc);
	int  GetWidth() { return nWidth; }
	int  GetHeight() { return nHeight; }

	//
	//	Dropzone support
	//
	bool	  RegisterDropZone(int id, RECT *rect, pDropZoneProc proc);
	bool	  DeleteDropZone(int id);

private:

	int		  GetNumDropZones() { return nNumDropZones; }
	DropZone* GetDropZoneFromRect(RECT *rect);

	//
	//	Window procedure - don't call
	//
	   LRESULT CALLBACK WndProc    (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK CardWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	//
	//	Private functions
	//
	void Paint(HDC hdc);
	void PaintCardRgn(HDC hdc, int dx, int dy, int width, int height, int sx, int sy);

	HPALETTE CreateCardPalette();

	CardButton *CardButtonFromPoint(int x, int y);
	CardRegion *CardRegionFromPoint(int x, int y);
	CardRegion *GetBestStack(int x, int y, int w, int h);

	//
	//	Private members
	//

	HWND m_hWnd;			//window handle!
	int  nWidth, nHeight;

	UINT	nBackCardIdx;	//all stacks share this card index by default

	HBITMAP	hbmBackImage;
	HDC		hdcBackImage;


	CardButton  * Buttons[MAXBUTTONS];
	int			  nNumButtons;

	CardRegion  * Regions[MAXCARDSTACKS];
	int			  nNumCardRegions;

	DropZone	* dropzone[MAXDROPZONES];
	int			  nNumDropZones;

	COLORREF  crBackgnd;

	pResizeWndProc  ResizeWndCallback;


};


#endif
