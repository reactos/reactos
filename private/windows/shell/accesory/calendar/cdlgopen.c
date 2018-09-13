/* ** This file contains routines required to display a standard open
      dialog box.  Apps can directly link to object file or modify this
      source for slightly different dialog box.

      Note - in order to use these routines, the application must
      export DlgfnOpen().  Also, an app that uses these routines must
      be running ss=ds, since they use near pointers into stack.
*/

#include "cal.h"

#define ATTRDIRLIST	0xC010	/* include directories and drives in listbox */
#define ATTRFILELIST	0x0000
#define ID_LISTBOX	10
#define ID_EDIT 	11
#define CBEXTMAX	6  /* Number of bytes in "\*.txt" */

CHAR szLastDir[120];    /* Dir where the last open occurred */
                        /* useful if file is picked up from other than current dir e.g path */
INT idEditSave;
INT idListboxSave;
INT idPathSave;
CHAR *	 szExtSave;
CHAR *	 szFileNameSave;
INT	*pfpSave;
OFSTRUCT *	rgbOpenSave;
INT	cbRootNameMax;
#define CCHNG	    15
CHAR	rgchNg[CCHNG] =  {'"', '\\', '/', '[', ']', ':', '|',
			  '<', '>', '+', '=', ';', ',', ' ', 0};


/*
 *  Function prototypes
 */

VOID cDlgAddCorrectExtension(CHAR *szEdit, WORD fSearching);


/*
 *  Functions
 */

#if 0
/************************* commented out unused code . L.Raman 12/12/90 */

/* int far DlgfnOpen(); */
/* void far cDlgCheckOkEnable(); */
/* BOOL far cDlgCheckFileName();*/

/* ** Display dialog box for opening files.  Allow user to interact with
      dialogbox, change directories as necessary, and try to open file if user
      selects one.  Automatically append extension to filename if necessary.
      The open dialog box contains an edit field, listbox, static field,
      and OK and CANCEL buttons.

      This routine correctly parses filenames containing KANJI characters.

      Input -	hInstance if app module instance handle.
		hwndParent is window handle of parent window
		idDlgIn is dialog id
		idEditIn is id of edit field
		idListboxIn is id of listbox
		idPathIn is id of static field which gets path name
		szExtIn is pointer to zero terminated string containing
			default extension to be added to filenames.
		cbFileNameMaxIn is number of bytes in edit field buffer.

      Output - *pfp gets value of file handle if file is opened.
		    or -1 if file could not be opened.
	       *rgbOpenIn is initialized with file info by OpenFile()
	       *szFileNameIn gets name of selected file (fully qualified)
	       Any leading blanks are removed from sszFileName.
	       Trailing blanks are replaced with a 0 terminator.

      Returns -     -1 if dialogbox() fails (out of memory).
		     0 if user presses cancel
		     1 if user enters legal filename and presses ok
                     2 if user enters illegal file name and presses ok
*/
INT APIENTRY cDlgOpen(
    HANDLE	hInstance,
    HWND	hwndParent,
    INT		idDlgIn,
    INT		idEditIn,
    INT		idListboxIn,
    INT		idPathIn,
    CHAR	*szExtIn,
    INT		cbFileNameMaxIn,
    CHAR	*szFileNameIn,
    OFSTRUCT *	 rgbOpenIn,
    INT		*pfp)
{
    BOOL    fResult;
    FARPROC lpProc;

    idEditSave = idEditIn;
    idListboxSave = idListboxIn;
    idPathSave = idPathIn;
    szExtSave = szExtIn;
    /* Limit for bytes in filename is max bytes in filename less
       space for extension and 0 terminator. */
    cbRootNameMax = cbFileNameMaxIn - CBEXTMAX - 1;
    szFileNameSave = szFileNameIn;
    rgbOpenSave = rgbOpenIn;
    pfpSave = pfp;
    lpProc = MakeProcInstance(cDlgfnOpen, hInstance);
    fResult = DialogBox(hInstance, MAKEINTRESOURCE(idDlgIn), hwndParent, lpProc );
    FreeProcInstance(lpProc);
    return fResult;
}


