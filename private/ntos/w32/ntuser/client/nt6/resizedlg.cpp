// ResizeDlg.cpp: implementation of the CResizeDlg class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ResizeDlg.h"
#include "debug.h"
#include "persctl.h"
// #include "w32err.h" // #include "E:\nt\private\NTOS\w32\w32inc\w32err.h"
// #include "usercli.h"	// #include "e:\nt\private\ntos\w32\ntuser\client\usercli.h"
// #include "resource.h"
// #include "E:\nt\private\NTOS\w32\w32inc\w32err.h"


#define MIN_COL_SPACE 4
#define MIN_ROW_SPACE 4

void* __cdecl operator new(size_t n)
{
    // return malloc(n);
	return GlobalAlloc( GPTR, n);
}

void __cdecl operator delete(void* p)
{
    // free(p);
	GlobalFree(p);
}

extern "C" {

LPVOID __cdecl MakeAResizeDlg(int id,HANDLE h)
{
	return new CResizeDlg(id, NULL, (HINSTANCE)h);
}

LRESULT ResizeDlgMessage( LPVOID pObject, 
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam )
{
	return ((CResizeDlg*)pObject)->DlgProc( hwnd, message, wParam, lParam );
}


};




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResizeDlg::CResizeDlg(int DlgID, HWND hWndParent, HINSTANCE hInst)
: BASECLASS(DlgID, hWndParent, hInst)
{
	SetAnnotate(FALSE);
	SetRowWeight(0);
	SetColWeight(0);
	m_hwndGripper=NULL;
}

CResizeDlg::~CResizeDlg()
{

}

