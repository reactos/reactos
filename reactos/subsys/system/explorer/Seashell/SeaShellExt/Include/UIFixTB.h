////////////////////////////////////////////////////////////////
// Copyright 1998 Paul DiLascia
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef __FIXTB_H
#define __FIXTB_H

//////////////////
// This class fixes the sizing bugs in MFC that calculates the
// size of toolbars incorrectly for modern toobars (comctl32 version >= 4.71)
// It also contains a number of wrappers for function that CToolBar doesn't
// have (so you don't have to use GetToolBarCtrl).
//
// Generally, you should use CFlatToolBar instead of CFixMFCToolBar (even
// if you're not using the flat style), but you can use CFixMFCToolBar if
// all you want is the sizing fix for MFC tooblars.
//
class CTRL_EXT_CLASS CFixMFCToolBar : public CToolBar {
public:
	CFixMFCToolBar();
	virtual ~CFixMFCToolBar();

	static int iVerComCtl32; // version of commctl32.dll (eg 471)

	// There is a bug in comctl32.dll, version 4.71+ that causes it to
	// draw vertical separators in addition to horizontal ones, when the
	// toolbar is vertically docked. If the toolbar has no dropdown buttons,
	// this is not a problem because the separators are ignored when calculating
	// the width of the toolbar. If, however, you have dropdown buttons, then the
	// width of a vertically docked toolbar will be too narrow to show the
	// dropdown arrow. This is in fact what happens in Visual Studio. If you
	// want to show the dropdown arrow when vertical, set this to TRUE
	// (default = FALSE)
	//
	BOOL m_bShowDropdownArrowWhenVertical;

	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);

	CSize CalcLayout(DWORD nMode, int nLength = -1);
	CSize CalcSize(TBBUTTON* pData, int nCount);
	int WrapToolBar(TBBUTTON* pData, int nCount, int nWidth);
	void SizeToolBar(TBBUTTON* pData, int nCount, int nLength,
		BOOL bVert = FALSE);
	virtual void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);

	virtual CSize GetButtonSize(TBBUTTON* pData, int iButton);

	// MFC has versions of these that are protected--but why?
	//
	void GetButton(int nIndex, TBBUTTON* pButton) const;
	void SetButton(int nIndex, TBBUTTON* pButton);

protected:
	LRESULT OnSizeHelper(CSize& sz, LPARAM lp);
	afx_msg LRESULT OnSetBitmapSize(WPARAM, LPARAM);
	afx_msg LRESULT OnSetButtonSize(WPARAM, LPARAM);
	afx_msg LRESULT OnSettingChange(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CFixMFCToolBar)
};

#endif
