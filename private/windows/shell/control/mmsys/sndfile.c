/*
 ***************************************************************
 *  sndfile.c
 *
 *  This file contains the code to fill up the list and combo boxes,
 *  show the RIFF Dibs and the current sound mappings 
 *
 *  Copyright 1993, Microsoft Corporation     
 *
 *  History:
 *
 *    07/94 - VijR (Created)
 *        
 ***************************************************************
 */
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <ole2.h>
#include <prsht.h>
#include <cpl.h>
#include "mmcpl.h"
#include "draw.h"
#include "sound.h"

/*
 ***************************************************************
 * Globals
 ***************************************************************
 */
HSOUND ghse;


/*
 ***************************************************************
 * extern
 ***************************************************************
 */
extern TCHAR        gszMediaDir[];
extern TCHAR        gszCurDir[];
extern BOOL        gfWaveExists;   // indicates wave device in system.
extern BOOL        gfChanged;      // Is set TRUE if any changes are made
extern BOOL        gfNewScheme;  

//Globals used in painting disp chunk display.
extern HBITMAP     ghDispBMP;
extern HPALETTE    ghPal;                     
extern HBITMAP     ghIconBMP;
extern HTREEITEM   ghOldItem;

/*
 ***************************************************************
 * Defines 
 ***************************************************************
 */                                                
#define DF_PM_SETBITMAP    (WM_USER+1)   
#define FOURCC_INFO mmioFOURCC('I','N','F','O')
#define FOURCC_DISP mmioFOURCC('D','I','S','P')
#define FOURCC_INAM mmioFOURCC('I','N','A','M')
#define FOURCC_ISBJ mmioFOURCC('I','S','B','J')
#define MAXDESCLEN    220

/*
 ***************************************************************
 * Prototypes
 ***************************************************************
 */
HANDLE PASCAL GetRiffDisp        (HMMIO);
BOOL PASCAL ShowSoundMapping    (HWND, PEVENT);
BOOL PASCAL ShowSoundDib        (HWND, LPTSTR,BOOL);
BOOL PASCAL ChangeSoundMapping  (HWND, LPTSTR, PEVENT);
BOOL PASCAL PlaySoundFile       (HWND, LPTSTR);
BOOL PASCAL ChangeSetting        (LPTSTR*, LPTSTR);
LPTSTR PASCAL NiceName(LPTSTR sz, BOOL fNukePath);

// Stuff in dib.c
//
HPALETTE WINAPI  bmfCreateDIBPalette(HANDLE);
HBITMAP  WINAPI  bmfBitmapFromDIB(HANDLE, HPALETTE);

// Stuff in drivers.c
//
LPTSTR lstrchr (LPTSTR, TCHAR);
int lstrnicmp (LPTSTR, LPTSTR, size_t);

// Stuff in scheme.c
//
void PASCAL AddMediaPath        (LPTSTR, LPTSTR);

/*
 ***************************************************************
 ***************************************************************
 */
STATIC void NEAR PASCAL ChopPath(LPTSTR lpszFile)
{
    TCHAR szTmp[MAX_PATH];
    size_t cchTest = lstrlen (gszCurDir);

    ExpandEnvironmentStrings(lpszFile, (LPTSTR)szTmp, MAXSTR);
    lstrcpy(lpszFile,szTmp);

    if (gszCurDir[ cchTest-1 ] == TEXT('\\'))
       --cchTest;
    
    lstrcpy((LPTSTR)szTmp, lpszFile);
    if (!lstrnicmp((LPTSTR)szTmp, (LPTSTR)gszCurDir, cchTest))
    {
        if (szTmp[ cchTest ] == TEXT('\\'))
        {
            lstrcpy(lpszFile, (LPTSTR)(szTmp+cchTest+1));
        }
    }
}
/*
 ***************************************************************
 * QualifyFileName
 *
 * Description:
 *    Verifies the existence and readability of a file.
 *
 * Parameters:
 *    LPTSTR    lpszFile    - name of file to check.
 *    LPTSTR    lpszPath    - returning full pathname of file.     
 *  int        csSize        - Size of return buffer
 *
 * Returns:    BOOL
 *         True if absolute path exists
 *
 ***************************************************************
 */

