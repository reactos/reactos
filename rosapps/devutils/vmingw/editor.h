/********************************************************************
*	Module:	editor.h. This is part of Visual-MinGW.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
*******************************************************************/
#ifndef EDITOR_H
#define EDITOR_H

#include "Scintilla.h"
#include "SciLexer.h"
#include "winui.h"

#define U_FILE	3
#define H_FILE	(U_FILE+1)
#define C_FILE	(U_FILE+2)
#define RC_FILE	(U_FILE+3)

// Default block size.
const int blockSize = 131072;

// Default colors.
const COLORREF black = RGB(0,0,0);
const COLORREF white = RGB(0xff,0xff,0xff);
const COLORREF darkBlue = RGB(0, 0, 0x7f);
const COLORREF Green = RGB(0, 0x7f, 0);
const COLORREF darkGreen = RGB(0x3f, 0x70, 0x3f);
const COLORREF Purple = RGB(0x7f, 0x00, 0x7f);
const COLORREF Ice = RGB(0x00, 0x7f, 0x7f);
const COLORREF Olive = RGB(0x7f, 0x7f, 0x00);

// Default Cpp keywords.
const char cppKeyWords[] = 
"asm auto bool break case catch char class const const_cast continue "
"default delete do double dynamic_cast else enum explicit export extern false float for "
"friend goto if inline int long mutable namespace new operator private protected public "
"register reinterpret_cast return short signed sizeof static static_cast struct switch "
"template this throw true try typedef typeid typename union unsigned using "
"virtual void volatile wchar_t while";

void	EnsureRangeVisible(HWND hwndCtrl, int posStart, int posEnd, bool enforcePolicy);
int LengthDocument(HWND hwndCtrl);
CharacterRange GetSelection(HWND hwndCtrl);

class CFileItem : public CNode
{
	public:
	CFileItem();
	~CFileItem();

	// File name.
	char		szFileName[MAX_PATH]; 
	WORD	nFileOffset; 
	WORD	nFileExtension;

	// Owner tree view.
	CTreeView * pTreeView;
	HTREEITEM 	_hItem;
	HTREEITEM 	_hDirItem;

	// Owner child window.
	CMDIChild * pMdiChild;
	int		show;
	bool		isInProject;

	protected:

	private:
};

void GetFileType(CFileItem * file);

class CEditor : public CScintilla
{
	public:
	CEditor();
	~CEditor();

	void LoadFile(CFileItem * file);
	void SaveFile(char * fullPath);
	int GetCurrentPos(void);
	void GotoLine(int line, char * fileName = NULL);
	int	caretPos;
	void SetLexer(int fileType);
	void SetCppLexer(void);
	bool MarginClick(int position, int modifiers);

	protected:

	private:   
	void DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back);
	void GetRange(int start, int end, char *text);
	void SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face);
	void Expand(int &line, bool doExpand, bool force = false,
	            int visLevels = 0, int level = -1);
};

#define FR_MAX_LEN	200

class CFindReplaceDlg : public CDlgBase
{
	public:
	CFindReplaceDlg();
	virtual ~CFindReplaceDlg();

	HWND Find(CScintilla * pEditor);
	HWND Replace(CScintilla * pEditor);

	protected:
	void FindNext(bool reverseDirection, bool showWarnings);
	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL	Find_OnInitDialog(void);
	BOOL	Replace_OnInitDialog(void);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
	BOOL Find_OnCommand(WORD wIDl);
	BOOL Replace_OnCommand(WORD wID);
	BOOL HandleReplaceCommand(int cmd);
	void ReplaceOnce(void);
	void	ReplaceAll(bool inSelection);

	private:   
	HWND hFindWhat;
	HWND hReplaceWith;
	HWND hWholeWord;
	HWND hMatchCase;
	HWND hRegExp;
	HWND hWrap;
	HWND hUnSlash;
	HWND hUp;
	HWND hDown;
	char findWhat[FR_MAX_LEN + 1];
	char replaceWhat[FR_MAX_LEN + 1];
	
	bool bWholeWord;
	bool bMatchCase;
	bool bRegExp;
	bool bWrapFind;
	bool bUnSlash;
	bool bReverseFind;
	bool bHavefound;
	
	CEditor * pEditor;
	HWND hEditor;
	int resId;
};

class CChooseFontDlg : public CWindow
{
	public:
	CChooseFontDlg();
	~CChooseFontDlg();

	bool Create(CWindow * pWindow);

	protected:
	CHOOSEFONT cf;
	LOGFONT lf;

	private:   
};

#endif
