// options.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

class CUnit
{
public:
	int m_nTPU;
	int m_nSmallDiv;	// small divisions - small line displayed
	int m_nMediumDiv;	// medium divisions - large line displayed
	int m_nLargeDiv;	// large divisions - numbers displayed
	int m_nMinMove;		// minimum tracking movements
	UINT m_nAbbrevID;
	BOOL m_bSpaceAbbrev; // put space before abbreviation
	CString m_strAbbrev;// cm, pt, pi, ", in, inch, inches

	CUnit() {}
	CUnit(int nTPU, int nSmallDiv, int nMediumDiv, int nLargeDiv, 
		int nMinMove, UINT nAbbrevID, BOOL bSpaceAbbrev);
	const CUnit& operator=(const CUnit& unit);
};

class CDocOptions
{
public:
	CDocOptions(int nDefWrap) {m_nDefWrap = nDefWrap;}
	CDockState m_ds1;
	CDockState m_ds2;

	int m_nWordWrap;
	int m_nDefWrap;

    struct CBarState
    {
        BOOL m_bRulerBar;
        BOOL m_bStatusBar;
        BOOL m_bToolBar;
        BOOL m_bFormatBar;
    }
    m_barstate[2];

	void SaveOptions(LPCTSTR lpsz);
	void LoadOptions(LPCTSTR lpsz);
	void SaveDockState(CDockState& ds, LPCTSTR lpszProfileName, 
		LPCTSTR lpszLayout);
	void LoadDockState(CDockState& ds, LPCTSTR lpszProfileName, 
		LPCTSTR lpszLayout);
	CDockState& GetDockState(BOOL bPrimary) {return (bPrimary) ? m_ds1 : m_ds2;}
    CBarState & GetBarState(BOOL bPrimary) {return bPrimary ? m_barstate[0] : m_barstate[1];}
};

/////////////////////////////////////////////////////////////////////////////
