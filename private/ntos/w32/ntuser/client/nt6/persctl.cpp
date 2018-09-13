//
// CResizeControl
//
// FelixA
//
// Resize controls

#include "pch.h"

#include "persctl.h"
#include "debug.h"
#define ABS(x) (x<0?-x:x)

////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Chesshire cat the controls.
//
//
// Need to catagorize controls - re-sizing and non-resizing controls
// use a registry based mapping from classname to object.
//
CResizeControl::CResizeControl(HWND hC,CParentInfo & pi)
{
	// Get the classname for the control.
	TCHAR szClassName[64];
	m_pC=NULL;
	::GetClassName(hC,szClassName,sizeof(szClassName));

	/*
	// See what class can persist this type of control.
	// can use lookup in the registry to determin.
	if(lstrcmpi(szClassName,"Edit")==0)
		m_pC= new CResizeEditControl(hC, pi);
	if(lstrcmpi(szClassName,"Button")==0)
		m_pC= new CResizeButtonControl(hC, pi);
	// if(lstrcmpi(szClassName,"ComboBox")==0)
	//	m_pC= new CResizeComboControl(hC, pi);
	*/

	// There is no Resize form of this control.
	if(!m_pC)
	{
		// OutputDebugString("Cannot persist:");
		// OutputDebugString(szClassName);
		// OutputDebugString("\n");
		//
		// We need to map it though.
		//
		m_pC=new CResizeControlGeneric( hC, pi, this );
	}
}


//
// Work out where we are on the dialog.
//
BOOL CResizeControlGeneric::DeterminLeftAttatchments()
{
	m_pLeftEdge		= m_Parent.AddEdge( GetLeftGap(), LEFT_AT, IsGrowsWide() );
	return FALSE;
}


//
// We determin the attachements differently for each edge - not sure if this is
// correct, or even good.
//
BOOL CResizeControlGeneric::DeterminRightAttatchments()
{
	m_pRightEdge=NULL;
	/*
	TRACE(L"Control %08d %s - Determin Location \n", GetDlgCtrlID( GetControl() ), GetClassName() );
	TRACE(L"Right Size is %03d\n", GetLeftGap() + GetWidth() ,m_RightSlop);
	*/

	m_pRightEdge	= m_Parent.AddEdge( GetLeftGap() + GetWidth(), RIGHT_AT, IsGrowsWide(), m_RightSlop );

#ifdef _DEBUG
	if( m_pRightEdge->GetPosition() != GetLeftGap() + GetWidth() )
		TRACE(L"RIGHT EDGE WRONG - In actuality %03d should be %03d\n", m_pRightEdge->GetPosition() , GetLeftGap() + GetWidth());
#endif

	return FALSE;
}

BOOL CResizeControlGeneric::DeterminTopAttatchments()
{
	m_pTopEdge		= m_Parent.AddEdge( GetTopGap(), TOP_AT, IsGrowsHigh(), 0 );
	return FALSE;
}

BOOL CResizeControlGeneric::DeterminBottomAttatchments()
{
	m_pBottomEdge	= m_Parent.AddEdge( GetTopGap()+GetHeight(), BOTTOM_AT, IsGrowsHigh(), 0 );
	return FALSE;
}