/* ** Dialog function for Open File */
INT APIENTRY cDlgfnOpen(
    HWND hwnd,
    WORD msg,
    WPARAM wParam,
    LONG lParam)
{
    INT item;
    CHAR rgch[256];
    INT cchFile, cchDir;
    CHAR *pchFile;
    BOOL    fWild;
    static BOOL bRO=FALSE;
    INT     result = 2;    /* Assume illegal filename */
    INT len, nx;

    switch (msg) {
    case WM_INITDIALOG:
        /* Save the global read-only status. */
        bRO=vfOpenFileReadOnly;

	/* Set edit field with default search spec */
        SetDlgItemText(hwnd, idEditSave, (LPSTR)(szExtSave+1));
        SendDlgItemMessage(hwnd, idEditSave, +++EM_SETSEL(use macros)+++, 0, MAKELONG(6,0));

	/* Don't let user type more than cbRootNameMax bytes in edit ctl. */
        SendDlgItemMessage(hwnd, idEditSave, EM_LIMITTEXT, cbRootNameMax-1, 0L);

	/* fill list box with filenames that match spec, and fill static
	   field with path name */
	if (!DlgDirList(hwnd, (LPSTR)(szExtSave+1), IDCN_LISTBOXDIR, idPathSave, ATTRDIRLIST))
	    EndDialog(hwnd, 0);
	if (!DlgDirList(hwnd, (LPSTR)(szExtSave+1), IDCN_LISTBOX, idPathSave, ATTRFILELIST))
	    EndDialog(hwnd, 0);
	break;

      case WM_COMMAND:
	wParam = GET_WM_COMMAND_ID(wParam, lParam);
#ifdef NEVER
/*  The following lines are commented out because they introduce this bug:
    When the edit field becomes empty, OK button is not greyed
    SANKAR 06-21-89.
*/
	if (wParam == idEditSave)
	    wParam = ID_EDIT;
#endif
	switch (wParam) {
        case IDOK:
LoadIt:
            vfOpenFileReadOnly=IsDlgButtonChecked(hwnd, IDCN_READONLY);

            if (IsWindowEnabled(GetDlgItem(hwnd, IDOK)))
                {
		/* Get contents of edit field */
		/* Add search spec if it does not contain one. */
		len = 7 + GetWindowTextLength (GetDlgItem(hwnd, IDCN_EDIT));
		GetDlgItemText(hwnd, idEditSave, (LPSTR)szFileNameSave, len);
		lstrcpy((LPSTR)rgch, (LPSTR)szFileNameSave);

		/* Append appropriate extension to user's entry */
		cDlgAddCorrectExtension(rgch, TRUE);

		/* Try to open directory.  If successful, fill listbox with
		   contents of new directory.  Otherwise, open datafile. */
                if (cFSearchSpec(rgch))
                    {
                    if (DlgDirList(hwnd, (LPSTR)rgch, IDCN_LISTBOXDIR, idPathSave, ATTRDIRLIST))
                        {
                        lstrcpy((LPSTR)szFileNameSave, (LPSTR)rgch);
                        DlgDirList(hwnd, (LPSTR)rgch, IDCN_LISTBOX, idPathSave, ATTRFILELIST);
			SetDlgItemText(hwnd, idEditSave, (LPSTR)szFileNameSave);
			break;
                        }
                    }


		cDlgAddCorrectExtension(szFileNameSave, FALSE);
		/* If no directory list and filename contained search spec,
		   honk and don't try to open. */
		if (cFSearchSpec(szFileNameSave)) {
		    MessageBeep(0);
		    break;
		}

		/* Make filename upper case and if it's a legal dos
		   name, try to open the file. */
		AnsiUpper((LPSTR)szFileNameSave);
		if (cDlgCheckFileName(szFileNameSave)) {
		    result = 1;
		    *pfpSave = MOpenFile((LPSTR)szFileNameSave, (LPOFSTRUCT)rgbOpenSave, OF_PROMPT+OF_CANCEL);
		    if ((*pfpSave == -1) &&
			(((LPOFSTRUCT)rgbOpenSave)->nErrCode == 0))
                            result = 2;
            else    {        /* successful file open */
                strcpy(szLastDir, ((LPOFSTRUCT)rgbOpenSave)->szPathName);
                szLastDir[strlen(szLastDir)-strlen(szFileNameSave)] = 0;
            }
        }
		EndDialog(hwnd, result);
	    }
	    break;

	case IDCANCEL:
            /* User pressed cancel.  Just take down dialog box. */
            vfOpenFileReadOnly=bRO; /* And restore this. */
	    EndDialog(hwnd, 0);
	    break;

	/* User single clicked or doubled clicked in listbox -
	   Single click means fill edit box with selection.
	   Double click means go ahead and open the selection. */
	case IDCN_LISTBOX:
	case IDCN_LISTBOXDIR:
	    switch (GET_WM_COMMAND_ID(wParam, lParam)) {

	    /* Single click case */
	    case 1:
                GetDlgItemText(hwnd, idEditSave, (LPSTR)rgch, cbRootNameMax+1);

		/* Get selection, which may be either a prefix to a new search
		   path or a filename. DlgDirSelectEx parses selection, and
                   appends a backslash if selection is a prefix */

                if (wParam==IDCN_LISTBOXDIR)
                    SendDlgItemMessage(hwnd, IDCN_LISTBOX, LB_SETCURSEL, -1, 0L);
                else
                    SendDlgItemMessage(hwnd, IDCN_LISTBOXDIR, LB_SETCURSEL, -1, 0L);

                nx=DLGDIRSELECT(hwnd, szFileNameSave, +++nLen+++, wParam);

                if (nx)
                    {
		    cchDir = lstrlen((LPSTR)szFileNameSave);
		    cchFile = lstrlen((LPSTR)rgch);
		    pchFile = rgch+cchFile;

                    /* Now see if there are any wild characters (* or ?) in
		       edit field.  If so, append to prefix. If edit field
		       contains no wild cards append default search spec
		       which is  "*.TXT" for notepad. */
		    fWild = (*pchFile == '*' || *pchFile == ':');
		    while (pchFile > rgch) {
			pchFile = (CHAR *)LOWORD((LONG)AnsiPrev((LPSTR)(rgch), (LPSTR)pchFile));
			if (*pchFile == '*' || *pchFile == '?')
			    fWild = TRUE;
			if (*pchFile == '\\' || *pchFile == ':') {
			    pchFile = (CHAR *)LOWORD((LONG)AnsiNext((LPSTR)pchFile));
			    break;
			}
		    }
		    if (fWild)
			lstrcpy((LPSTR)szFileNameSave + cchDir, (LPSTR)pchFile);
		    else
			lstrcpy((LPSTR)szFileNameSave + cchDir, (LPSTR)(szExtSave+1));
		}

		/* Set edit field to entire file/path name. */
		SetDlgItemText(hwnd, idEditSave, (LPSTR)szFileNameSave);

		break;

	    /* Double click case - first click has already been processed
	       as single click */
	    case 2:
		/* Basically the same as ok.  If new selection is directory,
                   open it and list it.  Otherwise, open file. */
#if NEVER
/* None of this code is necessary.  A double click is more than basically the
   same as pressing OK, it is EXACTLY the same as pressing OK.  No point in
   duplicating all this code, especially since it will bring up 2 consecutive
   System-Modal Dialogs if the path investigated references drive A with the
   door open.   Clark Cyr, 14 August 1989                                     */

                DlgDirList(hwnd, szFileNameSave, IDCN_LISTBOX, idPathSave,ATTRFILELIST);

                if (DlgDirList (hwnd, szFileNameSave, IDCN_LISTBOXDIR,IDCN_PATH, ATTRDIRLIST))
                    {
		    SetDlgItemText(hwnd, idEditSave, (LPSTR)szFileNameSave);
		    break;
                    }
#endif
		goto LoadIt;	/* go load it up */
	    }
	    break;

	case IDCN_EDIT:
	    cDlgCheckOkEnable(hwnd, idEditSave, GET_WM_COMMAND_ID(wParam, lParam));
		
	    break;

	default:
	    return(FALSE);
	}
    default:
	return FALSE;
    }
    return(TRUE);
}


