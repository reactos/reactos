/********************************************************************
*	Module:	editor.cpp. This is part of Visual-MinGW.
*
*	Purpose:	Procedures to manage Scintilla editor.
*
*	Authors:	These classes are based on SciTE release 1.39.
*			http://www.scintilla.org/
*			SciTE original code by Neil Hodgson.
*			Present revised code by Manu B.
*
*	License:	Both SciTE and Scintilla are covered by 
*			"License for Scintilla and SciTE" agreement terms detailed in license.htm.
*			Present revised code is covered by GNU General Public License.
*
*	Revisions:	
*
********************************************************************/
#include <windows.h>
#include <stdio.h>
#include "commctrl.h"
#include "commdlg.h"
#include "editor.h"
#include "rsrc.h"

extern CMessageBox MsgBox;

int	Minimum(int a, int b);
int	Maximum(int a, int b);

int Minimum(int a, int b){
	if (a < b)
		return a;
	else
		return b;
}

int Maximum(int a, int b){
	if (a > b)
		return a;
	else
		return b;
}

void EnsureRangeVisible(HWND hwndCtrl, int posStart, int posEnd, bool enforcePolicy){
	int lineStart = SendMessage(hwndCtrl, SCI_LINEFROMPOSITION, Minimum(posStart, posEnd), 0);
	int lineEnd = SendMessage(hwndCtrl, SCI_LINEFROMPOSITION, Maximum(posStart, posEnd), 0);
	for (int line = lineStart; line <= lineEnd; line++){
		SendMessage(hwndCtrl, 
			enforcePolicy ? SCI_ENSUREVISIBLEENFORCEPOLICY : SCI_ENSUREVISIBLE, line, 0);
	}
}

int LengthDocument(HWND hwndCtrl){
return SendMessage(hwndCtrl, SCI_GETLENGTH, 0, 0);
}

CharacterRange GetSelection(HWND hwndCtrl){
	CharacterRange crange;
	crange.cpMin = SendMessage(hwndCtrl, SCI_GETSELECTIONSTART, 0, 0);
	crange.cpMax = SendMessage(hwndCtrl, SCI_GETSELECTIONEND, 0, 0);
return crange;
}


/********************************************************************
*	Class:	CChooseFontDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CChooseFontDlg::CChooseFontDlg(){
	ZeroMemory(&lf, sizeof(LOGFONT));
/*	lf.lfHeight;
	lf.lfWidth;
	lf.lfEscapement;
	lf.lfOrientation;
	lf.lfWeight;
	lf.lfItalic;
	lf.lfUnderline;
	lf.lfStrikeOut;
	lf.lfCharSet;
	lf.lfOutPrecision;
	lf.lfClipPrecision;
	lf.lfQuality;
	lf.lfPitchAndFamily;
	lf.lfFaceName[LF_FACESIZE];*/

	cf.lStructSize		= sizeof(CHOOSEFONT);
	cf.hwndOwner		= 0;
	cf.hDC			= NULL;
	cf.lpLogFont		= &lf;//&(Profile.LogFont);
	cf.iPointSize		= 0;
	cf.Flags			= /*CF_INITTOLOGFONTSTRUCT
					|*/ CF_SCREENFONTS | CF_EFFECTS 
					/*| CF_ENABLEHOOK*/;
	cf.rgbColors		= 0;//Profile.rgbForeColor;
	cf.lCustData		= 0;
	cf.lpfnHook			= NULL;
	cf.lpTemplateName	= NULL;
	cf.hInstance		= NULL;
	cf.lpszStyle			= NULL;
	cf.nFontType		= SCREEN_FONTTYPE;
	cf.nSizeMin			= 0;
	cf.nSizeMax		= 0;
}

CChooseFontDlg::~CChooseFontDlg(){
}

bool CChooseFontDlg::Create(CWindow * pWindow){
	cf.hwndOwner = pWindow->_hWnd;
return ChooseFont(&cf);
}


