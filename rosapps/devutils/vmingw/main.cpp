/********************************************************************
*	Module:	main.cpp. This is part of Visual-MinGW.
*
*	Purpose:	Main module.
*
*	Authors:	Manu B.
*
*	License:	Visual-MinGW is a C/C++ Integrated Development Environment.
*			Copyright (C) 2001  Manu.
*
*			This program is free software; you can redistribute it and/or modify
*			it under the terms of the GNU General Public License as published by
*			the Free Software Foundation; either version 2 of the License, or
*			(at your option) any later version.
*
*			This program is distributed in the hope that it will be useful,
*			but WITHOUT ANY WARRANTY; without even the implied warranty of
*			MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*			GNU General Public License for more details.
*
*			You should have received a copy of the GNU General Public License
*			along with this program; if not, write to the Free Software
*			Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
*			USA.
*
*			(See license.htm for more details.)
*
*	Revisions:	
*			Manu B. 12/15/01	CFileList created.
*
********************************************************************/
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <ctype.h>
#include "process.h"
#include "project.h"
#include "main.h"
#include "rsrc.h"

#define MSG_MODIFIED "Modified"

CCriticalSection CriticalSection;
extern CMessageBox MsgBox;
CFindReplaceDlg EditorDlg;
void Main_CmdTest(HWND hwnd);

/* Globals */
char * g_env_path = NULL;
char * g_vm_path = NULL;
CWinApp	winApp;
CProject	Project;
CChrono	Chrono;

// File filters & flags.
DWORD singleFileOpen = OFN_EXPLORER | OFN_FILEMUSTEXIST;
DWORD multipleFileOpen = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
DWORD fileSave = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
				OFN_OVERWRITEPROMPT;

char * defFilter = "All Sources\0*.c;*.cpp;*.cxx;*.h;*.hpp;*.hxx;*.rc;*.mingw;*.prj\0"
			"C Files\0*.c;*.cpp;*.cxx;*.rc\0"
			"Headers\0*.h;*.hpp;*.hxx\0"
			"Ini file\0*.ini\0"
			"All Files (*.*)\0*.*\0\0";

char * prjFilter = "Project Files (*.prj)\0*.prj\0All Files (*.*)\0*.*\0\0";
char * prjDefExt = "prj";

char * srcFilter = "All Sources\0*.c;*.cpp;*.cxx;*.h;*.hpp;*.hxx;*.rc\0"
			"C Files\0*.c;*.cpp;*.cxx;*.rc\0"
			"Headers\0*.h;*.hpp;*.hxx\0"
			"All Files (*.*)\0*.*\0\0";
char * srcDefExt = "cpp";

/* For tests */
	CChooseFontDlg	CChooseFont;
void Main_CmdTest(HWND){
	winApp.Process.AddTask(
		"sh.exe", 
		IN_PIPE || OUTERR_PIPE,
		LVOUT_ERROR);
	winApp.Process.Run();
/*	CChooseFont.Create(&winApp);*/
return;
}


/********************************************************************
*	Class:	CFileDlg.
*
*	Purpose:	A CFileDlgBase for Open/Save dlg boxes.
*
*	Revisions:	
*
********************************************************************/
CFileDlg::CFileDlg(){
}

CFileDlg::~CFileDlg(){
}

bool CFileDlg::Open(CWindow * pWindow, char * pszFileName, DWORD nMaxFile, int fileflag){

	switch(fileflag){
		// Project file.
		case PRJ_FILE:
		SetData(prjFilter, prjDefExt, singleFileOpen);
		break;

		// Add multiple files to project.
		case ADD_SRC_FILE:
		Reset();
		SetTitle("Add files to project");
		nMaxFile = 2048;
		SetFilterIndex(1);
		SetData(srcFilter, srcDefExt, multipleFileOpen);
		break;

		default: // SRC_FILE
		SetData(defFilter, srcDefExt, singleFileOpen);
		SetFilterIndex(1);
		break;
	}
return OpenFileName(pWindow, pszFileName, nMaxFile);
}

bool CFileDlg::Save(CWindow * pWindow, char * pszFileName, DWORD nMaxFile, int fileflag){
	Reset();

	switch(fileflag){
		case SRC_FILE:
		SetData(defFilter, srcDefExt, fileSave);
		SetFilterIndex(1);
		break;

		default: // PRJ_FILE
		SetData(prjFilter, prjDefExt, fileSave);
		break;
	}
return SaveFileName(pWindow, pszFileName, nMaxFile);
}


/********************************************************************
*	Class:	CPreferencesDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CPreferencesDlg::CPreferencesDlg(){
}

CPreferencesDlg::~CPreferencesDlg(){
}

int CPreferencesDlg::Create(void){
return CreateModal(&winApp, IDD_PREFERENCES, (LPARAM) this);
}

LRESULT CALLBACK CPreferencesDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
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

BOOL CPreferencesDlg::OnInitDialog(HWND, LPARAM){
	// Tab control handle and TCITEM.
	_hWndTab = ::GetDlgItem(_hWnd, IDC_PREF_TABS);
	tcitem.mask = TCIF_TEXT | TCIF_PARAM;

	// Insert tabs.
	HWND hwndChild = EnvDlg.Create(this, IDD_ENVIRON, NULL, (long) NULL);
	tcitem.pszText = "Environment";
	tcitem.lParam = (long) &EnvDlg;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 0, (LPARAM)&tcitem);
	SetChildPosition(hwndChild);
	
/*	tcitem.pszText = "General";
	tcitem.lParam = (long) NULL;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 1, (LPARAM)&tcitem);

	tcitem.pszText = "Find in files";
	tcitem.lParam = (long) NULL;
	::SendMessage(_hWndTab, TCM_INSERTITEM, 2, (LPARAM)&tcitem);*/

	// Show the dialog and default pane.
	Show();
	EnvDlg.Show();
	EnvDlg.SetFocus();
return TRUE;
}

BOOL CPreferencesDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:
			EnvDlg.OnCommand(0, wID, 0);
			EndDlg(IDOK);
		return TRUE;

		case IDCANCEL:
			EndDlg(IDCANCEL);
		return FALSE;

		case IDAPPLY:
			EnvDlg.OnCommand(0, wID, 0);
		return TRUE;
	}
return FALSE;
}

BOOL CPreferencesDlg::EndDlg(int nResult){
	EnvDlg.EndDlg(0);
	EnvDlg.bIsVisible = false;
	if (_hWnd){
		BOOL result = ::EndDialog(_hWnd, nResult);
		_hWnd = 0;
		return result;
	}
return false;
}


/********************************************************************
*	Class:	CEnvDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CEnvDlg::CEnvDlg(){
}

CEnvDlg::~CEnvDlg(){
}

LRESULT CALLBACK CEnvDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;
	}
return FALSE;
}

BOOL CEnvDlg::OnInitDialog(HWND, LPARAM){
	bIsVisible = false;
	bModified = false;
	/* Get control handles */
	hApply		= ::GetDlgItem(_pParent->_hWnd, IDAPPLY);
	hSetCcBin		= GetItem(IDC_SET_CCBIN);
	hCcBinDir		= GetItem(IDC_CCBIN);
	hBrowseCc		= GetItem(IDC_BROWSE_CC);
	hSetCmdBin	= GetItem(IDC_SET_CMDBIN);
	hCmdBinDir		= GetItem(IDC_CMDBIN);
	hBrowseCmd	= GetItem(IDC_BROWSE_CMD);
	hAutoexec		= GetItem(IDC_AUTOEXEC);
	hEnvView		= GetItem(IDC_ENV_VIEW);

	if (winApp.bSetCcEnv)
		::SendMessage(hSetCcBin, BM_SETCHECK, BST_CHECKED, 0);
	if (winApp.bSetCmdEnv)
		::SendMessage(hSetCmdBin, BM_SETCHECK, BST_CHECKED, 0);
	if (winApp.bSetDefEnv)
		::SendMessage(hAutoexec, BM_SETCHECK, BST_CHECKED, 0);

	SetItemText(hCcBinDir, winApp.szCcBinDir);
	SetItemText(hCmdBinDir, winApp.szCmdBinDir);
	SetEnvText();
/*	hCcIncDir	= GetItem(IDC_CC_INCDIR);

	SetItemText(hCcIncDir, winApp.includeDir);*/
	bIsVisible = true;
return TRUE;
}

