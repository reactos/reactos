/********************************************************************
*	Module:	winui.h. This is part of WinUI.
*
*	License:	WinUI is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
********************************************************************/
#ifndef WINUI_H
#define WINUI_H

/* These are patched headers */
#include "commdlg.h"
#include "commctrl.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// OLE.
#include <shlobj.h>

#include "CList.h"

#define SPLSTYLE_HORZ 	0
#define SPLSTYLE_VERT 	1
#define SPLMODE_1		0
#define SPLMODE_2		1

char *StpCpy(char *dest, const char *src);
size_t strcpylen(char *dest, const char *src);

void SplitFileName(char * dirName, char * fileName);
bool ChangeFileExt(char * fileName, char * ext);

class CMessageBox
{
	public:
	CMessageBox();
	~CMessageBox();

	void SetParent(HWND hwnd);
	void	SetCaption(char * string);
	void	DisplayString(char * string, char * substring = NULL, UINT uType = MB_OK);
	void	DisplayWarning(char * string, char * substring = NULL);
	void	DisplayFatal(char * string, char * substring = NULL);
	void	DisplayLong(long number);
	void	DisplayRect(RECT * rect);
	int	Ask(char * question, bool canCancel);
	int	AskToSave(bool canCancel);

	protected:
	HWND		hParent;
	char 			msgBuf[256];
	char 			caption[64];

	private:   
};

class CPath
{
	public:
	CPath();
	~CPath();

	bool ChangeDirectory(char * dir);

	protected:

	private:   
	char pathBuf[MAX_PATH];
};

class CWindow
{
	public:
	CWindow();
	virtual ~CWindow();

	virtual HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, 
		LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, 
		int nWidth, int nHeight, HMENU hMenu, LPVOID lpParam);

	HWND GetId(void);
	LONG SetLong(int nIndex, LONG dwNewLong);
	LONG GetLong(int nIndex);
	LRESULT SendMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0);

	virtual bool SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags);
	virtual bool Show(int nCmdShow = SW_SHOWNORMAL);
	virtual bool Hide(void);
	HWND SetFocus(void);
	CWindow * _pParent;
	HWND 	_hWnd;
	HINSTANCE _hInst;
	LPVOID	_lParam;

	protected:

	private:   
};

class CWinBase : public CWindow
{
	public:
	CWinBase();
	~CWinBase();

	HINSTANCE 	hPrevInst;
	LPSTR 		lpCmdLine;
	int 			nCmdShow;

	char 			appName[64];
	char 			msgBuf[256];
	bool			isWinNT;

	bool	Init(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
												int nCmdShow);
	bool	SetName(char * name, char * version = NULL);
	void	ParseCmdLine(char * outBuff);
	bool	IsWinNT(void);

	bool ChangeDirectory(char * dir);

	protected:

	private:   
};

class CSDIBase : public CWinBase
{
	public:
	CSDIBase();
	virtual ~CSDIBase();

	virtual int	Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	virtual bool CustomInit(void);
	virtual bool Release(void);
	virtual bool CreateUI(void);
	bool MainRegisterEx(const char * className);
	virtual LRESULT CALLBACK CMainWndProc(UINT Message, WPARAM wParam, LPARAM lParam);

	protected:
	WNDCLASSEX	wc;
	HACCEL		hAccel;
	char			mainClass[16];

	private:   
};

class CMDIChild;

class CMDIClient : public CWindow
{
	public:
	CMDIClient();
	~CMDIClient();

	void Init(int menuIndex, UINT idFirstChild);
	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0);
	LPARAM GetParam(LPARAM lParam);

	bool 	initialized;
	char	childClass[16];
	CList	childList;

	protected:
	CLIENTCREATESTRUCT ccs;
	int 	nPos;

	private:   
};

class UseOle
{
	public:
	UseOle(){oleError = OleInitialize(NULL);}
	~UseOle(){OleUninitialize();}
	HRESULT GetError(void){return oleError;}
	protected:
	HRESULT oleError;
	private:   
};

class CMDIBase : public CSDIBase
{
	public:
	CMDIBase();
	virtual ~CMDIBase();

