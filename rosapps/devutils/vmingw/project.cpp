/********************************************************************
*	Module:	project.cpp. This is part of Visual-MinGW.
*
*	Purpose:	Procedures to manage a loaded project.
*
*	Authors:	Manu B.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
*	Revisions:	
*
********************************************************************/
#include <windows.h>
#include <stdio.h>

#include "project.h"
#include "rsrc.h"

extern CMessageBox MsgBox;

// For dialogs.
extern CWinApp 	winApp;

WORD AppendPath(char * dirBuffer, WORD offset, char * absolutePath);

bool CheckFile(CFileItem * file){
	if (!file)
		return false;

	char * fileName = file->szFileName;
	char * s = fileName;
	char * dot = fileName;
	char * bkslash = fileName+1;
	// Valid relative path ?
	if (*dot == '.' && *bkslash == '\\'){
		s+=2;
		// Find backslashes & dots.
		while (*s != '\0'){
			if (*s == '\\'){
				bkslash = s;
			}else if (*s == '.'){
				dot = s;
			}
			s++;
		}
		if (*bkslash != '\0'){
			file->nFileOffset = (bkslash - fileName) +1;
		}else{
			file->nFileOffset = 0;
			return false;
		}
			
		if (dot != fileName)
			file->nFileExtension = (dot - fileName) +1;
		else
			file->nFileExtension = 0;
		return true;
	}
return false;
}

WORD AppendPath(char * dirBuffer, WORD offset, char * absolutePath){
	WORD len = 0;
	if (absolutePath[0] == '.'){
		if (absolutePath[1] == '\\' && absolutePath[2] != '\0'){
			dirBuffer[offset-1] = '\\';
			strcpy(&dirBuffer[offset], &absolutePath[2]);
			len = strlen(&dirBuffer[offset]);
			len++;
		}
	}
return len;
}


/********************************************************************
*	Class:	COptionsDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
COptionsDlg::COptionsDlg(){
	pProject = NULL;
	pMakefile = NULL;
}

COptionsDlg::~COptionsDlg(){
}

LRESULT CALLBACK COptionsDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_NOTIFY:
			OnNotify((int) wParam, (LPNMHDR) lParam);
			break;
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}

BOOL COptionsDlg::OnInitDialog(HWND, LPARAM lInitParam){
	// Set pointers.
	pProject = (CProject *) lInitParam;
	if (pProject == NULL)
		return TRUE;

	// Tab control handle and TCITEM.
	_hWndTab = ::GetDlgItem(_hWnd, IDC_OPTION_TABS);
	tcitem.mask = TCIF_TEXT | TCIF_PARAM;

	// Insert General tab.
	HWND hwndChild = GeneralDlg.Create(this, IDD_GENERAL_PANE, NULL, (long) pProject);
	tcitem.pszText = "General";
	tcitem.lParam = (long) &GeneralDlg;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 0, 
		(LPARAM)&tcitem);
	SetChildPosition(hwndChild);
	
	// Insert Compiler tab item.
	CompilerDlg.Create(this, IDD_COMPILER, &Pos, (long) pProject);
	tcitem.pszText = "Compiler";
	tcitem.lParam = (long) &CompilerDlg;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 1, 
		(LPARAM)&tcitem);

	// Insert Linker tab.
	LinkerDlg.Create(this, IDD_LINKER, &Pos, (long) pProject);
	tcitem.pszText = "Linker";
	tcitem.lParam = (long) &LinkerDlg;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 2, 
		(LPARAM)&tcitem);

	// Insert Archives tab.
	ZipDlg.Create(this, IDD_ZIP, &Pos, (long) pProject);
	tcitem.pszText = "Archives";
	tcitem.lParam = (long) &ZipDlg;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 3, 
		(LPARAM)&tcitem);

	// Show the dialog and default pane.
	Show();
	GeneralDlg.Show();
	GeneralDlg.SetFocus();
return TRUE;
}

BOOL COptionsDlg::OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl){
	switch (wID){
		case IDOK:
			GeneralDlg.OnCommand(wNotifyCode, wID, hwndCtl);
			CompilerDlg.OnCommand(wNotifyCode, wID, hwndCtl);
			LinkerDlg.OnCommand(wNotifyCode, wID, hwndCtl);
			ZipDlg.OnCommand(wNotifyCode, wID, hwndCtl);
			pProject->buildMakefile = true;
			pProject->modified = true;
			EndDlg(IDOK);
			return TRUE;

		case IDCANCEL:
			EndDlg(IDCANCEL);
			break;
	}
return 0;
}

BOOL COptionsDlg::EndDlg(int nResult){
	GeneralDlg.EndDlg(0);
	CompilerDlg.EndDlg(0);
	LinkerDlg.EndDlg(0);
	ZipDlg.EndDlg(0);
	if (_hWnd){
		BOOL result = ::EndDialog(_hWnd, nResult);
		_hWnd = 0;
		return result;
	}
return false;
}


/********************************************************************
*	Class:	CGeneralDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CGeneralDlg::CGeneralDlg(){
	pProject = NULL;
	pMakefile = NULL;
}

CGeneralDlg::~CGeneralDlg(){
}

LRESULT CALLBACK CGeneralDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}

BOOL CGeneralDlg::OnInitDialog(HWND, LPARAM lInitParam){
	/* Set pointers */
	pProject = (CProject *) lInitParam;
	if (pProject == NULL || &pProject->Makefile == NULL)
		return TRUE;
	pMakefile = &pProject->Makefile;

	/* Get control handles */
	hStatLib 	= GetItem(IDC_STATLIB);
	hDll 		= GetItem(IDC_DLL);
	hConsole 	= GetItem(IDC_CONSOLE);
	hGuiExe 	= GetItem(IDC_GUIEXE);
	hDbgSym 	= GetItem(IDC_DBGSYM);
	hLangC 	= GetItem(IDC_LANGC);
	hLangCpp 	= GetItem(IDC_LANGCPP);
	hMkfName 	= GetItem(IDC_MKF_NAME);
	hMkfDir 	= GetItem(IDC_MKF_DIR);
	hUserMkf 	= GetItem(IDC_USER_MKF);
	hTgtName 	= GetItem(IDC_TGT_NAME);
	hTgtDir 	= GetItem(IDC_TGT_DIR);

	/* Set buttons state */
	switch(pMakefile->buildWhat){
		case BUILD_STATLIB:
			::SendMessage(hStatLib, BM_SETCHECK, BST_CHECKED, 0);
		break;
		case BUILD_DLL:
			::SendMessage(hDll, BM_SETCHECK, BST_CHECKED, 0);
		break;
		case BUILD_EXE:
			::SendMessage(hConsole, BM_SETCHECK, BST_CHECKED, 0);
		break;
		case BUILD_GUIEXE:
			::SendMessage(hGuiExe, BM_SETCHECK, BST_CHECKED, 0);
		break;
	}
	if (pMakefile->debug)
		::SendMessage(hDbgSym, BM_SETCHECK, BST_CHECKED, 0);

	if (pMakefile->lang == LANGCPP)
		::SendMessage(hLangCpp, BM_SETCHECK, BST_CHECKED, 0);
	else
		::SendMessage(hLangC, BM_SETCHECK, BST_CHECKED, 0);

	/* Set text */
	char name[64];
	if (pMakefile->nFileOffset){
		strcpy(name, &pMakefile->szFileName[pMakefile->nFileOffset]);
	}else{
		strcpy(name, "noname");
	}

	SetItemText(hMkfName,	name);
	SetItemText(hMkfDir,		pMakefile->mkfDir);
	SetItemText(hTgtName,	pMakefile->target);
	SetItemText(hTgtDir, 		pMakefile->tgtDir);
