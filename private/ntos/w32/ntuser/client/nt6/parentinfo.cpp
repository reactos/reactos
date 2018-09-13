// ParentInfo.cpp: implementation of the CParentInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ParentInfo.h"
#include "debug.h"
#include "persctl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CParentInfo::CParentInfo()
{
	SetRightBorder(0);
	SetLeftBorder(0);
	SetTopBorder(0);
	SetBottomBorder(0);
	m_pTopEdge=NULL;
	m_pBottomEdge=NULL;
	m_pLeftEdge=NULL;
	m_pRightEdge=NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::Init(HWND h)
{
	m_hWnd=h;
	DeterminSize();
	SetRightBorder( GetWidth() );
	SetLeftBorder( GetWidth() );
	SetTopBorder( GetHeight() );
	SetBottomBorder( GetHeight() );
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
CParentInfo::~CParentInfo()
{

}

////////////////////////////////////////////////////////////////////////////////////////
//
//
// This is the size of the client area of the window
// we don't have to take into account the frame and title bar when positioning
// indside it.
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::DeterminSize()
{
	GetClientRect( GetWindow(), &m_Size);
	RECT r;
	GetWindowRect( GetWindow(), &r);
	DIMENSION d;
	d.Width= r.right - r.left;
	d.Height = r.bottom - r.top;
	SetMinimumSize(d);
	TRACE(L"ParentDimensions : %d by %d \n",
			GetWidth(),
			GetHeight());
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Walks all the edges, finding one that is closest and thus re-usable.
//
////////////////////////////////////////////////////////////////////////////////////////
#define ABS(x) (x<0?-x:x)
CEdge * CParentInfo::AddEdge(int Position, int Axis, BOOL Flexible, int Slop)
{
	int i=0;
	CEdge * pEdge;
	CGuide * pClosest=NULL;
	int closest=0xffff;

	//
	// First see if the edge is what we wanted
	//
	while( pEdge=m_Edges.GetEdge(i++) )
	{
		if(pEdge->GetPosition() == Position )
		{
			if( pEdge->GetAxis() == Axis )
			{
				TRACE(L"Guide REUSE:: %03d, %02d\n",Position, Axis);
				return pEdge;
			}
		}
	}

	i=0;
	while ( pEdge=m_Edges.GetEdge(i++))
	{
		CGuide * pGuide=pEdge->GetGuide();

		if(pGuide->GetAxis() == Axis )
		{
			// if( pEdge->GetFlexible() == Flexible )
			{
				int distance=Position -pGuide->GetPosition() ;
				if( ABS(distance) <= Slop )
				{
					if( Slop == 0 )
						return pEdge;

					//
					// Within the range, but is it the best?
					//
					if(closest==0xffff)
					{
						closest=distance;
						pClosest=pGuide;
					}
					
					if( ABS(distance)<=ABS(closest) )
					{
						pClosest=pGuide;
						closest=distance;
					}
				}
			}
		}
	}

	if(pClosest)
	{
		//
		// Create a new edge with the same guide, different offset.
		//
		closest = Position - pClosest->GetPosition();
		TRACE(L"Guide CLOSE:: %03d, %02d, closest %d, actual pos %03d\n",pClosest->GetPosition(), pClosest->GetAxis(), closest, Position);
		return m_Edges.Create( pClosest, closest);
	}

	TRACE(L"Guide NEW  :: %03d, %02d\n",Position, Axis);
	return m_Edges.Create(Position, Axis, Flexible, 0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge & CParentInfo::FindCloseEdge(CEdge & Fixed, int Offset)
{
	int i=0;
	CEdge * pEdge;
	while ( pEdge=m_Edges.GetEdge(i++) )
	{
		if(pEdge->GetAxis() == Fixed.GetAxis() )
		{
			int distance=Fixed.GetPosition() -pEdge->GetPosition() ;
			if( ABS(distance) <= Offset )
			{
				return *pEdge;
			}
		}
	}
	return Fixed;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge * CParentInfo::AddEdge(CGuide * pGuide, int Offset)
{
	// TRACE(L"Associating another edge %03d from 0x%08x\n", Offset , Edge);
	return m_Edges.Create( pGuide, Offset);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// BORDERS FOR THE DIALOG - (not for the components).
// Buids some default guides for the borders.
// the borders can move
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::ConstructBorders()
{
	m_pLeftEdge=AddEdge( GetLeftBorder(), LEFT_AT, FALSE );
	m_pRightEdge=AddEdge( GetWidth() - GetRightBorder(), RIGHT_AT, TRUE );

	m_pTopEdge=AddEdge(  GetTopBorder(), TOP_AT, FALSE );
	m_pBottomEdge=AddEdge( GetHeight() - GetBottomBorder(), BOTTOM_AT, TRUE );
}

////////////////////////////////////////////////////////////////////////////////////////
//
// We know that the left and bottom edges move to follow the dialog
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::Resize(int width, int height)
{
	m_pRightEdge->SetPosition( width - GetRightBorder() );
	m_pBottomEdge->SetPosition( height - GetBottomBorder() );
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Walks all the edges building an array of the veritcal ones - not sorted
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge ** CParentInfo::GetVerticalEdges()
{
	int iCount=m_Edges.GetNumVert();
	CEdge ** ppEdges= (CEdge**)new (CEdge*[iCount]);
	CEdge * pEdge;
	int i=0,iDest=0;
	while( pEdge = m_Edges.GetEdge(i++) )
	{
		if(pEdge->IsVertical() )
			ppEdges[iDest++] = pEdge;
#ifdef _DEBUG
		if( iDest > iCount )
			TRACE(L"Vertical edge count is off %d vs %d\n", iDest, iCount);
#endif
	}
	return ppEdges;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Walks all the edges building an array of the horizontal ones - not sorted
//
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge ** CParentInfo::GetHorizontalEdges()
{
	int iCount=m_Edges.GetNumHoriz();
	CEdge ** ppEdges= new CEdge *[iCount];
	CEdge * pEdge;
	int i=0,iDest=0;
	while( pEdge = m_Edges.GetEdge(i++) )
	{
		if(pEdge->IsHorizontal())
			ppEdges[iDest++] = pEdge;
#ifdef _DEBUG
		if( iDest > iCount )
			TRACE(L"Horiz edge count is off %d vs %d\n", iDest, iCount);
#endif
	}
	return ppEdges;
}

void CParentInfo::Annotate(HDC hdc)
{
	int i=0;
	CEdge * pEdge;
	int iWidth=GetWidth();
	int iHeight=GetHeight();
	HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 0xff,0x00,0x00) );
	HGDIOBJ holdPen= SelectObject( hdc, hpen);
	while( pEdge=m_Edges.GetEdge(i++) )
	{
		if( pEdge->IsHorizontal() )
		{
			MoveToEx( hdc, 0, pEdge->GetPosition(), NULL );
			LineTo( hdc, iWidth , pEdge->GetPosition() );
		}
		else
		{
			MoveToEx( hdc, pEdge->GetPosition(), 0, NULL );
			LineTo( hdc, pEdge->GetPosition(), iHeight );
		}
	}
	SelectObject(hdc, holdPen);
	DeleteObject(hpen);
}

////////////////////////////////////////////////////////////////////////
//
// Walks all the controls, finds where the borders of the dialog are
//
////////////////////////////////////////////////////////////////////////
void CParentInfo::DeterminBorders(CControlList * pControlList)
{
	CResizeControl * pC;
	int i=0;

	//
	// Now walk and see if we have any borders - this is a lame way to do it.
	//
#define BORDER(Edge) 			if( pC->Get##Edge##Gap() < Get##Edge##Border() ) \
			Set##Edge##Border(pC->Get##Edge##Gap());

	i=0;
	while( pC=pControlList->GetControl(i++) )
	{
		if( pC->GetRightGap() < GetRightBorder() )
			SetRightBorder(pC->GetRightGap());
	}
	TRACE(L"Right border is %d\n", GetRightBorder() );

	i=0;
	while( pC=pControlList->GetControl(i++) )
	{
		BORDER(Left);
	}
	TRACE(L"Left border is %d\n", GetLeftBorder() );

	i=0;
	while( pC=pControlList->GetControl(i++) )
	{
		BORDER(Top);
	}
	TRACE(L"Top border is %d\n", GetTopBorder() );

	i=0;
	while( pC=pControlList->GetControl(i++) )
	{
		BORDER(Bottom);
	}
	TRACE(L"Bottom border is %d\n", GetBottomBorder() );

	//
	// Now add these edges to the parent info.
	// or should the be add these guides to the parent info?
	//
	ConstructBorders();
}