BOOL CEnvDlg::OnCommand(WORD wNotifyCode, WORD wID, HWND){
	char directory[MAX_PATH];

	switch (wID){
		case IDC_BROWSE_CC:
			if (winApp.ShellDlg.BrowseForFolder(&winApp, directory, "Browse", 
				BIF_RETURNONLYFSDIRS)){
				SetItemText(hCcBinDir, directory);
			}
		return TRUE;

		case IDC_BROWSE_CMD:
			if (winApp.ShellDlg.BrowseForFolder(&winApp, directory, "Browse", 
				BIF_RETURNONLYFSDIRS)){
				SetItemText(hCmdBinDir, directory);
			}
		return TRUE;

		case IDOK:
			winApp.bSetCcEnv = 
				(BST_CHECKED==::SendMessage(hSetCcBin, BM_GETCHECK, 0, 0));
			winApp.bSetCmdEnv = 
				(BST_CHECKED==::SendMessage(hSetCmdBin, BM_GETCHECK, 0, 0));
			winApp.bSetDefEnv = 
				(BST_CHECKED==::SendMessage(hAutoexec, BM_GETCHECK, 0, 0));

			GetItemText(hCcBinDir, winApp.szCcBinDir,	MAX_PATH);
			GetItemText(hCmdBinDir, winApp.szCmdBinDir,	MAX_PATH);
//			GetItemText(hCcIncDir, winApp.includeDir,	MAX_PATH);
			if (bModified)
				winApp.SetEnv();
		return TRUE;

		case IDCANCEL:
		return FALSE;

		case IDAPPLY:
			if (bModified){
				winApp.bSetCcEnv = 
					(BST_CHECKED==::SendMessage(hSetCcBin, BM_GETCHECK, 0, 0));
				winApp.bSetCmdEnv = 
					(BST_CHECKED==::SendMessage(hSetCmdBin, BM_GETCHECK, 0, 0));
				winApp.bSetDefEnv = 
					(BST_CHECKED==::SendMessage(hAutoexec, BM_GETCHECK, 0, 0));

				GetItemText(hCcBinDir, winApp.szCcBinDir,	MAX_PATH);
				GetItemText(hCmdBinDir, winApp.szCmdBinDir,	MAX_PATH);
				winApp.SetEnv();
				SetEnvText();
				bModified = false;
				::EnableWindow(hApply, false);
			}
		return TRUE;

		default:
			if (bIsVisible && !bModified){
				switch(wNotifyCode){
					case EN_CHANGE:
					case BN_CLICKED:
						bModified = true;
						::EnableWindow(hApply, true);
 					return TRUE;
				}
			}
		break;
	}
return FALSE;
}

void CEnvDlg::SetEnvText(void){
	if (g_vm_path){
		char * text = (char *) malloc(strlen(g_vm_path)+20); // 10 lines max.
		char * start = text;
		char * parse = g_vm_path;
		while (*parse){
			if (*parse == ';'){
				// Change ';' into CR/LF.
				*text = '\r';
				text++;
				*text = '\n';
				text++;
				parse++;
			}else if (*parse == '='){
				// Rewind buffer.
				text = start;
				parse++;
			}else{
				// Copy one char.
				*text = *parse;
				text++;
				parse++;
			}
		}
		*text = '\0';
		SetItemText(hEnvView, start);
		free(start);
	}
}


/********************************************************************
*	Class:	CGrepDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CGrepDlg::CGrepDlg(){
	*findWhat	= '\0';
	*gDir		= '\0';
}

CGrepDlg::~CGrepDlg(){
}

int CGrepDlg::Create(void){
return CreateModal(&winApp, IDD_GREP, (LPARAM) this);
}

LRESULT CALLBACK CGrepDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
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

BOOL CGrepDlg::OnInitDialog(HWND, LPARAM){
	hFindWhat	= GetItem(IDC_FINDWHAT);
	hgDir		= GetItem(IDC_GFILTER);

	SetItemText(hFindWhat, findWhat);
	SetItemText(hgDir, gDir);
	// Show the dialog and default pane.
	Show();
return TRUE;
}

BOOL CGrepDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:
			GetItemText(hFindWhat, findWhat, sizeof(findWhat));
			GetItemText(hgDir, gDir, sizeof(gDir));
			FindInFiles(findWhat, gDir);
		return TRUE;

		case IDCANCEL:
			EndDlg(IDCANCEL);
		return FALSE;
	}
return FALSE;
}

void CGrepDlg::FindInFiles(char * findWhat, char * fileFilter){
	if (!findWhat || !fileFilter || winApp.Process.isRunning())
		return;

	winApp.Report.Clear();
	winApp.Report.Append("Grep search...", LVOUT_NORMAL);

	winApp.Process.AddTask("grep -G -n -H ", OUTERR_PIPE, LVOUT_ERROR);
	winApp.Process.CmdCat(findWhat);
	winApp.Process.CmdCat(" ");
	winApp.Process.CmdCat(fileFilter);
	
	winApp.Process.Run();
}


/********************************************************************
*	Functions:	WinMain procedure.
*
*	Purpose:	Runs the application.
*
*	Revisions:	
*
********************************************************************/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
return winApp.Run(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

	
/********************************************************************
*	Class:	CWinApp.
*
*	Purpose:	Manages the all application.
*
*	Revisions:	
*
********************************************************************/
CWinApp::CWinApp(){
	*iniFileName = '\0';
	hmod	= NULL;
	*openFilesDir ='\0';
	*projectDir = '\0';
	*includeDir = '\0';
	bSetCcEnv = false;
	bSetCmdEnv = false;
	bSetDefEnv = false;
	*szCcBinDir = '\0';
	*szCmdBinDir = '\0';

	firstRun = false;
	// Child windows dimensions.
	deltaY 	= 0;
	tbarHeight 	= 26;
	sbarHeight 	= 20;
	tvWidth 	= 140;
	lvHeight 	= 120;

	hSplitter 	= 4;
	vSplitter 	= 4;
}

CWinApp::~CWinApp(){
}

bool CWinApp::CustomInit(void){
	/* Get PATH environment variable */
	char * env_path = getenv("PATH");
	if (env_path)
		g_env_path = strdup(env_path);

	SetName("Visual MinGW", APP_VERSION);
	MsgBox.SetCaption("Visual MinGW");
	IsWinNT();
	ReadIniFile("visual-mingw.ini");

	hAccel = LoadAccelerators(_hInst, "ACCELS");
	hmod = LoadLibrary("SciLexer.DLL");
	if (!hmod){
		MsgBox.DisplayFatal("Unable to load SciLexer.DLL");
		return false;
	}
return true;
}

bool CWinApp::Release(void){
	WriteIniFile();
	if (hmod)
		FreeLibrary(hmod);
	if (g_env_path)
		free(g_env_path);
return true;
}

bool CWinApp::ReadIniFile(char * fileName){
	ParseCmdLine(iniFileName);
	strcat(iniFileName, fileName);
	
	if (!IniFile.Load(iniFileName)){
		/* Create an empty file and fill it */
		firstRun = true;
		MsgBox.DisplayWarning("Visual-MinGW first run !\n"
			"Step 1: User interface initialization.\n"
			"Please report bugs to Visual-MinGW home page.\n"
			"See the Readme text for more information.");
		FILE * file = fopen(iniFileName, "wb");
		if (!file)
			return false;
		SaveIniFile(file);
		fclose(file);
		return false;
	}
	// [General] section
	IniFile.GetString(openFilesDir, 	"FilesDirectory", 		"General"	);
	IniFile.GetString(projectDir, 		"ProjectDirectory"				);
	bSetDefEnv = IniFile.GetInt( 		"SetDefEnv"				);
	bSetCmdEnv = IniFile.GetInt( 	"SetBinDir"					);
	IniFile.GetString(szCmdBinDir, 	"BinDir"					);
	// [Compiler] section
	IniFile.GetString(includeDir, 		"IncludeDir", 		"Compiler"	);
	bSetCcEnv = IniFile.GetInt( 		"SetBinDir"					);
	IniFile.GetString(szCcBinDir, 		"BinDir"					);

	SetEnv();
return true;
}

void CWinApp::SaveIniFile(FILE * file){
	// [General]
	fprintf (file, "\t; Generated automatically by Visual-MinGW.\n");
	fprintf (file, "\t   ; http://visual-mingw.sourceforge.net/\n");
	fprintf (file, "[General]\nSignature = 40");
	fprintf (file, "\nFilesDirectory = %s",	openFilesDir);
	fprintf (file, "\nProjectDirectory = %s",	projectDir);
	fprintf (file, "\nTvWidth = %d",	tvWidth);
	fprintf (file, "\nLvHeight = %d",	lvHeight);
	fprintf (file, "\nSetDefEnv = %d",	bSetDefEnv);
	fprintf (file, "\nSetBinDir = %d",	bSetCmdEnv);
	fprintf (file, "\nBinDir = %s",		szCmdBinDir);
	// [Compiler]
	fprintf (file, "\n\n[Compiler]\nIncludeDir = %s",	includeDir);
	fprintf (file, "\nSetBinDir = %d",	bSetCcEnv);
	fprintf (file, "\nBinDir = %s",		szCcBinDir);
}

bool CWinApp::WriteIniFile(void){
	if (*iniFileName == '\0')
		return false;
	FILE * file = fopen(iniFileName, "wb");
	if (!file)
		return false;
	SaveIniFile(file);
	fclose(file);
	IniFile.Close();
return true;
}

bool CWinApp::SetEnv(void){
	// Free previous variable.
	//getenv("PATH=");
	// Malloc a buffer.
	int len = 0;
	if (bSetCcEnv)
		len += strlen(winApp.szCcBinDir);
	if (bSetCmdEnv)
		len += strlen(winApp.szCmdBinDir);
	if (bSetDefEnv && g_env_path)
		len += strlen(g_env_path);
	g_vm_path = (char *) malloc(len+8);

	// Copy the environment variable.
	strcpy(g_vm_path, "PATH=");
	if (bSetCcEnv && *winApp.szCcBinDir){
		strcat(g_vm_path, winApp.szCcBinDir);
		strcat(g_vm_path, ";");
	}
	if (bSetCmdEnv && *winApp.szCmdBinDir){
		strcat(g_vm_path, winApp.szCmdBinDir);
		strcat(g_vm_path, ";");
	}
	if (bSetDefEnv && g_env_path)
		strcat(g_vm_path, g_env_path);

	len = strlen(g_vm_path) - 1;
	if (g_vm_path[len] == ';')
		g_vm_path[len] = '\0';
	if (putenv(g_vm_path) == -1){
		free(g_vm_path);
		g_vm_path = NULL;
		return false;
	}
return true;
}


/********************************************************************
*	CWinApp: Create each application's window.
********************************************************************/
bool CWinApp::CreateUI(void){

	InitCommonControls();

	// Custom values.
	wc.style			= 0;
	wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor			= NULL;
	wc.hbrBackground	= //NULL;
					   (HBRUSH)(COLOR_INACTIVEBORDER + 1);
	wc.lpszMenuName	= MAKEINTRESOURCE(ID_MENU);
	wc.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);

	if(!MainRegisterEx("main_class")) {
		MsgBox.DisplayFatal("Can't Register Main Window");
		return false;
	}

	// Custom values.
	wc.hbrBackground	= NULL;
	wc.lpszMenuName	= 0;

	if(!ChildRegisterEx("child_class")) {
		MsgBox.DisplayFatal("Can't Register MDI Class");
		return false;
	}

	// Use a CreateWindowEx like procedure.
	HWND hwnd = CreateEx(
		this,	// Owner class.
		0, 
		mainClass, 
		appName, 
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		0, 
		NULL);

	if(!hwnd) {
		MsgBox.DisplayFatal("Can't create Main window");
		return false;
	}
	MsgBox.SetParent(hwnd);

	// SW_SHOWMAXIMIZED.
	::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	::UpdateWindow(hwnd);
	if (firstRun)
		FirstRunTest();
	firstRun = false;
return true;
}