return TRUE;
}

BOOL CGeneralDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:{
			/* Get text */
			char name[64];
			GetItemText(hMkfName,	name,	64);
			GetItemText(hMkfDir,		pMakefile->mkfDir,	MAX_PATH);
			GetItemText(hTgtName, 	pMakefile->target,	64);
			GetItemText(hTgtDir,		pMakefile->tgtDir,	MAX_PATH);

			pMakefile->GetFullPath(pMakefile->szFileName, pProject->nFileOffset, name);
			//@@TODO check if directories exist.

			/* Get buttons state */
			char * pExt = strrchr(pMakefile->target, '.');
			if (!pExt){
				int len = strlen(pMakefile->target);
				if (!len){
					strcpy(pMakefile->target, "noname");
					len = strlen(pMakefile->target);
				}
				pExt = pMakefile->target + len;
			}
			if (BST_CHECKED == ::SendMessage(hStatLib, BM_GETCHECK, 0, 0)){
				pMakefile->buildWhat = BUILD_STATLIB;
				strcpy(pExt, ".a");
			}else if (BST_CHECKED == ::SendMessage(hDll, BM_GETCHECK, 0, 0)){
				pMakefile->buildWhat = BUILD_DLL;
				strcpy(pExt, ".dll");
			}else if (BST_CHECKED == ::SendMessage(hConsole, BM_GETCHECK, 0, 0)){
				pMakefile->buildWhat = BUILD_EXE;
				strcpy(pExt, ".exe");
			}else if (BST_CHECKED == ::SendMessage(hGuiExe, BM_GETCHECK, 0, 0)){
				pMakefile->buildWhat = BUILD_GUIEXE;
				strcpy(pExt, ".exe");
			}
			pMakefile->debug = 
				(BST_CHECKED==::SendMessage(hDbgSym, BM_GETCHECK, 0, 0));

			if (BST_CHECKED == ::SendMessage(hLangCpp, BM_GETCHECK, 0, 0)){
				pMakefile->lang = LANGCPP;
				strcpy(pMakefile->cc, "g++");
			}else{
				pMakefile->lang = LANGC;
				strcpy(pMakefile->cc, "gcc");
			}
		}
		return TRUE;

		case IDCANCEL:
		return FALSE;
	}
return FALSE;
}