//
// We resize the control, related to the width and height provided.
// The width and height come from the parent (for now).
//
BOOL CResizeControlGeneric::Resize(WORD width, WORD height)
{
	//
	// A control is deliniated by its guides
	//
	if(true)
	{
		int x,y,w,h;
		x=m_pLeftEdge->GetPosition();
		w=m_pRightEdge->GetPosition() - x;
		y=m_pTopEdge->GetPosition();
		h=m_pBottomEdge->GetPosition() - y;

		BOOL bMove=true;
		BOOL bSize=true;
		SetWindowPos( GetControl(), NULL, 
		x, y, w, h,
		(bMove ? 0: SWP_NOMOVE) |	(bSize ? 0: SWP_NOSIZE) | SWP_NOZORDER );
		
		return true;
	}

//
// N O T   U S E D 
//
	//
	// Hmm, interesting, a button bound on the bottom to a movable thing, 
	// but its top is not bound.
	//
	BOOL bMove=FALSE;
	int	x,y,w,h;

	x=m_pLeftEdge->GetPosition();

	if( IsGrowsWide() == FALSE )
	{
		if( m_pLeftEdge->GetFlexible() == TRUE )
		{
			bMove|=TRUE;
		}
		else
		{
			//
			// The left guide isn't movable.
			//
			bMove|=m_pBottomEdge->GetFlexible();
			if( m_pRightEdge->GetGuide() != m_pLeftEdge->GetGuide() )
				x=m_pRightEdge->GetPosition() - GetWidth();
		}
	}

	y=m_pTopEdge->GetPosition();
	if( m_pTopEdge->GetFlexible() == TRUE )
	{
		bMove|=TRUE;
	}
	else
	{
		bMove|=m_pBottomEdge->GetFlexible();
		y=m_pBottomEdge->GetPosition() - GetHeight();
	}

	//
	// We've done some calculation to work out new edges ... verify this.
	//
	BOOL bSize=FALSE;
	w = m_pRightEdge->GetPosition() -x ;
	h = m_pBottomEdge->GetPosition() - y;
	if((w != GetWidth()) || ( h != GetHeight() ))
		bSize=TRUE;


	//
	//
	//
	x=m_pLeftEdge->GetPosition();
	w=m_pRightEdge->GetPosition() - x;
	y=m_pTopEdge->GetPosition();
	h=m_pBottomEdge->GetPosition() - y;

	if( !IsGrowsWide() && ( w != GetWidth() ) )
	{
		//
		// We're trying to widen this control, 
		//
		//
		// Could be because the right edge has moved.
		//
		TRACE(L"Left edge off by %03d\n", w-GetWidth() );
		CGuide * pGuide=m_pLeftEdge->GetGuide();
		// BB is equal
		if( pGuide == m_Parent.GetLeftEdge()->GetGuide() ) 
		{
			//
			// Left edge is fixed to border, leave it.
			//
			int i=5;
		}
		else
		{
			// BB move method
			pGuide->Adjust( w-GetWidth() );
			bSize=FALSE;
		}
	}

	if( !IsGrowsHigh() && ( h!=GetHeight() ) )
	{
		TRACE(L"Top edge off by %03d\n", h-GetHeight() );
		CGuide * pGuide=m_pTopEdge->GetGuide();
		pGuide->Adjust( h-GetHeight() );
		bSize=FALSE;
	}

	if( (GetLeftGap() == x) && (GetTopGap()==y ))
		bMove=FALSE;
	else
		bMove=TRUE;

	//
	// We now have the resize calculated.
	//
	if( bMove || bSize )
	{
		SetWindowPos( GetControl(), NULL, 
		x, y, w, h,
		(bMove ? 0: SWP_NOMOVE) |	(bSize ? 0: SWP_NOSIZE) | SWP_NOZORDER );
		
		SetX( x );
		SetY( y );
		SetTempWidth( w );
		SetTempHeight( h );
		return TRUE;		// yes we-resized.
	}

	return FALSE;
}

//
// GetOur position on the screen.
//
void CResizeControlGeneric::DeterminLocation()
{
	/*
	TRACE(L"Control %08d %s - Determin Location \n", GetDlgCtrlID( GetControl() ), GetClassName() );
	*/
	GetWindowRect( GetControl(), &m_Location);

	//
	// Our location relative to our parents. ???
	//
	m_Location.left -= m_Parent.GetLeft();
	m_Location.right -= m_Parent.GetLeft();
	m_Location.top -= m_Parent.GetTop();
	m_Location.bottom -= m_Parent.GetTop();

	MapWindowPoints( NULL, m_Parent.GetWindow(), (LPPOINT)&m_Location, 2);

	TRACE(L"L:%03d R:%03d T:%03d B:%03d\n", m_Location.left, m_Location.right, m_Location.top, m_Location.bottom );
	m_PreferredSize.Width=m_Location.right - m_Location.left;
	m_PreferredSize.Height=m_Location.bottom - m_Location.top;

	// REVIEW - slop is a %age of width
	// right slop is 3% of my width.
	// m_RightSlop = GetWidth() * 3 / 100;
	m_RightSlop=3;

	// REVIEW - if wide, make resizable IanEl
	// things get wider if they are 66% the width of the screen.

	if(false)
	{
		m_GrowsWide = ((GetWidth() * 100 / m_Parent.GetWidth() ) > 66);
		m_GrowsHigh = ((GetHeight() * 100 / m_Parent.GetHeight() ) > 66);
	}
	else
	{
		m_GrowsWide=TRUE;
		m_GrowsHigh=TRUE;
		TRACE(L"Class %s",GetClassName());
		if( lstrcmp( GetClassName(), L"Button") ==0)
		{
			//
			// Check box button is OK though.
			//
			int ibs=GetWindowStyle( GetControl() );
			switch( ibs & 0xf)
			{
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
				m_GrowsWide=FALSE;		// TRUE; what dialog things checks get wider
				m_GrowsHigh=FALSE;
				break;

			case BS_GROUPBOX:
			default:
				m_GrowsHigh=FALSE;
				m_GrowsWide=FALSE;
			}
			TRACE(L"Button style 0x%08x\n", ibs );
		}
		else
		if(lstrcmp( GetClassName(), L"Static")==0 )
		{
			int iss=GetWindowStyle( GetControl() ); // SS_

			//
			// Wrong check to see if it's multi line. All statics are multiline
			//
			/*
			if(iss & ES_MULTILINE )	// ???
			{
				m_GrowsHigh=TRUE;
				m_GrowsWide=TRUE;
			}
			else
			*/
			{
				m_GrowsHigh=FALSE;
				m_GrowsWide=TRUE;		// statics can always get wider.
				m_GrowsWide=FALSE;		// statics can always get wider.
			}
		}
		else
		if( lstrcmp( GetClassName(), L"ComboBox")==0)
		{
			m_GrowsHigh=FALSE;
		}
		else
		if( lstrcmp( GetClassName(), L"SysTabControl32") == 0 )
		{
			m_GrowsHigh=FALSE;	// alas we can't size the inside of these
			m_GrowsWide=FALSE;
		}
		else
		if( lstrcmp( GetClassName(), L"Edit")==0)
		{
			int ies=GetWindowStyle( GetControl() );
			if(!(ies & ES_MULTILINE ))
				m_GrowsHigh=FALSE;
		}
		else
		{
			if( *GetClassName() == '#' )
			{
				TRACE(L"Punting unknown class %s\n", GetClassName() );
				m_GrowsHigh=FALSE;	// alas we can't size the inside of these
				m_GrowsWide=FALSE;
			}
		}
	}
}


