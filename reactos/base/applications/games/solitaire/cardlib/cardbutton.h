#ifndef CARDBUTTON_INCLUDED
#define CARDBUTTON_INCLUDED

#define MAXBUTTONTEXT 64

#include "cardlib.h"

class CardButton
{
	friend class CardWindow;

	//
	//	Constructor is PRIVATE - only a
	//  CardWindow can create buttons!
	//
	CardButton(CardWindow &parent, int id, TCHAR *szText, UINT style, bool visible,
		int x, int y, int width, int height);

	~CardButton();

public:

	void SetStyle(UINT uStyle);
	UINT GetStyle();

	void SetText(TCHAR *fmt, ...);
	void SetFont(HFONT font);

	void SetPlacement(UINT xJustify, UINT yJustify, int xAdjust, int yAdjust);

	void SetForeColor(COLORREF cr);
	void SetBackColor(COLORREF cr);

	void Move(int x, int y, int width, int height);
	void Show(bool fShow);
	void Redraw();
	int  Id();

	void SetIcon(HICON hicon, bool fRedraw);

	void SetButtonProc(pButtonProc proc);

	CardWindow &GetCardWindow() { return parentWnd; }

	bool Lock();
	bool UnLock();

	static COLORREF GetHighlight(COLORREF crBase);
	static COLORREF GetShadow(COLORREF crBase);
	static COLORREF GetFace(COLORREF crBase);

private:

	//
	//	Private member functions
	//
	void AdjustPosition(int winwidth, int winheight);

	void DrawRect(HDC hdc, RECT *rect, bool fNormal);
	void Draw(HDC hdc, bool fNormal);
	void Clip(HDC hdc);

	int  OnLButtonDown(HWND hwnd, int x, int y);
	int  OnMouseMove(HWND hwnd, int x, int y);
	int  OnLButtonUp(HWND hwnd, int x, int y);

	//
	//	Private members
	//
	CardWindow &parentWnd;

	RECT	rect;
	int		id;
	UINT	uStyle;
	bool	fVisible;

	int		xadjust;
	int		xjustify;
	int		yadjust;
	int		yjustify;

	HICON	hIcon;
	HFONT   hFont;

	TCHAR	szText[MAXBUTTONTEXT];

	COLORREF crBack;
	COLORREF crText;
	COLORREF crHighlight;
	COLORREF crShadow;
	COLORREF crShadow2;

	bool	fMouseDown;
	bool    fButtonDown;

	HANDLE	mxlock;

	pButtonProc	ButtonCallback;
};

#endif