/*bool ChooseNewFont(HWND hWndListBox){
	static CHOOSEFONT cf;
	static BOOL bFirstTime = TRUE;
	HFONT hFont;
	if(bFirstTime){
		bFirstTime = false;
	}    
	if(ChooseFont(&cf)){
		HDC    hDC;
		hFont = CreateFontIndirect( &(Profile.LogFont) );
		hDC = GetDC( hWndListBox );
		SelectObject( hDC, hFont );
		Profile.rgbForeColor = cf.rgbColors;
		InvalidateRect( hWndListBox, NULL, TRUE );
		SendMessage( hWndListBox, WM_CTLCOLORLISTBOX, (DWORD) hDC,       (LONG) hWndListBox );
		SendMessage( hWndListBox, WM_SETFONT, (DWORD) hFont, TRUE );
		ReleaseDC( hWndListBox, hDC );
	}
return true;
}*/


/********************************************************************
*	Class:	CFindReplaceDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CFindReplaceDlg::CFindReplaceDlg(){
	pEditor = NULL;
	hEditor = 0;
	resId = 0;
	*findWhat = '\0';
	*replaceWhat = '\0';
	
	bWholeWord 	= false;
	bMatchCase 	= true;
	bRegExp		= false;
	bWrapFind		= false;
	bUnSlash		= false;
	bReverseFind	= false;
	bHavefound	= false;
}

CFindReplaceDlg::~CFindReplaceDlg(){
}

HWND CFindReplaceDlg::Find(CScintilla * pEditor){
	if (_hWnd || !pEditor)
		return 0;
return CreateParam(pEditor, IDD_FIND, (long) IDD_FIND);
}

HWND CFindReplaceDlg::Replace(CScintilla * pEditor){
	if (_hWnd || !pEditor)
		return 0;
return CreateParam(pEditor, IDD_REPLACE, (long) IDD_REPLACE);
}

LRESULT CALLBACK CFindReplaceDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
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

BOOL CFindReplaceDlg::OnInitDialog(HWND, LPARAM lInitParam){
	// Set pointers.
	pEditor = (CEditor *) _pParent;
	if (pEditor == NULL)
		return TRUE;

	hEditor = pEditor->_hWnd;
	resId = lInitParam;

	hFindWhat	 		= GetItem(IDC_FINDWHAT);
	hWholeWord	 	= GetItem(IDC_WHOLEWORD);
	hMatchCase	 	= GetItem(IDC_MATCHCASE);
	hRegExp			= GetItem(IDC_REGEXP);
	hWrap		 	= GetItem(IDC_WRAP);
	hUnSlash			= GetItem(IDC_UNSLASH);

	if (resId == IDD_FIND)
		return Find_OnInitDialog();
	else if (resId == IDD_REPLACE)
		return Replace_OnInitDialog();
return FALSE;
}

BOOL CFindReplaceDlg::Find_OnInitDialog(void){

	hUp = GetItem(IDC_DIRECTIONUP);
	hDown = GetItem(IDC_DIRECTIONDOWN);

	SetItemText(hFindWhat, findWhat);
	//FillComboFromMemory(wFindWhat, memFinds);
	if (bWholeWord)
		::SendMessage(hWholeWord, BM_SETCHECK, BST_CHECKED, 0);
	if (bMatchCase)
		::SendMessage(hMatchCase, BM_SETCHECK, BST_CHECKED, 0);
	if (bRegExp)
		::SendMessage(hRegExp, 	BM_SETCHECK, BST_CHECKED, 0);
	if (bWrapFind)
		::SendMessage(hWrap, 	BM_SETCHECK, BST_CHECKED, 0);
	if (bUnSlash)
		::SendMessage(hUnSlash, 	BM_SETCHECK, BST_CHECKED, 0);

	if (bReverseFind) {
		::SendMessage(hUp, 		BM_SETCHECK, BST_CHECKED, 0);
	} else {
		::SendMessage(hDown, 	BM_SETCHECK, BST_CHECKED, 0);
	}
return TRUE;
}

BOOL CFindReplaceDlg::Replace_OnInitDialog(void){
	SetItemText(hFindWhat, findWhat);
	//FillComboFromMemory(wFindWhat, sci->memFinds);

	hReplaceWith = GetItem(IDC_REPLACEWITH);
	SetItemText(hReplaceWith, replaceWhat);
	//FillComboFromMemory(wReplaceWith, sci->memReplaces);

	if (bWholeWord)
		::SendMessage(hWholeWord, BM_SETCHECK, BST_CHECKED, 0);
	if (bMatchCase)
		::SendMessage(hMatchCase, BM_SETCHECK, BST_CHECKED, 0);
	if (bRegExp)
		::SendMessage(hRegExp, BM_SETCHECK, BST_CHECKED, 0);
	if (bWrapFind)
		::SendMessage(hWrap, BM_SETCHECK, BST_CHECKED, 0);
	if (bUnSlash)
		::SendMessage(hUnSlash, BM_SETCHECK, BST_CHECKED, 0);
	if ((findWhat) != '\0'){
		::SetFocus(hReplaceWith);
		return FALSE;
	}
return TRUE;
}

BOOL CFindReplaceDlg::OnCommand(WORD, WORD wID, HWND){
	if (resId == IDD_FIND)
		return Find_OnCommand(wID);
	else if (resId == IDD_REPLACE)
		return Replace_OnCommand(wID);
return FALSE;
}

BOOL CFindReplaceDlg::Find_OnCommand(WORD wID){
	switch (wID){
		case IDOK:
			char s[200];
			GetItemText(hFindWhat, s, sizeof(s));
			strcpy(findWhat, s);
			//memFinds.Insert(s);
			bWholeWord = BST_CHECKED ==
				::SendMessage(hWholeWord, BM_GETCHECK, 0, 0);
			bMatchCase = BST_CHECKED ==
				::SendMessage(hMatchCase, BM_GETCHECK, 0, 0);
			bRegExp = BST_CHECKED ==
				::SendMessage(hRegExp, BM_GETCHECK, 0, 0);
			bWrapFind = BST_CHECKED ==
				::SendMessage(hWrap, BM_GETCHECK, 0, 0);
			bUnSlash = BST_CHECKED ==
				::SendMessage(hUnSlash, BM_GETCHECK, 0, 0);
			bReverseFind = BST_CHECKED ==
				::SendMessage(hUp, BM_GETCHECK, 0, 0);

			FindNext(bReverseFind, true);
		return TRUE;

		case IDCANCEL:
			EndDlg(IDCANCEL);
		return FALSE;
	}
return FALSE;
}

BOOL CFindReplaceDlg::Replace_OnCommand(WORD wID){
	if (wID == IDCANCEL){
		EndDlg(IDCANCEL);
		return FALSE;
	}else{
		return HandleReplaceCommand(wID);
	}
return FALSE;
}

void CFindReplaceDlg::FindNext(bool reverseDirection, bool showWarnings){
	if (!hEditor){
		MsgBox.DisplayWarning("Can't get editor handle");
		return;
	}

	if (!findWhat[0]) { // nothing to found
		//Find(hEditor);
		return;
	}
	char findTarget[FR_MAX_LEN + 1];
	strcpy(findTarget, findWhat);

	// for C conversions ->	int lenFind = UnSlashAsNeeded(findTarget, unSlash, regExp); 
	int lenFind = strlen(findTarget); // normal return of UnSlashAsNeeded

	if (lenFind == 0)
		return;

	CharacterRange cr = GetSelection(hEditor);
	int startPosition = cr.cpMax;
	int endPosition = LengthDocument(hEditor);

	if (reverseDirection){
		startPosition = cr.cpMin - 1;
		endPosition = 0;
	}

	int flags = (bWholeWord ? SCFIND_WHOLEWORD : 0) |
	            (bMatchCase ? SCFIND_MATCHCASE : 0) |
	            (bRegExp ? SCFIND_REGEXP : 0);

	::SendMessage(hEditor, SCI_SETTARGETSTART, startPosition, 0);
	::SendMessage(hEditor, SCI_SETTARGETEND, endPosition, 0);
	::SendMessage(hEditor, SCI_SETSEARCHFLAGS, flags, 0);
	int posFind = ::SendMessage(hEditor, SCI_SEARCHINTARGET, lenFind, (LPARAM) findTarget);

	if (posFind == -1 && bWrapFind){ // not found with wrapFind

		// Failed to find in indicated direction
		// so search from the beginning (forward) or from the end (reverse)
		// unless wrapFind is false
		if (reverseDirection){
			startPosition = LengthDocument(hEditor);
			endPosition = 0;
		} else {
			startPosition = 0;
			endPosition = LengthDocument(hEditor);
		}
		::SendMessage(hEditor, SCI_SETTARGETSTART, startPosition, 0);
		::SendMessage(hEditor, SCI_SETTARGETEND, endPosition, 0);
		posFind = ::SendMessage(hEditor, SCI_SEARCHINTARGET, lenFind, (LPARAM) findTarget);
	}
	if (posFind == -1){	// not found
		bHavefound = false;
		if (showWarnings){

			/*warn that not found
			WarnUser(warnNotFound);*/
			
			if (strlen(findWhat) > FR_MAX_LEN)
				findWhat[FR_MAX_LEN] = '\0';
			char msg[FR_MAX_LEN + 50];
			strcpy(msg, "Cannot find the string \"");
			strcat(msg, findWhat);
			strcat(msg, "\".");
			if (_hWnd){
				MsgBox.DisplayWarning(msg);
			}else{
				MessageBox(0, msg, "Message", MB_OK);
			}
		}
	}else{	// found
		bHavefound = true;
		int start = ::SendMessage(hEditor, SCI_GETTARGETSTART, 0, 0);
		int end = ::SendMessage(hEditor, SCI_GETTARGETEND, 0, 0);
		EnsureRangeVisible(hEditor, start, end, true);
		::SendMessage(hEditor, SCI_SETSEL, start, end);
	}
}