/* ** Enable ok button in a dialog box if and only if edit item
      contains text.  Edit item must have id of idEditSave */
VOID APIENTRY cDlgCheckOkEnable(
    HWND	hwnd,
    INT	idEdit,
    WORD message)
{
    if (message == EN_CHANGE) {
	EnableWindow(GetDlgItem(hwnd, IDOK), (SendMessage(GetDlgItem(hwnd, idEdit), WM_GETTEXTLENGTH, 0, 0L)));
    }
}

/* ** Given filename or partial filename or search spec or partial
      search spec, add appropriate extension. */
VOID cDlgAddCorrectExtension(CHAR *szEdit, WORD fSearching)
{
    register CHAR    *pchLast;
    register CHAR    *pchT;
    INT ichExt;
    BOOL    fDone = FALSE;
    INT     cchEdit;

    pchT = pchLast = (CHAR *)LOWORD((LONG)AnsiPrev((LPSTR)szEdit, (LPSTR)(szEdit + (cchEdit = lstrlen((LPSTR)szEdit)))));

    if ((*pchLast == '.' && *(AnsiPrev((LPSTR)szEdit, (LPSTR)pchLast)) == '.') && cchEdit == 2)
	ichExt = 0;
    else if (*pchLast == '\\' || *pchLast == ':')
	ichExt = 1;
    else {
	ichExt = fSearching ? 0 : 2;
	for (; pchT > szEdit; pchT = (CHAR *)LOWORD((LONG)AnsiPrev((LPSTR)szEdit, (LPSTR)pchT))) {
	    /* If we're not searching and we encounter a period, don't add
	       any extension.  If we are searching, period is assumed to be
	       part of directory name, so go ahead and add extension. However,
	       if we are searching and find a search spec, do not add any
	       extension. */
	    if (fSearching) {
		if (*pchT == '*' || *pchT == '?')
		    return;
	    } else if (*pchT == '.'){
		return;
	    }
	    /* Quit when we get to beginning of last node. */
	    if (*pchT == '\\')
		break;
	}
	/* Special case hack fix since AnsiPrev can not return value less than
	   szEdit. If first char is wild card, return without appending. */
	if (fSearching && (*pchT == '*' || *pchT == '?'))
	    return;
    }
#ifdef DBCS
    lstrcpy((LPSTR)AnsiNext(pchLast), (LPSTR)(szExtSave+ichExt));
#else
    lstrcpy((LPSTR)(pchLast+1), (LPSTR)(szExtSave+ichExt));
#endif
}