//////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////
BOOL CALLBACK CResizeDlg::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch( uMessage )
	{
	case WM_NCHITTEST:
		{
			// Because we always turn on the resize frame, we must always hit test.
			// if( (GetRowWeight() !=0 ) || (GetColWeight() != 0 ) )
			{
				// call def window proc to see what he wants to do
				LONG lRes= HitTest( DefWindowProc( hDlg, uMessage, wParam, lParam ) );
				if( lRes != (HTERROR-1) )
				{
					SetWindowLong( hDlg, DWL_MSGRESULT,  lRes );
					return TRUE;
				}
			}
		}
		break;

	case WM_WINDOWPOSCHANGING:
		if( (GetRowWeight() !=0 ) || (GetColWeight() != 0 ) )
			DoChangePos( (WINDOWPOS*)lParam);
		break;

	case WM_ERASEBKGND:
		/*
		if( (GetRowWeight() !=0 ) || (GetColWeight() != 0 ) )
	    {
			HDC hdc=(HDC)wParam;
			RECT rcWindow;
			GetClientRect(hDlg, &rcWindow);
			int g_cxGrip = GetSystemMetrics( SM_CXVSCROLL );
			int g_cyGrip = GetSystemMetrics( SM_CYHSCROLL );
			rcWindow.left = rcWindow.right - g_cxGrip; // pWState->x_VSBArrow;
			rcWindow.top = rcWindow.bottom - g_cyGrip; // pWState->y_HSBArrow;
			DrawFrameControl(hdc, &rcWindow, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
		}
		*/
		break;

	case WM_INITDIALOG:
			SetWindow(hDlg);
			DoInitDialog();
			// Re-adjusts the dialogs to what we think they should be
			// breaks some apps who are completely owner draw.
			if( (GetRowWeight() !=0 ) || (GetColWeight() != 0 ) )
			{
				AddGripper(); // need to sit on the size meesage too.
				RECT r;
				GetWindowRect( hDlg, &r);
				SetWindowPos(
					NULL, NULL, 0,0, r.right - r.left, r.bottom - r.top, 
					SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER |
					SWP_NOREDRAW | SWP_NOZORDER );
			}
		return TRUE;
		break;

	case WM_SIZE:
		if( (GetRowWeight() !=0 ) || (GetColWeight() != 0 ) )
		{
			// Don't try to re-adjust to the size when initially shown.
			if( wParam == SIZE_RESTORED )
			{
				WINDOWPOS pos={NULL, NULL, 0,0, LOWORD(lParam), HIWORD(lParam), 
					SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER |
					SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOZORDER };
				DoChangePos( &pos );
				ResizeControls(LOWORD( lParam ) , HIWORD (lParam) );
			}
		}
		break;

	case WM_PAINT:
		Annotate();
		break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// Walks all the controls, finds their edges,
// attatches them to their edges/guides.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::WalkControls()
{
	m_ParentInfo.Init( GetWindow() );
	FindControls();
	m_ParentInfo.DeterminBorders( &m_ControlList );
	MakeAttatchments();
	FindCommonGuides();
	SpecialRowCol();
}

////////////////////////////////////////////////////////////////////////
//
// Walk the dialog
// build structures of what we think the relationships of the controls is.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::FindControls()
{
	HWND hCurrent;
	HWND hFirst;
	hCurrent=hFirst=::GetWindow(GetWindow(),GW_CHILD);
	TRACE(L"FindControls on 0x%0x\n", GetWindow() );
	TRACE(L"Parent size is %d,%d by %d,%d\n",
		m_ParentInfo.GetLeft(),
		m_ParentInfo.GetTop(),
		m_ParentInfo.GetRight(),
		m_ParentInfo.GetBottom() );

	if( hFirst )
	{
		do {
			CResizeControl * pC = new CResizeControl(hCurrent, m_ParentInfo);
			if( pC->Valid() )
			{
				//
				// Make sure the control is wihin the bounds of our parent.
				//
				RECT r=pC->GetLocation();
				TRACE(L"Control size is %d,%d by %d,%d\n",
					r.left,
					r.top,
					r.right,
					r.bottom);
				if( (r.right > m_ParentInfo.GetRight()) ||
					(r.bottom > m_ParentInfo.GetBottom()) ||
					(r.top < m_ParentInfo.GetTop()) ||
					(r.left < m_ParentInfo.GetLeft()) )
				{
					TRACE(L"Punting this control %s\n", pC->GetClassName() );
					delete pC;		// we don't play with these.
				}
				else
					m_ControlList.Append(pC);
			}
			else
				delete pC;
		} while (hCurrent=::GetWindow(hCurrent,GW_HWNDNEXT));
	}
}

////////////////////////////////////////////////////////////////////////
//
// We have data backing each control now.
// walk it and have it initialize itself.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::FindBorders()
{
}
 
////////////////////////////////////////////////////////////////////////
//
// Now we have determined the borders on the dialog, see
// if any controls can make assumptions about their individual
// attatchments.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::MakeAttatchments()
{
	CResizeControl * pC;
	int i=0;
	while( pC=m_ControlList.GetControl(i++) )
		pC->DeterminLeftAttatchments();

	i=0;
	while( pC=m_ControlList.GetControl(i++) )
		pC->DeterminTopAttatchments();

	i=0;
	while( pC=m_ControlList.GetControl(i++) )
		pC->DeterminRightAttatchments();

	i=0;
	while( pC=m_ControlList.GetControl(i++) )
		pC->DeterminBottomAttatchments();

	i=0;
	while( pC=m_ControlList.GetControl(i++) )
		pC->SetControlAssociations();
}

////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::ResizeControls(WORD width, WORD height)
{
	int iSpecialRow = GetNumRows()-1;
	int iSpecialCol = GetNumCols()-1;

	//
	// Add free space
	//
	if( GetColWeight()>0 )
	{
		int FreeW = width - m_Cols[ GetNumCols() ].Pos - m_ParentInfo.GetRightBorder();
		if( FreeW != 0 )
		{
			int iPadding=0;
			int iPad=FreeW/GetColWeight();
			for( int fi=0;fi<= GetNumCols(); fi++)
			{
				m_Cols[ fi ].Pos += iPadding;
				if( m_Cols[fi].iFixed>=0)
					iPadding+=iPad * m_Cols[fi].iWeight;
			}
		}
	}

	//
	// Add free row.
	//
	if( GetRowWeight()>0 )
	{
		int Free = height - m_Rows[ GetNumRows() ].Pos - m_ParentInfo.GetBottomBorder();
		if( Free != 0 )
		{
			int iPadding=0;
			int iPad=Free/GetRowWeight();
			for( int fi=0;fi<= GetNumRows(); fi++)
			{
				m_Rows[ fi ].Pos += iPadding;
				if( m_Rows[fi].iFixed>=0)
					iPadding+=iPad * m_Rows[fi].iWeight;
			}
		}
	}

	//
	// Now setup the information for all of the controls, as to which cell they are in.
	//
	HDWP hdwp = BeginDeferWindowPos( m_ControlList.GetControlCount() );
	int i=0;
	CResizeControl * pC;
	CConstraint	* pCons;
	while( pC=(CResizeControl *) m_ControlList.GetPointer(i++) )
	{
		pCons=&(pC->m_Cons);
		int x=m_Cols[pCons->GetCol()].Pos + pCons->GetPadLeft();
		int y=m_Rows[pCons->GetRow()].Pos + pCons->GetPadTop();
		int w,h;

		BOOL bMove=true; BOOL bSize=false;
		if( pC->IsGrowsWide() )
		{
			w=m_Cols[pCons->GetCol()+pCons->GetColW()].Pos + pCons->GetPadRight() -x;
			bSize=true;
		}
		else
			w=pC->GetWidth();

		if( pC->IsGrowsHigh() )
		{
			h=m_Rows[pCons->GetRow()+pCons->GetRowH()].Pos + pCons->GetPadBottom() -y;
			bSize=true;
		}
		else
			h=pC->GetHeight();

		//
		// Here we special case the ComboBox - yuck.
		//
		if( lstrcmp( pC->GetClassName(), L"Button") ==0)
		{
			//
			// Check box button is OK though.
			//
			int ibs=GetWindowStyle( pC->GetControl() );
			if( (ibs & 0xf) == BS_GROUPBOX )
			{
				//
				// Force the width / height.
				//
				w=m_Cols[pCons->GetCol()+pCons->GetColW()].Pos + pCons->GetPadRight() -x;
				h=m_Rows[pCons->GetRow()+pCons->GetRowH()].Pos + pCons->GetPadBottom() -y;
				bSize=true;
			}
		}

		//
		// If they are in special row / columns we do something special to them.
		//
		if( m_SpecialCol.bSpecial && ( pCons->GetCol() == iSpecialCol ) )
		{
			// alignment issues
			if(m_SpecialCol.iAlignment == -1 )
			{
				y=pC->GetTopGap();
			}
			else
			if(m_SpecialCol.iAlignment == 1 )
			{
				//
				// Right aligned.
				//
				y=pC->GetTopGap() + height - m_SpecialCol.iMax - m_ParentInfo.GetTopBorder();
			}
			else
			{
				//
				// Center aligned
				//
				y=pC->GetTopGap() + ((height - m_SpecialCol.iMax - m_ParentInfo.GetTopBorder())/2);
			}
		}

		if( m_SpecialRow.bSpecial && ( pCons->GetRow() == iSpecialRow ) )
		{
			// alignment issues
			if(m_SpecialRow.iAlignment == -1 )
			{
				x=pC->GetLeftGap();
			}
			else
			if(m_SpecialRow.iAlignment == 1 )	// right aligned.
			{
				x=pC->GetLeftGap() + width - m_SpecialRow.iMax - m_ParentInfo.GetRightBorder();
			}
			else
			{
				x=pC->GetLeftGap() + ((width - m_SpecialRow.iMax - m_ParentInfo.GetRightBorder()) / 2);
			}
		}

		hdwp = DeferWindowPos( hdwp, pC->GetControl(), NULL, 
			x, y, w , h,
			(bMove ? 0: SWP_NOMOVE) |	(bSize ? 0: SWP_NOSIZE) | SWP_NOZORDER );

	}

	//
	// Move the gripper
	//
	SetGripperPos(hdwp);

	EndDeferWindowPos( hdwp );
}

////////////////////////////////////////////////////////////////////////
//
// Tries to combine guides to be offsets from other guides.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::FindCommonGuides()
{
	CEdge ** ppVert = m_ParentInfo.GetVerticalEdges();
	CEdge ** ppHoriz = m_ParentInfo.GetHorizontalEdges();

	//
	// Work out which guides are used in columns.
	//
	int iCount,i;
	iCount=m_ParentInfo.GetNumVert();
	Sort(ppVert, iCount );
#ifdef _DEBUG
	TRACE(L"Vertical edge information %d edges\n", iCount );
	for(i=0;i<iCount;i++)
	{
		CEdge * pEdge=ppVert[i];
		TRACE(L"Edge %02d: Edge@%02d times as 0x%02x, position %08d, Guide@%03d\n",
			i, pEdge->GetControlCount(), 
			pEdge->GetGuide()->Attatchment(),
			pEdge->GetPosition(),
			pEdge->GetGuide()->NumAttatchments()
			);
	}
#endif

	iCount=m_ParentInfo.GetNumHoriz();
	Sort(ppHoriz, iCount );
#ifdef _DEBUG
	TRACE(L"Horiz edge information %d edges\n", iCount );
	for(i=0;i<iCount;i++)
	{
		CEdge * pEdge=ppHoriz[i];
		CGuide * pGuide=pEdge->GetGuide();
		TRACE(L"Edge %02d: Edge@%02d times as 0x%02x, position %08d, Guide@%03d\n",i,
			pEdge->GetControlCount(), 
			pEdge->Attatchment(),
			pGuide->GetPosition(),
			pEdge->GetGuide()->NumAttatchments()
			);
	}
#endif

	//
	// Determin rows and columns
	//
	DeterminCols( ppVert, m_ParentInfo.GetNumVert() );
	DeterminRows( ppHoriz, m_ParentInfo.GetNumHoriz() );
	PlaceControls();

	delete [] ppVert;
	delete [] ppHoriz;

	DeterminWeights();
}

////////////////////////////////////////////////////////////////////////
//
// Sorts an array of edges.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::Sort(CEdge * *ppEdges, int iCount)
{
	//
	// Lame sort BB fix later.
	//
	int iInsert=0;
	while ( iInsert < iCount )
	{
		int iSmallest=iInsert;
		for(int iMatch=iInsert; iMatch<iCount; iMatch++)
		{
			if( ppEdges[iMatch]->GetPosition() < ppEdges[iSmallest]->GetPosition() )
				iSmallest=iMatch;
		}
		if( iSmallest > iInsert )
		{
			CEdge * pTemp=ppEdges[iInsert];
			ppEdges[iInsert]=ppEdges[iSmallest];
			ppEdges[iSmallest]=pTemp;
		}
		// TRACE(L"%02d is %08d\n", iInsert, ppEdges[iInsert]->GetPosition() );
		iInsert++;
	}
}


////////////////////////////////////////////////////////////////////////
//
// State table kinda used to determin when a new col. is needed
// Prev	This	New Col?
// L	L		Yes, on this edge
// L	R		Yes, on this edge
// R	L		Yes, between these two edges
// R	R		Yes if last (no if on same guide)?
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DeterminCols(CEdge * * ppEdges, int iCount)
{
	//
	// First pass is just to work out positions of edges,
	// and if those edges constitute Columns.
	//
	int i;
	int iCols=0;
	int iLastGuide;
	int iThisGuide;
	int * iPos = new int[iCount];			// where the edges are
	iPos[0]=ppEdges[0]->GetPosition();		// we always use the first edge.
	int iLastPos=0;
	for(i=1; i<iCount;i++)
	{
		iThisGuide=ppEdges[i]->Attatchment() ;
		iLastGuide=ppEdges[i-1]->Attatchment();
		TRACE(L"Edge:%02d - Attatched as %02d\n", i, iThisGuide);

		//
		// Column between these two controls 
		//
		if( (iLastGuide & RIGHT_AT ) && ( iThisGuide & LEFT_AT ) )
		{
			iCols++;
			iPos[iCols] = ppEdges[i-1]->GetPosition() + ((ppEdges[i]->GetPosition() - ppEdges[i-1]->GetPosition())/2);
			continue;
		}

		//
		// If we're starting another left edge of a control, needs a new guide
		//
		if( (iThisGuide & LEFT_AT ) )
		{
			iCols++;
			iPos[iCols]= ppEdges[i]->GetPosition();
			continue;
		}

		//
		// If this is the last right edge
		//
		if( (iThisGuide & RIGHT_AT ) && ((i+1)==iCount) )
		{
			iCols++;
			iPos[iCols] = ppEdges[i]->GetPosition() ;
			continue;	// just incase you add anytyhing below
		}
	}


	//
	// Second pass is to make up the column information
	// we don't allow narrow colums, that is columns who are <2 appart.
	//
	TRACE(L"Column Widths are ...\n");
	SetNumCols( iCols );			// 0 through n are USED. Not n-1
	m_Cols = new CHANNEL[iCols+1];
	m_Cols[0].Pos=iPos[0];
	iLastPos=0;
	int iThisCol=1;
	for(int iThisPos=1;iThisPos<=iCols;iThisPos++)
	{
		int iWidth=iPos[iThisPos]-iPos[iLastPos];
		if( iWidth >= MIN_COL_SPACE )
		{
			m_Cols[iThisCol].Pos = iPos[iThisPos];
			m_Cols[iThisCol-1].Size = iWidth;
			m_Cols[iThisCol-1].iWeight=0;
			m_Cols[iThisCol-1].iFixed=FALSE;
			/*
			TRACE(L"Col:%02d Width:%03d Pos:%03d\n",	
				iThisCol-1, 
				m_Cols[iThisCol-1].Size, 
				m_Cols[iThisCol-1].Pos);
			*/
			iLastPos=iThisPos;
			iThisCol++;
		}
		else
		{
			// TRACE(L"Skipping col #%d as it's only %d wide\n",iThisPos,iWidth);
		}
	}
#ifdef _DEBUG
	if( (iThisCol-1) != iCols )
		TRACE(L"Skipped %d rows\n",iThisCol-1-iCols);
#endif
	SetNumCols( iThisCol-1 );

	delete [] iPos;
}

////////////////////////////////////////////////////////////////////////
//
// State table kinda used to determin when a new col. is needed
// Prev	This	New Col?
// L	L		Yes, on this edge
// L	R		Yes, on this edge
// R	L		Yes, between these two edges
// R	R		Yes if last (no if on same guide)?
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DeterminRows(CEdge * * ppEdges, int iCount)
{
	int i;
	int iRows=0;
	int iLastGuide;
	int iThisGuide;
	int * iPos = new int[iCount];

	//
	// brute force, each edge is a row.
	//
	iRows=0;
	iPos[0]=ppEdges[0]->GetPosition();
	int iLastPos=0;
	for(i=1; i<iCount;i++)
	{
		iThisGuide=ppEdges[i]->Attatchment() ;
		iLastGuide=ppEdges[i-1]->Attatchment();
		TRACE(L"Edge:%02d - Attatched as %02d pos:%03d\n", i, iThisGuide, ppEdges[i]->GetPosition() );

		//
		// row between these two controls 
		//
		if( (iLastGuide & BOTTOM_AT ) && ( iThisGuide & TOP_AT ) )
		{
			iRows++;
			iPos[iRows] = ppEdges[i-1]->GetPosition() + ((ppEdges[i]->GetPosition() - ppEdges[i-1]->GetPosition())/2);
			continue;
		}

		//
		// If we're starting another left edge of a control, needs a new guide
		//
		if( (iThisGuide & TOP_AT ) )
		{
			iRows++;
			iPos[iRows]= ppEdges[i]->GetPosition();
			continue;
		}

		//
		// If this is the last right edge
		//
		if( (iThisGuide & BOTTOM_AT ) && ((i+1)==iCount) )
		{
			iRows++;
			iPos[iRows] = ppEdges[i]->GetPosition() ;
			continue;	// just incase you add anytyhing below
		}
	}

	//
	// Second pass is to make up the column information
	// we don't allow narrow colums, that is columns who are <2 appart.
	//
	TRACE(L"Rowumn Widths are ...\n");
	SetNumRows( iRows );			// 0 through n are USED. Not n-1
	m_Rows = new CHANNEL[iRows+1];
	m_Rows[0].Pos=iPos[0];
	iLastPos=0;
	int iThisRow=1;
	for(int iThisPos=1;iThisPos<=iRows;iThisPos++)
	{
		int iWidth=iPos[iThisPos]-iPos[iLastPos];
		if( iWidth >= MIN_ROW_SPACE )
		{
			m_Rows[iThisRow].Pos = iPos[iThisPos];
			m_Rows[iThisRow-1].Size = iWidth;
			m_Rows[iThisRow-1].iWeight=0;
			m_Rows[iThisRow-1].iFixed=FALSE;
			/*
			TRACE(L"Row:%02d Width:%03d Pos:%03d\n",	
				iThisRow-1, 
				m_Rows[iThisRow-1].Size, 
				m_Rows[iThisRow-1].Pos);
			*/
			iLastPos=iThisPos;
			iThisRow++;
		}
		else
		{
			// TRACE(L"Skipping Row #%d as it's only %d wide\n",iThisPos,iWidth);
		}
	}
#ifdef _DEBUG
	if( (iThisRow-1) != iRows )
		TRACE(L"Skipped %d rows\n",iThisRow-1-iRows);
#endif
	SetNumRows( iThisRow-1 );

	delete [] iPos;
}

////////////////////////////////////////////////////////////////////////
//
// Draws the annotations on the dialog of where the column/rows are
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::Annotate()
{
	if(GetAnnotate()==false)
		return;

	HDC hdc=GetDC( GetWindow() );
	//
	// Draw all the control edges on the screen.
	//
	// m_ParentInfo.Annotate( hdc );

	//
	// Now show the cols/rows
	//
	int iCount=GetNumRows();
	int i;
	RECT r;
	GetWindowRect( GetWindow(), &r);
	int iWidth=r.right - r.left;
	int iHeight=r.bottom - r.top;

	HPEN hFixedPen = CreatePen( PS_SOLID, 2, RGB( 0x00,0x00,0xff) );
	HPEN hSizePen = CreatePen( PS_SOLID, 2, RGB( 0x00,0xff,0x00) );
	HGDIOBJ holdPen= SelectObject( hdc, hFixedPen);

	//
	// Horizontal lines
	//
	for(i=0;i<=iCount;i++)
	{
		if(m_Rows[i].iFixed >= 0 )
			SelectObject( hdc, hSizePen );
		else
			SelectObject( hdc, hFixedPen );
		MoveToEx( hdc, 0, m_Rows[i].Pos, NULL );
		LineTo( hdc, iWidth , m_Rows[i].Pos );
	}

	//
	// Vertical lines
	//
	iCount = GetNumCols();
	for(i=0;i<=iCount;i++)
	{
		if(m_Cols[i].iFixed >= 0 )
			SelectObject( hdc, hSizePen );
		else
			SelectObject( hdc, hFixedPen );
		MoveToEx( hdc, m_Cols[i].Pos, 0, NULL );
		LineTo( hdc, m_Cols[i].Pos, iHeight );
	}

	SelectObject(hdc, holdPen);
	DeleteObject(hFixedPen);
	DeleteObject(hSizePen);

	ReleaseDC( GetWindow(), hdc );
}

////////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////////
int CResizeDlg::FindRow(int pos)
{
	int i=0;
	while( i<=GetNumRows() )
	{
		if( m_Rows[i].Pos > pos )
			return i-1;
		i++;
	}
	return GetNumRows();
}

////////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////////
int CResizeDlg::FindCol(int pos)
{
	int i=0;
	while( i<=GetNumCols() )
	{
		if( m_Cols[i].Pos > pos )
			return i-1;
		i++;
	}
	return GetNumCols();
}

////////////////////////////////////////////////////////////////////////
//
// Walks the rows/columns and determins if they are resizable.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DeterminWeights()
{
	int iColWeight=0;
	{
		for( int fi=0;fi<= GetNumCols(); fi++)
			m_Cols[ fi ].iFixed<0 ? 0: iColWeight+=m_Cols[fi].iWeight;
	}
	int iRowWeight=0;
	{
		for( int fi=0;fi<= GetNumRows(); fi++)
			m_Rows[ fi ].iFixed<0 ? 0: iRowWeight+=m_Rows[fi].iWeight;
	}
	SetColWeight(iColWeight);
	SetRowWeight(iRowWeight);
}

////////////////////////////////////////////////////////////////////////
//
// Passed in the current hit test, allows you to override it.
// return HTERROR -1 if we didn't deal with it.
//
////////////////////////////////////////////////////////////////////////
LONG CResizeDlg::HitTest(LONG lCurrent)
{
#define WIDE 1
#define HIGH 2
	int iThisDlg = (GetRowWeight() ? HIGH : 0 ) | ( GetColWeight() ? WIDE : 0 );

	// TRACE(L"NC Hit Test %d - dlg is %d\n",lCurrent, iThisDlg);
	switch( lCurrent )
	{
		case HTLEFT: // In the left border of a window 
		case HTRIGHT: // In the rigcase HT border of a window 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( iThisDlg & WIDE )
				return lCurrent;	// OK
			return HTNOWHERE;

		case HTTOP: // In the upper horizontal border of a window 
		case HTBOTTOM: // In the lower horizontal border of a window 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( iThisDlg & HIGH ) 
				return lCurrent;	// OK
			return HTNOWHERE;		// Can't make taller

		case HTTOPLEFT: // In the upper-left corner of a window border 
		case HTTOPRIGHT: // In the upper rigcase HT corner of a window border 
			if( iThisDlg==0)
				return HTNOWHERE;

			if( (iThisDlg & (HIGH | WIDE)) == (HIGH | WIDE ))
				return lCurrent;
			if( iThisDlg & HIGH )
				return HTTOP;
			if( lCurrent == HTTOPLEFT )
				return HTLEFT;
			return HTRIGHT;

		case HTGROWBOX: // In a size box (same as case HTSIZE) 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( (iThisDlg & (HIGH | WIDE)) == (HIGH | WIDE ))
				return lCurrent;
			if( iThisDlg & HIGH )
				return HTBOTTOM;
			return HTRIGHT;

		case HTBOTTOMLEFT: // In the lower-left corner of a window border 
		case HTBOTTOMRIGHT: // In the lower-rigcase HT corner of a window border 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( (iThisDlg & (HIGH | WIDE)) == (HIGH | WIDE ))
				return lCurrent;
			if( iThisDlg & HIGH )
				return HTBOTTOM;
			if(lCurrent == HTBOTTOMRIGHT)
				return HTRIGHT;
			return HTLEFT;
	}
	return HTERROR-1;
}

////////////////////////////////////////////////////////////////////////
//
// Determins the location of the controls in the row/col space.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::PlaceControls()
{
	//
	// Now setup the information for all of the controls, as to which cell they are in.
	//
	int i=0;
	CResizeControl * pC;
	CConstraint	* pCons;
	while( pC=(CResizeControl *) m_ControlList.GetPointer(i++) )
	{
		int Row, Col, RowH, ColW;

		RECT r=pC->GetLocation();
		pCons=&(pC->m_Cons);

		Row  = FindRow( r.top);
		RowH = FindRow( r.bottom ) - Row +1 ;
		if( RowH + Row > GetNumRows() )
			RowH = GetNumRows() - Row;

		Col  = FindCol( r.left);
		ColW = FindCol( r.right ) - Col + 1;
		if( ColW + Col > GetNumCols() )
			ColW = GetNumCols() - Col;

		pCons->SetCol( Col );
		pCons->SetRow( Row );
		pCons->SetColW( ColW );
		pCons->SetRowH( RowH );

		//
		// Now adjust the padding from the edges of the cell.
		//
		pCons->SetPadTop   ( r.top    - m_Rows[ Row ].Pos );
		pCons->SetPadBottom( r.bottom - m_Rows[ Row + RowH ].Pos );
		pCons->SetPadLeft  ( r.left   - m_Cols[ Col ].Pos );
		pCons->SetPadRight ( r.right  - m_Cols[ Col + ColW ].Pos  );

		/*
		IF you're worried that you got the dimentions wrong
		TRACE(L"Component %03d [%06d] %s\n pos: l:%03d, r:%03d, t:%03d, b:%03d\nCell: l:%03d, r:%03d, t:%03d, b:%03d\n",
			i, GetDlgCtrlID( pC->GetControl()),  pC->GetClassName(), r.left, r.right, r.top, r.bottom,
			m_Cols[pCons->GetCol()].Pos + pCons->GetPadLeft(),
			m_Cols[pCons->GetCol()+pCons->GetColW()].Pos + pCons->GetPadRight(),
			m_Rows[pCons->GetRow()].Pos + pCons->GetPadTop(),
			m_Rows[pCons->GetRow()+pCons->GetRowH()].Pos + pCons->GetPadBottom() );
		*/

		TRACE(L"Component %03d [%06d] %s\n pos: Cell: %03d,%03d by %03d x %03d\n Padding:l:%03d, r:%03d, t:%03d, b:%03d\n",
			i, GetDlgCtrlID( pC->GetControl()),  pC->GetClassName(), 
			pCons->GetCol(), pCons->GetRow(), pCons->GetColW(), pCons->GetRowH(),
			pCons->GetPadLeft(), pCons->GetPadRight(), pCons->GetPadTop(), pCons->GetPadBottom() );

		//
		// Mark each column this control spans as gets wider.
		//
		int iCol=pCons->GetCol();
		int ic=pCons->GetColW();
		while( ic-- )
		{
			if( pC-> IsGrowsWide() == false )
				m_Cols[ iCol+ic ].iFixed-=1;	// can't make wider
			else
			{
				m_Cols[ iCol+ic ].iWeight++;
				m_Cols[ iCol+ic ].iFixed+=2;	// can make wider
			}
		}

		ic=pCons->GetRowH();
		int iRow=pCons->GetRow();
		while( ic-- )
		{
			if( pC-> IsGrowsHigh() == false )
				m_Rows[ iRow+ic ].iFixed-=1;	// can't make wider
			else
			{
				m_Rows[ iRow+ic ].iWeight++;
				m_Rows[ iRow+ic ].iFixed+=2;	// can make wider
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Determins if we can actually change size to that asked for.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DoChangePos(WINDOWPOS * lpwp)
{
	if((lpwp->flags & SWP_NOSIZE) == FALSE)
	{
		//
		// We're being sized - size in screen area (ie. included title bar)
		// m_ParentInfo EXCLUDES title bar.
		//
		RECT r;
		GetWindowRect( GetWindow(), &r );
		if( GetRowWeight() <= 0 )
			lpwp->cy=r.bottom- r.top;
		if( GetColWeight() <= 0 )
			lpwp->cx=r.right - r.left;
		DIMENSION dMin= m_ParentInfo.GetMinimumSize();
		if( lpwp->cx < dMin.Width )
			lpwp->cx=dMin.Width;
		if( lpwp->cy < dMin.Height )
			lpwp->cy=dMin.Height;	
	}
}

////////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DoInitDialog()
{
	AddToSystemMenu();
	WalkControls();
	ResizeControls( (WORD)m_ParentInfo.GetWidth() , (WORD)m_ParentInfo.GetHeight() );
}

////////////////////////////////////////////////////////////////////////
//
// Adds an annotate option to the system menu.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::AddToSystemMenu()
{
	/*
	//
	// Get the system menu - add an annotate option to it?
	//
	HMENU hm=GetSystemMenu(GetWindow(), false);
	TCHAR	szAnnotate[128];
	LoadString( GetInstance(), IDS_ANNOTATE, szAnnotate,  128);
	MENUITEMINFO minfo={
		sizeof(MENUITEMINFO),
		MIIM_ID | MIIM_TYPE,
		MFT_STRING,
		0,
		IDS_ANNOTATE,		// ID
		NULL,				// SubMenu
		NULL,				// Checked
		NULL,				// Unchecked
		0,					// Item data - app specific
		szAnnotate,			// Content of the menu item
		lstrlen(szAnnotate)+1// Length of menu item data
	};

	InsertMenuItem( hm, -1, TRUE, &minfo);
	*/
}

void CResizeDlg::AddGripper()
{
	int g_cxGrip = GetSystemMetrics( SM_CXVSCROLL );
	int g_cyGrip = GetSystemMetrics( SM_CYHSCROLL );
	RECT rc;
	GetClientRect( GetWindow(), &rc);
	m_hwndGripper = CreateWindow( TEXT("Scrollbar"),
                     NULL,
                     WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS |
                       WS_CLIPCHILDREN | SBS_BOTTOMALIGN | SBS_SIZEGRIP |
                       SBS_SIZEBOXBOTTOMRIGHTALIGN,
                     rc.right - g_cxGrip,
                     rc.bottom - g_cyGrip,
                     g_cxGrip,
                     g_cyGrip,
                     GetWindow(),
                     (HMENU)-1,
                     (HINSTANCE)GetWindowLong( GetWindow(), GWL_HINSTANCE ) , // g_hinst,
                     NULL );
}

HDWP CResizeDlg::SetGripperPos(HDWP hdwp)
{
	if( m_hwndGripper != NULL )
	{
		int g_cxGrip = GetSystemMetrics( SM_CXVSCROLL );
		int g_cyGrip = GetSystemMetrics( SM_CYHSCROLL );
		RECT rc;
		GetClientRect( GetWindow(), &rc);
		return DeferWindowPos( hdwp, m_hwndGripper, NULL,
				 rc.right - g_cxGrip,
				 rc.bottom - g_cyGrip,
				 g_cxGrip,
				 g_cyGrip,
					SWP_NOZORDER | SWP_NOSIZE );
	}
	else
		return hdwp;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CScaleDlg::CScaleDlg(int DlgID, HWND hWndParent, HINSTANCE hInst)
: BASECLASS(DlgID, hWndParent, hInst)
{
}

CScaleDlg::~CScaleDlg()
{

}

//
// Multiply the positions as a result of what they are now.
//
void CScaleDlg::ResizeControls(WORD width, WORD height)
{
	//
	// Perhaps we just scale the fontsize information too?
	//
	int OrigW = m_ParentInfo.GetRightBorder();
	OrigW = m_ParentInfo.GetWidth();
	int OrigH = m_ParentInfo.GetBottomBorder();
	OrigH = m_ParentInfo.GetHeight();

	//
	// SetDialogFont
	//
	HFONT hFont=(HFONT)SendMessage( GetWindow(), WM_GETFONT, 0,0 );
	LOGFONT lf;
	GetObject( (HGDIOBJ)hFont, sizeof(lf), &lf);
	int scalew= width * 100 / OrigW;
	int scaleh=height * 100 / OrigH;
	int scale=scalew;

	if( scalew < scaleh )
		scale=scaleh;

	lf.lfHeight = m_dwInitFontSize * scale / 100;
	HFONT hNewFont=CreateFontIndirect( &lf );

	HWND hCurrent;
	HWND hFirst;
	hCurrent=hFirst=::GetWindow(GetWindow(),GW_CHILD);
	if( hFirst )
	{
		do {
			SendMessage( hCurrent, WM_SETFONT, (WPARAM)hNewFont, 1 );
		} while (hCurrent=::GetWindow(hCurrent,GW_HWNDNEXT));
	}

	// newX=origX * width / origParentWidth
	//
	// Now setup the information for all of the controls, as to which cell they are in.
	//
	HDWP hdwp = BeginDeferWindowPos( m_ControlList.GetControlCount() +1 ); // 1 for the gripper
	int i=0;
	CResizeControl * pC;
	while( pC=(CResizeControl *) m_ControlList.GetPointer(i++) )
	{
		int x=pC->GetLeftGap(); // GetLeftEdge()->GetPosition();
		int y=pC->GetTopGap(); // GetTopEdge()->GetPosition();
		int w=OrigW - pC->GetRightGap() - x; // GetRightEdge()-GetPosition() - x;
		int h=OrigH - pC->GetBottomGap() - y; // BottomEdge()-GetPosition() -y;

		int newx=x * width / OrigW;
		int newy=y * height / OrigH;

		int neww=w * width / OrigW;
		int newh=h * height / OrigH;


		BOOL bMove=false;
		if( (newx != x) || (newy != y))
			bMove=true;
		
		BOOL bSize=false;
		if( (newh != h) || (neww != w))
			bSize=true;

		hdwp = DeferWindowPos( hdwp, pC->GetControl(), NULL, 
			newx, newy, neww , newh,
			(bMove ? 0: SWP_NOMOVE) |	(bSize ? 0: SWP_NOSIZE) | SWP_NOZORDER );

	}

	//
	// Move the gripper
	//
	SetGripperPos(hdwp);

	EndDeferWindowPos( hdwp );

}

void CScaleDlg::DeterminWeights()
{
	SetRowWeight(1);
	SetColWeight(1);
	HFONT hFont=(HFONT)SendMessage( GetWindow(), WM_GETFONT, 0,0 );
	LOGFONT lf;
	GetObject( hFont, sizeof(lf), &lf);
	m_dwInitFontSize = lf.lfHeight;
}

void CScaleDlg::DoChangePos(WINDOWPOS * lpwp)
{
	// Lock the apsect ratio.
	if((lpwp->flags & SWP_NOSIZE) == FALSE)
	{
		if( dwLastCX != lpwp->cx )
			dwLastCY = lpwp->cy = (lpwp->cx * m_ParentInfo.GetHeight()) / m_ParentInfo.GetWidth();

		if( dwLastCY != lpwp->cy )
			dwLastCX = lpwp->cx = (lpwp->cy * m_ParentInfo.GetWidth()) / m_ParentInfo.GetHeight();
	}
}

void CResizeDlg::DeterminNumberOfControls()
{
	int iControlCount=0;
	HWND hCurrent;
	HWND hFirst;
	hCurrent=hFirst=::GetWindow(GetWindow(),GW_CHILD);
	if( hFirst )
	{
		do {
			iControlCount++;
		} while (hCurrent=::GetWindow(hCurrent,GW_HWNDNEXT));
	}
	m_ControlCount=iControlCount;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Looks for the last row/col being full of buttons.
//
//////////////////////////////////////////////////////////////////////////////////////////
void CResizeDlg::SpecialRowCol()
{
	TRACE("There are %d rows, and %d columns\n", GetNumRows(), GetNumCols() );

	//
	// Walk the controls, find the row/col they are in.
	//
	BOOL bSpecialRow=TRUE;
	BOOL bSpecialCol=TRUE;
	int iNumRows=GetNumRows()-1;
	int iNumCols=GetNumCols()-1;

	//
	// It is a special row if all the heights are the same
	// it is a special col if all the widths are the same
	//
	int iRowHeight=0;
	int iColWidth=0;

	//
	// It's a row/col if there is more than one control there
	//
	int iRowCount=0;
	int iColCount=0;

	//
	// We remember the bounds of the rows/cols
	// left/right is the left of the left button, and the right of the right
	// top/bottom is the top of the topmost, and the bottom of the bottom most.
	//
	RECT	bounds;		// this is the 
	bounds.left=m_ParentInfo.GetWidth();
	bounds.right=0;
	bounds.top=m_ParentInfo.GetHeight();
	bounds.bottom=0;

	int i=0;
	CResizeControl * pC;
	CConstraint	* pCons;
	while( pC=(CResizeControl *) m_ControlList.GetPointer(i++) )
	{
		pCons=&(pC->m_Cons);

		//
		// Column work (widths same)
		//
		if( bSpecialCol )
		{
			if( ((pCons->GetCol() == iNumCols ) || (pCons->GetCol() + pCons->GetColW() > iNumCols) ))
			{
				if( iColWidth==0 )
					iColWidth=pC->GetWidth();
				if( pC->GetWidth() != iColWidth )
					bSpecialCol=FALSE;
				iColCount++;
			}

			//
			// If this item is wholy in this column
			//
			if( pCons->GetCol() == iNumCols )
			{
				RECT r=pC->GetLocation();
				if( r.top < bounds.top )
					bounds.top = r.top;
				if( r.bottom > bounds.bottom )
					bounds.bottom = r.bottom;
			}
		}

		if( bSpecialRow )
		{
			if( (pCons->GetRow() == iNumRows ) || (pCons->GetRow() + pCons->GetRowH() > iNumRows) )
			{
				if( iRowHeight==0 )
					iRowHeight=pC->GetHeight();
				if( pC->GetHeight() != iRowHeight )
					bSpecialRow=FALSE;
				iRowCount++;
			}

			//
			// If this item is wholy in this row
			//
			if( pCons->GetRow() == iNumRows )
			{
				RECT r=pC->GetLocation();
				if( r.left < bounds.left )
					bounds.left = r.left;
				if( r.right > bounds.right )
					bounds.right = r.right;
			}
		}

		if( (bSpecialCol==FALSE) && ( bSpecialRow==FALSE ) )
			break;
	}

	m_SpecialRow.bSpecial = iRowCount>1?bSpecialRow:FALSE;
	m_SpecialCol.bSpecial = iColCount>1?bSpecialCol:FALSE;

	//
	// Check the row alignment.
	//
	if( m_SpecialRow.bSpecial )
	{
		int lGap = bounds.left - m_ParentInfo.GetLeftBorder();
		int rGap = m_ParentInfo.GetWidth() - m_ParentInfo.GetRightBorder() - bounds.right;
		TRACE("Constraits on the special row are: left %d, right %d\n",bounds.left, bounds.right );
		TRACE("Parent info is: left %d, right %d\n", m_ParentInfo.GetLeftBorder(), m_ParentInfo.GetWidth() - m_ParentInfo.GetRightBorder() );
		TRACE("Gaps are: left %d, right %d\n", lGap, rGap );
		int GapDiff = lGap - rGap;
		m_SpecialRow.bSpecial = TRUE;
		m_SpecialRow.iMin = bounds.left;
		m_SpecialRow.iMax = bounds.right;
		if( GapDiff < -10 ) 
		{
			TRACE("Probably a left aligned thing\n");
			m_SpecialRow.iAlignment = -1;
		}
		else
		if( GapDiff > 10 )
		{
			TRACE(" Probably a right aligned thing\n");
			m_SpecialRow.iAlignment = 1;
		}
		else
		{
			TRACE(" Probably a centered thing\n");
			m_SpecialRow.iAlignment = 0;
		}
		m_SpecialRow.iDiff=GapDiff;
	}

	//
	// Check the Col alignment.
	//
	if( m_SpecialCol.bSpecial  )
	{
		int tGap = bounds.top - m_ParentInfo.GetTopBorder();
		int bGap = m_ParentInfo.GetHeight() - m_ParentInfo.GetBottomBorder() - bounds.bottom;
		TRACE("Constraits on the special Col are: top %d, bottom %d\n",bounds.top, bounds.bottom);
		TRACE("Parent info is: top %d, bottom %d\n", m_ParentInfo.GetTopBorder(), m_ParentInfo.GetHeight() - m_ParentInfo.GetBottomBorder() );
		TRACE("Gaps are: top %d, bottom %d\n", tGap, bGap );
		int GapDiff = tGap - bGap;
		m_SpecialCol.iMin = bounds.top;
		m_SpecialCol.iMax = bounds.bottom;
		if( GapDiff < -10 ) 
		{
			TRACE("Probably a left aligned thing\n");
			m_SpecialCol.iAlignment = -1;
		}
		else
		if( GapDiff > 10 )
		{
			TRACE(" Probably a right aligned thing\n");
			m_SpecialCol.iAlignment = 1;
		}
		else
		{
			TRACE(" Probably a centered thing\n");
			m_SpecialCol.iAlignment = 0;
		}
		m_SpecialCol.iDiff=GapDiff;
	}
}