BOOL CFindReplaceDlg::HandleReplaceCommand(int cmd){
	if (!hEditor){
		MsgBox.DisplayWarning("Can't get editor handle");
		return false;
	}

	if ((cmd == IDOK) || (cmd == IDC_REPLACE) || (cmd == IDC_REPLACEALL) || (cmd == IDC_REPLACEINSEL)) {
		GetItemText(hFindWhat, findWhat, sizeof(findWhat));
		//props.Set("find.what", findWhat);
		//memFinds.Insert(findWhat);

		bWholeWord = BST_CHECKED ==
			::SendMessage(hWholeWord, BM_GETCHECK, 0, 0);
		bMatchCase = BST_CHECKED ==
			::SendMessage(hMatchCase, BM_GETCHECK, 0, 0);
		bRegExp = BST_CHECKED ==
			::SendMessage(hRegExp, BM_GETCHECK, 0, 0);
		bWrapFind = BST_CHECKED ==
			::SendMessage(hWrap, BM_GETCHECK, 0, 0);
		bUnSlash = BST_CHECKED ==
			::SendMessage(hUnSlash, BM_GETCHECK, 0, 0);
	}
	if ((cmd == IDC_REPLACE) || (cmd == IDC_REPLACEALL) || (cmd == IDC_REPLACEINSEL)) {
		GetItemText(hReplaceWith, replaceWhat, sizeof(replaceWhat));
		//memReplaces.Insert(replaceWhat);
	}

	if (cmd == IDOK) {
		FindNext(bReverseFind, true);	// Find next
	} else if (cmd == IDC_REPLACE) {
		if (bHavefound){
			ReplaceOnce();
		} else {
			CharacterRange crange = GetSelection(hEditor);
			::SendMessage(hEditor, SCI_SETSEL, crange.cpMin, crange.cpMin);
			FindNext(bReverseFind, true);
			if (bHavefound){
				ReplaceOnce();
			}
		}
	} else if ((cmd == IDC_REPLACEALL) || (cmd == IDC_REPLACEINSEL)){
		ReplaceAll(cmd == IDC_REPLACEINSEL);
	}
return TRUE;
}

