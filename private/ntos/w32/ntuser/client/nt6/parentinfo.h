// ParentInfo.h: interface for the CParentInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PARENTINFO_H__CBCB8816_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_PARENTINFO_H__CBCB8816_7899_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "edge.h"

typedef struct tagDIMENSION
{
	int Width;
	int Height;
} DIMENSION, * PDIMENSION;


#define PROPERTY(type, Name) void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; }

class CParentInfo  
{
public:
	void DeterminBorders( CControlList * pControlList);
	void Annotate( HDC hdc );
	CEdge ** GetHorizontalEdges();
	CEdge ** GetVerticalEdges();
	void Resize(int width, int height);
	CParentInfo();
	virtual ~CParentInfo();
	void	Init(HWND h);

	LONG	GetWidth() const { return m_Size.right-m_Size.left; }
	LONG	GetHeight() const { return m_Size.bottom-m_Size.top; }

	LONG	GetLeft() const { return m_Size.left; }
	LONG	GetTop() const { return m_Size.top; }
	LONG	GetRight() const { return m_Size.right; }
	LONG	GetBottom() const { return m_Size.bottom; }

	void	DeterminSize();
	HWND	GetWindow() const { return m_hWnd;}

	PROPERTY(LONG, RightBorder);
	PROPERTY(LONG, LeftBorder);
	PROPERTY(LONG, TopBorder);
	PROPERTY(LONG, BottomBorder);

	CEdge	* AddEdge(int Position, int Axis, BOOL Flexible=false, int Offset=0);
	CEdge	* AddEdge(CGuide * pGuide, int Offset=0);
	void	ConstructBorders();
	CEdge & FindCloseEdge(CEdge & Fixed, int Offset);

	CEdge *	GetLeftEdge() { return m_pLeftEdge; }
	CEdge *	GetRightEdge() { return m_pRightEdge; }
	CEdge *	GetTopEdge() { return m_pTopEdge; }
	CEdge *	GetBottomEdge() { return m_pBottomEdge; }

	int		GetNumHoriz() const { return m_Edges.GetNumHoriz(); }
	int		GetNumVert() const { return m_Edges.GetNumVert(); }

	PROPERTY( DIMENSION, MinimumSize );

private:
	DIMENSION	m_MinimumSize;
	RECT	m_Size;
	HWND	m_hWnd;

	LONG	m_RightBorder;
	LONG	m_LeftBorder;
	LONG	m_TopBorder;
	LONG	m_BottomBorder;

	//
	// This is a list of the edges
	//
	CEdgeCache	m_Edges;

	//
	// These are the edges of the parent itself.
	//
	CEdge	*	m_pLeftEdge;
	CEdge	*	m_pRightEdge;
	CEdge	*	m_pTopEdge;
	CEdge	*	m_pBottomEdge;
};

#endif // !defined(AFX_PARENTINFO_H__CBCB8816_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