	virtual int	Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	virtual bool CustomInit(void);
	virtual bool Release(void);
	virtual bool CreateUI(void);
	bool ChildRegisterEx(const char * className);
	virtual LRESULT CALLBACK CMainWndProc(UINT Message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT CALLBACK CChildWndProc(CWindow * pWnd, UINT Message, WPARAM wParam, LPARAM lParam);

	CMDIClient	MdiClient;

	protected:
	UseOle useOle;

	private:   
};

class CMDIChild : public CNode, public CWindow
{
	public:
	CMDIChild();
	virtual ~CMDIChild();

	HWND CreateEx(CMDIClient * pMdiClient, DWORD dwExStyle, DWORD dwStyle, char * caption, UINT resId = 0, LPVOID lParam = NULL);

	CMDIBase * _pFrame;

	protected:

	private:   
};

class CFileDlgBase : public CWindow
{
	public:
	CFileDlgBase();
	~CFileDlgBase();

	void Reset(void);
	void SetData(char * filter, char * defExt, DWORD flags);
	void SetTitle(char * title);
	void SetFilterIndex(DWORD filterIndex);
	void SetFilter(char * filter);
	void SetDefExt(char * defExt);
	void SetFlags(DWORD flags);
	void SetInitialDir(char * lpstrInitialDir);

	WORD GetFileOffset(void);
	WORD GetFileExtension(void);
	WORD GetNextFileOffset(void);

	bool OpenFileName(CWindow * pWindow, char * pszFileName, DWORD nMaxFile);
	bool SaveFileName(CWindow * pWindow, char * pszFileName, DWORD nMaxFile);
	OPENFILENAME ofn;

	protected:

	WORD nNextFileOffset;
	private:   
};

class CDlgBase : public CWindow
{
	public:
	CDlgBase();
	virtual ~CDlgBase();

	HWND Create(CWindow * pWindow, WORD wResId, RECT * Pos, LPARAM lParam);
	int CreateModal(CWindow * pWindow, WORD wResId, LPARAM lParam);
	HWND CreateParam(CWindow * pWindow, WORD wResId, LPARAM lParam);
	BOOL EndDlg(int nResult);
	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	HWND GetItem(int nIDDlgItem);
	BOOL SetItemText(HWND hItem, LPCTSTR lpString);
 	UINT GetItemText(HWND hItem, LPTSTR lpString, int nMaxCount);
/*	BOOL SetItemInt(HWND hItem, UINT uValue, BOOL bSigned);
	UINT GetItemInt(HWND hItem, BOOL *lpTranslated, BOOL bSigned);
*/
	protected:

	private:   
};

class CTabbedDlg : public CDlgBase
{
	public:
	CTabbedDlg();
	virtual ~CTabbedDlg();

	virtual void OnNotify(int idCtrl, LPNMHDR notify);
	virtual void OnSelChanging(LPNMHDR notify);
	virtual void OnSelChange(LPNMHDR notify);
	virtual bool SetChildPosition(HWND hChild);

	protected:
	LPARAM GetParam(void);
	HWND _hWndTab;
	RECT Pos;
	TCITEM tcitem;

	private:   
};

class CToolBar : public CWindow
{
	public:
	CToolBar();
	~CToolBar();

	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0);
	LRESULT AddBitmap(UINT resId, int nBmp, HINSTANCE hInstance = 0);
	BOOL AddButtons(TBBUTTON * tbButtons, UINT numButtons);

	protected:

	private:   
};

class CBitmap
{
	public:
	CBitmap();
	~CBitmap();

	HBITMAP Load(CWindow * pWindow, LPCTSTR lpBitmapName);
	HBITMAP Load(CWindow * pWindow, WORD wResId);
	BOOL Destroy(void);

        HBITMAP hBitmap;

	protected:

	private:   
};

class CImageList
{
	public:
	CImageList();
	~CImageList();

	HIMAGELIST Create(int cx, int cy, UINT flags, int cInitial, int cGrow);
	int AddMasked(CBitmap * pBitmap, COLORREF crMask);
	HIMAGELIST GetId(void){return hImgList;};

	protected:
	HIMAGELIST hImgList;

	private:   
};

class CTabCtrl : public CWindow
{
	public:
	CTabCtrl();
	~CTabCtrl();

	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0, LPVOID lParam = NULL);
	BOOL SetItem_Param(int iItem, LPARAM lParam);
	LPARAM GetItem_Param(int iItem);
	int GetCurSel(void);
	int	InsertItem(int iItem, UINT mask, DWORD dwState, DWORD dwStateMask, 
				LPTSTR pszText, int cchTextMax, int iImage, LPARAM lParam);

	protected:
        TCITEM tcitem;

	private:   
};

