/*
 * PROPBOX.C
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * Dialog box for creating a new map.
 */
/* Revision history:
   March 92 Ported to 16/32 common code by Laurie Griffiths (LaurieGr)
*/

#include "preclude.h"
#include <windows.h>
#include <mmsystem.h>
#if defined(WIN32)
#include <port1632.h>
#endif
#include <string.h>
#include "hack.h"
#include "midimap.h"
#include "cphelp.h"
#include "midi.h"
#include "extern.h"


/* How about a f***ing comment ??? */
static UINT PackString(LPSTR szStr)
{
    UINT i;
    LPSTR lpstrBegin,lpstrPtr;

    // remove leading whitespace

    for (lpstrBegin=lpstrPtr=szStr; ISSPACE(*lpstrPtr) ; lpstrPtr++);

    // move backwards from the end

    for(i = lstrlen(szStr)-1; i > 0 && ISSPACE(szStr[i]);i--);

    if (lpstrPtr > szStr+i )
        return 0;

    szStr[i+1] = '\0';

    // while(*szStr++ = *lpstrPtr++);               // Was coded like this - I wonder why?
    strcpy(szStr, lpstrPtr);

    return lstrlen(lpstrBegin);
}



BOOL FAR PASCAL _loadds PropBox(
        HWND    hdlg,
        UINT    uMessage,
        WPARAM  wParam,
        LPARAM  lParam)
{
        static BOOL     fEnabled;
        DWORD   dwExists;
        int     idName = 0;
        int     idHelp = 0;
        char    szName [24],
                szCaption[80],
                szTextBuf [256],
                szRsrc [256],
                szBuf [MMAP_MAXNAME];

        switch (uMessage)
        {
        case WM_INITDIALOG:
                switch (iMap)
                {
                        case MMAP_SETUP:
                                LoadString(hLibInst, IDS_NEW_SETUP, szCaption, sizeof(szCaption));
                                break;
                        case MMAP_PATCH:
                                LoadString(hLibInst, IDS_NEW_PATCH, szCaption, sizeof(szCaption));
                                break;
                        case MMAP_KEY:
                                LoadString(hLibInst, IDS_NEW_KEY, szCaption, sizeof(szCaption));
                                break;
                }

                SetWindowText(hdlg, szCaption);
#ifdef FSAVEAS
                if (fSaveAs) {
                        SetDlgItemText(hdlg, ID_PROPNAME, szCurrent);
                        SetDlgItemText(hdlg, ID_PROPDESC, szCurDesc);
                        fEnabled = TRUE;
                } else
#endif
                        EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);

                SendDlgItemMessage(hdlg, ID_PROPNAME, EM_LIMITTEXT,
                        (WPARAM)(MMAP_MAXNAME - 1), (LPARAM)0);
                SendDlgItemMessage(hdlg, ID_PROPDESC, EM_LIMITTEXT,
                        (WPARAM)(MMAP_MAXDESC - 1), (LPARAM)0);
                fEnabled = FALSE;
                break;
        case WM_COMMAND:
                switch (LOWORD(wParam)) {
                case  IDH_DLG_MIDI_NEW:
                  goto DoHelp;

                case IDOK:
                        GetDlgItemText(hdlg, ID_PROPNAME, szBuf,
                                MMAP_MAXNAME);
            if (PackString(szBuf) == 0)
                return FALSE;
                       //fSaveAs is never set
//!!                    if (/* !fSaveAs || */ lstrcmpi(szBuf, szCurrent)) {
                                dwExists = mapExists(iMap, szBuf);
                                if (LOWORD(dwExists) != MMAPERR_SUCCESS) {
                                        VShowError(hdlg, LOWORD(dwExists));
                                        EndDialog(hdlg, FALSE);
                                        return TRUE;
                                }
                                if (HIWORD(dwExists)) {
                                        switch (iMap)
                                        {
                                                case MMAP_SETUP:
                                                        idName = IDS_SETUP;
                                                        break;
                                                case MMAP_PATCH:
                                                        idName = IDS_PATCH;
                                                        break;
                                                case MMAP_KEY:
                                                        idName = IDS_KEY;
                                                        break;
                                        }
                                        LoadString(hLibInst, idName, szName,
                                                sizeof(szName));
//                                      AnsiUpperBuff(szName, 1);
                                        LoadString(hLibInst, IDS_DUPLICATE,
                                                szRsrc, sizeof(szRsrc));
                                        wsprintf(szTextBuf, szRsrc, (LPSTR)
                                                szName);
                                        if (IDNO == MessageBox(hdlg,
                                                        szTextBuf, szBuf,
                                                        MB_YESNO |
                                                        MB_ICONEXCLAMATION))
                                                break;
                                        fNew = FALSE;
                                }
                                lstrcpy(szCurrent, szBuf);
//!!                    }
                        GetDlgItemText(hdlg, ID_PROPDESC, szCurDesc,
                                MMAP_MAXDESC);
                        EndDialog(hdlg, TRUE);
                        break;
                case IDCANCEL:
                        EndDialog(hdlg, FALSE);
                        break;
                case ID_PROPNAME:
                        if (!GetDlgItemText(hdlg, ID_PROPNAME, szBuf,
                              MMAP_MAXNAME))
                        {
                                if (fEnabled)
                                {
                                        EnableWindow(GetDlgItem(hdlg, IDOK),
                                                FALSE);
                                        fEnabled = FALSE;
                                }
                        } else
                                if (!fEnabled)
                                {
                                  EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
                                  fEnabled = TRUE;
                                }
                        break;
                default:
                        return FALSE;
                }
                break;
        default:
                if (uMessage == uHelpMessage)
                {
DoHelp:
                   switch (iMap)
                   {
                        case MMAP_SETUP:
                                idHelp = IDH_DLG_MIDI_SETUPNEW;
                                break;
                        case MMAP_PATCH:
                                idHelp = IDH_DLG_MIDI_PATCHNEW;
                                break;
                        case MMAP_KEY:
                                idHelp = IDH_DLG_MIDI_KEYNEW;
                                break;
                   }
                   WinHelp(hWnd, szMidiHlp, HELP_CONTEXT, idHelp);
                   return TRUE;
                }
                else
                         return FALSE;
                break;
        }
        return TRUE;
} /* PropBox */