void CWinApp::FirstRunTest(void){
	MsgBox.DisplayWarning("Visual-MinGW first run !\n"
		"Step 2: You will now set your environment variables.\n"
		"\"Use default environment variables\" should be checked.\n"
		"Then Visual-MinGW will try to launch the compiler.");
	PreferencesDlg.Create();
	MsgBox.DisplayWarning("Visual-MinGW first run !\n"
		"Step 3: Installation checking.\n"
		"Try to launch rm and gcc.\n"
		"See \"Main\" or \"Log\" report views for results.\n");
	winApp.Report.Clear();
	winApp.Report.Append("Testing for first run...", LVOUT_NORMAL);

	// Put the command line and the run flag in the command stack.
	winApp.Process.AddTask("gcc -v", OUTERR_PIPE, LVOUT_NORMAL);
	winApp.Process.AddTask("rm --version", OUTERR_PIPE, LVOUT_NORMAL);
	winApp.Process.Run();
return;
}

void CWinApp::CreateToolbar(void){
	Toolbar.CreateEx(
		this, 
		0,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
		|TBSTYLE_FLAT | TBSTYLE_TOOLTIPS
		| CCS_NORESIZE);
	
	Toolbar.AddBitmap(IDB_TOOLBAR, 15);

	TBBUTTON tbButtons [] = 
	{	{ 0, 0, 		TBSTATE_ENABLED, TBSTYLE_SEP, 	{0, 0}, 0, 0},
		{ 0, IDM_NEW, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 1, IDM_OPEN, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 2, IDM_SAVE, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 0, 0, 		TBSTATE_ENABLED, TBSTYLE_SEP, 	{0, 0}, 0, 0},
		{ 3, IDM_CUT, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 4, IDM_COPY, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 5, IDM_PASTE,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 0, 0, 		TBSTATE_ENABLED, TBSTYLE_SEP, 	{0, 0}, 0, 0},
		{ 6, IDM_UNDO, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0},
		{ 7, IDM_REDO, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, 	{0, 0}, 0, 0}		};

	int numbutton = sizeof tbButtons/sizeof tbButtons[0];

	Toolbar.AddButtons(&tbButtons[0], numbutton);
}

void CWinApp::CreateSplitter(void){
	MainSplitter.Init(&ChildSplitter, &Report, SPLSTYLE_HORZ, lvHeight, SPLMODE_2);
	ChildSplitter.Init(&Manager, &MdiClient, SPLSTYLE_VERT, tvWidth, SPLMODE_1);

	// File Manager.
	Manager.Create(this);
	// MDI client.
	CreateMDI();
	// ListView.
	Report.Create(this);
}

void CWinApp::CreateMDI(void){
	MdiClient.Init(3, ID_FIRSTCHILD);
	MdiClient.CreateEx(
		this, 
		WS_EX_CLIENTEDGE,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
			| WS_VSCROLL | WS_HSCROLL
		);
}

HWND CWinApp::CreateChild(char * caption, LPVOID lParam){

	CChildView	* mdiChild = new CChildView;

	HWND hwnd = mdiChild->CreateEx(
		&MdiClient,	// MUST be an MdiClient *.
		WS_EX_MDICHILD, 				
		MDIS_ALLCHILDSTYLES | WS_CHILD | WS_SYSMENU | WS_CAPTION 
		| WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
		| WS_MAXIMIZE,
		caption,
		0,
		lParam);

	if (!hwnd)
		delete mdiChild;
return hwnd;
}

void CWinApp::CreateStatusBar(void){
	Sbar.CreateEx(
		this, 
		0,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);

	int sbWidths[] = {60, 120, -1};

	int numparts = sizeof sbWidths/sizeof sbWidths[0];

	Sbar.SetParts(numparts, &sbWidths[0]);
}

void CWinApp::SendCaretPos(int caretPos) {
	// To display the "Line : xxxx" message, we use our standard msgBuf[256].
	sprintf(msgBuf, "Line : %d", caretPos);
	Sbar.SendMessage(SB_SETTEXT, 0, (LPARAM) msgBuf);
return;
}


/********************************************************************
*	CWinApp: Message handling procedures.
********************************************************************/
LRESULT CALLBACK CWinApp::CMainWndProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_CREATE:
			return OnCreate((LPCREATESTRUCT) lParam);
		
		case WM_PAINT:
			return OnPaint((HDC) wParam);

		case WM_SIZE:
			return OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
		
		case WM_DESTROY:
			return OnDestroy();

		case WM_COMMAND:
			return OnCommand(wParam, lParam);
		
		case WM_CLOSE:
			return OnClose();

		case WM_NOTIFY:
			return OnNotify((int) wParam, (LPNMHDR) lParam);
		
		case WM_LBUTTONDOWN:
			return OnLButtonDown((short) LOWORD(lParam), (short) HIWORD(lParam), wParam);
	
		case WM_MOUSEMOVE:
			return OnMouseMove((short) LOWORD(lParam), (short) HIWORD(lParam), wParam);
	
		case WM_LBUTTONUP:
			return OnLButtonUp((short) LOWORD(lParam), (short) HIWORD(lParam), wParam);
	
		case WM_SETCURSOR:
			OnSetCursor((HWND) wParam, (UINT) LOWORD(lParam), (UINT) HIWORD(lParam));
			return DefWindowProc(_hWnd, Message, wParam, lParam);

		default:
			return DefFrameProc(_hWnd, MdiClient.GetId(), Message, wParam, lParam);
	}
return 0;
}

BOOL CWinApp::OnCreate(LPCREATESTRUCT){
	// Toolbar.
	CreateToolbar();
	// Splitter.
	CreateSplitter();
	// Statusbar.
	CreateStatusBar();
return TRUE;
} 

BOOL CWinApp::OnPaint(HDC){
	PAINTSTRUCT ps;
	BeginPaint(_hWnd, &ps);
	EndPaint(_hWnd, &ps);
return 0;
}

BOOL CWinApp::OnSize(UINT, int width, int height){
	// TreeView and MDI Client delta-height.
	deltaY =	height-sbarHeight-lvHeight-vSplitter-tbarHeight;

	if (deltaY>3){
		Toolbar.SetPosition(0,
			0, 					0, 
			width, 				tbarHeight,
			0);

		MainSplitter.SetPosition(0,	
			0, 					tbarHeight, 
			width, 				height-tbarHeight-sbarHeight,
			0);
	
		Sbar.SetPosition(0,
			0, 					height-sbarHeight, 
			width, 				sbarHeight,
			0);
	}
	InvalidateRect(_hWnd, NULL, false);
return 0;
} 

BOOL CWinApp::OnDestroy(void){
	PostQuitMessage(0);
return 0;
}

BOOL CWinApp::OnClose(void){
	if (IDCANCEL == Manager.SaveAll(IDASK)) // Ask to save.
		return TRUE; // Cancelled by user.
	::DestroyWindow(_hWnd);
return 0;
}

BOOL	CWinApp::OnNotify(int idCtrl, LPNMHDR notify){
	Manager.OnNotify(idCtrl, notify);
	Report.OnNotify(idCtrl, notify);
return 0;
}

BOOL CWinApp::OnLButtonDown(short xPos, short yPos, UINT){
	MainSplitter.OnLButtonDown(_hWnd, xPos, yPos);
	ChildSplitter.OnLButtonDown(_hWnd, xPos, yPos);
return 0;
}

BOOL CWinApp::OnMouseMove(short xPos, short yPos, UINT){
	MainSplitter.OnMouseMove(_hWnd, xPos, yPos);
	ChildSplitter.OnMouseMove(_hWnd, xPos, yPos);
return 0;
}

BOOL CWinApp::OnLButtonUp(short xPos, short yPos, UINT){
	MainSplitter.OnLButtonUp(_hWnd, xPos, yPos);
	ChildSplitter.OnLButtonUp(_hWnd, xPos, yPos);
return 0;
}