void CFindReplaceDlg::ReplaceOnce(void){
	if (bHavefound){
		char replaceTarget[FR_MAX_LEN + 1];
		strcpy(replaceTarget, replaceWhat);
		// for C conversions ->	int replaceLen = UnSlashAsNeeded(replaceTarget, unSlash, regExp); 
		int replaceLen = strlen(replaceTarget); // normal return of UnSlashAsNeeded
		
		CharacterRange cr = GetSelection(hEditor);
		::SendMessage(hEditor, SCI_SETTARGETSTART, cr.cpMin, 0);
		::SendMessage(hEditor, SCI_SETTARGETEND, cr.cpMax, 0);
		int lenReplaced = replaceLen;
		if (bRegExp)
			lenReplaced = ::SendMessage(hEditor, SCI_REPLACETARGETRE, replaceLen, (LPARAM) replaceTarget);
		else	// Allow \0 in replacement
			::SendMessage(hEditor, SCI_REPLACETARGET, replaceLen, (LPARAM) replaceTarget);
		::SendMessage(hEditor, SCI_SETSEL, cr.cpMin + lenReplaced, cr.cpMin);
		bHavefound = false;
	}
	FindNext(bReverseFind, true);
}

void CFindReplaceDlg::ReplaceAll(bool inSelection){
	char findTarget[FR_MAX_LEN + 1];
	strcpy(findTarget, findWhat);

	// for C conversions ->	int findLen = UnSlashAsNeeded(findTarget, unSlash, regExp);
	int findLen = strlen(findTarget); // normal return of UnSlashAsNeeded
		
	if (findLen == 0) {
		MessageBox(_hWnd, "Find string for \"Replace All\" must not be empty.", "Message", MB_OK | MB_ICONWARNING);
		return;
	}

	CharacterRange cr = GetSelection(hEditor);
	int startPosition = cr.cpMin;
	int endPosition = cr.cpMax;
	if (inSelection) {
		if (startPosition == endPosition) {
			MessageBox(_hWnd, "Selection for \"Replace in Selection\" must not be empty.", "Message", MB_OK | MB_ICONWARNING);
			return;
		}
	} else {
		endPosition = LengthDocument(hEditor);
		if (bWrapFind) {
			// Whole document
			startPosition = 0;
		}
		// If not wrapFind, replace all only from caret to end of document
	}

	char replaceTarget[FR_MAX_LEN + 1];
	strcpy(replaceTarget, replaceWhat);

	// for C conversions ->	int replaceLen = UnSlashAsNeeded(replaceTarget, unSlash, regExp); 
	int replaceLen = strlen(replaceTarget); // normal return of UnSlashAsNeeded
		
	int flags = (bWholeWord ? SCFIND_WHOLEWORD : 0) |
	            (bMatchCase ? SCFIND_MATCHCASE : 0) |
	            (bRegExp ? SCFIND_REGEXP : 0);
	::SendMessage(hEditor, SCI_SETTARGETSTART, startPosition, 0);
	::SendMessage(hEditor, SCI_SETTARGETEND, endPosition, 0);
	::SendMessage(hEditor, SCI_SETSEARCHFLAGS, flags, 0);
	int posFind = ::SendMessage(hEditor, SCI_SEARCHINTARGET, findLen, (LPARAM) findTarget);
	if ((findLen == 1) && bRegExp && (findTarget[0] == '^')) {
		// Special case for replace all start of line so it hits the first line
		posFind = startPosition;
		::SendMessage(hEditor, SCI_SETTARGETSTART, startPosition, 0);
		::SendMessage(hEditor, SCI_SETTARGETEND, startPosition, 0);
	}
	if ((posFind != -1) && (posFind <= endPosition)) {
		int lastMatch = posFind;
		::SendMessage(hEditor, SCI_BEGINUNDOACTION, 0, 0);
		while (posFind != -1) {
			int lenTarget = ::SendMessage(hEditor, SCI_GETTARGETEND, 0, 0) - ::SendMessage(hEditor, SCI_GETTARGETSTART, 0, 0);
			int lenReplaced = replaceLen;
			if (bRegExp)
				lenReplaced = ::SendMessage(hEditor, SCI_REPLACETARGETRE, replaceLen, (LPARAM) replaceTarget);
			else
				::SendMessage(hEditor, SCI_REPLACETARGET, replaceLen, (LPARAM) replaceTarget);
			// Modify for change caused by replacement
			endPosition += lenReplaced - lenTarget;
			lastMatch = posFind + lenReplaced;
			// For the special cases of start of line and end of line
			// Something better could be done but there are too many special cases
			if (lenTarget <= 0)
				lastMatch++;
			::SendMessage(hEditor, SCI_SETTARGETSTART, lastMatch, 0);
			::SendMessage(hEditor, SCI_SETTARGETEND, endPosition, 0);
			posFind = ::SendMessage(hEditor, SCI_SEARCHINTARGET, findLen, (LPARAM) findTarget);
		}
		if (inSelection)
			::SendMessage(hEditor, SCI_SETSEL, startPosition, endPosition);
		else
			::SendMessage(hEditor, SCI_SETSEL, lastMatch, lastMatch);
		::SendMessage(hEditor, SCI_ENDUNDOACTION, 0, 0);
	} else {
		if (strlen(findWhat) > FR_MAX_LEN)
			findWhat[FR_MAX_LEN] = '\0';
		char msg[FR_MAX_LEN + 50];
		strcpy(msg, "No replacements because string \"");
		strcat(msg, findWhat);
		strcat(msg, "\" was not present.");
		MessageBox(_hWnd, msg, "Message", MB_OK | MB_ICONWARNING);
	}
}


