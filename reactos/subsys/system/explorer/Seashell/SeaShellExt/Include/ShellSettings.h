//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#ifndef __SHELLSETTINGS_H__
#define __SHELLSETTINGS_H__

////////////////////////////////////////////////
// CShellSettings
////////////////////////////////////////////////
class CShellSettings
{
public:
	CShellSettings();
	virtual ~CShellSettings();

public:
// attributes
	bool DesktopHTML() const { return m_sfs.fDesktopHTML != 0; }
	bool DontPrettyPath() const { return m_sfs.fDontPrettyPath != 0; }
	bool DoubleClickInWebView() const { return m_sfs.fDoubleClickInWebView != 0; }
	bool HideIcons() const { return m_sfs.fHideIcons != 0; }
	bool MapNetDrvBtn() const { return m_sfs.fMapNetDrvBtn != 0; }
	bool NoConfirmRecycle() const { return m_sfs.fNoConfirmRecycle != 0; }
	bool ShowAllObjects() const { return m_sfs.fShowAllObjects != 0; }
	bool ShowAttribCol() const { return m_sfs.fShowAttribCol != 0; }
	bool ShowCompColor() const { return m_sfs.fShowCompColor != 0; }
	bool ShowExtensions() const { return m_sfs.fShowExtensions != 0; }
	bool ShowInfoTip() const { return m_sfs.fShowInfoTip != 0; }
	bool ShowSysFiles() const { return m_sfs.fShowSysFiles != 0; }
	bool Win95Classic() const { return m_sfs.fWin95Classic != 0; }
// operations
	bool GetSettings();
	const SHELLFLAGSTATE &GetSFS() const { return m_sfs; } 
// conversions
    operator const SHELLFLAGSTATE& () { return m_sfs; }
    operator const SHELLFLAGSTATE* () { return &m_sfs; }
protected:
private:
	 SHELLFLAGSTATE  m_sfs; 
	 HINSTANCE       m_hinstShell32;
};

#endif //__SHELLSETTINGS_H__