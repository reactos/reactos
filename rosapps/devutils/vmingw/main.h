/********************************************************************
*	Module:	main.h. This is part of Visual-MinGW.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
********************************************************************/
#ifndef MAIN_H
#define MAIN_H

#include "CList.h"
#include "winui.h"
#include "editor.h"
#include "process.h"

#define LVOUT_NORMAL (STDOUT_USER)
#define LVOUT_ERROR (STDOUT_USER+1)
#define IDASK		21

#define WORKSPACE	0
#define PROJECT	1
#define DIR		2

#define PRJ_FILE		0
#define SRC_FILE		1
#define ADD_SRC_FILE	2

#define FILES_TAB 	0
#define PROJECT_TAB 	1

#define REPORT_MAIN_TAB 	0
#define REPORT_LOG_TAB 	1


class CChildView : public CMDIChild
{
	public:
	CChildView();
	virtual ~CChildView();

	bool	modified;

	bool OnCreate(LPCREATESTRUCT lParam);
	bool OnSize(UINT wParam, int width, int height);
	BOOL OnClose(void);
	BOOL OnDestroy(void);

	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	BOOL OnNotify(int idCtrl, LPNMHDR notify);
	BOOL OnSetFocus(HWND hwndLoseFocus);
	BOOL	OnActivate(HWND hwndChildDeact, HWND hwndChildAct);

	void CmdSave(void);
	void CmdSaveAs(void);
	CEditor Editor;

	protected:

	private:   
};

class CFileList : public CList
{
	public:
	CFileList();
	~CFileList();

	protected:
	virtual int Compare(CNode *node1, CNode *node2);

	private:
};

class CProjectView : public CTreeView, public CFileList
{
	public:
	CProjectView();
	~CProjectView();

	CFileItem * NewFile(char * name);
	bool OpenFile(CFileItem * file);
	void	RemoveFile(void);
	void	RemoveModule(void);
	int DestroyFile(CFileItem * file, int decision=IDASK);
	int	SaveAll(int decision);
	bool Close();

	void CreateRoot(char * projectName);
	void DestroyRoot(void);
	CFileItem * FindFile(char * szFileName);
	char * GetFileName(CFileItem * currentNode, bool flag);

	HWND Create(CWindow * pParent, CImageList * imgList);
	HTREEITEM CreateDirItem(HTREEITEM hParent, char * dir);

	protected:

	private:   
	bool CreateSubDirItem(CFileItem * file);
	HTREEITEM FindDirItem(HTREEITEM hItem, char * dir);
	HTREEITEM hRoot;
};

class CFilesView : public CTreeView, public CFileList
{
	public:
	CFilesView();
	~CFilesView();

	bool OpenFile(CFileItem * file);
	void	New (void);
	void	CloseFile(CFileItem * file);
	int	SaveAll(int decision);

	HWND Create(CWindow * pParent, CImageList * imgList);

	protected:

	private:   
	HTREEITEM hRoot;
};

class CManager : public CTabCtrl
{
	public:
	CManager();
	~CManager();

	void	OpenFileDialog(void);
	bool OpenFile(CFileItem * file);
	int SaveAll(int silent);

	bool NewProjectDialog(void);
	bool OpenProjectDialog(void);
	bool CloseProject(void);
	int SaveProjectFiles(int decision);
	void	RemoveProjectFile(void);
	void	RemoveProjectModule(void);

	void	Create(CWindow * pParent);
	bool	SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags);

	BOOL	OnNotify(int idCtrl, LPNMHDR notify);
	void	Tv_OnDeleteItem(LPNMTREEVIEW notify);
	void	Tv_OnSelchanged(LPNMTREEVIEW notify);
	void	OnSelChanging(LPNMHDR notify);
	void	OnSelChange(LPNMHDR notify);

	CImageList	ImgList;
	CFilesView		FilesView;
	CProjectView	ProjectView;

	protected:

	private:
	void CreateImageList(void);

};

class CLogList : public CListView
{
	public:
	CLogList();
	~CLogList();

	void	Create(CWindow * pParent);
	bool SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags);
	bool Append(char * line, WORD outputFlag);

	protected:

	private:   
	char szMsg[1024];
};

class CMainList : public CListView
{
	public:
	CMainList();
	~CMainList();

	void	Create(CWindow * pParent);
	void Lv_OnDbClick(LPNMLISTVIEW lpnmlv);
	bool Append(char * line, WORD outputFlag);

	protected:

	private:   
	char szLine[512];
	char szUnit[512];
	char szMsg[512];

	bool SplitErrorLine(char * line);
};

class CReport : public CTabCtrl
{
	public:
	CReport();
	~CReport();