BOOL PASCAL QualifyFileName(LPTSTR lpszFile, LPTSTR lpszPath, int cbSize, BOOL fTryCurDir)
{
    BOOL     fErrMode;
    BOOL     f = FALSE;
    BOOL     fHadEnvStrings;
    TCHAR     szTmpFile[MAXSTR];
    int len;
    BOOL fTriedCurDir;
    TCHAR*      pszFilePart;
    HFILE   hFile;

    if (!lpszFile)
        return FALSE;

    fHadEnvStrings = (lstrchr (lpszFile, TEXT('%')) != NULL) ? TRUE : FALSE;

    ExpandEnvironmentStrings (lpszFile, (LPTSTR)szTmpFile, MAXSTR);
    len =  lstrlen((LPTSTR)szTmpFile)+1;

    fErrMode = SetErrorMode(TRUE);  // we will handle errors

    AddExt (szTmpFile, cszWavExt);

    fTriedCurDir = FALSE;

TryOpen:
    hFile = (HFILE)HandleToUlong(CreateFile(szTmpFile,GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if (-1 != hFile)
    {
        if (fHadEnvStrings)
            lstrcpyn(lpszPath, lpszFile, cbSize);
        else
            GetFullPathName(szTmpFile,cbSize/sizeof(TCHAR),lpszPath,&pszFilePart);
        f = TRUE;

        CloseHandle((HANDLE)hFile);
    }
    else
    /*
    ** If the test above failed, we try converting the name to OEM
    ** character set and try again.
    */
    {
        /*
        ** First, is it in MediaPath?
        **
        */
        if (lstrchr (lpszFile, TEXT('\\')) == NULL)
        {
            TCHAR szCurDirFile[MAXSTR];
            AddMediaPath (szCurDirFile, lpszFile);
            if (-1 != (HFILE)HandleToUlong(CreateFile(szCurDirFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)))
            {
                GetFullPathName(szCurDirFile,cbSize/sizeof(TCHAR),lpszPath,&pszFilePart);
                f = TRUE;
                goto DidOpen;
            }
        }

        //AnsiToOem((LPTSTR)szTmpFile, (LPTSTR)szTmpFile);
        if (-1 != (HFILE)HandleToUlong(CreateFile(szTmpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)))
        {
            if (fHadEnvStrings)
                lstrcpyn(lpszPath, lpszFile, cbSize);
            else
                GetFullPathName(szTmpFile,cbSize/sizeof(TCHAR),lpszPath,&pszFilePart);
            f = TRUE;
        }
        else if (fTryCurDir && !fTriedCurDir)
        {
            TCHAR szCurDirFile[MAXSTR];

            //OemToAnsi((LPTSTR)szTmpFile, (LPTSTR)szTmpFile);
            lstrcpy (szCurDirFile, gszCurDir);
            lstrcat (szCurDirFile, cszSlash);
            lstrcat (szCurDirFile, szTmpFile);
            lstrcpy((LPTSTR)szTmpFile, (LPTSTR)szCurDirFile);
            fTriedCurDir = TRUE;
            goto  TryOpen;
        }
    }

DidOpen:
    SetErrorMode(fErrMode);
    return f;
}



/*
 ***************************************************************
 * ChangeSoundMapping
 *
 * Description:
 *      Change the sound file associated with a sound
 *
 * Parameters:
 *      HWND    hDlg   - handle to dialog window.
 *      LPTSTR    lpszFile    - New filename for current event
 *      LPTSTR    lpszDir    - New foldername for current event     
 *      LPTSTR    lpszPath    - New absolute path for file
 *
 * Returns:        BOOL
 *      
 ***************************************************************
 */
BOOL PASCAL ChangeSoundMapping(HWND hDlg, LPTSTR lpszPath, PEVENT pEvent)
{
    TCHAR    szValue[MAXSTR];    
    
    if (!pEvent)
    {
        if(!ghse)
            EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);            
        EnableWindow(GetDlgItem(hDlg, IDC_SOUND_FILES), FALSE);
        ShowSoundMapping(hDlg,NULL);
        return TRUE;
    }
    szValue[0] = TEXT('\0');
    if (!ChangeSetting((LPTSTR *)&(pEvent->pszPath), lpszPath))
        return FALSE;        
    if(!ghse)
        EnableWindow(GetDlgItem(hDlg, ID_PLAY), TRUE);            
    EnableWindow(GetDlgItem(hDlg, IDC_SOUND_FILES), TRUE);            
    ShowSoundMapping(hDlg,pEvent);
    gfChanged = TRUE;
    gfNewScheme = TRUE;
    PropSheet_Changed(GetParent(hDlg),hDlg);
    return TRUE;
}

STATIC void SetTreeStateIcon(HWND hDlg, int iImage)
{
    TV_ITEM tvi;
    HWND hwndTree = GetDlgItem(hDlg, IDC_EVENT_TREE);
    HTREEITEM hti;

    if (ghOldItem)
        hti = ghOldItem;
    else
        hti = TreeView_GetSelection(hwndTree);
    if (hti)
    {
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.hItem = hti;
        tvi.iImage = tvi.iSelectedImage = iImage;
        TreeView_SetItem(hwndTree, &tvi);
        RedrawWindow(hwndTree, NULL, NULL, RDW_ERASE|RDW_ERASENOW|RDW_INTERNALPAINT|RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

/*
 ***************************************************************
 * ShowSoundMapping
 *
 * Description:
 *      Highlights the label and calls ShowSoundDib to display the Dib 
 *        associated with the current event.
 *
 * Parameters:
 *    HWND    hDlg   - handle to dialog window.
 *
 * Returns:    BOOL
 *      
 ***************************************************************
 */
BOOL PASCAL ShowSoundMapping(HWND hDlg, PEVENT pEvent)
{
    TCHAR    szOut[MAXSTR];            

    EnableWindow(GetDlgItem(hDlg, ID_DETAILS), FALSE);
    if (!pEvent)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SOUND_FILES), FALSE);            
        EnableWindow(GetDlgItem(hDlg, ID_BROWSE), FALSE);            
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NAME), FALSE);    
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PREVIEW), FALSE);    
    //    wsprintf((LPTSTR)szCurSound, (LPTSTR)gszSoundGrpStr, (LPTSTR)gszNull);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SOUND_FILES), TRUE);            
        EnableWindow(GetDlgItem(hDlg, ID_BROWSE), TRUE);            
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_NAME), TRUE);    
        EnableWindow(GetDlgItem(hDlg, IDC_STATIC_PREVIEW), TRUE);    
    //    wsprintf((LPTSTR)szCurSound, (LPTSTR)gszSoundGrpStr, (LPTSTR)pEvent->pszEventLabel);
    }
    //SetWindowText(GetDlgItem(hDlg, IDC_SOUNDGRP), (LPTSTR)szCurSound);
    //RedrawWindow(GetDlgItem(hDlg, IDC_EVENT_TREE), NULL, NULL, RDW_ERASE|RDW_ERASENOW|RDW_INTERNALPAINT|RDW_INVALIDATE | RDW_UPDATENOW);

    if (!pEvent || !QualifyFileName(pEvent->pszPath, szOut, sizeof(szOut), FALSE))
    {
        int iLen;

        if(!ghse)
            EnableWindow(GetDlgItem(hDlg, ID_PLAY), FALSE);
        SendDlgItemMessage(hDlg, ID_DISPFRAME, DF_PM_SETBITMAP, 0, 0L);
        
        if(pEvent)
            iLen = lstrlen(pEvent->pszPath);
        if (pEvent && iLen > 0)
        {
            if (iLen < 5)
            {
                lstrcpy(pEvent->pszPath, gszNull);
                gfChanged = TRUE;
                gfNewScheme = TRUE;
                PropSheet_Changed(GetParent(hDlg),hDlg);
            }
            else
            {
                TCHAR szMsg[MAXSTR];
                TCHAR szTitle[MAXSTR];

                LoadString(ghInstance, IDS_NOSNDFILE, szTitle, sizeof(szTitle)/sizeof(TCHAR));
                wsprintf(szMsg, szTitle, pEvent->pszPath);
                LoadString(ghInstance, IDS_NOSNDFILETITLE, szTitle, sizeof(szTitle)/sizeof(TCHAR));
                if (MessageBox(hDlg, szMsg, szTitle, MB_YESNO) == IDNO)
                {
                    lstrcpy(pEvent->pszPath, gszNull);
                    ComboBox_SetText(GetDlgItem(hDlg, IDC_SOUND_FILES), gszNone);                
                    gfChanged = TRUE;
                    gfNewScheme = TRUE;
                    PropSheet_Changed(GetParent(hDlg),hDlg);
                    if (pEvent && pEvent->fHasSound)
                    {
                        SetTreeStateIcon(hDlg, 2);
                        pEvent->fHasSound = FALSE;
                    }
                }
                else
                {
                    lstrcpy(szOut ,pEvent->pszPath); 
                    ChopPath((LPTSTR)szOut);
                    NiceName((LPTSTR)szOut, FALSE);
                    ComboBox_SetText(GetDlgItem(hDlg, IDC_SOUND_FILES), szOut);
                    if (!pEvent->fHasSound)
                    {
                        SetTreeStateIcon(hDlg, 1);
                        pEvent->fHasSound = TRUE;
                    }
                }
            }
        }
        else
        {
            ComboBox_SetText(GetDlgItem(hDlg, IDC_SOUND_FILES), gszNone);                
            if (pEvent && pEvent->fHasSound)
            {
                SetTreeStateIcon(hDlg, 2);
                pEvent->fHasSound = FALSE;
            }
        }
    }
    else
    {
        if(!ghse)
            EnableWindow(GetDlgItem(hDlg, ID_PLAY),gfWaveExists);
        ShowSoundDib(hDlg, szOut, FALSE);
        ChopPath((LPTSTR)szOut);
        NiceName((LPTSTR)szOut, FALSE);
        ComboBox_SetText(GetDlgItem(hDlg, IDC_SOUND_FILES), szOut);
        if (!pEvent->fHasSound)
        {
            SetTreeStateIcon(hDlg, 1);
            pEvent->fHasSound = TRUE;
        }

    }
    return TRUE;
}

