//
// CResizeControl
//
// FelixA
//
// Resize controls

#ifndef __PERSISTCONTROL
#define __PERSISTCONTROL

class CRegUiData;

#include "parentinfo.h"
#include "edge.h"


//
// Generic version of the PersistenControls
//
#define PROPERTY(type, Name) void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; }

class CResizeControlGeneric
{
public:
	int Attatchment( CGuide * pGuide);
	virtual ~CResizeControlGeneric();
	CResizeControlGeneric(HWND hC, CParentInfo & pi, CResizeControl * pUs);
	void SetControl(HWND h) { m_hC=h; }
	HWND GetControl() const { return m_hC; }

	//
	// Attatches itself to each guide.
	//
	BOOL SetControlAssociations();

	//
	// Everything is in dialog units, as that's how we get laid out.
	//
	RECT GetLocation() { return m_Location; }

	// these are obvious ones, like I'm on the edge - 
	virtual BOOL DeterminLeftAttatchments();		
	virtual BOOL DeterminRightAttatchments();		
	virtual BOOL DeterminTopAttatchments();		
	virtual BOOL DeterminBottomAttatchments();		

	virtual BOOL Resize(WORD width, WORD height);

	LONG	GetRightGap() const { return m_Parent.GetRight() - m_Location.right; }
	LONG	GetTopGap() const { return m_Location.top; }
	LONG	GetLeftGap() const { return m_Location.left; }
	LONG	GetBottomGap() const { // return m_Parent.GetBottom() - m_Location.bottom;// 
	return m_Parent.GetHeight() - m_Location.bottom; }

	LONG	GetWidth() const { return m_PreferredSize.Width; } // { return m_Location.right - m_Location.left; }
	LONG	GetHeight() const { return m_PreferredSize.Height; } // m_Location.bottom - m_Location.top; }

	BOOL	IsGrowsHigh() const { return m_GrowsHigh; }
	BOOL	IsGrowsWide() const { return m_GrowsWide; }

	CEdge	* GetRightEdge() { return m_pRightEdge; }

	PROPERTY( int, X );
	PROPERTY( int, Y );
	PROPERTY( int, TempWidth );
	PROPERTY( int, TempHeight );
	DIMENSION GetPreferredSize();
	PROPERTY( LPTSTR, ClassName);

private:
	int m_X;
	int m_Y;
	int m_TempWidth;
	int m_TempHeight;
	DIMENSION m_PreferredSize;

	HWND	m_hC;
	RECT	m_Location;
	CEdge	*	m_pLeftEdge;
	CEdge	*	m_pRightEdge;
	CEdge	*	m_pTopEdge;
	CEdge	*	m_pBottomEdge;

protected:
	int		m_RightSlop;		// ammount we can be off finding the right edge.
	void	DeterminLocation();	// works out where we are on the dialog, and its width / height
	BOOL	m_GrowsWide;
	BOOL	m_GrowsHigh;
	CParentInfo	& m_Parent;
	CResizeControl	*	m_pUs; // what we should hand out.
	LPTSTR	m_ClassName;
};


//
// How the user see's it.
//
#define WRAP( returnType, method ) returnType method() { if(m_pC) return m_pC->##method(); return (returnType)0; }

class CResizeControl
{
public:
	~CResizeControl(){};
	CResizeControl(HWND hC,CParentInfo & pi);
	BOOL Valid() const { if(m_pC) return true; return false; }
	CResizeControlGeneric * m_pC;

	// these are obvious ones, like I'm on the edge - 
	WRAP (BOOL, DeterminLeftAttatchments )
	WRAP (BOOL, DeterminRightAttatchments )
	WRAP (BOOL, DeterminTopAttatchments )
	WRAP (BOOL, DeterminBottomAttatchments )

	CEdge * GetRightEdge() { if(m_pC) return m_pC->GetRightEdge(); return NULL; }

	BOOL Resize(WORD width, WORD height) { if(m_pC) return m_pC->Resize(width, height); return FALSE; }
	WRAP(LONG, GetRightGap );
	WRAP(LONG, GetLeftGap );
	WRAP(LONG, GetTopGap );
	WRAP(LONG, GetBottomGap );

	WRAP(LONG, GetWidth );
	WRAP(LONG, GetHeight );

	WRAP(BOOL, IsGrowsWide );
	WRAP(BOOL, IsGrowsHigh );

	WRAP(HWND, GetControl );
	WRAP(BOOL, SetControlAssociations );

	WRAP( int, GetX );
	WRAP( int, GetY );
	WRAP( int, GetTempWidth );
	WRAP( int, GetTempHeight );
	WRAP( LPTSTR, GetClassName );

	void SetX(int i ) { m_pC->SetX(i); }
	void SetY(int i ) { m_pC->SetY(i); }
	void SetWidth(int i ) { m_pC->SetTempWidth(i); }
	void SetHeight(int i ) { m_pC->SetTempHeight(i); }

	DIMENSION GetPreferredSize() { return m_pC->GetPreferredSize(); }
	int	Attatchment(CGuide *pGuide) { return m_pC->Attatchment(pGuide); }
	RECT GetLocation() { return m_pC->GetLocation(); }

	CConstraint	m_Cons;
};


#endif