/********************************************************************
*	Class:	CCompilerDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CCompilerDlg::CCompilerDlg(){
	pProject = NULL;
	pMakefile = NULL;
}

CCompilerDlg::~CCompilerDlg(){
}

LRESULT CALLBACK CCompilerDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}

BOOL CCompilerDlg::OnInitDialog(HWND, LPARAM lInitParam){
	// Set pointers.
	pProject = (CProject *) lInitParam;
	if (pProject == NULL || &pProject->Makefile == NULL)
		return TRUE;
	pMakefile = &pProject->Makefile;

	hCppFlags	= GetItem(IDC_CPPFLAGS);
	hWarning	= GetItem(IDC_WARNING);
	hOptimiz	= GetItem(IDC_OPTIMIZ);
	hCFlags	= GetItem(IDC_CFLAGS);
	hIncDirs	= GetItem(IDC_INCDIRS);

	SetItemText(hCppFlags, pMakefile->cppFlags);
	SetItemText(hWarning, pMakefile->warning);
	SetItemText(hOptimiz, 	pMakefile->optimize);
	SetItemText(hCFlags, 	pMakefile->cFlags);
	SetItemText(hIncDirs, 	pMakefile->incDirs);
return TRUE;
}

BOOL CCompilerDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:
			GetItemText(hCppFlags,	pMakefile->cppFlags,	256);
			GetItemText(hWarning,	pMakefile->warning,	64);
			GetItemText(hOptimiz, 	pMakefile->optimize,	64);
			GetItemText(hCFlags, 	pMakefile->cFlags,	64);
			GetItemText(hIncDirs, 	pMakefile->incDirs, 	256);
		return TRUE;

		case IDCANCEL:
		return FALSE;
	}
return FALSE;
}


/********************************************************************
*	Class:	CLinkerDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CLinkerDlg::CLinkerDlg(){
	pProject = NULL;
	pMakefile = NULL;
}

CLinkerDlg::~CLinkerDlg(){
}

LRESULT CALLBACK CLinkerDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}

BOOL CLinkerDlg::OnInitDialog(HWND, LPARAM lInitParam){
	// Set pointers.
	pProject = (CProject *) lInitParam;
	if (pProject == NULL || &pProject->Makefile == NULL)
		return TRUE;
	pMakefile = &pProject->Makefile;

	hLdStrip	= GetItem(IDC_LDSTRIP);
	hLdOpts	= GetItem(IDC_LDOPTS);
	hLdLibs	= GetItem(IDC_LDLIBS);
	hLibsDirs	= GetItem(IDC_LIBDIRS);

	SetItemText(hLdStrip, 	pMakefile->ldStrip);
	SetItemText(hLdOpts, 	pMakefile->ldOpts);
	SetItemText(hLdLibs, 	pMakefile->ldLibs);
	SetItemText(hLibsDirs, 	pMakefile->libDirs);
return TRUE;
}

BOOL CLinkerDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:
			GetItemText(hLdStrip, 	pMakefile->ldStrip,	32);
			GetItemText(hLdOpts, 	pMakefile->ldOpts,	64);
			GetItemText(hLdLibs, 	pMakefile->ldLibs,	64);
			GetItemText(hLibsDirs,	pMakefile->libDirs,	256);
		return TRUE;

		case IDCANCEL:
		return FALSE;
	}
return FALSE;
}


/********************************************************************
*	Class:	CZipDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CZipDlg::CZipDlg(){
	pProject = NULL;
	pMakefile = NULL;
}

CZipDlg::~CZipDlg(){
}

LRESULT CALLBACK CZipDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}

BOOL CZipDlg::OnInitDialog(HWND, LPARAM lInitParam){
	// Set pointers.
	pProject = (CProject *) lInitParam;
	if (pProject == NULL || &pProject->Makefile == NULL)
		return TRUE;
	pMakefile = &pProject->Makefile;

	hZipDir = GetItem(IDC_ZIP_DIR);
	hZipFlags = GetItem(IDC_ZIPFLAGS);
	SetItemText(hZipDir, pProject->zipDir);
	SetItemText(hZipFlags, pProject->zipFlags);
return TRUE;
}

BOOL CZipDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:
			GetItemText(hZipDir, pProject->zipDir, MAX_PATH);
			GetItemText(hZipFlags, pProject->zipFlags, 256);
		return TRUE;

		case IDCANCEL:
		return FALSE;
	}
return FALSE;
}


/********************************************************************
*	Class:	CNewModuleDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CNewModuleDlg::CNewModuleDlg(){
	pProject = NULL;
}

CNewModuleDlg::~CNewModuleDlg(){
}

LRESULT CALLBACK CNewModuleDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){

	char		fileName[64];
	bool		createHeader = true;
	HWND wCreateHeader = GetItem(IDC_HEADER);

	switch(Message){
		case WM_INITDIALOG:
			pProject = (CProject *) lParam;
			if (createHeader)
				::SendMessage(wCreateHeader, BM_SETCHECK, BST_CHECKED, 0);
				strcpy(fileName, "new.cpp");
				::SetDlgItemText(_hWnd, 301, fileName);
			return TRUE;
	
		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL){
				fileName[0] = 0;
				EndDlg(IDCANCEL);
				return FALSE;
			}else if(LOWORD(wParam) == IDOK){
				::GetDlgItemText(_hWnd, 301, fileName, 64);
				createHeader = BST_CHECKED ==
						 ::SendMessage(wCreateHeader, BM_GETCHECK, 0, 0);
				pProject->NewModule(fileName, createHeader);
				EndDlg(IDCANCEL);
				return TRUE;
			}
			break; 

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}


/********************************************************************
*	Class:	CProject.
*
*	Purpose:	Project management.
*
*	Revisions:	
*
********************************************************************/
//@@TODO close project Dlg's before closing Project.
CProject::CProject(){
	prjVer = 40;
}

CProject::~CProject(){
}

void CProject::Reset(){
	szFileName[MAX_PATH - 1] = '\0';	// security.
	szFileName[0] 		= '\0';
	szDirBuffer[0] 		= '\0';
	nFileOffset			= 0;
	nFileExtension		= 0;

	strcpy	(zipDir,	".");
	strcpy	(zipFlags,	"*.*");
	numFiles 			= 0;
	loaded			= false;
	modified			= false;
	buildMakefile		= true;

	compilerName[0]		= '\0';

	Makefile.Init();
}

bool CProject::NoProject(void){
	if (!loaded){
		MsgBox.DisplayWarning("No project loaded");
		// Or directly show open project dlg.
		return true;
	}
return false;
}

int CProject::CloseDecision(void){
	if (loaded){
		return MsgBox.Ask("Close current project ?", false);
	}
return IDNO;
}

bool CProject::SwitchCurrentDir(void){
	// Switch to Project Directory
	szDirBuffer[nFileOffset-1] = '\0';
return ::SetCurrentDirectory(szDirBuffer);
}

