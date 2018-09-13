/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   config.c - Multimedia Systems Media Control Interface
            driver for AVI - configuration dialog.

*****************************************************************************/
#include "graphic.h"
#include "cnfgdlg.h"

#define comptypeNONE            mmioFOURCC('n','o','n','e')

#ifndef WIN32
#define SZCODE char _based(_segname("_CODE"))
#else
#define SZCODE TCHAR
#endif

SZCODE szDEFAULTVIDEO[] =	TEXT("DefaultVideo");
SZCODE szVIDEOWINDOW[] =	TEXT("Window");
SZCODE szVIDEO240[] =           TEXT("240 Line Fullscreen");
SZCODE szSEEKEXACT[] =		TEXT("AccurateSeek");
SZCODE szZOOMBY2[] =		TEXT("ZoomBy2");
//SZCODE szSTUPIDMODE[] =         TEXT("DontBufferOffscreen");
SZCODE szSKIPFRAMES[] =         TEXT("SkipFrames");
SZCODE szUSEAVIFILE[] =         TEXT("UseAVIFile");

SZCODE szIni[] =		TEXT("MCIAVI");

SZCODE sz1[] = TEXT("1");
SZCODE sz0[] = TEXT("0");

SZCODE szIntl[] =               TEXT("Intl");
SZCODE szDecimal[] =            TEXT("sDecimal");
SZCODE szThousand[] =           TEXT("sThousand");

/* Make sure we only have one configure box up at a time.... */
HWND	ghwndConfig = NULL;

#ifdef DEBUG
BOOL FAR PASCAL _LOADDS DebugDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    static NPMCIGRAPHIC npMCI = NULL;
    HWND cb;
    int i;
    TCHAR ach[40];

    extern int	giDebugLevel;	// current debug level (common.h)

    switch (wMsg) {
        case WM_INITDIALOG:
            npMCI = (NPMCIGRAPHIC)(UINT)lParam;

            //
            // set the DEBUG stuff.
            //
            CheckDlgButton(hDlg, ID_SCREEN, GetProfileInt(TEXT("DrawDib"), TEXT("DecompressToScreen"), 2));
            CheckDlgButton(hDlg, ID_BITMAP, GetProfileInt(TEXT("DrawDib"), TEXT("DecompressToBitmap"), 2));
            CheckDlgButton(hDlg, ID_SUCKS,  GetProfileInt(TEXT("DrawDib"), TEXT("DrawToBitmap"), 2));
            CheckDlgButton(hDlg, ID_USE_AVIFILE, (npMCI->dwOptionFlags & MCIAVIO_USEAVIFILE) != 0);
            SetScrollRange(GetDlgItem(hDlg, ID_RATE), SB_CTL, 0, 1000, TRUE);
            SetScrollPos(GetDlgItem(hDlg, ID_RATE), SB_CTL, (int)npMCI->PlaybackRate,TRUE);

            cb = GetDlgItem(hDlg, ID_LEVEL);
            SetWindowFont(cb, GetStockObject(ANSI_VAR_FONT), FALSE);

            ComboBox_AddString(cb, "0 - None");
	    ComboBox_AddString(cb, "1 - Level 1");
            ComboBox_AddString(cb, "2 - Level 2");
            ComboBox_AddString(cb, "3 - Level 3");
            ComboBox_AddString(cb, "4 - Level 4");

            ComboBox_SetCurSel(cb, giDebugLevel);

            #include "..\verinfo\usa\verinfo.h"
            wsprintf(ach,TEXT("Build %d.%02d.%02d"), MMVERSION, MMREVISION, MMRELEASE);
            SetDlgItemText(hDlg, ID_BUILD, ach);

            return TRUE;

        case WM_HSCROLL:
            i = GetScrollPos((HWND)HIWORD(lParam),SB_CTL);

            switch (LOWORD(wParam)) {
                case SB_LINEDOWN:      i+=10;  break;
                case SB_LINEUP:        i-=10;  break;
                case SB_PAGEDOWN:      i+=100; break;
                case SB_PAGEUP:        i-=100; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: i = LOWORD(lParam); break;
            }

            if (i<0) i=0;
            if (i>1000) i=1000;

            SetScrollPos((HWND)HIWORD(lParam),SB_CTL,i,TRUE);
            break;

	case WM_COMMAND:
	    switch (wParam) {
		case IDCANCEL:
                case IDOK:
                    i = IsDlgButtonChecked(hDlg, ID_SUCKS);
                    if (i == 2)
                        WriteProfileString(TEXT("DrawDib"), TEXT("DrawToBitmap"), NULL);
                    else
                        WriteProfileString(TEXT("DrawDib"), TEXT("DrawToBitmap"), (LPTSTR)(i ? sz1 : sz0));

                    i = IsDlgButtonChecked(hDlg, ID_SCREEN);
                    if (i == 2)
                        WriteProfileString(TEXT("DrawDib"), TEXT("DecompressToScreen"),NULL);
                    else
                        WriteProfileString(TEXT("DrawDib"), TEXT("DecompressToScreen"),(LPTSTR)(i ? sz1 : sz0));

                    i = IsDlgButtonChecked(hDlg, ID_BITMAP);
                    if (i == 2)
                        WriteProfileString(TEXT("DrawDib"), TEXT("DecompressToBitmap"),NULL);
                    else
                        WriteProfileString(TEXT("DrawDib"), TEXT("DecompressToBitmap"),(LPTSTR)(i ? sz1 : sz0));

                    npMCI->PlaybackRate = GetScrollPos(GetDlgItem(hDlg, ID_RATE), SB_CTL);

                    giDebugLevel = ComboBox_GetCurSel(GetDlgItem(hDlg, ID_LEVEL));
                    wsprintf(ach,TEXT("%d"),giDebugLevel);
		    WriteProfileString(TEXT("Debug"),TEXT("MCIAVI"),ach);

                    if (IsDlgButtonChecked(hDlg, ID_USE_AVIFILE))
                        npMCI->dwOptionFlags |= MCIAVIO_USEAVIFILE;
                    else
                        npMCI->dwOptionFlags &= ~MCIAVIO_USEAVIFILE;

                    EndDialog(hDlg, TRUE);
                    break;

                case ID_RATE:
                    break;
	    }
	    break;
    }
    return FALSE;
}
#endif // DEBUG