/*
 ***************************************************************
 * ShowSoundDib
 *
 * Description:
 *      Opens the file and reads the dib out of the info chunk 
 *      and displays the dib
 *
 * Parameters:
 *    HWND        hDlg    - Handle to Window
 *      LPTSTR        lpszFile  -     entire pathname of file to display.
 *
 * Returns:        BOOL
 *      
 ***************************************************************
 */

STATIC BOOL PASCAL ShowSoundDib(HWND hDlg, LPTSTR lpszFile, BOOL fDetailsDlg)
{
    HANDLE  hDib = NULL;
    HMMIO   hmmio;
        
    EnableWindow(GetDlgItem(hDlg, ID_DETAILS), TRUE);

    if (ghDispBMP && !fDetailsDlg)
    {
        DeleteObject(ghDispBMP);
        ghDispBMP = 0;
    }
    if (!lpszFile || !*lpszFile)
        goto ERR_DISP;
    

    /* Open the file */
    hmmio = mmioOpen(lpszFile, NULL, MMIO_ALLOCBUF | MMIO_READ);
    if (hmmio)
    {
        hDib = GetRiffDisp(hmmio);
        mmioClose(hmmio, 0);        
    }
    if (fDetailsDlg)
    {
        SendDlgItemMessage(hDlg, (int)ID_DISPFRAME, (UINT)DF_PM_SETBITMAP, (WPARAM)ghDispBMP, 
                                                                (LPARAM)ghPal);
        return TRUE;
    }
    
    if (ghPal)
        DeleteObject(ghPal);

    if (hDib)
    {
        ghPal = bmfCreateDIBPalette(hDib);
        ghDispBMP = bmfBitmapFromDIB(hDib, ghPal);

        GlobalUnlock(hDib);
        hDib = GlobalFree(hDib);
    }

    if (!ghDispBMP)
    {
        if (!ghIconBMP)
        {                                  
            HICON hIcon; 

            hIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_AUDIO));
            ghIconBMP = bmfBitmapFromIcon (hIcon, GetSysColor(COLOR_WINDOW));
            DestroyIcon(hIcon);
        }
        SendDlgItemMessage(hDlg, (int)ID_DISPFRAME, (UINT)DF_PM_SETBITMAP, (WPARAM)ghIconBMP, 
                                                              (LPARAM)NULL);

    }
    else
    {
ERR_DISP:
    SendDlgItemMessage(hDlg, (int)ID_DISPFRAME, (UINT)DF_PM_SETBITMAP, (WPARAM)ghDispBMP, 
                                                                (LPARAM)ghPal);
    }
    return TRUE;
}