bool CProject::RelativeToAbsolute(char * relativePath){
	if (*szDirBuffer && nFileOffset){
		if (relativePath[0] == '.' && relativePath[1] == '\\' 
									&& relativePath[2] != '\0'){
			szDirBuffer[nFileOffset-1] = '\\';
			strcpy(&szDirBuffer[nFileOffset], &relativePath[2]);
			return true;
		}
	}
return false;
}


/********************************************************************
*	New Project.
********************************************************************/
bool CProject::New(char * fileName, WORD fileOffset){
	// Load default values.
	Reset();

	// Copy project file's directory.
	strcpy(szFileName, fileName);
	nFileOffset = fileOffset;

	strncpy(szDirBuffer, szFileName, (nFileOffset - 1));
	szDirBuffer[nFileOffset-1] = '\0';
	strcpy(winApp.projectDir, szDirBuffer);

	// Makefile: Get target name.
	nFileExtension = winApp.FileDlg.GetFileExtension();
	strcpy(Makefile.target, &szFileName[nFileOffset]);
	char * pExt = strrchr(Makefile.target, '.');
	if (pExt){
		*pExt='\0';
	}
	loaded = true;

	AddFiles();
	
	Makefile.GetFullPath(szFileName, nFileOffset, "makefile");

	buildMakefile = true;
	OptionsDlg();
	SavePrjFile(IDYES);
return true;
}

/********************************************************************
*	Open Project.
********************************************************************/
bool CProject::Open(char * fileName, WORD fileOffset){
	// Initialize project tree view.
	Reset();

	// Copy project file's directory.
	strcpy(szFileName, fileName);
	nFileOffset = fileOffset;

	strncpy(szDirBuffer, szFileName, (nFileOffset - 1));
	szDirBuffer[nFileOffset-1] = '\0';
	strcpy(winApp.projectDir, szDirBuffer);

	// Load project file in a buffer.
	if (!PrjFile.Load(szFileName)){
		MsgBox.DisplayFatal("Can't load project file !");
		//@@ should close inifile ?
		return false;
	}

	char name[64];
	*name = '\0';
	// [Project] section
	int signature = PrjFile.GetInt(		"Signature", 		"Project"	);
	if (signature != prjVer){
		MsgBox.DisplayFatal("Bad signature in the project file !");
		return false;
	}

	numFiles = PrjFile.GetInt(			"NumFiles"	);
	PrjFile.GetString(compilerName, 		"Compiler"	);
	buildMakefile = PrjFile.GetInt(			"BuildMakefile"	);

	// [Archives] section
	PrjFile.GetString(zipDir, 			"Directory", 	"Archives"	);
	PrjFile.GetString(zipFlags, 			"Flags");

	// [Makefile] section
	PrjFile.GetString(Makefile.make, 		"Make", 		"Makefile"	);
	PrjFile.GetString(Makefile.cc, 			"CC"		);
	PrjFile.GetString(Makefile.wres, 		"WRES"	);
	PrjFile.GetString(Makefile.test, 		"TEST"	);
	PrjFile.GetString(name, 			"Makefile"	);
	PrjFile.GetString(Makefile.mkfDir, 		"MakefileDir");
	PrjFile.GetString(Makefile.target, 		"Target"	);
	PrjFile.GetString(Makefile.tgtDir, 		"TargetDir"	);
	Makefile.buildWhat 	= PrjFile.GetInt( 	"Build"	);
	Makefile.debug 		= PrjFile.GetInt( 	"Debug"	);
	Makefile.lang 		= PrjFile.GetInt( 	"Lang"	);
	PrjFile.GetString(Makefile.cppFlags,		"CppFlags"	);
	PrjFile.GetString(Makefile.warning, 		"CcWarning");
	PrjFile.GetString(Makefile.optimize, 		"CcOptimize");
	PrjFile.GetString(Makefile.cFlags, 		"CcFlags"	);
	PrjFile.GetString(Makefile.incDirs,		"IncDirs"	);
	PrjFile.GetString(Makefile.ldStrip, 		"LdStrip"	);
	PrjFile.GetString(Makefile.ldOpts, 		"LdOptions"	);
	PrjFile.GetString(Makefile.ldLibs, 		"LdLibraries");
	PrjFile.GetString(Makefile.libDirs, 		"LdLibDirs"	);

	Makefile.GetFullPath(szFileName, nFileOffset, name);

	if (numFiles){
		CFileItem * srcFile;
		// [FileXX] section
		char fileNumber [8];
		char fileSection [16];
	
		for (int n=1; n<=numFiles; n++){
			itoa(n, fileNumber, 10);
			strcpy(fileSection, "File");
			strcat(fileSection, fileNumber);
	
			// SrcFile
			srcFile = new CFileItem;
			srcFile->isInProject = true;
			PrjFile.GetString(srcFile->szFileName,	"Name", 	fileSection);
			CheckFile(srcFile);
			::GetFileType(srcFile);
			srcFile->show 	= PrjFile.GetInt(	"Show"	);
	
			if(!winApp.Manager.OpenFile(srcFile)){
				delete srcFile;
				return false;
			}
		}
	}
	loaded = true;
return true;
}