LONG AVIGetFileSize(LPTSTR szFile)
{
    LONG        lSize;

#ifdef WIN32
    DWORD  dwHigh;
    HANDLE      fh;
    fh = CreateFile(szFile, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, 0, 0);

    if (fh == (HANDLE)HFILE_ERROR)
        return 0;

    lSize = (LONG)GetFileSize(fh, &dwHigh);
    CloseHandle(fh);
#else
    OFSTRUCT    of;
    HFILE       fh;
    fh = OpenFile(szFile, &of, OF_READ | OF_SHARE_DENY_NONE);

    if (fh == HFILE_ERROR)
        fh = OpenFile(szFile, &of, OF_READ);

    if (fh == HFILE_ERROR)
        return 0;

    lSize = _llseek(fh, 0, SEEK_END);
    _lclose(fh);
#endif

    return lSize;
}

//
//fill in the info box.
//
BOOL ConfigInfo(NPMCIGRAPHIC npMCI, HWND hwnd)
{
    PTSTR  pchInfo;
    LONG  len;
    DWORD time;
    PTSTR pch;
    TCHAR ach[80];
    TCHAR achDecimal[4];
    TCHAR achThousand[4];
    int i;

    if (npMCI == NULL)
        return FALSE;

    achDecimal[0]  = '.'; achDecimal[1] = 0;
    achThousand[0] = ','; achThousand[1] = 0;

    GetProfileString(szIntl, szDecimal,  achDecimal,  achDecimal,   sizeof(achDecimal));
    GetProfileString(szIntl, szThousand, achThousand, achThousand,  sizeof(achThousand));

    pchInfo = (PTSTR)LocalAlloc(LPTR, 8192*sizeof(TCHAR));

    if (pchInfo == NULL)
        return FALSE;

    pch = pchInfo;

    //
    // display file name
    //
    //  File: full path.
    //
    LoadString(ghModule, INFO_FILE, ach, sizeof(ach));
    pch += wsprintf(pch, ach, (LPTSTR)npMCI->szFilename);

    //
    // display file type
    //
    //  Type: type
    //
    if (npMCI->pf) {
#ifdef USEAVIFILE
	AVIFILEINFO info;
	LoadString(ghModule, INFO_FILETYPE, ach, sizeof(ach));
        npMCI->pf->lpVtbl->Info(npMCI->pf, &info, sizeof(info));
        pch += wsprintf(pch, ach, (LPSTR)info.szFileType);
#endif
    }
    else if (npMCI->dwFlags & MCIAVI_NOTINTERLEAVED) {
        LoadString(ghModule, INFO_FILETYPE_AVI, pch, 80);
        pch += lstrlen(pch);
    }
    else {
        LoadString(ghModule, INFO_FILETYPE_INT, pch, 80);
        pch += lstrlen(pch);
    }

    //
    // display length
    //
    //  Length: ## Frames (#.## sec)
    //
    LoadString(ghModule, INFO_LENGTH, ach, sizeof(ach));

    time = muldivru32(npMCI->lFrames, npMCI->dwMicroSecPerFrame, 1000L);

    pch += wsprintf(pch, ach,
        npMCI->lFrames, time/1000, achDecimal[0], (int)(time%1000));

    //
    // display the average data rate
    //
    //  Data Rate: #k/sec
    //
    len = npMCI->dwBigListEnd - npMCI->dwMovieListOffset;

    if (len == 0)
        len = AVIGetFileSize(npMCI->szFilename);

    if (len > 0) {
        LoadString(ghModule, INFO_DATARATE, ach, sizeof(ach));
        pch += wsprintf(pch, ach,muldiv32(len,1000,time) / 1024);
    }

    //
    // dump info on each stream.
    //
    for (i=0; i<npMCI->streams; i++) {

        STREAMINFO *psi = SI(i);

        LONG rate = muldiv32(psi->sh.dwRate,1000,psi->sh.dwScale);

        //
        // display video format
        //
        //  Video: MSVC 160x120x8 (cram) 15.000 fps
        //
        if (psi->lpFormat && psi->sh.fccType == streamtypeVIDEO) {

            LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)psi->lpFormat;

            DWORD fccHandler = psi->sh.fccHandler;
            DWORD fccFormat  = lpbi->biCompression;

            if (fccHandler == 0)        fccHandler = comptypeNONE;
            if (fccHandler == BI_RLE8)  fccHandler = comptypeRLE;

            if (fccFormat == 0)         fccFormat = comptypeNONE;
            if (fccFormat == BI_RLE8)   fccFormat = comptypeRLE;

            LoadString(ghModule, INFO_VIDEOFORMAT, ach, sizeof(ach));

            pch += wsprintf(pch, ach,
                (LPVOID)&fccHandler,
                (int)lpbi->biWidth,
                (int)lpbi->biHeight,
                (int)lpbi->biBitCount,
                (LPVOID)&fccFormat,
                (UINT)(rate/1000), achDecimal[0], (UINT)(rate%1000));
        }

        //
        // display audio format
        //
        //  Audio: Mono 11.024KHz 8bit
        //
        else if (psi->lpFormat && psi->sh.fccType == streamtypeAUDIO) {

            LPWAVEFORMAT pwf = (LPWAVEFORMAT)psi->lpFormat;

            if (pwf->nChannels == 1)
                LoadString(ghModule, INFO_MONOFORMAT, ach, sizeof(ach));
            else
                LoadString(ghModule, INFO_STEREOFORMAT, ach, sizeof(ach));

            pch += wsprintf(pch, ach,
                (int)(pwf->nSamplesPerSec/1000),achDecimal[0],
                (int)(pwf->nSamplesPerSec%1000),
                (int)(pwf->nAvgBytesPerSec * 8 / (pwf->nSamplesPerSec * pwf->nChannels)));

            if (pwf->wFormatTag == WAVE_FORMAT_PCM) {
            }

            else if (pwf->wFormatTag == 2) {  // ADPCM
                pch -= 2; // skip over \r\n
                LoadString(ghModule, INFO_ADPCM, pch, 20);
                pch += lstrlen(pch);
            }

            else {
                pch -= 2; // skip over \r\n
                LoadString(ghModule, INFO_COMPRESSED, pch, 20);
                pch += lstrlen(pch);
            }

        }

        //
        // Other stream.
        //
        else if (psi->sh.fccType != 0) {

            LoadString(ghModule, INFO_STREAM, ach, sizeof(ach));

            pch += wsprintf(pch, ach,
                (LPSTR)&psi->sh.fccType,
                (LPSTR)&psi->sh.fccHandler);
        }

        if (!(psi->dwFlags & STREAM_ENABLED)) {
            pch -= 2; // skip over \r\n
            LoadString(ghModule, INFO_DISABLED, ach, sizeof(ach));
            pch += wsprintf(pch, ach);
        }
    }