BOOL CWinApp::OnSetCursor(HWND, UINT nHittest, UINT){
	if (nHittest == HTCLIENT) {
		if (MainSplitter.OnSetCursor(_hWnd, 0)){
			return 0;
		}else if (ChildSplitter.OnSetCursor(_hWnd, 0)){
			return 0;
		}else{
			::SetCursor(::LoadCursor(NULL, IDC_ARROW));
		}
	}
return 0;
}


/********************************************************************
*	CWinApp: Dispatch command messages.
********************************************************************/
BOOL CWinApp::OnCommand(WPARAM wParam, LPARAM lParam){
	int wID = LOWORD(wParam);

	switch (wID){
		/* File Menu */
		case IDM_NEW:
			Manager.FilesView.New();
			break;
	
		case IDM_OPEN:
			Manager.OpenFileDialog();
			break;
		
		case IDM_NEW_PROJECT:
			Manager.NewProjectDialog();
			break;

		case IDM_OPEN_PROJECT:
			Manager.OpenProjectDialog();
			break;
		
		case IDM_SAVE_PROJECT:
			Manager.SaveProjectFiles(IDYES);
			break;
		
		case IDM_CLOSE_PROJECT:
			Manager.CloseProject();
			break;

		case IDM_PREFERENCES:
			PreferencesDlg.Create();
			break;

		case IDM_QUIT:
			PostMessage(_hWnd, WM_CLOSE, 0, 0);
			break;

		/* Find Menu */
		case IDM_GREP:
			GrepDlg.Create();
			break;

		/* Window Menu */
		case IDM_CASCADE:
			PostMessage(MdiClient.GetId(), WM_MDICASCADE, 0, 0);
			break;
		case IDM_TILEHORZ:
			PostMessage(MdiClient.GetId(), WM_MDITILE, MDITILE_HORIZONTAL, 0);
			break;
		case IDM_TILEVERT:
			PostMessage(MdiClient.GetId(), WM_MDITILE, MDITILE_VERTICAL, 0);
			break;
		case IDM_ARRANGE:
			PostMessage(MdiClient.GetId(), WM_MDIICONARRANGE, 0, 0);
			break;
	
		/* Project Menu */
		case IDM_NEW_MODULE:
			Project.NewModuleDlg();
			break;

		case IDM_ADD:
			Project.AddFiles();
			break;

		case IDM_REMOVE_FILE:
			Manager.RemoveProjectFile();
			break;

		case IDM_REMOVE_MODULE:
			Manager.RemoveProjectModule();
			break;

		case IDM_OPTION:
			Project.OptionsDlg();
			break;

		case IDM_ZIP_SRCS:
			Project.ZipSrcs();
			break;

		case IDM_EXPLORE:
			Project.Explore(_hWnd);
			break;

		/* Build Menu */
		case IDM_BUILD:
			Project.Build();
			break;

		case IDM_REBUILDALL:
			Project.RebuildAll();
			break;

		case IDM_RUN_TARGET:
			Project.RunTarget();
			break;

		case IDM_MKCLEAN:
			Project.MakeClean();
			break;

		case IDM_MKF_BUILD:
			Project.BuildMakefile();
			break;

		case IDM_RUN_CMD:
			winApp.Process.CommandDlg.Create();
			break;

		case IDM_TEST:
			Main_CmdTest(_hWnd);
			break;

		default:{
			if (wID >= ID_FIRSTCHILD){
				DefFrameProc(_hWnd, MdiClient.GetId(), WM_COMMAND, wParam, lParam);
			}else{
				HWND hChildWindow = (HWND) MdiClient.SendMessage(WM_MDIGETACTIVE);

				if (hChildWindow)
					::SendMessage(hChildWindow, WM_COMMAND, wParam, lParam);
			}
		}
	}
return TRUE;
}


/********************************************************************
*	CWinApp: Handles child messages.
********************************************************************/
LRESULT CALLBACK CWinApp::CChildWndProc(CWindow * pWnd, UINT Message, WPARAM wParam, LPARAM lParam){

	CChildView * childView = (CChildView *) pWnd;
	HWND hwndChild = childView->_hWnd;

	switch(Message){
		case WM_CREATE:
			childView->OnCreate((LPCREATESTRUCT) lParam);
			break;
		
		case WM_SIZE:
			childView->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
			break;
		
		case WM_COMMAND:
			childView->OnCommand(wParam, lParam);
			break;
		
		case WM_DESTROY:
			childView->OnDestroy();
			break;
		
		case WM_CLOSE:
			if (childView->OnClose()){
				MdiClient.SendMessage(WM_MDIDESTROY,(WPARAM) hwndChild, 0);
			}
			return true;

		case WM_NOTIFY:
			childView->OnNotify((int) wParam, (LPNMHDR) lParam);
			break;
		
		case WM_SETFOCUS:
			childView->OnSetFocus((HWND) wParam);
			break;
		
		case WM_MDIACTIVATE:
			childView->OnActivate((HWND) wParam, (HWND) lParam);
			break;
	}
return DefMDIChildProc(hwndChild, Message, wParam, lParam);
}


/********************************************************************
*	Class:	CChildView.
*
*	Purpose:	MDI child window class.
*
*	Revisions:	
*
********************************************************************/
CChildView::CChildView(){
	modified = false;
}

CChildView::~CChildView(){
}

bool CChildView::OnCreate(LPCREATESTRUCT){
	CFileItem * file = (CFileItem *) GetLong(GWL_USERDATA);

	// Create Scintilla Editor Control.
	HWND hwnd = Editor.CreateEx(
		this, 
		0,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
		| WS_VSCROLL | WS_HSCROLL);

	if (!hwnd)
		return false; // @@TODO need to manage creation failure.

	// Set window handles
	file->pMdiChild = this;

	// Load a file if there is one to load.
	Editor.LoadFile(file);
return true;
}

bool CChildView::OnSize(UINT wParam, int width, int height){
	if(wParam != SIZE_MINIMIZED){
		Editor.SetPosition(0, 
					0, 
					0, 
					width, 
					height,
		0);
	}
return true;
}

BOOL CChildView::OnClose(void){
	if (modified){
		int decision = MsgBox.AskToSave(true);
		switch (decision){
			case IDCANCEL:
			return FALSE;

			case IDYES:
			CmdSave();
			break;
		}
	}
return TRUE;
}

BOOL CChildView::OnDestroy(void){
	CFileItem * file = (CFileItem*) GetLong(GWL_USERDATA);

	if (file){
		if (!file->isInProject){
			// A simple file.
			winApp.Manager.FilesView.CloseFile(file);
		}else{
			// A project one.
			file->pMdiChild 	= NULL;
			//modified 		= false;
		}
	}
return 0;
}

BOOL CChildView::OnNotify(int, LPNMHDR notify){
	SCNotification * notification = (SCNotification *) notify;

	// Notify Message from Current Editor Control.
	if (notify->hwndFrom == Editor._hWnd){
		switch (notify->code){
			case SCN_UPDATEUI:
			Editor.GetCurrentPos();
			winApp.SendCaretPos(Editor.caretPos);
			break;
	
			case SCN_SAVEPOINTREACHED:
			modified = false;
			winApp.Sbar.WriteString("", 1);
			break;
		
			case SCN_SAVEPOINTLEFT:
			modified = true;
			winApp.Sbar.WriteString(MSG_MODIFIED, 1);
			break;

			case SCN_MARGINCLICK:
			if (notification->margin == 2)
				Editor.MarginClick(notification->position, notification->modifiers);
			break;
		}
	}
return 0;
}

BOOL CChildView::OnSetFocus(HWND){
	CFileItem * file = (CFileItem*) GetLong(GWL_USERDATA);
	if (!file)
		return false;
	// Select corresponding TreeView item.
	CTreeView * pTreeView = file->pTreeView;

	if(!pTreeView)
		return false;

	pTreeView->SendMessage(TVM_SELECTITEM, (WPARAM)TVGN_CARET, (LPARAM)file->_hItem);

	// Set Focus on Editor Control.
	Editor.SetFocus();

	// Display "Modified" message or nothing in the Status Bar.
	winApp.SendCaretPos(Editor.caretPos);

	if(modified)
		winApp.Sbar.SendMessage(SB_SETTEXT, 1, (LPARAM) MSG_MODIFIED);
	else
		winApp.Sbar.SendMessage(SB_SETTEXT, 1, (LPARAM) "");

	int selectedTab = winApp.Manager.SendMessage(TCM_GETCURSEL);

	if (file->isInProject == true && selectedTab != PROJECT_TAB){
		winApp.Manager.SendMessage(TCM_SETCURFOCUS, PROJECT_TAB);
	}else if (file->isInProject == false && selectedTab != FILES_TAB){
		winApp.Manager.SendMessage(TCM_SETCURFOCUS, FILES_TAB);
	}
return 0;
}

BOOL CChildView::OnActivate(HWND, HWND hwndChildAct){
	HMENU hMenu;
	HMENU hFileMenu;
	BOOL EnableFlag;
	HWND hwndMain = winApp._hWnd;

	hMenu = GetMenu(hwndMain);

	if(_hWnd == hwndChildAct){
		EnableFlag = TRUE;    //being activated
	}else{
		EnableFlag = FALSE;   //being de-activated
	}
	// Menu items.
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | (EnableFlag ? MF_ENABLED : MF_GRAYED));

	// Sub-menu items.
	hFileMenu = GetSubMenu(hMenu, 0);
	EnableMenuItem(hFileMenu, IDM_SAVE, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hFileMenu, IDM_SAVEAS, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));
	hFileMenu = GetSubMenu(hMenu, 2);
	EnableMenuItem(hFileMenu, IDM_FIND, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hFileMenu, IDM_REPLACE, MF_BYCOMMAND | (EnableFlag ? MF_ENABLED : MF_GRAYED));
	DrawMenuBar(hwndMain); 