//
// We attatch this control to its guides.
//
BOOL CResizeControlGeneric::SetControlAssociations()
{
	m_pRightEdge->Attatch(m_pUs);
	m_pLeftEdge->Attatch(m_pUs);
	m_pTopEdge->Attatch(m_pUs);
	m_pBottomEdge->Attatch(m_pUs);

	/*
	TRACE(L"Control %08d %s - EdgePos, Guides, GuidePos\n", GetDlgCtrlID( GetControl() ), GetClassName() );
	TRACE( "[%08d %08d %08d %08d] {0x%08lx 0x%08lx 0x%08lx 0x%08lx}\n(%08d %08d %08d %08d)\n", 
		m_pLeftEdge->GetPosition(),
		m_pRightEdge->GetPosition(),
		m_pTopEdge->GetPosition(),
		m_pBottomEdge->GetPosition(),

		m_pLeftEdge->GetGuide(),
		m_pRightEdge->GetGuide(),
		m_pTopEdge->GetGuide(),
		m_pBottomEdge->GetGuide(),

		m_pLeftEdge->GetGuide()->GetPosition(),
		m_pRightEdge->GetGuide()->GetPosition(),
		m_pTopEdge->GetGuide()->GetPosition(),
		m_pBottomEdge->GetGuide()->GetPosition()
		);
	*/
	return TRUE;
}

//
// Constructor for a generic control.
//
CResizeControlGeneric::CResizeControlGeneric(HWND hC, CParentInfo & pi, CResizeControl * pUs)
	: m_hC(hC), m_Parent(pi), m_pLeftEdge(NULL), m_pRightEdge(NULL), m_pTopEdge(NULL),
	m_pBottomEdge(NULL), m_GrowsWide(FALSE), m_GrowsHigh(FALSE),
	m_RightSlop(0), m_pUs(pUs)
{
	TCHAR szClassName[256];
	int iSize=::GetClassName(hC,szClassName,sizeof(szClassName));
	m_ClassName=new TCHAR[iSize+1];
	lstrcpy( m_ClassName, szClassName);
	//
	// Remember the initial size, this is its preferred size.
	//
	DeterminLocation();
	
}

CResizeControlGeneric::~CResizeControlGeneric()
{
	delete m_ClassName;
}


//
// Returns a bitfield where the guide is attatched.
//
int CResizeControlGeneric::Attatchment(CGuide * pGuide)
{
	int iRet=0;
	if( pGuide->IsEqual( m_pLeftEdge->GetGuide() ) )
		iRet |= LEFT_AT;
	if( pGuide->IsEqual( m_pRightEdge->GetGuide() ) )
		iRet |= RIGHT_AT;
	if( pGuide->IsEqual( m_pTopEdge->GetGuide() ) )
		iRet |= TOP_AT;
	if( pGuide->IsEqual( m_pBottomEdge->GetGuide() ) )
		iRet |= BOTTOM_AT;
	return iRet;
}
