// ResizeDlg.h: interface for the CResizeDlg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZEDLG_H__CBCB8815_7899_11D1_96A4_00C04FB177B1__INCLUDED_)
#define AFX_RESIZEDLG_H__CBCB8815_7899_11D1_96A4_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "dlg.h"
#include "list.h"
#include "parentinfo.h"

typedef struct tagCHANNEL
{
	int Pos;
	int Size;
	int iFixed;		// if we can't resize this column.
	int iWeight;	// if we can, this is the weight.
} CHANNEL, * PCHANNEL;

#undef PROPERTY
#define PROPERTY(type, Name) public: void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; } private: type m_##Name; public:
typedef struct _tagSPECIAL
{
	BOOL	bSpecial;	// is this infact a special row / col
	int		iMin;		// top or left of the data
	int		iMax;		// right or bottom of the data
	int		iAlignment;	// -1 left/top, 0 center, 1 right/bottom
	int		iDiff;		// difference between iMin and iMax
} SPECIAL, * PSPECIAL;


class CResizeDlg : public CDlg  
{
	typedef CDlg BASECLASS;
public:
	CResizeDlg(int DlgID, HWND hWndParent, HINSTANCE hInst);
	virtual ~CResizeDlg();

	virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
	PROPERTY( int, NumCols );
	PROPERTY( int, NumRows );
	PROPERTY( BOOL, Annotate);
	PROPERTY( int, RowWeight);
	PROPERTY( int, ColWeight);
	PROPERTY( int, ControlCount);
	// PROPERTY( BOOL, SpecialRow );
	// PROPERTY( BOOL, SpecialCol );
protected:
	void SpecialRowCol();
	void DeterminNumberOfControls();
	virtual void	DoChangePos( WINDOWPOS * lpwp);
	int		FindCol(int pos);
	int		FindRow(int pos);
	void	AddToSystemMenu();
	void	DoInitDialog();
	void	PlaceControls();
	LONG	HitTest(LONG lCurrent);
	virtual void	DeterminWeights();
	void	Annotate();
	void	Sort( CEdge **, int iCount);
	void	WalkControls();
	virtual void	ResizeControls( WORD width, WORD height );
	CControlList	m_ControlList;
	CParentInfo	m_ParentInfo;

	void	FindControls();
	void	MakeAttatchments();
	void	FindBorders();
	void	FindCommonGuides();
	void	DeterminCols( CEdge ** ppEdges, int iCount);
	void	DeterminRows( CEdge ** ppEdges, int iCount);

	CHANNEL	*	m_Rows;
	CHANNEL *	m_Cols;

	virtual void	AddGripper();
	virtual HDWP	SetGripperPos(HDWP hdwp);
	HWND	m_hwndGripper;

	SPECIAL	m_SpecialRow;
	SPECIAL	m_SpecialCol;
};

#undef PROPERTY
#endif // !defined(AFX_RESIZEDLG_H__CBCB8815_7899_11D1_96A4_00C04FB177B1__INCLUDED_)



#if !defined(AFX_SCALEDLG_H__C4C6558B_9289_11D1_A5CC_00C04FB177B1__INCLUDED_)
#define AFX_SCALEDLG_H__C4C6558B_9289_11D1_A5CC_00C04FB177B1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ResizeDlg.h"

class CScaleDlg : public CResizeDlg  
{
	typedef CResizeDlg BASECLASS;
public:
	CScaleDlg(int DlgID, HWND hWndParent, HINSTANCE hInst);
	virtual ~CScaleDlg();

protected:
	void DoChangePos( WINDOWPOS * lpwp);
	LONG m_dwInitFontSize;
	void DeterminWeights();
	void ResizeControls(WORD width, WORD height);
private:
	int dwLastCX;
	int dwLastCY;
};

#endif // !defined(AFX_SCALEDLG_H__C4C6558B_9289_11D1_A5CC_00C04FB177B1__INCLUDED_)