#ifdef DEBUG
    //
    // show the frames skipped on the last play
    //
    if (npMCI->lFramesPlayed > 0) {
        LoadString(ghModule, INFO_SKIP, ach, sizeof(ach));
        pch += wsprintf(pch, ach,
            npMCI->lSkippedFrames,
            npMCI->lFramesPlayed,
            (int)(100L * npMCI->lSkippedFrames / npMCI->lFramesPlayed));
	
	//
	// show the frames not read on the last play
	//
	if (npMCI->lFramesSeekedPast > 0) {
	    LoadString(ghModule, INFO_NOTREAD, ach, sizeof(ach));
	    pch += wsprintf(pch, ach,
		npMCI->lFramesSeekedPast,
		(int)(100L * npMCI->lFramesSeekedPast / npMCI->lFramesPlayed));
	}

    }

    //
    // show the # audio breaks on the last play
    //
    if (npMCI->lFramesPlayed > 0 && npMCI->lAudioBreaks > 0) {
        LoadString(ghModule, INFO_SKIPAUDIO, ach, sizeof(ach));
        pch += wsprintf(pch, ach, npMCI->lAudioBreaks);
    }
#endif

    if (npMCI->dwKeyFrameInfo == 1) {
        LoadString(ghModule, INFO_ALLKEYFRAMES, ach, sizeof(ach));
        pch += wsprintf(pch, ach, (int)npMCI->dwKeyFrameInfo);
    }
    else if (npMCI->dwKeyFrameInfo == 0) {
        LoadString(ghModule, INFO_NOKEYFRAMES, ach, sizeof(ach));
        pch += wsprintf(pch, ach, (int)npMCI->dwKeyFrameInfo);
    }
    else {
        LoadString(ghModule, INFO_KEYFRAMES, ach, sizeof(ach));
        pch += wsprintf(pch, ach, (int)npMCI->dwKeyFrameInfo);
    }

