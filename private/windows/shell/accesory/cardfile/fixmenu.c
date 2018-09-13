#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

void NEAR PASCAL MakeMenuString(TCHAR *szCtrl, TCHAR *szMenuStr, TCHAR *szVerb, TCHAR *szClass, TCHAR *szObject);

/* Position of the Edit.Object menu. Have to deal with this BY_POSITION
 * since it can be either be a Popup/Menuitem.
 */
#define MENUPOS_EDITOBJECT      14

NOEXPORT int GetKeyCount(
    HKEY hKeyVerb);

NOEXPORT BOOL FixEditObject(
    HMENU hEditMenu);

void DisableEditObject(
    HMENU hEditMenu);

void UpdateMenu(
    void)
{
    HMENU hMenu;

    hMenu = GetMenu(hIndexWnd);

    CheckMenuItem(hMenu, I_TEXT, EditMode == I_TEXT ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, I_OBJECT, EditMode == I_OBJECT ? MF_CHECKED : MF_UNCHECKED);

    if (CardPhone == CCARDFILE)
    {
        if ((EditMode == I_TEXT) && SendMessage(hEditWnd, EM_CANUNDO, 0, 0L))
            EnableMenuItem(hMenu, UNDO, MF_ENABLED);
        else
            EnableMenuItem(hMenu, UNDO, MF_GRAYED);

        EnableMenuItem(hMenu, I_TEXT, MF_ENABLED);
        EnableMenuItem(hMenu, I_OBJECT, MF_ENABLED);
        EnableMenuItem(hMenu, RESTORE, MF_ENABLED);
        EnableMenuItem(hMenu, FIND, hDlgFind? MF_GRAYED: MF_ENABLED);
        if (szSearch[0] != 0 && !hDlgFind)
            EnableMenuItem(hMenu, FINDNEXT, MF_ENABLED);
        else
            EnableMenuItem(hMenu, FINDNEXT, MF_GRAYED);
        EnableMenuItem(hMenu, PRINT, fCanPrint ? MF_ENABLED : MF_GRAYED);
#ifdef JAPAN    //KKBUGFIX     //  added  18 Aug. 1992 by Hiraisi (in Japan)
        EnableMenuItem(hMenu, PRINTALL, fCanPrint ? MF_ENABLED : MF_GRAYED);
#endif
        OleMenuItemFix(hMenu, EditMode);    /* Also does Cut/Copy/Paste */

//      EnableMenuItem(hMenu, GOTO, MF_ENABLED);
    }
    else
    {
        /* all of these don't apply to list mode */

        EnableMenuItem(hMenu, UNDO, MF_GRAYED);
        EnableMenuItem(hMenu, I_TEXT, MF_GRAYED);
        EnableMenuItem(hMenu, I_OBJECT, MF_GRAYED);
        EnableMenuItem(hMenu, RESTORE, MF_GRAYED);
        EnableMenuItem(hMenu, CUT, MF_GRAYED);
        EnableMenuItem(hMenu, COPY, MF_GRAYED);
        EnableMenuItem(hMenu, PASTE, MF_GRAYED);
        EnableMenuItem(hMenu, PASTELINK, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_PASTESPECIAL, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_INSERTOBJECT, MF_GRAYED);
//      EnableMenuItem(hMenu, GOTO, MF_GRAYED);
        EnableMenuItem(hMenu, FIND, MF_GRAYED);
        EnableMenuItem(hMenu, FINDNEXT, MF_GRAYED);
        EnableMenuItem(hMenu, PRINT, MF_GRAYED);
#ifdef JAPAN    //KKBUGFIX     //  added  18 Aug. 1992 by Hiraisi (in Japan)
        EnableMenuItem(hMenu, PRINTALL, MF_GRAYED);
#endif
        EnableMenuItem(hMenu, LINKSDIALOG, MF_GRAYED);
        DisableEditObject(GetSubMenu(hMenu, 1));
        EnableMenuItem(hMenu, EDITOBJECT, MF_GRAYED);
    }
}

void DisableEditObject(
    HMENU hEditMenu)
{
    TCHAR szObject[25];

    LoadString(hIndexInstance, IDS_OBJECTCMD, szObject, CharSizeOf(szObject));
    DeleteMenu(hEditMenu, MENUPOS_EDITOBJECT, MF_BYPOSITION);
    InsertMenu(hEditMenu, MENUPOS_EDITOBJECT, MF_BYPOSITION, EDITOBJECT, szObject);
    EnableMenuItem(hEditMenu, MENUPOS_EDITOBJECT, MF_GRAYED|MF_BYPOSITION);
}