int CProject::SavePrjFile(int decision){
	if (!loaded || !modified)
		return decision;
	if (decision == IDNO || decision == IDCANCEL)
		return decision;

	/* Ask ? */
	if (decision == IDASK){
		decision = MsgBox.AskToSave(true); // Cancel button.
		if (decision != IDYES)
			return decision; // IDNO or IDCANCEL.
	}

	CProjectView * pProjectView = &winApp.Manager.ProjectView;
 	FILE * file;
	CFileItem * srcFile;
	int count = 0;

	numFiles = pProjectView->Length();

	file = fopen(szFileName, "w");
	if (!file){
		MsgBox.DisplayFatal("Can't save project file !");
		return decision;
	}

	// [Project]
	fprintf (file, "[Project]\nSignature = %d"		, prjVer			);
	fprintf (file, "\nNumFiles = %d"			, numFiles			);
	fprintf (file, "\nCompiler = %s"			, compilerName		);
	fprintf (file, "\nBuildMakefile = %d"			, buildMakefile		);

	// [Archives]
	fprintf (file, "\n\n[Archives]\nDirectory = %s"	, zipDir			);
	fprintf (file, "\nFlags = %s"				, zipFlags			);

	// [Makefile]
	fprintf (file, "\n\n[Makefile]\nMAKE = %s"		, Makefile.make		);
	fprintf (file, "\nCC = %s"				, Makefile.cc		);
	fprintf (file, "\nWRES = %s"				, Makefile.wres		);
	fprintf (file, "\nTEST = %s"				, Makefile.test		);
	fprintf (file, "\nMakefile = %s"				, &Makefile.szFileName[Makefile.nFileOffset]);
	fprintf (file, "\nMakefileDir = %s"			, Makefile.mkfDir		);
	fprintf (file, "\nTarget = %s"				, Makefile.target		);
	fprintf (file, "\nTargetDir = %s"			, Makefile.tgtDir		);
	fprintf (file, "\nBuild = %d"				, Makefile.buildWhat	);
	fprintf (file, "\nDebug = %d"				, Makefile.debug		);
	fprintf (file, "\nLang = %d"				, Makefile.lang		);
	fprintf (file, "\nCppFlags = %s"			, Makefile.cppFlags	);
	fprintf (file, "\nCcWarning = %s"			, Makefile.warning	);
	fprintf (file, "\nCcOptimize = %s"			, Makefile.optimize	);
	fprintf (file, "\nCcFlags = %s"				, Makefile.cFlags		);
	fprintf (file, "\nIncDirs = %s"				, Makefile.incDirs		);
	fprintf (file, "\nLdStrip = %s"				, Makefile.ldStrip		);
	fprintf (file, "\nLdOptions = %s"			, Makefile.ldOpts		);
	fprintf (file, "\nLdLibraries = %s"			, Makefile.ldLibs		);
	fprintf (file, "\nLdLibDirs = %s"			, Makefile.libDirs		);

	/* [Filexx] */
	srcFile = (CFileItem *) pProjectView->First();
	while (srcFile){
		count++;
		fprintf (file, "\n\n[File%d"			, count			);
		fprintf (file, "]\nName = %s"			, srcFile->szFileName	);
		fprintf (file, "\nShow = %d" 			, 0				);
		srcFile = (CFileItem *) pProjectView->Next();
	}
	fprintf (file, "\n");
	fclose(file);
	modified = false;
return decision;
}

bool CProject::NewModule(char * name, bool createHeader){
	if (NoProject())
		return false;

	SwitchCurrentDir();

	CFileItem * srcFile = winApp.Manager.ProjectView.NewFile(name);
	if (!srcFile){
		MsgBox.DisplayWarning("Can't create file : %s", name);
		return false;
	}
	if (createHeader && srcFile->type != H_FILE){
		char header[64];
		strcpy(header, name);
		char ext[] = "h";
		ChangeFileExt(header, ext);

		if (!winApp.Manager.ProjectView.NewFile(header)){
			MsgBox.DisplayWarning("Can't create file : %s", header);
			return false;
		}
	}
return true;
}

bool CProject::AddFiles(void){
	if (NoProject())
		return false;

	CFileItem * srcFile;
	char srcFiles[2048];
	srcFiles [0] = 0;
	WORD fileOffset;

	// Show Open dialog.
	if (!winApp.FileDlg.Open(&winApp, srcFiles, 2048, ADD_SRC_FILE))
		return false;  // canceled by user

	// Check if srcFiles path includes projectDir.
	int n = 0;
	int maxlen = nFileOffset - 1;
	while (n<maxlen){
		// @@ shouldn't be case sensitive.
		if (srcFiles[n] != szFileName[n])
			break;
		n++;
	}

	if (srcFiles[n] == '\\' || srcFiles[n] == '\0'){
		// We are in the project, copy directory name.
		n++;
		char relativePath[MAX_PATH];
		relativePath[0] = '.';
		relativePath[1] = '\\';
		int nn = 2;

		fileOffset = winApp.FileDlg.GetFileOffset();
		maxlen = fileOffset - 1;
		while (n<maxlen){
			relativePath[nn] = srcFiles[n];
			n++;
			nn++;
		}
		if (nn > 2){
			relativePath[nn] = '\\';
			nn++;
		}
		relativePath[nn] = '\0';

		// Append each file name.
		fileOffset = winApp.FileDlg.GetFileOffset();

		while (fileOffset){
			// Try to add each file to the project.
			srcFile = new CFileItem;
			strcpy(srcFile->szFileName, relativePath);

			strcat(srcFile->szFileName, &srcFiles[fileOffset]);

			CheckFile(srcFile);
			::GetFileType(srcFile);
			srcFile->show 	= 0;
			srcFile->isInProject = true;
	
			if(!winApp.Manager.ProjectView.OpenFile(srcFile)){
				delete srcFile;
				return false;
			}

			fileOffset = winApp.FileDlg.GetNextFileOffset();
		}
		modified = true;
		buildMakefile = true;
		return true;
	}
	MsgBox.DisplayString("Out of the project");
return false;
}

/********************************************************************
*	Dialogs.
********************************************************************/
bool CProject::OptionsDlg(void){
/**/	if (NoProject())
		return false;

	_OptionsDlg.CreateModal(&winApp, IDD_OPTION, (LPARAM) this);
return true;
}