#ifdef DEBUG
    //
    // timing info
    //
    #define SEC(time)    (UINT)(time / 1000l) , (UINT)(time % 1000l)
    #define SECX(time,t) SEC(time) , (t ? (UINT)(time * 100l / t) : 0)

    if (npMCI->lFramesPlayed > 0) {

        DRAWDIBTIME ddt;

        pch += wsprintf(pch, TEXT("MCIAVI-------------------------------------\r\n"));
        pch += wsprintf(pch, TEXT("timePlay:    \t%3d.%03dsec\r\n"),SEC(npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeRead:    \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeRead, npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeWait:    \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeWait, npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeYield:   \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeYield, npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeVideo:   \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeVideo, npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeDraw:    \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeDraw,  npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeDecomp:  \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeDecompress, npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timeAudio:   \t%3d.%03dsec (%d%%)\r\n"),SECX(npMCI->timeAudio, npMCI->timePlay));
        pch += wsprintf(pch, TEXT("timePaused:  \t%3d.%03dsec\r\n"),SEC(npMCI->timePaused));
        pch += wsprintf(pch, TEXT("timePrepare: \t%3d.%03dsec\r\n"),SEC(npMCI->timePrepare));
        pch += wsprintf(pch, TEXT("timeCleanup: \t%3d.%03dsec\r\n"),SEC(npMCI->timeCleanup));

        if (npMCI->hdd && DrawDibTime(npMCI->hdd, &ddt)) {
            pch += wsprintf(pch, TEXT("DrawDib-------------------------------------\r\n"));
            pch += wsprintf(pch, TEXT("timeDraw:        \t%3d.%03dsec\r\n"), SEC(ddt.timeDraw));
            pch += wsprintf(pch, TEXT("timeDecompress:  \t%3d.%03dsec (%d%%)\r\n"), SECX(ddt.timeDecompress, ddt.timeDraw));
            pch += wsprintf(pch, TEXT("timeDither:      \t%3d.%03dsec (%d%%)\r\n"), SECX(ddt.timeDither,     ddt.timeDraw));
            pch += wsprintf(pch, TEXT("timeStretch:     \t%3d.%03dsec (%d%%)\r\n"), SECX(ddt.timeStretch,    ddt.timeDraw));
            pch += wsprintf(pch, TEXT("timeSetDIBits:   \t%3d.%03dsec (%d%%)\r\n"), SECX(ddt.timeSetDIBits,  ddt.timeDraw));
            pch += wsprintf(pch, TEXT("timeBlt:         \t%3d.%03dsec (%d%%)\r\n"), SECX(ddt.timeBlt,        ddt.timeDraw));
        }
    }
#endif

    //
    // now shove this mess into the info window.
    //
    Assert(pch - pchInfo < 8192);
    SetWindowFont(GetDlgItem(hwnd, ID_INFO), GetStockObject(ANSI_VAR_FONT), TRUE);
    SetDlgItemText(hwnd, ID_INFO, pchInfo);

    LocalFree((HLOCAL)pchInfo);

    return TRUE;
}

BOOL FAR PASCAL _LOADDS ConfigDlgProc(HWND hDlg, UINT wMsg,
						WPARAM wParam, LPARAM lParam)
{
    static NPMCIGRAPHIC npMCI = NULL;
    DWORD dwOptions;
    TCHAR ach[80];

    switch (wMsg) {
	case WM_INITDIALOG:
            npMCI = (NPMCIGRAPHIC)(UINT)lParam;
	    ghwndConfig = hDlg;

            if (npMCI)
                dwOptions = npMCI->dwOptionFlags;
            else
                dwOptions = ReadConfigInfo();
	
#ifndef WIN32
            // On NT we do not support full screen.  I wonder if this
            // will ever change ?
	    CheckRadioButton(hDlg, ID_WINDOW, ID_VGA240,
                (dwOptions & MCIAVIO_USEVGABYDEFAULT) ?
					    ID_VGA240 : ID_WINDOW);
#endif

	    CheckDlgButton(hDlg, ID_ZOOM2,
                                (dwOptions & MCIAVIO_ZOOMBY2) != 0);

	    CheckDlgButton(hDlg, ID_SKIPFRAMES,
                                (dwOptions & MCIAVIO_SKIPFRAMES) != 0);

#if 0  /////////////////////////////////////////////////////////////////////
	    CheckDlgButton(hDlg, ID_FAILIFNOWAVE,
                                (dwOptions & MCIAVIO_FAILIFNOWAVE) != 0);
	
	    CheckDlgButton(hDlg, ID_SEEKEXACT,
                                (dwOptions & MCIAVIO_SEEKEXACT) == 0);
#endif /////////////////////////////////////////////////////////////////////
	
//	    CheckDlgButton(hDlg, ID_STUPIDMODE,
//                                (dwOptions & MCIAVIO_STUPIDMODE) != 0);
	
	    EnableWindow(GetDlgItem(hDlg, ID_ZOOM2), TRUE);
//                                (dwOptions & MCIAVIO_STUPIDMODE) == 0);

            if (npMCI == NULL) {
                GetDlgItemText(hDlg, ID_DEFAULT, ach, sizeof(ach)/sizeof(TCHAR));
                SetDlgItemText(hDlg, IDOK, ach);
                ShowWindow(GetDlgItem(hDlg, ID_DEFAULT),SW_HIDE);
            }

            if (!ConfigInfo(npMCI, hDlg)) {
                RECT rcWindow;
                RECT rc;

                GetWindowRect(hDlg, &rcWindow);
                GetWindowRect(GetDlgItem(hDlg, ID_SIZE), &rc);

                SetWindowPos(hDlg, NULL, 0, 0,
                    rcWindow.right-rcWindow.left,
                    rc.top - rcWindow.top,
                    SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            }
            return TRUE;

        case WM_ENDSESSION:
            if (wParam)
                EndDialog(hDlg, FALSE);
            break;

	case WM_COMMAND:
	    switch (wParam) {
                case ID_DEFAULT:
                case IDOK:
                    if (npMCI)
                        dwOptions = npMCI->dwOptionFlags;
                    else
                        dwOptions = 0;

                    /* Clear the flags we might set */

                    dwOptions &= ~(MCIAVIO_USEVGABYDEFAULT |
                                   MCIAVIO_SKIPFRAMES |
////                               MCIAVIO_FAILIFNOWAVE |
////                               MCIAVIO_SEEKEXACT |
                                   MCIAVIO_ZOOMBY2 |
                                   MCIAVIO_STUPIDMODE);
		
#ifndef WIN32
            // On NT we do not support full screen.  I wonder if this
            // will ever change ?
		    if (!IsDlgButtonChecked(hDlg, ID_WINDOW))
                        dwOptions |= MCIAVIO_USEVGABYDEFAULT;
#endif
				
		    if (IsDlgButtonChecked(hDlg, ID_SKIPFRAMES))
                        dwOptions |= MCIAVIO_SKIPFRAMES;

////                if (IsDlgButtonChecked(hDlg, ID_FAILIFNOWAVE))
////                    dwOptions |= MCIAVIO_FAILIFNOWAVE;

////                if (!IsDlgButtonChecked(hDlg, ID_SEEKEXACT))
                        dwOptions |= MCIAVIO_SEEKEXACT;
				
//		    if (IsDlgButtonChecked(hDlg, ID_STUPIDMODE))
//                        dwOptions |= MCIAVIO_STUPIDMODE;
//
//                    else if (IsDlgButtonChecked(hDlg, ID_ZOOM2))
                    if (IsDlgButtonChecked(hDlg, ID_ZOOM2))
                        dwOptions |= MCIAVIO_ZOOMBY2;

                    if (wParam == ID_DEFAULT || npMCI == NULL)
                        WriteConfigInfo(dwOptions);

                    if (wParam == IDOK) {

                        if (npMCI)
                            npMCI->dwOptionFlags = dwOptions;

                        EndDialog(hDlg, TRUE);
                    }
		    break;
		
//                case ID_STUPIDMODE:
//		    EnableWindow(GetDlgItem(hDlg, ID_ZOOM2),
//			    !IsDlgButtonChecked(hDlg, ID_STUPIDMODE));
//
//		    /* Clear "zoom" if stupid mode checked */
//		    if (IsDlgButtonChecked(hDlg, ID_STUPIDMODE))
//			CheckDlgButton(hDlg, ID_ZOOM2, FALSE);
//		    break;
		
		case IDCANCEL:
		    EndDialog(hDlg, FALSE);
		    break;
#ifdef DEBUG
                case ID_DEBUG:
                    DialogBoxParam(ghModule, MAKEINTRESOURCE(IDA_DEBUG),
                            hDlg, DebugDlgProc, (DWORD)(UINT)npMCI);
                    break;
#endif
	    }
	    break;
    }
    return FALSE;
}

DWORD FAR PASCAL ReadConfigInfo(void)
{
    int		i;
    DWORD	dwOptions = 0L;
    HDC		hdc;
    //
    // ask the display device if it can do 256 color.
    //
    hdc = GetDC(NULL);
    i = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);

    i = GetProfileInt(szIni, szDEFAULTVIDEO,
		(i < 8 && (GetWinFlags() & WF_CPU286)) ? 240 : 0);

    if (i >= 200)
	dwOptions |= MCIAVIO_USEVGABYDEFAULT;

////if (GetProfileInt(szIni, szSEEKEXACT, 1))
	dwOptions |= MCIAVIO_SEEKEXACT;

    if (GetProfileInt(szIni, szZOOMBY2, 0))
	dwOptions |= MCIAVIO_ZOOMBY2;

////if (GetProfileInt(szIni, szFAILIFNOWAVE, 0))
////    dwOptions |= MCIAVIO_FAILIFNOWAVE;

//    if (GetProfileInt(szIni, szSTUPIDMODE, 0))
//	dwOptions |= MCIAVIO_STUPIDMODE;

    if (GetProfileInt(szIni, szSKIPFRAMES, 1))
        dwOptions |= MCIAVIO_SKIPFRAMES;

    if (GetProfileInt(szIni, szUSEAVIFILE, 0))
        dwOptions |= MCIAVIO_USEAVIFILE;

    return dwOptions;
}

void FAR PASCAL WriteConfigInfo(DWORD dwOptions)
{
    // !!! This shouldn't get written out if it is the default!
    WriteProfileString(szIni, szDEFAULTVIDEO,
	 (dwOptions & MCIAVIO_USEVGABYDEFAULT) ? szVIDEO240 : szVIDEOWINDOW);

////WriteProfileString(szIni, szSEEKEXACT,
////        (dwOptions & MCIAVIO_SEEKEXACT) ? sz1 : sz0);

    WriteProfileString(szIni, szZOOMBY2,
	    (dwOptions & MCIAVIO_ZOOMBY2) ? sz1 : sz0);

////WriteProfileString(szIni, szFAILIFNOWAVE,
////        (dwOptions & MCIAVIO_FAILIFNOWAVE) ? sz1 : sz0);

//    WriteProfileString(szIni, szSTUPIDMODE,
//            (dwOptions & MCIAVIO_STUPIDMODE) ? sz1 : sz0);

    WriteProfileString(szIni, szSKIPFRAMES,
            (dwOptions & MCIAVIO_SKIPFRAMES) ? sz1 : sz0);

    WriteProfileString(szIni, szUSEAVIFILE,
            (dwOptions & MCIAVIO_USEAVIFILE) ? sz1 : sz0);
}

BOOL FAR PASCAL ConfigDialog(HWND hwnd, NPMCIGRAPHIC npMCI)
{
    #define MAX_WINDOWS 10
    HWND    hwndActive[MAX_WINDOWS];
    BOOL    f;
    int     i;
    HWND    hwndT;

    if (ghwndConfig) {
        MessageBeep(0);
        return FALSE;
    }

    if (hwnd == NULL)
        hwnd = GetActiveWindow();

    //
    // enum all the toplevel windows of this task and disable them!
    //
    for (hwndT = GetWindow(GetDesktopWindow(), GW_CHILD), i=0;
         hwndT && i < MAX_WINDOWS;
         hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {

        if (IsWindowEnabled(hwndT) &&
            	IsWindowVisible(hwndT) &&
            	(HTASK)GetWindowTask(hwndT) == GetCurrentTask() &&
	    	hwndT != hwnd) {	// don't disable our parent
            hwndActive[i++] = hwndT;
            EnableWindow(hwndT, FALSE);
        }
    }

    f = DialogBoxParam(ghModule, MAKEINTRESOURCE(IDA_CONFIG),
            hwnd, ConfigDlgProc, (DWORD)(UINT)npMCI);

    //
    // restore all windows
    //
    while (i-- > 0)
        EnableWindow(hwndActive[i], TRUE);

    if (hwnd)
        SetActiveWindow(hwnd);

    ghwndConfig = NULL;

    return f;
}
