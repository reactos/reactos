/****************************************************************************
 *
 *  AVIOPTS.C
 *
 *  routine for bringing up the compression options dialog
 *
 *      AVISaveOptions()
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <mmreg.h>
#include <msacm.h>
#include <compman.h>
#include "avifile.h"
#include "aviopts.h"
#include "aviopts.dlg"

#ifdef WIN32
	//!!! ACK the ACM does not work on NT
	#define acmGetVersion()	0
        #define acmFormatChoose(x) 1  // some error.
        #define ICCompressorChoose(hwnd,a,b,c,d,e) 0
        #define ICCompressorFree(x)
#endif

/****************************************************************************
 ***************************************************************************/

extern HINSTANCE ghMod;

LONG FAR PASCAL _export AVICompressOptionsDlgProc(HWND hwnd, unsigned msg, WORD wParam, LONG lParam);

/****************************************************************************
 ***************************************************************************/


int  gnNumStreams = 0;			// how many streams in array
int  gnCurStream = 0;			// which stream's options we're setting
PAVISTREAM FAR *gapAVI;	        	// array of stream pointers
LPAVICOMPRESSOPTIONS FAR *gapOpt;	// array of option structures to fill
UINT	  guiFlags;
COMPVARS  gCompVars;                    // for ICCompressorChoose

/****************************************************************************
 ***************************************************************************/
/*************************************************************
* @doc EXTERNAL AVISaveOptions
*
* @api BOOL | AVISaveOptions | This function gets the save options for 
*      a file and returns them in a buffer.
*
* @parm HWND | hwnd | Specifies the parent window handle for the Compression Options 
*       dialog box.
*
* @parm UINT | uiFlags | Specifies the flags for displaying the 
*       Compression Options dialog box. The following flags are defined: 
*
* @flag ICMF_CHOOSE_KEYFRAME | Displays a "Key frame every" box for 
*       the video options. This is the same flag used in <f ICCompressorChoose>.
*
* @flag ICMF_CHOOSE_DATARATE | Displays a "Data rate" box for the video
*       options. This is the same flag used in <f ICCompressorChoose>.
*
* @flag ICMF_CHOOSE_PREVIEW | Displays a "Preview" button for 
*       the video options. This button previews the compression 
*       using a frame from the stream. This is the same flag 
*      used in <f ICCompressorChoose>.
*
* @parm int | nStreams | Specifies the number of streams 
*       that will have their options set by the dialog box.
*
* @parm PAVISTREAM FAR * | ppavi | Specifies a pointer to an 
*       array of stream interface pointers. The <p nStreams> 
*       parameter indicates the number of pointers in the array.
*
* @parm LPAVICOMPRESSOPTIONS FAR * | plpOptions | Specifies a pointer 
*       to an array of <t LPAVICOMPRESSOPTIONS> pointers 
*       to hold the compression options set by the dialog box. The 
*       <p nStreams> parameter indicates the number of 
*       pointers in the array.
*
* @rdesc Returns TRUE if the user pressed OK, FALSE for CANCEL or an error.
* 
* @comm This function presents a standard Compression Options dialog
*       box using <p hwnd> as the parent window handle. When the 
*       user is finished selecting the compression options for 
*       each stream, the options are returned in the <t AVICOMPRESSOPTIONS> 
*       structures in the array referenced by <p lpOptions>. The caller 
*       must pass the interface pointers for the streams 
*       in the array referenced by <p ppavi>.
*
******************************************************************/
STDAPI_(BOOL) AVISaveOptions(HWND hwnd, UINT uiFlags, int nStreams, PAVISTREAM FAR *ppavi, LPAVICOMPRESSOPTIONS FAR *plpOptions)
{
    BOOL        f;
    AVICOMPRESSOPTIONS FAR *aOptions;
    int		i;

    /* Save the stream pointer */
    gnNumStreams = nStreams;
    gnCurStream = -1;
    gapAVI = ppavi;
    gapOpt = plpOptions;
    guiFlags = uiFlags;

    //
    // Remember the old compression options in case we cancel and need to 
    // restore them
    //
    aOptions = (AVICOMPRESSOPTIONS FAR *)GlobalAllocPtr(GMEM_MOVEABLE,
			nStreams * sizeof(AVICOMPRESSOPTIONS));
    if (!aOptions)
	return FALSE;
    for (i = 0; i < nStreams; i++)
	aOptions[i] = *plpOptions[i];

    f = DialogBox (ghMod, "AVICompressOptionsDialog", hwnd,
		(DLGPROC)AVICompressOptionsDlgProc);
 
    //
    // The user cancelled... put the old compression options back.
    //
    if (f == 0)
        for (i = 0; i < nStreams; i++)
	    *plpOptions[i] = aOptions[i];
	
    // Couldn't bring up the dialog
    if (f == -1)
	f = 0;

    GlobalFreePtr(aOptions);

    // !!! Returning TRUE doesn't guarantee something actually changed...
    return f;
}