bool CProject::NewModuleDlg(void){
	if (NoProject())
		return false;

	_NewModuleDlg.CreateModal(&winApp, IDD_NEW_MODULE, (LPARAM) this);
return true;
}

/********************************************************************
*	Project Commands.
********************************************************************/
void CProject::ZipSrcs(void){
	if (NoProject() || winApp.Process.isRunning())
		return;

	// Switch to Project Directory
	SwitchCurrentDir();

	winApp.Report.Clear();

	char fileName[64];
	char date[16];
	date[0] = 0;

	// Archive name = zipDir\month+day+year+src.zip
	GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, "MMddyyyy", 
				date, 16);

	if (*zipDir == '\0')
		strcpy (zipDir, ".");
	strcpy (fileName, zipDir);
	strcat (fileName, "\\");
	strcat (fileName, date);
	strcat (fileName, "src");
	strcat (fileName, ".zip");

	char msgBuf[128];
	sprintf(msgBuf, "Create archive > %s", fileName);
	winApp.Report.Append(msgBuf, LVOUT_NORMAL);

	// Put the command line and the run flag in the command stack.
	winApp.Process.AddTask(
		"zip ", 
		OUTERR_PIPE,
		LVOUT_NORMAL);

	winApp.Process.CmdCat(fileName);
	if (*zipFlags){
		winApp.Process.CmdCat(" ");
		winApp.Process.CmdCat(zipFlags);
	}
	
	winApp.Process.Run();
}

void CProject::Explore(HWND hwnd){
	if (NoProject())
		return;

	if(ShellExecute(hwnd, "explore", szDirBuffer, NULL, NULL, SW_SHOWMAXIMIZED) 
			< (HINSTANCE) 32){
		MsgBox.DisplayString("Can't launch Explorer");
		return;
	}
}

/********************************************************************
*	Compiler Commands.
********************************************************************/
void CProject::Build(void){
	if (NoProject() || winApp.Process.isRunning())
		return;

	winApp.Report.Clear();
	winApp.Report.Append("Invoking compiler...", LVOUT_NORMAL);

	// Save modified files
	winApp.Manager.ProjectView.SaveAll(IDYES); // Silent.

	// Switch to Makefile Directory
	Makefile.SwitchCurrentDir();

	/* Build makefile ? */
	if (buildMakefile){
		winApp.Report.Append("Building makefile...", LVOUT_NORMAL);

		// Fill buffers and initialize a new process.
		Makefile.Build(&winApp.Manager.ProjectView, &winApp.Process);
		buildMakefile = false;
		modified = true;
	}

	// Put the command line and the run flag in the command stack.
	winApp.Process.AddTask(
		Makefile.make, 
		OUTERR_PIPE,
		LVOUT_ERROR);

	winApp.Process.CmdCat(" -f ");
	winApp.Process.CmdCat(&Makefile.szFileName[Makefile.nFileOffset]);
	if (Makefile.debug)
		winApp.Process.CmdCat(" debug");

	winApp.Process.Run();
}

void CProject::RebuildAll(void){
	if (NoProject() || winApp.Process.isRunning())
		return;

	// Switch to Makefile Directory
	Makefile.SwitchCurrentDir();

	winApp.Report.Clear();
	winApp.Report.Append("Invoking compiler...", LVOUT_NORMAL);

	// Save modified files
	winApp.Manager.ProjectView.SaveAll(IDYES); // Silent.

	// Build makefile.
	Makefile.Build(&winApp.Manager.ProjectView, &winApp.Process);
	buildMakefile = false;
	modified = true;

	// Make clean.
	winApp.Process.AddTask(
		Makefile.make, 
		OUTERR_PIPE,
		LVOUT_ERROR);

	winApp.Process.CmdCat(" -f ");
	winApp.Process.CmdCat(&Makefile.szFileName[Makefile.nFileOffset]);
	winApp.Process.CmdCat(" clean");
	
	// Build.
	winApp.Process.AddTask(
		Makefile.make, 
		OUTERR_PIPE,
		LVOUT_ERROR);

	winApp.Process.CmdCat(" -f ");
	winApp.Process.CmdCat(&Makefile.szFileName[Makefile.nFileOffset]);
	if (Makefile.debug)
		winApp.Process.CmdCat(" debug");

	winApp.Process.Run();
}

void CProject::RunTarget(void){
	if (NoProject() || winApp.Process.isRunning())
		return;

	if (Makefile.buildWhat == BUILD_STATLIB 
		|| Makefile.buildWhat ==BUILD_DLL)
		return;

	winApp.Report.Clear();
	winApp.Report.Append("Run target...", LVOUT_NORMAL);

	// Put the command line and the run flag in the command stack.
	winApp.Process.AddTask(szDirBuffer, NO_PIPE, STDOUT_NONE);
	winApp.Process.CmdCat("\\");
	if (Makefile.tgtDir[0] == '.'){
		if (Makefile.tgtDir[1] == '.' && Makefile.tgtDir[2] == '\\' 
									&& Makefile.tgtDir[3] != '\0'){
			winApp.Process.CmdCat(&Makefile.tgtDir[3]);
		}else{
			// Invalid dir, try ".\target".
			winApp.Process.CmdCat(".");
		}
	}
	winApp.Process.CmdCat("\\");
	winApp.Process.CmdCat(Makefile.target);
	
	winApp.Process.Run();
}