return 0;
}

void CChildView::CmdSave(void){
	CFileItem * file = (CFileItem*) GetLong(GWL_USERDATA);

	if (!file)
		return;
	/* Untitled file ? */
	if (file->nFileOffset == 0){
		CmdSaveAs();
		return;
	}

	if (!file->isInProject){
		// A simple file.
		Editor.SaveFile(file->szFileName);
	}else{
		// A project one.
		Project.szDirBuffer[Project.nFileOffset - 1] = '\\';
		strcpy(&Project.szDirBuffer[Project.nFileOffset], file->szFileName);
		Editor.SaveFile(Project.szDirBuffer);
		Project.szDirBuffer[Project.nFileOffset - 1] = '\0';
	}
}

void CChildView::CmdSaveAs(void){
	CFileItem * file = (CFileItem*) GetLong(GWL_USERDATA);
	if (!file)
		return;

	char fileName[MAX_PATH];
	if (!winApp.FileDlg.Save(&winApp, fileName, MAX_PATH, SRC_FILE)) //@@ 
		     return;  // canceled by user

	::SetWindowText(_hWnd, fileName);	
	strcpy(file->szFileName, fileName);

	Editor.SaveFile(file->szFileName);
	//@@ TODO we need to check for errors
}

BOOL CChildView::OnCommand(WPARAM wParam, LPARAM){
	CFileItem * file = (CFileItem*) GetLong(GWL_USERDATA);

	if(!file)
		return false;

	switch (LOWORD(wParam)){
		case IDM_SAVE:
			CmdSave();
			break;
		
		case IDM_SAVEAS:
			CmdSaveAs();
			break;

		case IDM_SAVEALL:
			winApp.Manager.SaveAll(IDYES); // Silent.
			break;

/*		case IDM_CLOSE:
			PostMessage(pWnd, WM_CLOSE, 0, 0);
			break;
*/
		// To Scintilla control.
		case IDM_FIND:
			EditorDlg.Find(&Editor);
			break;

		case IDM_REPLACE:
			EditorDlg.Replace(&Editor);
			break;

		case IDM_CUT:
			Editor.SendMessage(SCI_CUT);
			break;
		case IDM_COPY:
			Editor.SendMessage(SCI_COPY);
			break;
		case IDM_PASTE:
			Editor.SendMessage(SCI_PASTE);
			break;
		case IDM_UNDO:
			Editor.SendMessage(SCI_UNDO);
			break;
		case IDM_REDO:
			Editor.SendMessage(SCI_REDO);
			break;
		case IDM_SELECTALL:
			Editor.SendMessage(SCI_SELECTALL);
			break;
	}
return TRUE;
}


/********************************************************************
*	Class:	CManager.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CManager::CManager(){
}

CManager::~CManager(){
}
	
void CManager::OpenFileDialog(void){
	CFileItem * file = new CFileItem;

	/* Show the "Open file" dialog */
	winApp.FileDlg.Reset();
	winApp.FileDlg.SetInitialDir(winApp.openFilesDir);

	if(!winApp.FileDlg.Open(&winApp, file->szFileName, MAX_PATH, SRC_FILE)){
		delete file;
		return;  // canceled by user
	}

	/* Get file information */
	file->nFileExtension = winApp.FileDlg.GetFileExtension();
	file->nFileOffset = winApp.FileDlg.GetFileOffset();
	GetFileType(file);
	// Copy file directory.
	strncpy(winApp.openFilesDir, file->szFileName, (file->nFileOffset - 1));
	winApp.openFilesDir[file->nFileOffset-1] = '\0';

	/* Load the file */
	if(!OpenFile(file)){
		delete file;
		MsgBox.DisplayString("This file is already opened.");
	}
}

bool CManager::OpenFile(CFileItem * file){
	if (!file)
		return false;

	if (!file->isInProject){
		if (!FilesView.OpenFile(file))
			return false;
	}else{
		if (!ProjectView.OpenFile(file))
			return false;
	}
return true;
}

bool CManager::NewProjectDialog(void){
	if(IDYES == Project.CloseDecision())
		CloseProject();

	char fileName[MAX_PATH];
	WORD fileOffset;
	*fileName = '\0';


	if (!winApp.FileDlg.Save(&winApp, fileName, MAX_PATH, PRJ_FILE)){
		return false;  // canceled by user
	}
	// Copy prj file's directory.
	fileOffset = winApp.FileDlg.GetFileOffset();

	ProjectView.CreateRoot("Project");
	if (!Project.New(fileName, fileOffset)){
		ProjectView.DestroyRoot();
		ProjectView.DestroyList();
		return false;
	}
return true;
}

bool CManager::OpenProjectDialog(void){
	if(IDYES == Project.CloseDecision())
		CloseProject();

	char fileName[MAX_PATH];
	WORD offset;
	*fileName = '\0';

	// Load default values.
	winApp.FileDlg.Reset();
	winApp.FileDlg.SetInitialDir(winApp.projectDir);

	if (!winApp.FileDlg.Open(&winApp, fileName, MAX_PATH, PRJ_FILE)){
		return false;  // canceled by user
	}
	// Copy project file's directory.
	offset = winApp.FileDlg.GetFileOffset();

	// Initialize project tree view.
	ProjectView.CreateRoot(fileName+offset);

	if (!Project.Open(fileName, offset)){
		ProjectView.DestroyRoot();
		ProjectView.DestroyList();
		return false;
	}
return true;
}

bool CManager::CloseProject(void){
return ProjectView.Close();
}

void CManager::RemoveProjectFile(void){
	ProjectView.RemoveFile();
}

void CManager::RemoveProjectModule(void){
	ProjectView.RemoveModule();
}

int CManager::SaveProjectFiles(int decision){
return ProjectView.SaveAll(decision);
}
	
int CManager::SaveAll(int decision){
	/* Save open files ? */
	decision = FilesView.SaveAll(decision);
	/* Save project files ? */
	decision = ProjectView.SaveAll(decision);
return decision;
}
	
void CManager::CreateImageList(void){ 
	// Create an empty image list.
	ImgList.Create(16, 16, ILC_COLORDDB|ILC_MASK, 8, 1);
	
	// Load treeview bmp and add it to the image list.
	CBitmap	tvBitmap;
	tvBitmap.Load(this, IDB_TREEVIEW);
	ImgList.AddMasked(&tvBitmap, RGB(255,0,255));
	
	// We no longer need treeview bmp.
	tvBitmap.Destroy();
}

void CManager::Create(CWindow * pParent){ 
	// Create the Tab Control.
	CreateEx(
		pParent, 
		WS_EX_CLIENTEDGE,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
			| TCS_BOTTOM | TCS_FOCUSNEVER);

	// Give it the default font, create tabs, select first one and show the control.
	SendMessage(WM_SETFONT, (long) GetStockObject(DEFAULT_GUI_FONT), 0);

	InsertItem(FILES_TAB, TCIF_TEXT, 0, 0, "Files", 16, 0, 0);
	InsertItem(PROJECT_TAB, TCIF_TEXT, 0, 0, "Project", 16, 0, 0);
	Show();

	// Create an Image list and then the Project TreeView.
	CreateImageList();
	ProjectView.Create(this, &ImgList);
	SetItem_Param(PROJECT_TAB, (long) &ProjectView);
	FilesView.Create(this, &ImgList);
	SetItem_Param(FILES_TAB, (long) &FilesView);
}

bool CManager::SetPosition(HWND, int x, int y, int width, int height, UINT){
	/* Overwrites CTabCtrl::SetPosition() so that all child windows are also resized */

	// Tab Control.
	::SetWindowPos(_hWnd, 0, 
				x, 
				y, 
				width, 
				height,
	0);
	// Child windows.
	RECT Rect;
	::GetClientRect(_hWnd, &Rect);
	ProjectView.SetPosition(0, Rect.top +5, Rect.left +5, 
		Rect.right-10, Rect.bottom-30, 0);

	FilesView.SetPosition(0, Rect.top +5, Rect.left +5, 
		Rect.right-10, Rect.bottom-30, 0);
return true;
}

BOOL CManager::OnNotify(int, LPNMHDR notify){
	// Dispatch messages.
	switch (notify->code){
		// Tab Control.
		case TCN_SELCHANGING:
			OnSelChanging(notify);
		break;

		case TCN_SELCHANGE:
			OnSelChange(notify);
		break;

		// TreeView.
		case TVN_SELCHANGED:
			Tv_OnSelchanged((LPNMTREEVIEW) notify);
		break;
	}
return TRUE;
}

void CManager::OnSelChanging(LPNMHDR notify){
	if (_hWnd == notify->hwndFrom){
		CTreeView * pTreeView = (CTreeView *) GetItem_Param(GetCurSel());
		if (pTreeView){
			pTreeView->Hide();
		}
	}
}

void CManager::OnSelChange(LPNMHDR notify){
	if (_hWnd == notify->hwndFrom){
		CTreeView * pTreeView = (CTreeView *) GetItem_Param(GetCurSel());
		if (pTreeView){
			pTreeView->Show();
		}
	}
}