/*************************************************************
* @doc EXTERNAL AVISaveOptionsFree
*
* @api LONG | AVISaveOptionsFree | This function frees the resources allocated
*      by <f AVISaveOptions>.
*
* @parm int | nStreams | Specifies the number of <t AVICOMPRESSOPTIONS>
*       structures in the array passed in as the next parameter.
*
* @parm LPAVICOMPRESSOPTIONS FAR * | plpOptions | Specifies a pointer 
*       to an array of <t LPAVICOMPRESSOPTIONS> pointers 
*       to hold the compression options set by the dialog box. The 
*       resources in each of these structures that were allocated by
*       <f AVISaveOptions> will be freed.
*
* @rdesc This function always returns AVIERR_OK (zero)
* 
* @comm This function frees the resources allocated by <f AVISaveOptions>.
**************************************************************/
STDAPI AVISaveOptionsFree(int nStreams, LPAVICOMPRESSOPTIONS FAR *plpOptions)
{
    for (; nStreams > 0; nStreams--) {
	if (plpOptions[nStreams-1]->lpParms)
	    GlobalFreePtr(plpOptions[nStreams-1]->lpParms);
	plpOptions[nStreams-1]->lpParms = NULL;
	if (plpOptions[nStreams-1]->lpFormat)
	    GlobalFreePtr(plpOptions[nStreams-1]->lpFormat);
	plpOptions[nStreams-1]->lpFormat = NULL;
    }
    return AVIERR_OK;
}

/****************************************************************************
 	Bring up the compression options for the current stream
 ***************************************************************************/