void CProject::MakeClean(void){
	if (NoProject() || winApp.Process.isRunning())
		return;

	// Switch to Makefile Directory
	Makefile.SwitchCurrentDir();

	winApp.Report.Clear();
	winApp.Report.Append("Deleting objects...", LVOUT_NORMAL);

	// Put the command line and the output flag in the command stack.
	winApp.Process.AddTask(
		Makefile.make, 
		OUTERR_PIPE,
		LVOUT_ERROR);

	winApp.Process.CmdCat(" -f ");
	winApp.Process.CmdCat(&Makefile.szFileName[Makefile.nFileOffset]);
	winApp.Process.CmdCat(" clean");
	
	winApp.Process.Run();
}

void CProject::BuildMakefile(void){
	if (NoProject() || winApp.Process.isRunning())
		return;

	// Switch to Project Directory
	Makefile.SwitchCurrentDir();

	winApp.Report.Clear();
	winApp.Report.Append("Building makefile...", LVOUT_NORMAL);

	// Fill buffers and initialize a new process.
	Makefile.Build(&winApp.Manager.ProjectView, &winApp.Process);
	buildMakefile = false;
	modified = true;
	// Run the process.
	winApp.Process.Run();
}


/********************************************************************
*	Class:	CMakefile.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CMakefile::CMakefile(){
	Init();
}

CMakefile::~CMakefile(){
}

void CMakefile::Init(void){
	szFileName[MAX_PATH - 1] = '\0';	// security.
	szFileName[0] 		= '\0';
	nFileOffset			= 0;
	strcpy	(mkfDir,	".");

	strcpy	(cc, 		"gcc");
	strcpy	(make, 	"make");
	strcpy	(wres, 	"windres");
	strcpy	(test, 	"gcc -v");

	target[0] 			= '\0';
	strcpy	(tgtDir,	".");
	buildWhat			= BUILD_EXE;
	debug			= false;
	lang				= LANGC;

	cppFlags[0]		= '\0';
	strcpy	(warning, 	"-W -Wall -pedantic");
	strcpy	(optimize, 	"-O2");
	cFlags[0]			= '\0';
	incDirs[0] 			= '\0';

	strcpy	(ldStrip, 	"-s");
	ldOpts[0]			= '\0';
	ldLibs[0]			= '\0';
	libDirs[0] 			= '\0';
	*arFlags = '\0';
}

void CMakefile::GetFullPath(char * prjFileName, WORD offset, char * name){
	// Copy project directory and append makefile relative dir.
	strncpy(szFileName, prjFileName, offset);
	WORD len = AppendPath(szFileName, offset, mkfDir);
	// Increment file offset.
	if (len){
		offset += len;
	}else{
		strcpy(mkfDir, ".");
	}
	// Append makefile name.
	szFileName[offset-1] = '\\';
	if (*name){
		strcpy(&szFileName[offset], name);
	}else{
		strcpy(name, "makefile");
	}
	nFileOffset = offset;
}

bool CMakefile::SwitchCurrentDir(void){
	// Switch to Makefile Directory.
	if (nFileOffset < 2)
		return false;
	szFileName[nFileOffset-1] = '\0';
	bool result = SetCurrentDirectory(szFileName);
	szFileName[nFileOffset-1] = '\\';
return result;
}

void CMakefile::Build(CProjectView * Tree, CProcess* /*Process*/){
	SrcList2Buffers(Tree);
	// Write the first part of the Makefile.
	Write();

	/* Invokes compiler to get dependencies with something like: 
		"gcc -MM file1.cpp file2.cpp ..." */

	CTask * task = winApp.Process.AddTask(
		depBuf, 
		OUTERR_PIPE,
		STDOUT_FILE_APPEND);
	strcpy(task->szFileName, szFileName);
}

void CMakefile::SrcList2Buffers(CProjectView * Tree){

	// 1. Begin to fill each buffer.
	strcpy(depBuf, cc);
	if (*cppFlags != '\0'){
		strcat(depBuf, " ");
		strcat(depBuf, cppFlags);
	}
	strcat(depBuf, " -MM ");
	if (*incDirs != '\0'){
		strcat(depBuf, incDirs);
		strcat(depBuf, " ");
	}

	strcpy(srcBuf, "\nSRCS\t=\\\n");
	strcpy(objBuf, "\nOBJS\t=\\\n");
	resBuf [0] = 0;

	// 2. Parse the module list and retrieve sources files names.
	CFileItem* srcFile;

	if(!Tree->First())
		return; // The list is empty, nothing to search.

	do {	srcFile = (CFileItem *) Tree->GetCurrent();

		if (srcFile->type == C_FILE || srcFile->type == RC_FILE){
			// Source files and objects buffers.
			strcat (srcBuf, "\t");
			strcat (srcBuf, &srcFile->szFileName[srcFile->nFileOffset]);
			strcat (srcBuf, "\\\n");

			// Change file extension.
			char ext[] = "o";
			strcpy(objFile, &srcFile->szFileName[srcFile->nFileOffset]);
			ChangeFileExt(objFile, ext);
			strcat (objBuf, "\t");
			strcat (objBuf, objFile);
			strcat (objBuf, "\\\n");

			if (srcFile->type == C_FILE){
				// Dependencies buffer.
				strcat(depBuf, &srcFile->szFileName[srcFile->nFileOffset]);
				strcat(depBuf, " ");
			}else if (srcFile->type == RC_FILE){
				// Resource buffer.
				strcat (resBuf, objFile);
				strcat (resBuf, ": ");
				strcat (resBuf, &srcFile->szFileName[srcFile->nFileOffset]);
				strcat (resBuf, "\n\n");
			}
		}
	} while (Tree->Next());

	int len = strlen(srcBuf);
	srcBuf[len-2] = '\n';
	srcBuf[len-1] = 0;

	len = strlen(objBuf);
	objBuf[len-2] = '\n';
	objBuf[len-1] = 0;
}