	bool Append(char * line, WORD outputFlag);
	void Clear(void);
	void	Create(CWindow * pParent);
	bool	SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags);

	BOOL	OnNotify(int idCtrl, LPNMHDR notify);
	void	OnSelChanging(LPNMHDR notify);
	void	OnSelChange(LPNMHDR notify);

	CMainList	MainList;
	CLogList	LogList;

	protected:

	private:
	void CreateImageList(void);
};

class CFileDlg : public CFileDlgBase
{
	public:
	CFileDlg();
	~CFileDlg();

	bool	Open(CWindow * pWindow, char * pszFileName, DWORD nMaxFile, int fileflag);
	bool	Save(CWindow * pWindow, char * pszFileName, DWORD nMaxFile, int fileflag);

	protected:

	private:   
};

class CGrepDlg : public CDlgBase
{
	public:
	CGrepDlg();
	~CGrepDlg();

	int Create(void);
	LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
	void FindInFiles(char * findWhat, char * fileFilter);
	char gDir[MAX_PATH];
	char findWhat[200];

	protected:

	private:   
	HWND hFindWhat;
	HWND hgDir;
};

class CEnvDlg : public CDlgBase
{
	public:
	CEnvDlg();
	virtual ~CEnvDlg();

	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
	void	SetEnvText(void);
	bool	bIsVisible;
	bool	bModified;

	protected:

	private:   
	HWND hApply;
	HWND hSetCcBin;
	HWND hCcBinDir;
	HWND hBrowseCc;
	HWND hSetCmdBin;
	HWND hCmdBinDir;
	HWND hBrowseCmd;
	HWND hAutoexec;
	HWND hEnvView;
};

class CPreferencesDlg : public CTabbedDlg
{
	public:
	CPreferencesDlg();
	virtual ~CPreferencesDlg();

	int Create(void);
	BOOL EndDlg(int nResult);
	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	protected:

	private:   
	CEnvDlg	EnvDlg;
};

class CWinApp : public CMDIBase
{
	public:
	CWinApp();
	~CWinApp();

	void FirstRunTest(void);
	bool	ReadIniFile(char * iniFile);
	void SaveIniFile(FILE * file);
	bool	WriteIniFile(void);
	bool	CustomInit(void);
	bool	Release(void);
	bool	SetEnv(void);

	bool	CreateUI(void);
	void	CreateToolbar(void);
	void	CreateSplitter(void);
	void	CreateMDI(void);
	HWND CreateChild(char * caption, LPVOID lParam);
	void	CreateStatusBar(void);

	void	SendCaretPos(int caretPos);

	// Main window.
	LRESULT CALLBACK CMainWndProc(UINT Message, WPARAM wParam, LPARAM lParam);

	BOOL	OnCreate(LPCREATESTRUCT lParam);
	BOOL	OnPaint(HDC wParam);
	BOOL	OnSize(UINT wParam, int width, int height);
	BOOL	OnDestroy(void);
	BOOL	OnClose (void);
	BOOL	OnNotify(int idCtrl, LPNMHDR notify);

	BOOL	OnLButtonDown(short xPos, short yPos, UINT fwKeys);
	BOOL	OnMouseMove(short xPos, short yPos, UINT fwKeys);
	BOOL	OnLButtonUp(short xPos, short yPos, UINT fwKeys);
	BOOL	OnSetCursor(HWND hwnd, UINT nHittest, UINT wMouseMsg);

	BOOL	OnCommand(WPARAM wParam, LPARAM lParam);

	// Child window.
	LRESULT CALLBACK CChildWndProc(CWindow * pWnd, UINT Message, WPARAM wParam, LPARAM lParam);

	HMODULE 		hmod;
	char			iniFileName[MAX_PATH];
	CIniFile 		IniFile;
	CPreferencesDlg	PreferencesDlg;
	CGrepDlg		GrepDlg;
	CShellDlg		ShellDlg;
	CFileDlg 		FileDlg;
	CProcess		Process;

	CToolBar		Toolbar;
	CSplitter 		MainSplitter;
	CSplitter 		ChildSplitter;
	CManager		Manager;
	CReport		Report;
	CStatusBar		Sbar;

	/* Preferences */
	char			openFilesDir[MAX_PATH];
	char			projectDir[MAX_PATH];
	bool			bSetCcEnv;
	bool			bSetCmdEnv;
	bool			bSetDefEnv;
	char			szCcBinDir[MAX_PATH];
	char			szCmdBinDir[MAX_PATH];
	char			includeDir[MAX_PATH];

	protected:

	private:
	bool firstRun;
	// Child windows dimensions.
	int deltaY;
	int tbarHeight;
	int sbarHeight;
	int tvWidth;
	int lvHeight;
	
	int hSplitter;
	int vSplitter;
};

#endif