BOOL StreamOptions(HWND hwnd) {
    AVISTREAMINFO	avis;
    BOOL		f = FALSE;
    LONG		lTemp;
    UINT		w;
    
    // Get the stream type
    if (AVIStreamInfo(gapAVI[gnCurStream], &avis, sizeof(avis)) != 0)
        return FALSE;

    //
    // Video stream -- bring up the video compression dlg
    //
    if (avis.fccType == streamtypeVIDEO) {

        // The structure we have now is not filled in ... init it
        if (!(gapOpt[gnCurStream]->dwFlags & AVICOMPRESSF_VALID)) {
	    _fmemset(gapOpt[gnCurStream], 0,
		    sizeof(AVICOMPRESSOPTIONS));
	    gapOpt[gnCurStream]->fccHandler = comptypeDIB;
	    gapOpt[gnCurStream]->dwQuality = DEFAULT_QUALITY;
        }

        _fmemset(&gCompVars, 0, sizeof(gCompVars));
        gCompVars.cbSize = sizeof(gCompVars);
        gCompVars.dwFlags = ICMF_COMPVARS_VALID;
        gCompVars.fccHandler = gapOpt[gnCurStream]->fccHandler;
        gCompVars.lQ = gapOpt[gnCurStream]->dwQuality;
        gCompVars.lpState = gapOpt[gnCurStream]->lpParms;
        gCompVars.cbState = gapOpt[gnCurStream]->cbParms;
        gCompVars.lKey =
	    (gapOpt[gnCurStream]->dwFlags & AVICOMPRESSF_KEYFRAMES)?
	    (gapOpt[gnCurStream]->dwKeyFrameEvery) : 0;
        gCompVars.lDataRate =
	    (gapOpt[gnCurStream]->dwFlags & AVICOMPRESSF_DATARATE) ?
	    (gapOpt[gnCurStream]->dwBytesPerSecond / 1024) : 0;
    
        // !!! Don't pass flags verbatim if others are defined!!!
        f = ICCompressorChoose(hwnd, guiFlags, NULL,
		    gapAVI[gnCurStream], &gCompVars, NULL);

        /* Set the options to our new values */
        gapOpt[gnCurStream]->lpParms = gCompVars.lpState;
        gapOpt[gnCurStream]->cbParms = gCompVars.cbState;
	gCompVars.lpState = NULL;	// so it won't be freed
        gapOpt[gnCurStream]->fccHandler = gCompVars.fccHandler;
        gapOpt[gnCurStream]->dwQuality = gCompVars.lQ;
        gapOpt[gnCurStream]->dwKeyFrameEvery = gCompVars.lKey;
        gapOpt[gnCurStream]->dwBytesPerSecond = gCompVars.lDataRate
	    * 1024;
        if (gCompVars.lKey)
	    gapOpt[gnCurStream]->dwFlags |= AVICOMPRESSF_KEYFRAMES;
        else
	    gapOpt[gnCurStream]->dwFlags &=~AVICOMPRESSF_KEYFRAMES;
        if (gCompVars.lDataRate)
	    gapOpt[gnCurStream]->dwFlags |= AVICOMPRESSF_DATARATE;
        else
	    gapOpt[gnCurStream]->dwFlags &=~AVICOMPRESSF_DATARATE;

        // If they pressed OK, we have valid stuff in here now.
        if (f)
	    gapOpt[gnCurStream]->dwFlags |= AVICOMPRESSF_VALID;
	
        // Close the stuff opened by ICCompressorChoose
        ICCompressorFree(&gCompVars);

    //
    // Bring up the ACM format dialog and stuff it in our
    // compression options structure
    //
    } else if (avis.fccType == streamtypeAUDIO) {

        ACMFORMATCHOOSE acf;
	LONG lsizeF = 0;

        if (acmGetVersion() < 0x02000000L) {
	    char achACM[160];
	    char achACMV[40];
	    
	    LoadString(ghMod, IDS_BADACM, achACM, sizeof(achACM));
	    LoadString(ghMod, IDS_BADACMV, achACMV, sizeof(achACMV));

	    MessageBox(hwnd, achACM, achACMV, MB_OK | MB_ICONHAND);
	    return FALSE;
        }

        _fmemset(&acf, 0, sizeof(acf));	// or ACM blows up
        acf.cbStruct = sizeof(ACMFORMATCHOOSE);
        // If our options struct has valid data, use it to init
        // the acm dialog with, otherwise pick a default.
        acf.fdwStyle = (gapOpt[gnCurStream]->dwFlags & AVICOMPRESSF_VALID)
			       ? ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT : 0;
        acf.hwndOwner = hwnd;

	// Make sure the AVICOMPRESSOPTIONS has a big enough lpFormat
	acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, (LPVOID)&lTemp);
	if ((gapOpt[gnCurStream]->cbFormat == 0 ||
			gapOpt[gnCurStream]->lpFormat == NULL) && lTemp) {
	    gapOpt[gnCurStream]->lpFormat =
			GlobalAllocPtr(GMEM_MOVEABLE, lTemp);
	    gapOpt[gnCurStream]->cbFormat = lTemp;
	} else if (gapOpt[gnCurStream]->cbFormat < (DWORD)lTemp && lTemp) {
	    gapOpt[gnCurStream]->lpFormat =
			GlobalReAllocPtr(gapOpt[gnCurStream]->lpFormat, lTemp,
				GMEM_MOVEABLE);
	    gapOpt[gnCurStream]->cbFormat = lTemp;
	}
	
	if (!gapOpt[gnCurStream]->lpFormat)
	    return FALSE;

        acf.pwfx = gapOpt[gnCurStream]->lpFormat;
        acf.cbwfx = gapOpt[gnCurStream]->cbFormat;

	//
	// Only ask for choices that we can actually convert to
	//
	AVIStreamReadFormat(gapAVI[gnCurStream],
		AVIStreamStart(gapAVI[gnCurStream]), NULL, &lsizeF);

	// !!! Work around ACM bug by making sure our format is big enough
	lsizeF = max(lsizeF, sizeof(WAVEFORMATEX));
	acf.pwfxEnum = (LPWAVEFORMATEX)
		       GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, lsizeF);
	
	if (acf.pwfxEnum) {
	    AVIStreamReadFormat(gapAVI[gnCurStream],
		AVIStreamStart(gapAVI[gnCurStream]), acf.pwfxEnum, &lsizeF);
	    acf.fdwEnum |= ACM_FORMATENUMF_CONVERT;
	}

        // If they pressed OK, we now have valid stuff in here!
        w = acmFormatChoose(&acf);

	if (w == MMSYSERR_NOERROR)
	    gapOpt[gnCurStream]->dwFlags |= AVICOMPRESSF_VALID;
	else if (w != ACMERR_CANCELED) {
	    MessageBeep(0); // !!! Should really be a message box!
	}

	if (acf.pwfxEnum)
	    GlobalFreePtr(acf.pwfxEnum);

	f = (w == MMSYSERR_NOERROR);
    }

    return f;
}