/* Depending on the current mode, enable or disable menu items */
void OleMenuItemFix(
    HMENU hMenu,
    int Mode)
{
    int wFmt;
    int mfPaste = MF_GRAYED, mfPasteLink = MF_GRAYED, mfCutCopy;
    DWORD lSelection;
    HMENU hEditMenu;

    hEditMenu = GetSubMenu(hMenu, 1);    /* handle to Edit popup */
    if (Mode == I_TEXT)
    {
        lSelection = SendMessage(hEditWnd, EM_GETSEL, 0, 0L);
        mfCutCopy = (HIWORD(lSelection) == LOWORD(lSelection))
                            ? MF_GRAYED : MF_ENABLED;

#ifdef UNICODE
        mfPaste = IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED;
#else
        mfPaste = IsClipboardFormatAvailable(CF_TEXT) ? MF_ENABLED : MF_GRAYED;
#endif
        EnableMenuItem(hMenu, LINKSDIALOG, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_INSERTOBJECT, MF_GRAYED);
        DisableEditObject(hEditMenu);
    }
    else
    {
        mfCutCopy = (CurCard.lpObject) ? MF_ENABLED : MF_GRAYED;
        if (lpObjectUndo)
            EnableMenuItem(hMenu, UNDO, MF_ENABLED);

        if (fOLE)
        {
            if (OleQueryCreateFromClip(szPStdFile, olerender_draw, 0) == OLE_OK ||
                OleQueryCreateFromClip(szPStatic, olerender_draw, 0) == OLE_OK)
                mfPaste = MF_ENABLED;

            if (OleQueryLinkFromClip(szPStdFile, olerender_draw, 0) == OLE_OK)
                mfPasteLink = MF_ENABLED;

            /* Enable Links..., only for a linked object */
            EnableMenuItem(hMenu, LINKSDIALOG,
                (CurCard.lpObject && (CurCard.otObject == LINK)) ?
                 MF_ENABLED : MF_GRAYED);

            if (FixEditObject(hEditMenu))
                EnableMenuItem(hMenu, MENUPOS_EDITOBJECT, MF_ENABLED|MF_BYPOSITION);
            else
                DisableEditObject(hEditMenu);
            EnableMenuItem(hMenu, IDM_INSERTOBJECT, MF_ENABLED);
        }
        else
        {
            if (OpenClipboard(hIndexWnd))
            {
                wFmt = 0;
                while (wFmt = EnumClipboardFormats(wFmt))
                {
                    if (wFmt == CF_BITMAP)
                        mfPaste = MF_ENABLED;
                }
                CloseClipboard();
            }
            EnableMenuItem(hMenu, LINKSDIALOG, MF_GRAYED);
            DisableEditObject(hEditMenu);
            EnableMenuItem(hMenu, IDM_INSERTOBJECT, MF_GRAYED);
        }
    }

    EnableMenuItem(hMenu, PASTELINK, mfPasteLink);
    EnableMenuItem(hMenu, PASTE, mfPaste);
    if (fOLE && EditMode == I_OBJECT &&
        (mfPaste == MF_ENABLED || mfPasteLink == MF_ENABLED))
        EnableMenuItem(hMenu, IDM_PASTESPECIAL, MF_ENABLED);
    else
        EnableMenuItem(hMenu, IDM_PASTESPECIAL, MF_GRAYED);

    EnableMenuItem(hMenu, CUT, mfCutCopy);
    EnableMenuItem(hMenu, COPY, mfCutCopy);
}

/* Enumerate the given key and return key count */
NOEXPORT int GetKeyCount(
    HKEY hKeyVerb)
{
    int i;
    TCHAR szKey[KEYNAMESIZE];

    for (i = 0; !RegEnumKey(hKeyVerb, i, szKey, KEYNAMESIZE); i++)
        continue;
    return i;
}

/*
 * Fix the Edit.Object menu item
 * return TRUE on success,FALSE on failure.
 *
 * For the given class name, get all the verbs.
 * If (only one verb)
 *     if (Edit is the only verb)
 *         Put "<Obj Name> &Object" in the menu item.
 *     else
 *         Put "<Verb> <Obj Name> &Object" in the menu item.
 * else
 *     Put a Popup menu "<Obj Name> &Object" with all the verbs
 */
