#ifndef CARDREGION_INCLUDED
#define CARDREGION_INCLUDED

#include "globals.h"
#include "cardstack.h"
#include "cardlib.h"

class CardWindow;

//
//	This class defines a physical card-stack,
//	which draws the cards, supports
//

class CardRegion
{
	friend class CardWindow;
	friend class CardStack;

	//
	//	Constructor is PRIVATE - only
	//  a CardWindow can create cardstacks!
	//
	CardRegion(CardWindow &parent, int id, bool fVisible,
		int x, int y, int xOffset, int yOffset);

	~CardRegion();

public:

	void SetBackColor(COLORREF cr);

	void	          SetCardStack(const CardStack &cs);
	const CardStack & GetCardStack();

	//
	//	Event-callback support
	//
	bool SetDragRule(UINT uDragType, pCanDragProc proc = 0);
	bool SetDropRule(UINT uDropType, pCanDropProc proc = 0);

	void SetClickProc    (pClickProc proc);
	void SetDblClickProc (pClickProc proc);

	void SetAddCardProc    (pAddProc proc);
	void SetRemoveCardProc (pRemoveProc proc);

	//
	//	Physical attribute support
	//
	bool SetThreedCount (int count);
	void SetOffsets     (int x, int y);
	void SetPos         (int x, int y);
	void Show           (bool fShow);
	bool IsVisible      ();

	void SetEmptyImage	(UINT uImage);
	void SetBackCardIdx (UINT uBackIdx);
	void SetPlacement   (UINT xJustify, UINT yJustify, int xAdjust, int yAdjust);

	void Update();
	void Redraw();

	void SetFaceDirection(UINT uDirType, int nOption);
	UINT GetFaceDirection(int *pnOption);

	void Flash(int count, int timeout);
	void StopFlash();

	int  Id();

	CardWindow &GetCardWindow() { return parentWnd; }

	bool PlayCard(CardRegion *pDestStack, int value, int num);
	bool MoveCard(CardRegion *pDestStack, int nNumCards, bool fAnimate);
	bool SimulateDrag(CardRegion *pDestStack, int nNumCards, bool fAnimate);

	bool Lock();
	bool UnLock();

	//
	//	Common wrappers for the CardStack object
	//
	int         NumCards() const;
	void        NewDeck()        { cardstack.NewDeck();  }
	void        Shuffle()        { cardstack.Shuffle();  }
	void        Clear()          { cardstack.Clear();    }

	void		Reverse()        { cardstack.Reverse();  }

	void		Push(const Card card)     { cardstack.Push(card); }
	void		Push(const CardStack &cs) { cardstack.Push(cs);   }

	Card		Pop()          { return cardstack.Pop();      }
	CardStack	Pop(int items) { return cardstack.Pop(items); }

	Card		Top()          { return cardstack.Top();      }
	CardStack	Top(int items) { return cardstack.Top(items); }


private:

	void DoFlash();
	void RedrawIfNotDim(CardRegion *compare, bool fFullRedraw);

	void UpdateFaceDir(CardStack &cards);
	void Clip(HDC hdc);
	void Render(HDC hdc);
	int	 GetOverlapRatio(int x, int y, int width, int height);

	void MoveDragCardTo(HDC hdc, int x, int y);
	void ZoomCard(HDC hdc, int xpos, int ypos, CardRegion *dest);

	void RenderBottomMost(HDC hdc, int minustopmost = 0);
	void PrepareDragBitmaps(int numtodrag);
	void PrepareDragBitmapsThreed(int numtodrag);
	void ReleaseDragBitmaps(void);

	bool CanDragCards(int iNumCards);
	bool CanDropCards(CardStack &cards);

	void CalcApparentCards();
	int	 CalcApparentCards(int realnum);

	void UpdateSize();
	void AdjustPosition(int winwidth, int winheight);

	bool IsPointInStack(int x, int y);

	int	  GetNumDragCards(int x, int y);
	bool  OnLButtonDown(int x, int y);
	bool  OnLButtonDblClk(int x, int y);
	bool  OnMouseMove(int x, int y);
	bool  OnLButtonUp(int x, int y);


	//
	//	Private data members
	//

	int		id;

	CardWindow &parentWnd;

	CardStack  cardstack;	//cards in this stack
	CardStack  dragstack;	//cards which we might be dragging

	bool	fMouseDragging;

	int		xpos;			//coordinates of stack
	int		ypos;

	int		xoffset;		//direction that cards take
	int		yoffset;

	int		width;			//stack-size of all cards
	int		height;

	//
	//	justify / placement vars
	int		xjustify;
	int		yjustify;
	int		xadjust;
	int		yadjust;

	//
	//	Used for mouse-dragging / moving cards
	//
	int		iNumDragCards;
	int		mousexoffset;
	int		mouseyoffset;
	int		oldx;
	int		oldy;

	int		nDragCardWidth;
	int		nDragCardHeight;

	HDC		hdcBackGnd;
	HBITMAP	hbmBackGnd;
	HDC		hdcDragCard;
	HBITMAP	hbmDragCard;

	int		nNumApparentCards;
	int		nThreedCount;
	bool	fVisible;

	int		nFlashCount;
	bool	fFlashVisible;
	UINT	uFlashTimer;

	COLORREF crBackgnd;

	UINT	uEmptyImage;
	UINT	uFaceDirType;
	int		nFaceDirOption;
	int		nBackCardIdx;

	UINT	uDragRule;
	UINT	uDropRule;

	//
	//	Stack callback support
	//
	pCanDragProc	CanDragCallback;
	pCanDropProc	CanDropCallback;
	pClickProc		ClickCallback;
	pClickProc		DblClickCallback;
	pAddProc		AddCallback;
	pRemoveProc		RemoveCallback;

	//locking mechanism to prevent user dragging etc
	HANDLE			mxlock;
};

#endif