/*
 ***************************************************************
 * PlaySoundFile
 *
 * Description:
 *        Plays the given sound file.
 *
 * Parameters:
 *    HWND  hDlg   - Window handle
 *      LPTSTR    lpszFile - absolute path of File to play.
 *
 * Returns:    BOOL 
 *  
 ***************************************************************
 */
BOOL PASCAL PlaySoundFile(HWND hDlg, LPTSTR lpszFile)
{
        
    TCHAR     szOut[MAXSTR];            
    TCHAR     szTemp[MAXSTR];            
        
    if (!QualifyFileName(lpszFile, szTemp, sizeof(szTemp), FALSE))
        ErrorBox(hDlg, IDS_ERRORPLAY, lpszFile);

    else
    {
        MMRESULT err = MMSYSERR_NOERROR;

        ExpandEnvironmentStrings (szTemp, szOut, MAXSTR);

        if((soundOpen(szOut, hDlg, &ghse) != MMSYSERR_NOERROR) || ((err = soundPlay(ghse)) != MMSYSERR_NOERROR))
        {
            if (err >= (MMRESULT)MMSYSERR_LASTERROR)
                ErrorBox(hDlg, IDS_ERRORUNKNOWNPLAY, lpszFile);
            else if (err ==  (MMRESULT)MMSYSERR_ALLOCATED)
                ErrorBox(hDlg, IDS_ERRORDEVBUSY, lpszFile);
            else
                ErrorBox(hDlg, IDS_ERRORFILEPLAY, lpszFile);
            ghse = NULL;
        }
    }
    return TRUE;    
}

