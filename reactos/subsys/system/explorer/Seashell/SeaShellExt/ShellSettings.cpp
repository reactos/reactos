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

#include "stdafx.h"
#include "ShellSettings.h"

typedef void (WINAPI *PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE lpsfs, DWORD dwMask);  

CShellSettings::CShellSettings()
{
	// Since SHGetSettings is not implemented in all versions of the shell, 
	// get the function address manually at run time. 
	// This allows the extension to run on all platforms. 
	m_hinstShell32 = LoadLibrary(TEXT("shell32.dll")); 
	ZeroMemory(&m_sfs, sizeof(m_sfs));  
	// The default is classic Windows 95 style. 
	m_sfs.fWin95Classic = TRUE; 
}

CShellSettings::~CShellSettings()
{
	if (m_hinstShell32) 
		FreeLibrary(m_hinstShell32);  
}

bool CShellSettings::GetSettings() 
{
	if (m_hinstShell32 == NULL) 
		 return false;
	PFNSHGETSETTINGSPROC pfnSHGetSettings; 
	pfnSHGetSettings = (PFNSHGETSETTINGSPROC)GetProcAddress(m_hinstShell32, "SHGetSettings"); 
	if(pfnSHGetSettings) 
	{   
		ZeroMemory(&m_sfs, sizeof(m_sfs));  
		 (*pfnSHGetSettings)(&m_sfs, 
			SSF_DESKTOPHTML |			// The fDesktopHTML member is being requested.  
			SSF_DONTPRETTYPATH |		// The fDontPrettyPath member is being requested.  
			SSF_DOUBLECLICKINWEBVIEW |  // The fDoubleClickInWebView member is being requested.  
			SSF_HIDEICONS |				// The fHideIcons member is being requested.  
			SSF_MAPNETDRVBUTTON |		// The fMapNetDrvBtn member is being requested.  
			SSF_NOCONFIRMRECYCLE |		// The fNoConfirmRecycle member is being requested.  
			SSF_SHOWALLOBJECTS |		// The fShowAllObjects member is being requested.  
			SSF_SHOWATTRIBCOL |			// The fShowAttribCol member is being requested.  
			SSF_SHOWCOMPCOLOR |			// The fShowCompColor member is being requested.  
			SSF_SHOWEXTENSIONS |		// The fShowExtensions member is being requested.  
			SSF_SHOWINFOTIP |			// The fShowInfoTip member is being requested.  
			SSF_SHOWSYSFILES |			// The fShowSysFiles member is being requested.  
			SSF_WIN95CLASSIC);			// The fWin95Classic member is being requested.  
	}  
	return true;
}