void CManager::Tv_OnSelchanged(LPNMTREEVIEW notify){
	// Get lParam of current tree item.
	CFileItem * file = (CFileItem *) notify->itemNew.lParam;

	if (file){
		CChildView * pMdiChild = (CChildView *) file->pMdiChild;

		if(pMdiChild){
			// An editor, focus it.
			::SetFocus((HWND) pMdiChild->_hWnd);
		}else{
			// No editor, the item is part of a project.
			Project.SwitchCurrentDir();
			winApp.CreateChild(file->szFileName, file);
		}
	}
} 
 

/********************************************************************
*	Class:	CFilesView.
*
*	Purpose:	Open files TreeView.
*
*	Revisions:	
*
********************************************************************/
CFilesView::CFilesView(){
	hRoot = NULL;
}

CFilesView::~CFilesView(){
}

void CFilesView::New(void){
	CFileItem * file = new CFileItem;
	if(!winApp.Manager.OpenFile(file)){
		delete file;
		MsgBox.DisplayString("Untitled file already exist.");
	}
}

bool CFilesView::OpenFile(CFileItem * file){
	if (!file)
		return false;

	int listAction;
	char * fileName = file->szFileName + file->nFileOffset;

	/* Untitled file ? */
	if (file->nFileOffset == 0){
		//@@TODO add a counter to get Untitled01, 02, etc...
		strcpy(file->szFileName, "Untitled");
	}
	
	/* Check if this file is already opened */
	listAction = InsertSorted_New(file);

	if (listAction == FILE_FOUND){
		/* Focus the editor window */
		CFileItem * currentFile = (CFileItem *) GetCurrent();
		if (currentFile){
			CMDIChild * pMdiChild = currentFile->pMdiChild;
			if (pMdiChild)
				pMdiChild->SetFocus();
		}
		return false;
	}

	/* Create the editor window */
	if (!winApp.CreateChild(file->szFileName, file)){
		MsgBox.DisplayFatal("Can't create child window");
		return false;
	}
	// Note: A WM_SETFOCUS message will be send to the child window.

	/* Append the file to the list */
	InsertLast(file);

	/* Create a Tree View item */
	file->_hItem = CreateItem(
		hRoot, //@@ use a GetRootItem() ?
		TVI_LAST,
		file->type,
		fileName,
		(LPARAM) file);

	file->pTreeView = this;
return true;
}

void CFilesView::CloseFile(CFileItem * file){
	SendMessage(TVM_DELETEITEM, 0, (LPARAM) file->_hItem);		
	Destroy(file);
}

int CFilesView::SaveAll(int decision){
	if (decision == IDNO || decision == IDCANCEL)
		return decision;
	CFileItem * file = (CFileItem*) First();
	while (file){	
		if (file->pMdiChild){
			CChildView * childView = (CChildView *) file->pMdiChild;
			/* Modified ? */
			if (childView->modified){
				/* Ask ? */
				if (decision == IDASK){
					decision = MsgBox.AskToSave(true); // Cancel button.
					if (decision != IDYES)
						return decision; // IDNO or IDCANCEL.
				}
				childView->CmdSave();
			}
		}
		file = (CFileItem*) Next();
	}
return decision;
}

HWND CFilesView::Create(CWindow * pParent, CImageList * imgList){
	// Create TreeView.
	CreateEx(
		pParent, 
		WS_EX_CLIENTEDGE,
		WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
			| TVS_EDITLABELS | TVS_HASLINES | TVS_HASBUTTONS,
		0,
		(void*) 12);

	// Assign the image list to the treeview control.
	SendMessage(TVM_SETIMAGELIST, (long)TVSIL_NORMAL, (long) imgList->GetId());
	hRoot = CreateItem(TVI_ROOT, TVI_LAST, WORKSPACE, "Workspace", 0);
return _hWnd;
}


/********************************************************************
*	Class:	CProjectView.
*
*	Purpose:	Project TreeView.
*
*	Revisions:	
*
********************************************************************/
CProjectView::CProjectView(){
	hRoot = NULL;
}

CProjectView::~CProjectView(){
}

CFileItem * CProjectView::NewFile(char * name){
	CFileItem * current = (CFileItem *) GetCurrent();
	CFileItem * srcFile = new CFileItem;

	// Copy directory name.
	if (current){
		strncpy(srcFile->szFileName, current->szFileName, current->nFileOffset);
		srcFile->nFileOffset = current->nFileOffset;
	}else{
		// No files in the project yet, use makefile directory.
		strcpy(srcFile->szFileName, Project.Makefile.mkfDir);
		srcFile->nFileOffset = strlen(srcFile->szFileName)+1;
	}

	srcFile->szFileName[srcFile->nFileOffset - 1] = '\\';

	// Append file name.
	strcpy(&srcFile->szFileName[srcFile->nFileOffset], name);
	//MsgBox.DisplayString(srcFile->szFileName);

	if (::CheckFile(srcFile)){
		::GetFileType(srcFile);
		srcFile->isInProject = true;
		HANDLE hFile;      
		hFile = ::CreateFile(srcFile->szFileName, 
					0, 
					0, 
					NULL,
					CREATE_NEW, 
					FILE_ATTRIBUTE_ARCHIVE,
					NULL);
	
		if (hFile != INVALID_HANDLE_VALUE){
			CloseHandle(hFile);
			// Try to add new file to the project.
			if(OpenFile(srcFile)){
				Project.modified = true;
				return srcFile;
			}
		}
	}
	delete srcFile;
return NULL;
}

bool CProjectView::OpenFile(CFileItem * file){
	if (!file)
		return false;

	int listAction;
	char * fileName = file->szFileName + file->nFileOffset;

	/* Check if this file is already opened */
	listAction = InsertSorted_New(file);

	if (listAction == FILE_FOUND){
		/* Focus the editor window */
		CFileItem * currentFile = (CFileItem *) GetCurrent();
		if (currentFile){
			CMDIChild * pMdiChild = currentFile->pMdiChild;
			if (!pMdiChild){
				/* Create a child window */
				Project.SwitchCurrentDir();
				winApp.CreateChild(file->szFileName, file);
			}else{
				pMdiChild->SetFocus();
			}
		}
		return false;
	}

	file->_hDirItem = hRoot;
	CreateSubDirItem(file);

	if (listAction == EMPTY_LIST){
		InsertFirst(file);
	}else if (listAction == INSERT_FIRST){
		InsertFirst(file);
	}else if (listAction == INSERT_LAST){
		InsertLast(file);
	}else if (listAction == INSERT_BEFORE){
		InsertBefore(file);
	}else if (listAction == INSERT_AFTER){
		InsertAfter(file);
	}

	/* Create the file icon */
	file->_hItem = CreateItem(
		file->_hDirItem, 
		TVI_SORT, 
		file->type, 
		fileName,
		(LPARAM) file);

	file->pTreeView = this;

	/* Create an editor view */
	if (file->show){
		winApp.CreateChild(file->szFileName, file);
	}
return true;
}

bool CProjectView::Close(){
	if (Project.NoProject())
		return false;

	int decision = IDASK;
	decision = SaveAll(decision);
	if (decision == IDCANCEL)
		return false;

	// Parse the list while there's a next node.
	CFileItem * srcFile = (CFileItem *) First();
	while(srcFile){
		DestroyFile(srcFile, decision);
		srcFile = (CFileItem *) Next();
	}
	Project.loaded = false;

	DestroyRoot();
	DestroyList();
	winApp.Report.Clear();
return true;
}

void CProjectView::RemoveFile(void){
	if (Project.NoProject())
		return;

	CFileItem * srcFile = (CFileItem *) GetSelectedItemParam();
	
	if (srcFile){
		if (srcFile->pMdiChild)
			DestroyFile(srcFile);
		TreeView_DeleteItem(_hWnd, srcFile->_hItem);		
		if (!TreeView_GetChild(_hWnd, srcFile->_hDirItem))
			TreeView_DeleteItem(_hWnd, srcFile->_hDirItem);		
/*		else
			TreeView_SelectItem(_hWnd, srcFile->_hDirItem);*/
		Destroy(srcFile);

		// we need to save prj file before exit.
		//@@ Project.CloseFile, modified & buildMakefile should be private.
		Project.modified = true;
		Project.buildMakefile = true;
	}else{
		MsgBox.DisplayWarning("No project file selected");
	}
}

void CProjectView::RemoveModule(void){
	if (Project.NoProject())
		return;

	CFileItem * srcFile = (CFileItem *) GetSelectedItemParam();
	CFileItem * otherFile;
	
	if (srcFile){
		if (srcFile->prev){
			otherFile = (CFileItem *) srcFile->prev;
			if (otherFile->nFileExtension != 0){
				if (0 == strnicmp(srcFile->szFileName, otherFile->szFileName, otherFile->nFileExtension)){
					if (otherFile->pMdiChild)
						DestroyFile(otherFile);
					TreeView_DeleteItem(_hWnd, otherFile->_hItem);		
					Destroy(otherFile);
				}
			}
		}
		if (srcFile->next){
			otherFile = (CFileItem *) srcFile->next;
			if (otherFile->nFileExtension != 0){
				if (0 == strnicmp(srcFile->szFileName, otherFile->szFileName, otherFile->nFileExtension)){
					if (otherFile->pMdiChild)
						DestroyFile(otherFile);
					TreeView_DeleteItem(_hWnd, otherFile->_hItem);		
					Destroy(otherFile);
				}
			}
		}
		if (srcFile->pMdiChild)
			DestroyFile(srcFile);
		TreeView_DeleteItem(_hWnd, srcFile->_hItem);		
		Destroy(srcFile);

		// we need to save prj file before exit.
		//@@ Project.CloseFile, modified & buildMakefile should be private.
		Project.modified = true;
		Project.buildMakefile = true;
	}else{
		MsgBox.DisplayWarning("No project file selected");
	}
}