/*
 ***************************************************************
 * ChangeSetting
 *
 * Description:
 *        Displays the labels of all the links present in the lpszDir folder
 *        in the LB_SOUNDS listbox
 *
 * Parameters:
 *    HWND  hDlg   - Window handle
 *      LPTSTR lpszDir -  Name of sound folder whose files must be displayed.
 *
 * Returns:    BOOL
 *  
 ***************************************************************
 */
BOOL PASCAL ChangeSetting(LPTSTR *npOldString, LPTSTR lpszNew)
{
    int len =  (lstrlen(lpszNew)*sizeof(TCHAR))+sizeof(TCHAR);

    if (*npOldString)
    {
        *npOldString = (LPTSTR)LocalReAlloc((HLOCAL)*npOldString, 
                                    len, LMEM_MOVEABLE);
    }
    else
    {
        DPF("Current file Does not exist\n");        
        *npOldString = (LPTSTR)LocalAlloc(LPTR, len);
    }

    if (*npOldString == NULL)
    {
        DPF("ReAlloc Failed\n");        
        return FALSE;            
    }                                                
    lstrcpy(*npOldString, lpszNew);
    DPF("New file is %s\n", (LPTSTR)*npOldString);    
    return TRUE;
}



STATIC HANDLE PASCAL GetRiffDisp(HMMIO hmmio)
{
    MMCKINFO    ck;
    MMCKINFO    ckRIFF;
    HANDLE    h = NULL;
    LONG        lSize;
    DWORD       dw;

    mmioSeek(hmmio, 0, SEEK_SET);

    /* descend the input file into the RIFF chunk */
    if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0)
        goto error;

    if (ckRIFF.ckid != FOURCC_RIFF)
        goto error;

    while (!mmioDescend(hmmio, &ck, &ckRIFF, 0))
    {
        if (ck.ckid == FOURCC_DISP)
        {
            /* Read dword into dw, break if read unsuccessful */
            if (mmioRead(hmmio, (LPVOID)&dw, sizeof(dw)) != (LONG)sizeof(dw))
                goto error;

            /* Find out how much memory to allocate */
            lSize = ck.cksize - sizeof(dw);

            if ((int)dw == CF_DIB && h == NULL)
            {
                /* get a handle to memory to hold the description and 
                    lock it down */
                
                if ((h = GlobalAlloc(GHND, lSize+4)) == NULL)
                    goto error;

                if (mmioRead(hmmio, GlobalLock(h), lSize) != lSize)
                    goto error;
            }
        }
        //
        // if we have both a picture and a title, then exit.
        //
        if (h != NULL)
            break;

        /* Ascend so that we can descend into next chunk
         */
        if (mmioAscend(hmmio, &ck, 0))
            break;
    }

    goto exit;

error:
    if (h)
    {
        GlobalUnlock(h);
        GlobalFree(h);
    }
    h = NULL;

exit:
    return h;
}