NOEXPORT BOOL FixEditObject(
    HMENU hEditMenu)
{
    HKEY hKeyVerb;
    TCHAR szObject[25];
    TCHAR szVerbPath[4*KEYNAMESIZE], /* full path for the verb */
          szShortVerb[KEYNAMESIZE],  /* verb, short form */
          szVerb[KEYNAMESIZE],       /* verb, long form */
          szClass[KEYNAMESIZE],      /* class name for the object */
          szShortClass[KEYNAMESIZE], /* short name of class       */
          szMenuStr[100];
    DWORD dwSize = KEYNAMESIZE;
    HANDLE hData=NULL, hPopupNew;
    LPTSTR lpstrData;
    int VerbCount, i;
    TCHAR szWordOrder2[10], szWordOrder3[10];

    /* If no object exists or if the object is STATIC disable Edit.Object */
    if (!CurCard.lpObject || CurCard.otObject == STATIC)
        return FALSE;
    if (OLE_OK != OleGetData(
            CurCard.lpObject,
            (OLECLIPFORMAT)(CurCard.otObject == LINK ? vcfLink : vcfOwnerLink),
            &hData))
    {
        ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
        return FALSE;
    }

    LoadString(hIndexInstance, IDS_PopupVerbs, szWordOrder2, CharSizeOf(szWordOrder2));
    LoadString(hIndexInstance, IDS_SingleVerb, szWordOrder3, CharSizeOf(szWordOrder3));

    /* Both link formats are:  "szClass0szDocument0szItem00" */

    /* Get the full class name for user to see in menu */
    if (lpstrData = GlobalLock(hData))
    {
        /* OleGetData returns ASCII!  convert to unicode */
        MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             (LPCSTR) lpstrData,
                             lstrlen(lpstrData),
                             szShortClass, 
                             CharSizeOf(szShortClass) );
        if(RegQueryValue(HKEY_CLASSES_ROOT,szShortClass,szClass,(LONG*)&dwSize))
            lstrcpy(szClass, szShortClass); /* be content with the short name */
        GlobalUnlock(hData);
    }
    else
        return FALSE;

    LoadString(hIndexInstance, IDS_OBJECTCMD, szObject, CharSizeOf(szObject));
    /* Get a handle to the Verb key */
    wsprintf(szVerbPath, TEXT("%s\\protocol\\StdFileEditing\\verb"), szShortClass );
    if (!RegOpenKey(HKEY_CLASSES_ROOT, szVerbPath, &hKeyVerb))
    {
        VerbCount = GetKeyCount(hKeyVerb);
    }
    else
      {
        VerbCount = 0;   /* failed to get verb key */
        hKeyVerb = NULL; /* Bug 10660:  Don't call RegCloseKey() if failed */
      }

    /* Delete existing menu item at Edit.Object */
    DeleteMenu(hEditMenu, MENUPOS_EDITOBJECT, MF_BYPOSITION);

    /* Build the new Edit.Object menu */
    if (VerbCount <= 1)     /* only one verb or no verbs */
    {
        if (hKeyVerb)
        {
            dwSize = KEYNAMESIZE;
            lstrcpy(szShortVerb, TEXT("0"));
            if (RegQueryValue(hKeyVerb, szShortVerb, szVerb, (LONG FAR *)&dwSize))
            {   /* assume Edit if we can't get the verb */
                LoadString(hIndexInstance, IDS_Edit, szVerb, CharSizeOf(szVerb));
            }
        }
        else    /* couldn't get verb from data base  default: Edit */
        {
            LoadString(hIndexInstance, IDS_Edit, szVerb, CharSizeOf(szVerb));
        }

        /* Show "<Verb> <Obj Name> &Object" */
        MakeMenuString(szWordOrder3, szMenuStr, szVerb, szClass, szObject);
        InsertMenu(hEditMenu, MENUPOS_EDITOBJECT, MF_BYPOSITION, OLE_VERB, szMenuStr);
    }
    else        /* multiple verbs, create a Popup */
    {
        hPopupNew = CreatePopupMenu();
        /* add the verbs to the Edit.Object popup menu
         * Assuming verbs start from zero - 0,1...*/
        for (i = 0; ;i++)
        {
            wsprintf(szShortVerb, TEXT("%d"), i);
            dwSize = KEYNAMESIZE;
/* Bug 16440:  Pass in szVerb, don't put in an & and pass in
 *             szVerb+1.  Don't change case of returned string.
 *    2 November 1991        Clark R. Cyr
 */
            if (!RegQueryValue(hKeyVerb, szShortVerb, szVerb, (LONG FAR *)&dwSize))
            {
                AppendMenu(hPopupNew, MF_BYCOMMAND, OLE_VERB+i, szVerb);
            }
            else
                break;
        }

        MakeMenuString(szWordOrder2, szMenuStr, NULL, szClass, szObject);
        InsertMenu(hEditMenu, MENUPOS_EDITOBJECT, MF_BYPOSITION | MF_POPUP,
            (UINT)hPopupNew, szMenuStr);
    }
    if (hKeyVerb)
        RegCloseKey(hKeyVerb);

    return TRUE;
}

void NEAR PASCAL MakeMenuString(TCHAR *szCtrl, TCHAR *szMenuStr, TCHAR *szVerb, TCHAR *szClass, TCHAR *szObject)
{
    TCHAR *pStr;
    register TCHAR c;

    while (c = *szCtrl++)
    {
        switch(c)
        {
            case TEXT('c'): // class
            case TEXT('C'): // class
                pStr = szClass;
            break;
            case TEXT('v'): // class
            case TEXT('V'): // class
                pStr = szVerb;
            break;
            case TEXT('o'): // object
            case TEXT('O'): // object
                pStr = szObject;
            break;
            default:
                *szMenuStr++ = c;
                *szMenuStr = TEXT('\0'); // just in case
            continue;
        }

        if (pStr) // should always be true
        {
            lstrcpy(szMenuStr,pStr);
            szMenuStr += lstrlen(pStr); // point to '\0'
        }
    }
}
