#if !defined(AFX_EXTENSIONSAW_H__8DEDB8FA_5C6D_11D1_8CCD_00C04FD918D0__INCLUDED_)
#define AFX_EXTENSIONSAW_H__8DEDB8FA_5C6D_11D1_8CCD_00C04FD918D0__INCLUDED_

// Extensionsaw.h : header file
//

class CDialogChooser;

// All function calls made by mfcapwz.dll to this custom AppWizard (except for
//  GetCustomAppWizClass-- see Extensions.cpp) are through this class.  You may
//  choose to override more of the CCustomAppWiz virtual functions here to
//  further specialize the behavior of this custom AppWizard.
class CExtensionsAppWiz : public CCustomAppWiz
{
    CDialogChooser* _pChooser;
public:
	virtual CAppWizStepDlg* Next(CAppWizStepDlg* pDlg);
	virtual CAppWizStepDlg* Back(CAppWizStepDlg* pDlg);
		
	virtual void InitCustomAppWiz();
	virtual void ExitCustomAppWiz();
	virtual void CustomizeProject(IBuildProject* pProject);
};

// This declares the one instance of the CExtensionsAppWiz class.  You can access
//  m_Dictionary and any other public members of this class through the
//  global Extensionsaw.  (Its definition is in Extensionsaw.cpp.)
extern CExtensionsAppWiz Extensionsaw;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXTENSIONSAW_H__8DEDB8FA_5C6D_11D1_8CCD_00C04FD918D0__INCLUDED_)
