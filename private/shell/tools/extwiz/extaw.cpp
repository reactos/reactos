// Extensionsaw.cpp : implementation file
//

#include "stdafx.h"
#include "Ext.h"
#include "Extaw.h"
#include "chooser.h"

#ifdef _PSEUDO_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This is called immediately after the custom AppWizard is loaded.  Initialize
//  the state of the custom AppWizard here.
void CExtensionsAppWiz::InitCustomAppWiz()
{
    GUID guidTemp;
    WCHAR wszGUID[50];

    _pChooser = new CDialogChooser;

	SetNumberOfSteps(2);

	// Inform AppWizard that we're making a DLL.
	m_Dictionary[_T("PROJTYPE_DLL")] = _T("1");

    	// Set template macros based on the project name entered by the user.

	// Get value of $$root$$ (already set by AppWizard)
	CString strRoot;
	m_Dictionary.Lookup(_T("root"), strRoot);
	
	// Set value of $$Doc$$, $$DOC$$
	CString strDoc = strRoot.Left(6);
	m_Dictionary[_T("Doc")] = strDoc;
	strDoc.MakeUpper();
	m_Dictionary[_T("DOC")] = strDoc;

	// Set value of $$MAC_TYPE$$
	strRoot = strRoot.Left(4);
	int nLen = strRoot.GetLength();
	if (strRoot.GetLength() < 4)
	{
		CString strPad(_T(' '), 4 - nLen);
		strRoot += strPad;
	}
	strRoot.MakeUpper();
	m_Dictionary[_T("MAC_TYPE")] = strRoot;

    if (SUCCEEDED(CoCreateGuid(&guidTemp)))
    {
        StringFromGUID2(guidTemp, wszGUID, ARRAYSIZE(wszGUID));
        Extensionsaw.m_Dictionary[TEXT("LibGUID")] = StripCurly(wszGUID);
    }
}

// This is called just before the custom AppWizard is unloaded.
void CExtensionsAppWiz::ExitCustomAppWiz()
{
    if (_pChooser)
    {
        delete _pChooser;
        _pChooser = NULL;
    }
}

// This is called when the user clicks "Create..." on the New Project dialog
CAppWizStepDlg* CExtensionsAppWiz::Next(CAppWizStepDlg* pDlg)
{
    return _pChooser->Next(pDlg);
}

// This is called when the user clicks "Back" on one of the custom
//  AppWizard's steps.
CAppWizStepDlg* CExtensionsAppWiz::Back(CAppWizStepDlg* pDlg)
{
	// Delegate to the dialog chooser
	return _pChooser->Back(pDlg);
}


void CExtensionsAppWiz::CustomizeProject(IBuildProject* pProject)
{
	// TODO: Add code here to customize the project.  If you don't wish
	//  to customize project, you may remove this virtual override.
	
	// This is called immediately after the default Debug and Release
	//  configurations have been created for each platform.  You may customize
	//  existing configurations on this project by using the methods
	//  of IBuildProject and IConfiguration such as AddToolSettings,
	//  RemoveToolSettings, and AddCustomBuildStep. These are documented in
	//  the Developer Studio object model documentation.

	// WARNING!!  IBuildProject and all interfaces you can get from it are OLE
	//  COM interfaces.  You must be careful to release all new interfaces
	//  you acquire.  In accordance with the standard rules of COM, you must
	//  NOT release pProject, unless you explicitly AddRef it, since pProject
	//  is passed as an "in" parameter to this function.  See the documentation
	//  on CCustomAppWiz::CustomizeProject for more information.



}


// Here we define one instance of the CExtensionsAppWiz class.  You can access
//  m_Dictionary and any other public members of this class through the
//  global Extensionsaw.
CExtensionsAppWiz Extensionsaw;

CString StripCurly(CString str)
{
    return str.Mid(1, str.GetLength() - 2);
}