/********************************************************************
*	Class:	CEditor.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CEditor::CEditor(){
	caretPos = 1;
}

CEditor::~CEditor(){
}

void CEditor::LoadFile(CFileItem * file){
	if (!file || !_hWnd)
		return;

	if (file->nFileOffset == 0)
		return; // Untitled file.

	SetLexer(file->type);
	::SendMessage(_hWnd, SCI_CANCEL, 0, 0);
	::SendMessage(_hWnd, SCI_SETUNDOCOLLECTION, 0, 0);

	FILE *fp = fopen(file->szFileName, "rb");
	if (fp){
		char data[blockSize];
		int lenFile = fread(data, 1, sizeof(data), fp);

		while (lenFile > 0){
			::SendMessage(_hWnd, SCI_ADDTEXT, lenFile, (LPARAM) data);
			lenFile = fread(data, 1, sizeof(data), fp);
		}

		fclose(fp);

	}else{
		MsgBox.DisplayWarning("Can't load file %s", file->szFileName);
	}
	
	::SendMessage(_hWnd, SCI_SETUNDOCOLLECTION, 1, 0);
	::SendMessage(_hWnd, EM_EMPTYUNDOBUFFER, 0, 0);
	::SendMessage(_hWnd, SCI_SETSAVEPOINT, 0 , 0);
	::SendMessage(_hWnd, SCI_GOTOPOS, 0, 0);
}

void GetFileType(CFileItem * file){
	if (!file)
		return;

	if (file->nFileExtension){
		char * ext = file->szFileName + file->nFileExtension;
		// H_FILE ?
		if (!stricmp(ext, "h")){
			file->type = H_FILE;
			return;
		}else if (!stricmp(ext, "hpp")){
			file->type = H_FILE;
			return;
		}else if (!stricmp(ext, "hxx")){
			file->type = H_FILE;
			return;
		// C_FILE ?
		}else if (!stricmp(ext, "c")){
			file->type = C_FILE;
			return;
		}else if (!stricmp(ext, "cpp")){
			file->type = C_FILE;
			return;
		}else if (!stricmp(ext, "cxx")){
			file->type = C_FILE;
			return;
		// RC_FILE ?
		}else if (!stricmp(ext, "rc")){
			file->type = RC_FILE;
			return;
		}
	}
	file->type = U_FILE;
return;
}

void CEditor::SaveFile(char * fullPath){
	if (!_hWnd)
		return;

	FILE *fp = fopen(fullPath, "wb");
	if (fp){
		char data[blockSize + 1];
		int lengthDoc = ::SendMessage(_hWnd, SCI_GETLENGTH, 0, 0);
		for (int i = 0; i < lengthDoc; i += blockSize) {
			int grabSize = lengthDoc - i;
			if (grabSize > blockSize)
				grabSize = blockSize;
			GetRange(i, i + grabSize, data);
			fwrite(data, grabSize, 1, fp);
		}
		fclose(fp);
		::SendMessage(_hWnd, SCI_SETSAVEPOINT, 0, 0);

	}else{
		MsgBox.DisplayWarning("Can't save file %s", fullPath);
	}
}

int CEditor::GetCurrentPos(void){
	int currentPos = ::SendMessage(_hWnd, SCI_GETCURRENTPOS, 0,0);
	caretPos = ::SendMessage(_hWnd, SCI_LINEFROMPOSITION, currentPos, 0) + 1;
return caretPos;
}

void CEditor::GetRange(int start, int end, char *text){
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	::SendMessage(_hWnd, SCI_GETTEXTRANGE, 0, (LPARAM) &tr);
}

void CEditor::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face){
	::SendMessage(_hWnd, SCI_STYLESETFORE, style, fore);
	::SendMessage(_hWnd, SCI_STYLESETBACK, style, back);
	if (size >= 1)
		::SendMessage(_hWnd, SCI_STYLESETSIZE, style, size);
	if (face) 
		::SendMessage(_hWnd, SCI_STYLESETFONT, style, (LPARAM) face);
}

void CEditor::DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back) {
	::SendMessage(_hWnd, SCI_MARKERDEFINE, marker, markerType);
	::SendMessage(_hWnd, SCI_MARKERSETFORE, marker, fore);
	::SendMessage(_hWnd, SCI_MARKERSETBACK, marker, back);
}

void CEditor::SetLexer(int fileType){
	switch (fileType){

	case H_FILE:
	case C_FILE:
	case RC_FILE:
	    SetCppLexer();
        return;

	default:
	// Global default style.
	SetAStyle(STYLE_DEFAULT, black, white, 10, "Verdana");
	::SendMessage(_hWnd, SCI_STYLECLEARALL, 0, 0);	// Copies to all other styles.

	}
}

void CEditor::SetCppLexer(void){
	::SendMessage(_hWnd, SCI_SETLEXER, SCLEX_CPP, 0);
	::SendMessage(_hWnd, SCI_SETSTYLEBITS, 5, 0);

	::SendMessage(_hWnd, SCI_SETKEYWORDS, 0, (LPARAM) cppKeyWords);

	// Global default style.
	SetAStyle(STYLE_DEFAULT, black, white, 10, "Verdana");
	::SendMessage(_hWnd, SCI_STYLECLEARALL, 0, 0);	// Copies to all other styles.

	// C Styles. 
	SetAStyle(SCE_C_DEFAULT, black, white, 10, "Verdana");				//0
	SetAStyle(SCE_C_COMMENT, Green, white, 0, 0);					//1
	SetAStyle(SCE_C_COMMENTLINE, Green, white, 0, 0);				//2
	SetAStyle(SCE_C_COMMENTDOC, darkGreen, white, 0, 0);				//3
	SetAStyle(SCE_C_NUMBER, Ice, white, 0, 0);						//4
	SetAStyle(SCE_C_WORD, darkBlue, white, 0, 0);					//5
	::SendMessage(_hWnd, SCI_STYLESETBOLD, SCE_C_WORD, 1);		
	SetAStyle(SCE_C_STRING, Purple, white, 0, 0);						//6
	SetAStyle(SCE_C_CHARACTER, Purple, white, 0, 0);					//7
	SetAStyle(SCE_C_PREPROCESSOR, Olive, white, 0, 0);				//9
	SetAStyle(SCE_C_OPERATOR, black, white, 0, 0);					//10
	::SendMessage(_hWnd, SCI_STYLESETBOLD, SCE_C_OPERATOR, 1);		
//	SetAStyle(SCE_C_STRINGEOL, darkBlue, white, 0, 0);					//12
//	SetAStyle(SCE_C_COMMENTLINEDOC, darkBlue, white, 0, 0);			//15
//	SetAStyle(SCE_C_WORD2, darkBlue, white, 0, 0);					//16
	::SendMessage(_hWnd, SCI_SETPROPERTY, (long)"fold", (long)"1");
	::SendMessage(_hWnd, SCI_SETPROPERTY, (long)"fold.compact", (long)"1");
	::SendMessage(_hWnd, SCI_SETPROPERTY, (long)"fold.symbols", (long)"1");

	::SendMessage(_hWnd, SCI_SETFOLDFLAGS, 16, 0);

	// To put the folder markers in the line number region
	//SendEditor(SCI_SETMARGINMASKN, 0, SC_MASK_FOLDERS);

	::SendMessage(_hWnd, SCI_SETMODEVENTMASK, SC_MOD_CHANGEFOLD, 0);

	// Create a margin column for the folding symbols
	::SendMessage(_hWnd, SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);

	::SendMessage(_hWnd, SCI_SETMARGINWIDTHN, 2, /*foldMargin ? foldMarginWidth :*/ 16);

	::SendMessage(_hWnd, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	::SendMessage(_hWnd, SCI_SETMARGINSENSITIVEN, 2, 1);

	DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS, white, black);
	DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_PLUS, white, black);
	DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, white, black);
	DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, white, black);
	DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, white, black);
	DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, white, black);
	DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, white, black);