void CMakefile::Write(void){
	FILE * file;

	file = fopen(szFileName, "w");
	if (!file){
		MsgBox.DisplayString("Can't open file :\r\n%s", szFileName);
		return;
	}

	/* Signature */
	fprintf (file, "# Generated automatically by Visual-MinGW.\n");
	fprintf (file, "# http://visual-mingw.sourceforge.net/\n");

	/* Standard defines */
	fprintf (file, "\nCC = %s", 			cc);
	fprintf (file, "\nWRES = %s", 			wres		);
	fprintf (file, "\nDLLWRAP = dllwrap" 			);
	fprintf (file, "\nCPPFLAGS = %s", 				cppFlags	);

	if (buildWhat == BUILD_GUIEXE)
		fprintf (file, "\nLDBASEFLAGS = -mwindows %s %s", 	ldOpts, ldLibs	);
	else
		fprintf (file, "\nLDBASEFLAGS = %s %s", 		ldOpts, ldLibs	);
	fprintf (file, "\nINCDIRS = %s",				incDirs	);
	fprintf (file, "\nOPTIMIZ = %s",				optimize	);
	fprintf (file, "\nSTRIP = %s",					ldStrip	);
	/* Debug symbols ? Language ? */
	fprintf (file, "\n\nifeq ($(MAKECMDGOALS),debug)");
	if (lang == LANGCPP){
		fprintf (file, "\nCXXFLAGS = %s $(INCDIRS) -g %s", warning, cFlags);
		fprintf (file, "\nLDFLAGS = $(LDBASEFLAGS)");
		fprintf (file, "\nelse");
		fprintf (file, "\nCXXFLAGS = %s $(INCDIRS) $(OPTIMIZ) %s", warning, cFlags);
		fprintf (file, "\nLDFLAGS = $(STRIP) $(LDBASEFLAGS)");
		fprintf (file, "\nendif");
	}else{
		fprintf (file, "\nCFLAGS = %s $(INCDIRS) -g %s", warning, cFlags);
		fprintf (file, "\nLDFLAGS = $(LDBASEFLAGS)");
		fprintf (file, "\nelse");
		fprintf (file, "\nCFLAGS = %s $(INCDIRS) $(OPTIMIZ) %s", warning, cFlags);
		fprintf (file, "\nLDFLAGS = $(STRIP) $(LDBASEFLAGS)");
		fprintf (file, "\nendif");
	}
	/* Directories */
	fprintf (file, "\n\nSRCDIR = %s", 						mkfDir	);
	fprintf (file, "\nBINDIR = %s", 						tgtDir		);
	fprintf (file, "\nLIBDIRS = %s",						libDirs	);
	/* Rule to compile rc files */
	fprintf (file, "\n\n%c.o : %c.rc\n\t$(WRES) $(CPPFLAGS) $< $@", 	'%', '%'				);
	/* List of objects */
	fprintf (file, "\n%s", 								objBuf	);
	/* Target */
	fprintf (file, "\nTARGET =\t$(BINDIR)\\%s" , 				target	);
	/* all, alldebug */
	fprintf (file, "\n\n# Targets\n"								);
	fprintf (file, "all:\t$(TARGET)\n\ndebug:\t$(TARGET)\n\n"									);
	/* clean */
	fprintf (file, "cleanobjs:\n\trm -f $(OBJS)\n\n"						);
	fprintf (file, "cleanbin:\n\trm -f $(TARGET)\n\n"						);
	fprintf (file, "clean:\tcleanobjs cleanbin\n\n"						);
	/* Dependencies */
	fprintf (file, "# Dependency rules\n"								);
	/* Language */
	if (buildWhat == BUILD_STATLIB){
		fprintf (file, "$(TARGET): $(OBJS)\n\t$(AR) -ru $(BINDIR)\\%s", 	target);
		fprintf (file, " $(OBJS) $(INCDIRS) $(LIBDIRS)\n\n%s", 		resBuf);
	}else if (buildWhat == BUILD_DLL){
		fprintf (file, "$(TARGET): $(OBJS)\n\t$(DLLWRAP) -o $@ $(OBJS) $(LDFLAGS)\n\n"	);
	}else if (lang == LANGCPP){
		fprintf (file, "$(TARGET): $(OBJS)\n\t$(CXX) -o $(BINDIR)\\%s", 	target);
		fprintf (file, " $(OBJS) $(INCDIRS) $(LIBDIRS) $(LDFLAGS)\n\n%s",resBuf);
	}else{
		fprintf (file, "$(TARGET): $(OBJS)\n\t$(CC) -o $(BINDIR)\\%s", 	target);
		fprintf (file, " $(OBJS) $(INCDIRS) $(LIBDIRS) $(LDFLAGS)\n\n%s",resBuf);
	}
	fclose(file);
}


/********************************************************************
*	Class:	CCompiler.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CCompiler::CCompiler(){
	make[0] 		= '\0';
	cc[0] 		= '\0';
	cFlags[0] 		= '\0';
	ldFlags[0]		= '\0';
	wres[0] 		= '\0';
	debug[0] 		= '\0';
	test[0] 		= '\0';
}

CCompiler::~CCompiler(){
}

bool CCompiler::LoadData(char * fullpath){
	Load(fullpath);
	// [Common] section
	GetString(make, 		"MAKE", 		"Common");
	GetString(cc,		"CC"	 		);
	GetString(cFlags, 	"CFLAGS" 		);
	GetString(ldFlags, 	"LDFLAGS" 		);
	GetString(wres, 		"WRES" 		);
	GetString(debug, 	"DEBUG" 		);
	GetString(test, 		"TEST" 		);
return true;
}