int CProjectView::DestroyFile(CFileItem * file, int decision){
	if (file && file->pMdiChild){
		CChildView * pMdiChild = (CChildView *) file->pMdiChild;
	
		if (pMdiChild->modified && decision != IDNO){
			// Ask ?
			if (decision == IDASK){
				decision = MsgBox.AskToSave(true); // (Cancel button)
				if (decision == IDCANCEL) 
					return decision;
			}
			pMdiChild->CmdSave();
		}
	
		if (pMdiChild->_hWnd)	// have an editor window, so destroy it.
			winApp.MdiClient.SendMessage(WM_MDIDESTROY, (WPARAM)pMdiChild->_hWnd, 0);
	}
return decision;
}

int CProjectView::SaveAll(int decision){
	if (!Project.loaded)
		return 0;

	if (decision == IDNO || decision == IDCANCEL)
		return decision;

	CFileItem * file = (CFileItem*) First();
	while (file){	
		if (file->pMdiChild){
			CChildView * childView = (CChildView *) file->pMdiChild;
			/* Modified ? */
			if (childView->modified){
				/* Ask ? */
				if (decision == IDASK){
					decision = MsgBox.AskToSave(true); // Cancel button.
					if (decision != IDYES)
						return decision; // IDNO or IDCANCEL.
				}
				childView->CmdSave();
			}
		}
		file = (CFileItem*) Next();
	}

	if (Project.modified)
		return Project.SavePrjFile(decision);
return decision;
}

bool CProjectView::CreateSubDirItem(CFileItem * file){
	/* Initialize _hDirItem and get a pointer to current file */
	file->_hDirItem = hRoot;
	CFileItem * currentFile = (CFileItem *) GetCurrent();

	/* See if our new file is in the same directory than current file */
	if (currentFile){
		// There's some files in the list.
		if (file->nFileOffset == currentFile->nFileOffset){
			// Same directory length, we may have found the directory.
			if (0 == strnicmp(file->szFileName, currentFile->szFileName, currentFile->nFileOffset)){
				/* We have found the directory, then copy _hDirItem */
				file->_hDirItem = currentFile->_hDirItem;
				return true;
			}
		}
	}

	/* We need to parse the tree view and create directory icons */
	char * parse = file->szFileName;
	if (*parse == '.' && *(parse+1) == '\\'){
		/* This is a valid relative path */
		char dir[MAX_PATH];
		strcpy(dir, file->szFileName);
		parse = dir+2;
		char * dirStart;
		HTREEITEM hParent = hRoot;
		HTREEITEM hFound;
		if (*parse){
			for ( ; ; ){
				/* Found each backslash */
				dirStart = parse;
				parse = strchr(parse, '\\');
				if (!parse)
					break; // No more backslash.
				else if (parse == dirStart)
					return false; // Avoids an endless loop.
				*parse = '\0';

				/* Find the directory */
				hFound = FindDirItem(hParent, dirStart);
				if (!hFound){
					/* Append a new directory icon */
					hParent = CreateDirItem(hParent, dirStart);
				}
				parse++;
			}
		}
		file->_hDirItem = hParent;
	}
return true;
}

HTREEITEM CProjectView::FindDirItem(HTREEITEM hItem, char * dir){
	char buffer[_MAX_DIR];
	HTREEITEM hNext = TreeView_GetChild(_hWnd, hItem);
	while (hNext){
		_TvItem.hItem = hNext;
		_TvItem.mask = TVIF_HANDLE | TVIF_TEXT;
		_TvItem.pszText = buffer;
		_TvItem.cchTextMax = _MAX_DIR;
		if (TreeView_GetItem(_hWnd, &_TvItem)){
			if (!stricmp(dir, buffer))
				return hNext;
		}
		hNext = TreeView_GetNextSibling(_hWnd, hNext);
	}
return NULL;
}

HWND CProjectView::Create(CWindow * pParent, CImageList * imgList){
	// Create TreeView.
	CreateEx(
		pParent, 
		WS_EX_CLIENTEDGE,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
		| TVS_EDITLABELS | TVS_HASLINES | TVS_HASBUTTONS);

	// Assign the image list to the treeview control.
	SendMessage(TVM_SETIMAGELIST, (long)TVSIL_NORMAL, (long) imgList->GetId());
return _hWnd;	
}

void CProjectView::CreateRoot(char * projectName){

	_pParent->SendMessage(TCM_SETCURFOCUS, 1, 0);

	// Create Root Item.
	hRoot = CreateItem(TVI_ROOT, TVI_LAST, PROJECT, projectName, 0);
	SendMessage(TVM_EXPAND, (long) TVE_EXPAND, (long) hRoot);
}

void CProjectView::DestroyRoot(void){
	TreeView_DeleteItem(_hWnd, hRoot);		
	hRoot = 0;

	_pParent->SendMessage(TCM_SETCURFOCUS, 0, 0);
}

HTREEITEM CProjectView::CreateDirItem(HTREEITEM hParent, char * dir){
return CreateItem(hParent, TVI_SORT, DIR, dir, 0);
}

CFileItem * CProjectView::FindFile(char * szFileName){
	if (!szFileName || !*szFileName)
		return NULL;

	char * currentFile;
	bool stripDir = true;
	if (*szFileName == '.')
		stripDir = false;
	// Get the current node.
	CFileItem * currentNode = (CFileItem *) GetCurrent();

	if(!currentNode)
		return NULL; // The list is empty.

	currentFile = GetFileName(currentNode, stripDir);
	int cmpResult = stricmp(szFileName, currentFile);
	// Compare names to know if we must parse Up 
	// or Down from current node.
	if (cmpResult == 0){
		return currentNode; // Found !
	}
	// Search Up -----------------------------------------------------------------
	else if (cmpResult == -1){
		// Parse the list while there's a previous node.
		while (Prev()){
			currentNode = (CFileItem *) GetCurrent();
			currentFile = GetFileName(currentNode, stripDir);
			if(!stricmp(szFileName, currentFile))
				return currentNode; // Found !
		}
	}
	// Search Down --------------------------------------------------------------
	else if (cmpResult == 1){
		// Parse the list while there's a next node.
		while (Next()){
			currentNode = (CFileItem *) GetCurrent();
			currentFile = GetFileName(currentNode, stripDir);
			if(!stricmp(szFileName, currentFile))
				return currentNode; // Found !
		}
	}
return NULL;
}

char * CProjectView::GetFileName(CFileItem * currentNode, bool flag){
	char * fileName = currentNode->szFileName;
	if (flag == true){
		fileName += currentNode->nFileOffset;
	} 
return fileName;
}


/********************************************************************
*	Class:	CReport.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CReport::CReport(){
}

CReport::~CReport(){
}
	
void CReport::Create(CWindow * pParent){ 
	// Create the Tab Control.
	CreateEx(
		pParent, 
		WS_EX_CLIENTEDGE,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
			/*| TCS_BOTTOM*/ | TCS_MULTILINE | TCS_VERTICAL
			| TCS_FOCUSNEVER);

	// Give it a smart font, create tabs, select first one and show the control.
	SendMessage(WM_SETFONT, (long) GetStockObject(DEFAULT_GUI_FONT), 0);

	InsertItem(REPORT_MAIN_TAB, TCIF_TEXT, 0, 0, "Main", 16, 0, 0);
	InsertItem(REPORT_LOG_TAB, TCIF_TEXT, 0, 0, "Log", 16, 0, 0);
	Show();

	// Create an Image list and then the Project TreeView.
	MainList.Create(this);
	SetItem_Param(REPORT_MAIN_TAB, (long) &MainList);
	LogList.Create(this);
	SetItem_Param(REPORT_LOG_TAB, (long) &LogList);
}

bool CReport::SetPosition(HWND, int x, int y, int width, int height, UINT){
	/* Overwrites CTabCtrl::SetPosition() so that all child windows are also resized */

	// Tab Control.
	::SetWindowPos(_hWnd, 0, 
				x, 
				y, 
				width, 
				height,
	0);

	// Get tab's display area.
	RECT area;
	area.left = 0;
	area.top = 0;
	area.right = width;
	area.bottom = height;
	::SendMessage(_hWnd, TCM_ADJUSTRECT, FALSE, (LPARAM) &area);
	area.right -= area.left;
	area.bottom -= area.top;
	/* WS_EX_CLIENTEDGE correction */
	area.top -= 2;
	area.right -= 2;
	// Borders.
	area.left += 3;
	area.top += 3;
	area.right -= 6;
	area.bottom -= 6;

	// Child windows.
	MainList.SetPosition(0, area.left, area.top,
		area.right, area.bottom, 0);
	LogList.SetPosition(0, area.left, area.top,
		area.right, area.bottom, 0);
return true;
}