/* ** Check for legal filename. Strip leading blanks and
      0 terminate */
BOOL  APIENTRY cDlgCheckFilename(register CHAR	*pch)
{
#ifndef CRISPY

    OFSTRUCT	ofT;
    return (MOpenFile((LPSTR)pch, (LPOFSTRUCT)&ofT, OF_PARSE) == 0);
}
#else


    INT     cchFN;
    register INT     cchT;
    CHAR	*pchIn;
    CHAR	*pchFirst;
    CHAR	*pchSave;
    INT     s;
    BOOL	fBackSlash;

    s = 0;
    fBackSlash = FALSE;
    pchIn = pch;
    for (;; pch = (CHAR *)AnsiNext((LPSTR)pch)) {

	switch (s) {

	/* Trim leading blanks */
	case 0:
	    if (*pch == ' ')
		break;

	    if (*pch == '\\') {
		pchFirst = pch;
		cchT = 0;
		s = 2;
	    } else if (*pch == 0 || !cIsChLegal(*pch)) {
		return FALSE;

	    } else {
		pchFirst = pch;
		cchT = 1;
		if (*pch == '.')
		    s = 5;
		else
		    s = 1;
	    }
	    break;

	/* Volume, drive, subdirectory	node or filename */
	case 1:
	    if (*pch == ':' && cchT == 1) {
		if (*(pch-1) < 'A' || *(pch-1) > 'Z')
		    return FALSE;
		s = 2;
		cchT--;
	    } else if (*pch == '\\') {
		if (*(pch+1) == '\\')
		    return FALSE;
		cchT = 0;
	     } else if (*pch == '.') {
		cchT = 0;
		s = 3;
	    } else if (!cIsChLegal(*pch))
		return FALSE;
	    else if (*pch == 0)
		goto RetGood;
	    else if (cchT++) {
		s++;
	    }
	    break;

	/* sub directory node or filename */
	case 2:
	    if (*pch == '\\') {
		if (*(pch+1) == '\\')
		    return FALSE;
		fBackSlash = TRUE;
		cchT = 0;
	    } else if (*pch == '.') {
		if (*pch+1 == '.') {
		    if (fBackSlash || cchT)
			return FALSE;
		    s = 5;
		    cchT = 1;
		} else {
		    s++;
		    cchT = 0;
		}
	    } else if (*pch == 0)
		goto RetGood;
	    else if (cchT++ > 7 || *pch == ' ' || !IsChLegal(*pch))
		return FALSE;
	    break;

	/* up to three characters in extension */
	case 3:
	    if (*pch == 0) {
		goto   RetGood;
	    }

	    if (cchT++ > 2 || *pch == '.' || !IsChLegal(*pch))
		return FALSE;

	    if (*pch == ' ') {
		pchSave = pch;
		s++;
	    }
	    break;

	/* Trim trailing blanks */
	case 4:
	    if (*pch == 0) {
		*pchSave = 0;
		goto RetGood;
	    } else if (*pch != ' ')
		return FALSE;
	    break;

	/* check for ..\ */
	case 5:
	    if (*pch++ != '.' || *pch != '\\')
		return FALSE;
	    cchT = 0;
	    fBackSlash = TRUE;
	    s = 2;
	    break;
	}
    }