void NEAR PASCAL NewStreamChosen(HWND hwnd)
{
    BOOL	    f;
    AVISTREAMINFO   avis;
    DWORD	    dw;
    HIC		    hic;
    ICINFO	    icinfo;
    ACMFORMATDETAILS acmfmt;
    ACMFORMATTAGDETAILS	aftd;
    LONG	    lsizeF;
    LPBITMAPINFOHEADER lp = NULL;
    char	    szFFDesc[80];
    char	    szDesc[120];

    // Set the interleave options for the selection we're leaving
    // !!! This code also appears in the OK button
    if (gnCurStream >= 0) {		// there is a previous sel
	if (IsDlgButtonChecked(hwnd, IDC_intINTERLEAVE)) {
	    dw = (DWORD)GetDlgItemInt(hwnd, IDC_intINTERLEAVEEDIT,
		    NULL, FALSE);
	    gapOpt[gnCurStream]->dwInterleaveEvery = dw;
	    gapOpt[gnCurStream]->dwFlags |= AVICOMPRESSF_INTERLEAVE;
	} else {
	    dw = (DWORD)GetDlgItemInt(hwnd, IDC_intINTERLEAVEEDIT,
		    NULL, FALSE);
	    gapOpt[gnCurStream]->dwInterleaveEvery = dw;
	    gapOpt[gnCurStream]->dwFlags &=~AVICOMPRESSF_INTERLEAVE;
	}
    }

    gnCurStream = (int)SendDlgItemMessage(hwnd, IDC_intCHOOSESTREAM,
			    CB_GETCURSEL, 0, 0L);
    if (gnCurStream < 0)
	return;

    if (AVIStreamInfo(gapAVI[gnCurStream], &avis, sizeof(avis)) != 0)
	return;

    //
    // Show a string describing the current format
    //
    szDesc[0] = '\0';

    lsizeF = 0;
    AVIStreamReadFormat(gapAVI[gnCurStream],
	    AVIStreamStart(gapAVI[gnCurStream]), NULL, &lsizeF);
    if (lsizeF) {
	lp = (LPBITMAPINFOHEADER)GlobalAllocPtr(GHND, lsizeF);
	if (lp) {
	    if (AVIStreamReadFormat(gapAVI[gnCurStream],
				    AVIStreamStart(gapAVI[gnCurStream]),
				    lp, &lsizeF) == AVIERR_OK) {
		if (avis.fccType == streamtypeVIDEO) {
		    wsprintf(szDesc, "%ldx%ldx%d\n", lp->biWidth,
			     lp->biHeight, lp->biBitCount);
		    if (lp->biCompression == BI_RGB) {
			LoadString(ghMod, IDS_FFDESC, szFFDesc,
				   sizeof(szFFDesc));
			lstrcat(szDesc, szFFDesc);
		    } else {
			hic = ICDecompressOpen(ICTYPE_VIDEO,avis.fccHandler,
					       lp, NULL);
			if (hic) {
			    if (ICGetInfo(hic, &icinfo,sizeof(icinfo)) != 0)
				lstrcat(szDesc, icinfo.szDescription);
			    ICClose(hic);
			}
		    }
		} else if (avis.fccType == streamtypeAUDIO) {
		    _fmemset(&acmfmt, 0, sizeof(acmfmt));
		    acmfmt.pwfx = (LPWAVEFORMATEX) lp;
		    acmfmt.cbStruct = sizeof(ACMFORMATDETAILS);
		    acmfmt.dwFormatTag = acmfmt.pwfx->wFormatTag;
		    acmfmt.cbwfx = lsizeF;
		    aftd.cbStruct = sizeof(aftd);
		    aftd.dwFormatTag = acmfmt.pwfx->wFormatTag;
		    aftd.fdwSupport = 0;

		    if ((acmFormatTagDetails(NULL, 
					     &aftd,
					     ACM_FORMATTAGDETAILSF_FORMATTAG) == 0) && 
			(acmFormatDetails(NULL, &acmfmt,
					  ACM_FORMATDETAILSF_FORMAT) == 0)) {
			wsprintf(szDesc, "%s %s", (LPSTR) acmfmt.szFormat,
				 (LPSTR) aftd.szFormatTag);
		    }
		}
	    }
		
	    GlobalFreePtr(lp);
	}
    }
    SetDlgItemText(hwnd, IDC_intFORMAT, szDesc);

    //
    // AUDIO and VIDEO streams have a compression dialog
    //
    if (avis.fccType == streamtypeAUDIO ||
		    avis.fccType == streamtypeVIDEO)
	EnableWindow(GetDlgItem(hwnd, IDC_intOPTIONS), TRUE);
    else
	EnableWindow(GetDlgItem(hwnd, IDC_intOPTIONS), FALSE);

    //
    // Every stream but the first has an interleave options
    //
    if (gnCurStream > 0) {
	EnableWindow(GetDlgItem(hwnd, IDC_intINTERLEAVE), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_intINTERLEAVEEDIT),
		    TRUE);
	EnableWindow(GetDlgItem(hwnd, IDC_intINTERLEAVETEXT),
		    TRUE);
	// Set the interleave situation for this stream
	f = (gapOpt[gnCurStream]->dwFlags & AVICOMPRESSF_INTERLEAVE)
		    != 0;
	dw = gapOpt[gnCurStream]->dwInterleaveEvery;
	CheckDlgButton(hwnd, IDC_intINTERLEAVE, f);
	SetDlgItemInt(hwnd, IDC_intINTERLEAVEEDIT, (int)dw, FALSE);
    } else {
	EnableWindow(GetDlgItem(hwnd, IDC_intINTERLEAVE),FALSE);
	EnableWindow(GetDlgItem(hwnd, IDC_intINTERLEAVEEDIT),
		    FALSE);
	EnableWindow(GetDlgItem(hwnd, IDC_intINTERLEAVETEXT),
		    FALSE);
    }
    
}