return;
}

void CEditor::GotoLine(int line, char * /*fileName*/){
	::SendMessage(_hWnd, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);
	::SendMessage(_hWnd, SCI_GOTOLINE, line, 0);
}

bool CEditor::MarginClick(int position, int modifiers){
	int lineClick = ::SendMessage(_hWnd, SCI_LINEFROMPOSITION, position, 0);
	//Platform::DebugPrintf("Margin click %d %d %x\n", position, lineClick,
	//	::SendMessage(_hWnd, SCI_GETFOLDLEVEL, lineClick) & SC_FOLDLEVELHEADERFLAG);
/*	if ((modifiers & SCMOD_SHIFT) && (modifiers & SCMOD_CTRL)) {
		FoldAll();
	} else {*/
		int levelClick = ::SendMessage(_hWnd, SCI_GETFOLDLEVEL, lineClick, 0);
		if (levelClick & SC_FOLDLEVELHEADERFLAG) {
			if (modifiers & SCMOD_SHIFT) {
				// Ensure all children visible
				::SendMessage(_hWnd, SCI_SETFOLDEXPANDED, lineClick, 1);
				Expand(lineClick, true, true, 100, levelClick);
			} else if (modifiers & SCMOD_CTRL) {
				if (::SendMessage(_hWnd, SCI_GETFOLDEXPANDED, lineClick, 0)) {
					// Contract this line and all children
					::SendMessage(_hWnd, SCI_SETFOLDEXPANDED, lineClick, 0);
					Expand(lineClick, false, true, 0, levelClick);
				} else {
					// Expand this line and all children
					::SendMessage(_hWnd, SCI_SETFOLDEXPANDED, lineClick, 1);
					Expand(lineClick, true, true, 100, levelClick);
				}
			} else {
				// Toggle this line
				::SendMessage(_hWnd, SCI_TOGGLEFOLD, lineClick, 0);
			}
		}
/*	}*/
	return true;
}

