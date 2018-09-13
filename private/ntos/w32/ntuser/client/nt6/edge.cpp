// Edge.cpp: implementation of the CEdge class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "Edge.h"
#include "debug.h"
#include "persctl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEdge::CEdge()
: m_Guide( new CGuide() ), m_Attatched(NULL)
{
	SetGuide(m_Guide);
}

CEdge::~CEdge()
{

}

CEdge::CEdge( int Position, int Axis, BOOL Flexible, int Offset)
: m_Guide( new CGuide(Position, Axis, Flexible) ), m_Attatched(NULL)
{
	SetOffsetFromGuide(Offset);
	SetGuide(m_Guide);
}

CEdge::CEdge( CGuide * pGuide, int Offset)
: m_Guide(pGuide), m_Attatched(NULL)
{
	SetOffsetFromGuide(Offset);
	SetGuide(m_Guide);
}

//
// Need to take all the attatched components from this guide, and
// add them to the other guide.
//
void	CEdge::SetGuide(CGuide * pGuide)
{ 
	//
	// We still have the same number of controls attatched to this edge
	// remove these controls from the guide.
	//
	if( pGuide != m_Guide )
	{
		TRACE(L"Changing the guide - need to re-attatch?\n");
		m_Guide->DeAttatch(this);
	}

	m_Guide=pGuide;
	pGuide->Attatch( this );
}

void CEdge::Attatch(CResizeControl * pC)
{
	if(m_Attatched==NULL)
		m_Attatched=new CControlList();
	m_Attatched->Append(pC);
}

int CEdge::Attatchment()
{
	CResizeControl * pControl;
	int iRet=0;
	int iNumControls=GetControlCount();
	while( --iNumControls >= 0)
	{
		pControl = GetControl(iNumControls);
		iRet |= pControl->Attatchment( GetGuide() );
	}
	return iRet;
}

///////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////
CEdgeCache::CEdgeCache()
{
	m_NumVert=0;
	m_NumHoriz=0;
}

CEdgeCache::~CEdgeCache()
{

}

CEdge *	CEdgeCache::Create(int Position, int Axis, int Flexible, int Offset)
{
	CEdge * pEdge=new CEdge(Position, Axis, Flexible, Offset);
	AddEdge(pEdge);
	return pEdge;
}

CEdge * CEdgeCache::Create( CGuide * pGuide, int Offset )
{
	CEdge * pEdge=new CEdge(pGuide, Offset);
	AddEdge(pEdge);
	return pEdge;
}

BOOL CEdgeCache::AddEdge(CEdge *pEdge)
{
	if( pEdge->IsHorizontal() )
		SetNumHoriz( GetNumHoriz() + 1 );
	else
		SetNumVert( GetNumVert() + 1 );
	return Append( pEdge );
}

/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGuide::CGuide()
 : m_Axis(0), m_Position(0), m_Flexible(FALSE)
{
	m_Attatched=NULL;
}

CGuide::~CGuide()
{
	if(m_Attatched)
		delete m_Attatched;
}

CGuide::CGuide(int Position, int Axis, BOOL Flexible)
{
	m_Attatched=NULL;
	SetPosition(Position);
	SetAxis(Axis);
	SetFlexible(Flexible);
}

//
// Guides remember what edges are attatched to them.
//
void CGuide::Attatch(CEdge * pEdge)
{
	if(m_Attatched==NULL)
		m_Attatched=new CEdgeList();
	m_Attatched->Append(pEdge);
}

BOOL CGuide::IsEqual(CGuide *pGuide)
{
	return pGuide == this;
}

void CGuide::Adjust(int adjust)
{
	SetPosition( GetPosition() + adjust );
}

int CGuide::NumAttatchments()
{
	if(m_Attatched==NULL)
		return 0;

	int iRet=0;
	int iCount=0;
	int iNumEdges= m_Attatched->GetEdgeCount();
	while( --iNumEdges>=0 )
	{
		CEdge * pEdge= GetEdge(iNumEdges);
		iRet += pEdge->GetControlCount();
	}
	return iRet;
}

//
// Attempts to work out if this is a top/bottom/left/right guide.
//
int CGuide::Attatchment()
{
	if(m_Attatched==NULL)
		return 0;

	int iRet=0;
	int iCount=0;
	int iNumEdges= m_Attatched->GetEdgeCount();
	while( --iNumEdges>=0 )
	{
		CEdge * pEdge= GetEdge(iNumEdges);
		iRet |= pEdge->Attatchment();
	}
	return iRet;
}

CEdge * CGuide::GetEdge(int i)
{
	if(m_Attatched==NULL)
		return NULL;
	return m_Attatched->GetEdge(i);
}

void CGuide::DeAttatch( CEdge * pEdge )
{
	//
	// Go throught the list - null our this edge - it's no longer ours.
	//
	int iCount=m_Attatched->GetEdgeCount();
	while (--iCount>=0)
	{
		CEdge * p=GetEdge(iCount);
		if(p==pEdge)
			m_Attatched->Remove(iCount);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////
// Constraint.cpp: implementation of the CConstraint class.
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConstraint::CConstraint()
{

}

CConstraint::~CConstraint()
{

}

////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////
// ControlList.cpp: implementation of the CControlList class.
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CControlList::CControlList()
{

}

CControlList::~CControlList()
{

}