/*--------------------------------------------------------------+
* Dialog Proc for the main compression options dialog		*
+--------------------------------------------------------------*/
LONG FAR PASCAL _export AVICompressOptionsDlgProc(HWND hwnd, unsigned msg, WORD wParam, LONG lParam)
{
  int   nVal;
  AVISTREAMINFO avis;
  DWORD dw;
  
  switch(msg){
    case WM_INITDIALOG:

	    //
	    // If we've only got one stream to set the options for, it seems
	    // silly to bring up a box to let you choose which stream you want.
	    // Let's skip straight to the proper options dlg box.
	    //
	    if (gnNumStreams == 1) {
		gnCurStream = 0;
		EndDialog(hwnd, StreamOptions(hwnd));
		return TRUE;
	    }

            /* Add the list of streams to the drop-down box */
            for (nVal = 0; nVal < gnNumStreams; nVal++) {
		// Get the name of this stream
		AVIStreamInfo(gapAVI[nVal], &avis, sizeof(avis));
                SendDlgItemMessage(hwnd, IDC_intCHOOSESTREAM, CB_ADDSTRING, 0,
                                (LONG) (LPSTR)avis.szName);
	    }

            // Set our initial selection to the first item
            SendDlgItemMessage(hwnd, IDC_intCHOOSESTREAM, CB_SETCURSEL, 0, 0L);
	    // Make sure we see it
            SendMessage(hwnd, WM_COMMAND, IDC_intCHOOSESTREAM,
            	MAKELONG(GetDlgItem(hwnd, IDC_intCHOOSESTREAM), CBN_SELCHANGE));

            return TRUE;
	    
    case WM_COMMAND:
	switch(wParam){
            case IDOK:
		// Set the interleave options for the selection we're on
		// !!! This code also appears in the SELCHANGE code
		if (gnCurStream >= 0) {		// there is a valid selection
    		    if (IsDlgButtonChecked(hwnd, IDC_intINTERLEAVE)) {
		        dw = (DWORD)GetDlgItemInt(hwnd, IDC_intINTERLEAVEEDIT,
				NULL, FALSE);
		        gapOpt[gnCurStream]->dwInterleaveEvery = dw;
		        gapOpt[gnCurStream]->dwFlags |= AVICOMPRESSF_INTERLEAVE;
		    } else {
			// why not remember edit box entry anyway?
		        dw = (DWORD)GetDlgItemInt(hwnd, IDC_intINTERLEAVEEDIT,
				NULL, FALSE);
		        gapOpt[gnCurStream]->dwInterleaveEvery = dw;
		        gapOpt[gnCurStream]->dwFlags &=~AVICOMPRESSF_INTERLEAVE;
		    }
		}
		// fall through	(AAAAaaaahhhhh.....)

	    case IDCANCEL:
                EndDialog(hwnd, wParam == IDOK);
                break;

            case IDC_intOPTIONS:
		StreamOptions(hwnd);
		break;

	    //
	    // Somebody chose a new stream.  Do we need to grey InterleaveOpts?
	    // Set the current stream.
	    //
            case IDC_intCHOOSESTREAM:
                if (HIWORD(lParam) != CBN_SELCHANGE)
                    break;

		NewStreamChosen(hwnd);
                break;

	    case IDC_intINTERLEAVE:
		break;

	    default:
		break;
	}
	break;
	    
    default:
	return FALSE;
  }
  return FALSE;
}