RetGood:
    if (pchFirst != pchIn)
	lstrcpy((LPSTR)pchIn, (LPSTR)pchFirst);
    return TRUE;
}

/* ** Check for legal MS-DOS filename characters.
      return TRUE if legal, FALSE otherwise. */
BOOL APIENTRY cIsChLegal(INT	ch)
{
	register CHAR	 *pch = rgchNg;
	register INT ich = 0;

	if (ch < ' ')
	    return FALSE;

	rgchNg[CCHNG-1] = ch;
#ifdef DBCS
        while (ch != *pch){
	    if( IsDBCSLeadByte( *pch ) )
		ich++;
            ich++;
            pch = AnsiNext(pch);
        }
#else
	while (ch != *pch++)
	    ich++;
#endif

	return (ich == CCHNG-1);
}
#endif

/* ** return TRUE iff 0 terminated string contains a '*' or '\' */
BOOL  APIENTRY cFSearchSpec(register CHAR *sz)
{
#ifdef DBCS
    for (; *sz;sz=AnsiNext(sz)){
	if (*sz == '*' || *sz == '?')
	    return TRUE;
    }
#else
    for (; *sz;sz++) {
	if (*sz == '*' || *sz == '?')
	    return TRUE;
    }
#endif
    return FALSE;
}
#endif