BOOL CReport::OnNotify(int, LPNMHDR notify){
	// Dispatch messages.
	switch (notify->code){
		// Tab Control.
		case TCN_SELCHANGING:
			OnSelChanging(notify);
		break;

		case TCN_SELCHANGE:
			OnSelChange(notify);
		break;

		// Main list.
		case NM_DBLCLK:
			MainList.Lv_OnDbClick((LPNMLISTVIEW) notify);
		break;
	}
return TRUE;
}

void CReport::OnSelChanging(LPNMHDR notify){
	if (_hWnd == notify->hwndFrom){
		CWindow * pWindow = (CWindow *) GetItem_Param(GetCurSel());
		if (pWindow){
			pWindow->Hide();
		}
	}
}

void CReport::OnSelChange(LPNMHDR notify){
	if (_hWnd == notify->hwndFrom){
		CWindow * pWindow = (CWindow *) GetItem_Param(GetCurSel());
		if (pWindow){
			pWindow->Show();
		}
	}
}

void CReport::Clear(void){
	MainList.Clear();
	LogList.Clear();
}

bool CReport::Append(char * line, WORD outputFlag){
	LogList.Append(line, outputFlag);
	MainList.Append(line, outputFlag);
return true;
}


/********************************************************************
*	Class:	CMainList.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CMainList::CMainList(){
}

CMainList::~CMainList(){
}

void CMainList::Create(CWindow * pParent){
	CreateEx(
		pParent, 
		WS_EX_CLIENTEDGE,
		WS_VISIBLE | WS_CHILD| WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
		| LVS_REPORT);

	SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 
						LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
	SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE,
						LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	// Insert columns.
	LVCOLUMN	lvc;	
	lvc.mask		= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt		= LVCFMT_LEFT;
	
	lvc.iSubItem	= 0;
	lvc.cx		= 35;
	lvc.pszText		= "Line";
	SendMessage(LVM_INSERTCOLUMN, 0, (LPARAM) &lvc);

	lvc.iSubItem	= 1;
	lvc.cx		= 70;
	lvc.pszText		= "Unit";
	SendMessage(LVM_INSERTCOLUMN, 1, (LPARAM) &lvc);

	lvc.iSubItem	= 2;
	lvc.cx		= 640;
	lvc.pszText		= "Message";
	SendMessage(LVM_INSERTCOLUMN, 2, (LPARAM) &lvc);
}

void CMainList::Lv_OnDbClick(LPNMLISTVIEW lpnmlv){
	if (_hWnd == lpnmlv->hdr.hwndFrom){
		char lineBuff[256];
		*lineBuff = '\0';
		char * c = lineBuff;
		LV_ITEM		lvi;
		lvi.mask		= LVIF_TEXT;
		lvi.iItem		= lpnmlv->iItem;
		lvi.iSubItem		= 0;
		lvi.pszText		= lineBuff;
		lvi.cchTextMax	= 256;
		lvi.lParam		= 0;
		if (!SendMessage(LVM_GETITEMTEXT, lpnmlv->iItem, (long) &lvi))
			return;
		while(*c){
			if (!isdigit(*c))
				return;
			c++;
		}
		int line = atoi(lineBuff);

		//MsgBox.DisplayLong((long) line);

		lvi.iSubItem		= 1;
		if (!SendMessage(LVM_GETITEMTEXT, lpnmlv->iItem, (long) &lvi))
			return;
		CFileItem * item = winApp.Manager.ProjectView.FindFile(lineBuff);
		if (item && item->isInProject){
			CChildView * pMdiChild = (CChildView *) item->pMdiChild;
	
			if(pMdiChild){
				// An editor, focus it.
				::SetFocus((HWND) pMdiChild->_hWnd);
			}else{
				// No editor, the item is part of a project.
				Project.SwitchCurrentDir();
				winApp.CreateChild(item->szFileName, item);
			}
			pMdiChild = (CChildView *) item->pMdiChild;
			if (pMdiChild)
				pMdiChild->Editor.GotoLine(line-1);
		}
	}
}

bool CMainList::Append(char * line, WORD outputFlag){
	int	row;

	*szLine = '\0';
	*szUnit = '\0';
	*szMsg = '\0';

	if (outputFlag == LVOUT_ERROR){
		if (!SplitErrorLine(line))
			return false;
	}else if (outputFlag == LVOUT_NORMAL){
		strcpy (szMsg, line);
	}else{
		strcpy (szMsg, "Unrecognized outputFlag");
	}

	// Fill in List View columns, first is column 0.
	LV_ITEM		lvi;
	lvi.mask		= LVIF_TEXT; // | LVIF_PARAM;
	lvi.iItem		= 0x7FFF;
	lvi.iSubItem		= 0;
	lvi.pszText		= szLine;
	lvi.cchTextMax	= strlen(lvi.pszText)+1;
	lvi.lParam		= 0;

	row = SendMessage(LVM_INSERTITEM, 0, (LPARAM) &lvi);

	// Continue with column 1.
	lvi.iSubItem		= 1;
	lvi.pszText		= szUnit;
	lvi.cchTextMax	= strlen(lvi.pszText)+1;
	SendMessage(LVM_SETITEMTEXT, (WPARAM)row, (LPARAM)&lvi);

	// Continue with column 2.
	lvi.iSubItem		= 2;
	lvi.pszText		= szMsg;
	lvi.cchTextMax	= strlen(lvi.pszText)+1;
	SendMessage(LVM_SETITEMTEXT, (WPARAM)row, (LPARAM)&lvi);

	// Save last row position
	lastRow = row+1;

return true;
}

bool CMainList::SplitErrorLine(char * line){
	char * chr = line;
	char * col;
	// line => 	[unit]:[line_n°]: [error message]
	// or	 => 	[unit]: [error message]

	if (!*line)
		return false;

	/* Unit */
	col = szUnit;
	for ( ; ; ){
		if (!*chr){
			/* Not an error line */
			//strcpy(szMsg, szUnit);
			*szUnit = '\0';
			return false;
		}else if (*chr == ':'){
			if (*(chr+1) == '\\'){
				*col = *chr;
				col++;
				chr++;
				continue;
			}else{
				chr++;
				break;
			}
		}
		*col = *chr;
		col++;
		chr++;
	}
	*col = '\0';

	/* Line number ? */
	col = szLine;
	if (*chr && isdigit(*chr)){	//@@ *chr=0 ?
		while (*chr && *chr != ':'){
			*col = *chr;
			col++;
			chr++;
		}
		*col = '\0';
		chr++;
	}

	/* Message */
	col = szMsg;
	if (isspace(*chr)){
		/**col = '>';
		col++;
		*col = ' ';
		col++;*/
		chr++;
	}

	while (*chr){
		*col = *chr;
		col++;
		chr++;
	}
	*col = '\0';
return true;
}	


/********************************************************************
*	Class:	CLogList.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CLogList::CLogList(){
}

CLogList::~CLogList(){
}

void CLogList::Create(CWindow * pParent){
	CreateEx(
		pParent, 
		WS_EX_CLIENTEDGE,
		WS_CHILD| WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT 
		| LVS_NOCOLUMNHEADER);

	SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE,
						LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	// Insert columns.
	LVCOLUMN	lvc;	
	lvc.mask		= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt		= LVCFMT_LEFT;
	
	lvc.iSubItem	= 0;
	lvc.cx		= 100;
	lvc.pszText		= "Message";
	SendMessage(LVM_INSERTCOLUMN, 0, (LPARAM) &lvc);
}

bool CLogList::SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags){
	::SendMessage(_hWnd, WM_SETREDRAW, FALSE, 0);
	::SetWindowPos(_hWnd, hInsertAfter, x, y, width, height, uFlags);
	::SendMessage(_hWnd, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM((int) width-22, 0));
	::SendMessage(_hWnd, WM_SETREDRAW, TRUE, 0);
return true;
}

bool CLogList::Append(char * line, WORD /*outputFlag*/){
	int	row;

	*szMsg = '\0';

/*	if (outputFlag != LVOUT_ERROR)
		return false;
*/
	// Fill in List View columns, first is column 0.
	LV_ITEM		lvi;
	lvi.mask		= LVIF_TEXT; // | LVIF_PARAM;
	lvi.iItem		= 0x7FFF;
	lvi.iSubItem		= 0;
	lvi.pszText		= line;
	lvi.cchTextMax	= strlen(lvi.pszText)+1;
	lvi.lParam		= 0;

	row = SendMessage(LVM_INSERTITEM, 0, (LPARAM) &lvi);

	// Save last row position
	lastRow = row+1;

return true;
}


/********************************************************************
*	Class:	CFileItem.
*
*	Purpose:	Linked List Node for file parameters.
*
*	Revisions:	
*
********************************************************************/
CFileItem::CFileItem(){
	type 			= U_FILE;

	*szFileName 	= '\0'; 
	szFileName[MAX_PATH - 1] = '\0';	// security.
	nFileOffset		= 0;
	nFileExtension	= 0;

	pTreeView		= NULL; 
	_hDirItem		= 0;
	_hItem 		= 0;

	pMdiChild		= NULL;
	show 		= 0;
	isInProject		= false;
}

CFileItem::~CFileItem(){
}


/********************************************************************
*	Class:	CFileList.
*
*	Purpose:	A CList with a dedicated Compare() procedure.
*
*	Revisions:	
*
********************************************************************/
CFileList::CFileList(){
}

CFileList::~CFileList(){
}

int CFileList::Compare(CNode *node1, CNode *node2){
return stricmp(((CFileItem *)node1)->szFileName, ((CFileItem *)node2)->szFileName);
}