void CEditor::Expand(int &line, bool doExpand, bool force, int visLevels, int level){
	int lineMaxSubord = ::SendMessage(_hWnd, SCI_GETLASTCHILD, line, level & SC_FOLDLEVELNUMBERMASK);
	line++;
	while (line <= lineMaxSubord) {
		if (force) {
			if (visLevels > 0)
				::SendMessage(_hWnd, SCI_SHOWLINES, line, line);
			else
				::SendMessage(_hWnd, SCI_HIDELINES, line, line);
		} else {
			if (doExpand)
				::SendMessage(_hWnd, SCI_SHOWLINES, line, line);
		}
		int levelLine = level;
		if (levelLine == -1)
			levelLine = ::SendMessage(_hWnd, SCI_GETFOLDLEVEL, line, 0);
		if (levelLine & SC_FOLDLEVELHEADERFLAG) {
			if (force) {
				if (visLevels > 1)
					::SendMessage(_hWnd, SCI_SETFOLDEXPANDED, line, 1);
				else
					::SendMessage(_hWnd, SCI_SETFOLDEXPANDED, line, 0);
				Expand(line, doExpand, force, visLevels - 1);
			} else {
				if (doExpand) {
					if (!::SendMessage(_hWnd, SCI_GETFOLDEXPANDED, line, 0))
						::SendMessage(_hWnd, SCI_SETFOLDEXPANDED, line, 1);
					Expand(line, true, force, visLevels - 1);
				} else {
					Expand(line, false, force, visLevels - 1);
				}
			}
		} else {
			line++;
		}
	}
}