class CTreeView : public CWindow
{
	public:
	CTreeView();
	~CTreeView();

	TVINSERTSTRUCT tvi; 
	TVITEM _TvItem;
	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0, LPVOID lParam = NULL);
	HTREEITEM CreateItem(HTREEITEM hParent, HTREEITEM hInsertAfter, int iImage, LPTSTR pszText, LPARAM lParam);
	LPARAM GetSelectedItemParam(void);

	protected:
	private:   
};

class CScintilla : public CWindow
{
	public:
	CScintilla();
	~CScintilla();

	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0, LPVOID lParam = NULL);

	protected:
	private:   
};

class CListView : public CWindow
{
	public:
	CListView();
	~CListView();

	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0);
	void Clear(void);

	protected:
	DWORD lastRow;

	private:   
};

class CStatusBar : public CWindow
{
	public:
	CStatusBar();
	~CStatusBar();

	HWND CreateEx(CWindow * pWindow, DWORD dwExStyle, DWORD dwStyle = WS_CHILD, UINT resId = 0);
	void SetParts(int nParts, int * aWidths);

	void WriteString(char * string, int part);
	void WriteLong(long number, int part);

	protected:

	private:   
	int numParts;
};

class CSplitter : public CWindow
{
	public:
	CSplitter();
	~CSplitter();

	void Init(CWindow * pane1, CWindow * pane2, bool vertical, 
				int barPos, int barMode);
	bool SetPosition(HWND hInsertAfter, int x, int y, int width, int height, UINT uFlags);
	bool Show(int nCmdShow = SW_SHOWNORMAL);
	bool Hide(void);
	bool HaveMouse(HWND hwnd, short xPos, short yPos);
	void Move(int mouseX, int mouseY);
	bool OnLButtonDown(HWND hwnd, short xPos, short yPos);
	void OnMouseMove(HWND hwnd, short xPos, short yPos);
	void OnLButtonUp(HWND hwnd, short xPos, short yPos);
	bool OnSetCursor(HWND hwnd, LPARAM lParam);

	protected:
	void SetVertPosition(void);
	void SetHorzPosition(void);
	void DrawXorBar(HDC hdc, int x1, int y1, int width, int height);
	void ClientToWindow(HWND hwnd, POINT * pt, RECT * rect);

	CWindow * Pane1;
	CWindow * Pane2;
	int 		p;
	int 		size;
	int 		psize;
	bool 		isVertical;
	int 		mode;
	bool		isActive;
	RECT		pos;
	RECT		barPos;
	int	 	deltaPos;
	POINT 	initialXorPos;

	POINT 	initialPos;
	POINT 	newPos;
	private:   
	bool 		initialized;
};

#define MAX_BLOC_SIZE 16384
class CIniFile
{
	public:
	CIniFile();
	~CIniFile();

	bool	Load(char * fullPath);
	void	Close(void);
	bool	GetString(char * data, char * key, char * section = NULL);
	int	GetInt(char * key, char * section = NULL);

	protected:

	private:   

	char * buffer;
	char * pcurrent;
	char * psection;

	bool FindSection(char * pcurrent, char * section);
	bool FindData(char * s, char * key, char * data);
	long CopyData(char * data, char * s);
	char *SkipUnwanted(char *s);
};

class CChrono
{
	public:
	CChrono();
	~CChrono();

	void Start(void);
	DWORD Stop(void);

	protected:
	DWORD _time;

	private:   
};

class CShellDlg
{
	public:
	CShellDlg();
	~CShellDlg();
	bool BrowseForFolder(CWindow * pWindow, LPSTR pszDisplayName, LPCSTR lpszTitle, 
		UINT ulFlags, BFFCALLBACK lpfn=0, LPARAM lParam=0, int iImage=0);

	protected:
	IMalloc* pMalloc;
	BROWSEINFO bi;

	private:   
};

class CCriticalSection
{
public:
	CCriticalSection();
	~CCriticalSection();
	void Enter();
	void Leave();
private:
	CRITICAL_SECTION cs; 
};

#endif
