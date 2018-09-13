//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       imgart.cxx
//
//  Contents:   Image filter for .art files
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_JGPLAY_H_
#define X_JGPLAY_H_
#include "jgplay.h"
#endif

MtDefine(CImgTaskArt, Dwn, "CImgTaskArt")
MtDefine(CArtPlayer, Dwn, "CArtPlayer")

/* ----------------------------- Defines --------------------------------*/
#define XX_DMsg(x, y)

/* JGPCRSEC -- Defines a critical section type for Win32 support. */
#if defined(WIN32)
	typedef CRITICAL_SECTION JGPCRSEC;
#else
	typedef UINTW JGPCRSEC;
#endif

/* Size of the buffer passed to JgpQueryStream */
#define JG_SIZE_INFO_BUFFER      1024
#define SIZE_DATA_BUFFER         512
   
/*
 * Offsets used to determine the start of the image data from
 * the pointer returned by the decompression library. The
 * pointer returned is a pointer to a BITMAPINFO structure.
 */

#define JG_COLORMAP_SIZE8  (sizeof(RGBQUAD) * 256)
#define JG_COLORMAP_SIZE4  (sizeof(RGBQUAD) * 16)
#define JG_COLORMAP_SIZE1  (sizeof(RGBQUAD) * 2)
#define JG_BMI_SIZE_1      (sizeof(BITMAPINFOHEADER) + JG_COLORMAP_SIZE1)
#define JG_BMI_SIZE_4      (sizeof(BITMAPINFOHEADER) + JG_COLORMAP_SIZE4)
#define JG_BMI_SIZE_8      (sizeof(BITMAPINFOHEADER) + JG_COLORMAP_SIZE8)
#define JG_BMI_SIZE_24     (sizeof(BITMAPINFOHEADER))

/* -----------------------------------------------------------------
 * Dynamic loading of DLL
 * ----------------------------------------------------------------- */

/* Name of decompression library */
#define JG_MODULE_NAME "JGPL400.DLL"

/* Function Names */

#define strJgpHeartBeat		"JgpHeartBeat"
#define strJgpQueryStream	"JgpQueryStream"
#define strJgpDoTest		"JgpDoTest"
#define strJgpOpen			"JgpOpen"
#define strJgpClose			"JgpClose"
#define strJgpSetEOFMark	"JgpSetEOFMark"
#define strJgpInputStream	"JgpInputStream"
#define strJgpStartPlay		"JgpStartPlay"
#define strJgpResumePlay	"JgpResumePlay"
#define strJgpPausePlay		"JgpPausePlay"
#define strJgpStopPlay		"JgpStopPlay"
#define strJgpReleaseSound	"JgpReleaseSound"
#define strJgpResumeSound	"JgpResumeSound"
#define strJgpSetPosition	"JgpSetPosition"
#define strJgpGetPosition	"JgpGetPosition"
#define strJgpGetImage		"JgpGetImage"
#define strJgpGetMask		"JgpGetMask"
#define strJgpGetReport		"JgpGetReport"

/* -----------------------------------------------------------------
 *  Function Pointers
 * ----------------------------------------------------------------- */

typedef JGERR (JGFFUNC *pfnJgpHeartBeatProto)(
		JGHANDLE SHandle);				// In: Show handle

typedef JGERR (JGFFUNC *pfnJgpQueryStreamProto)(
		UINT8 JGHUGE *pARTStream,		// In: ART Stream
		UINT32 nARTStreamBytes,			// In: Size of ARTStream in Bytes
		JGP_STREAM JGFAR *pInfo); 		// Out: Info structure

typedef JGERR (JGFFUNC *pfnJgpDoTestProto)(
		JGP_TEST JGFAR *pInfo);			// In: Info struct to be filled

typedef JGERR (JGFFUNC *pfnJgpOpenProto)(
		JGHANDLE JGFAR *pSHandle,		// Out: Place to receive handle   
		JGP_SETUP JGFAR *pSetup);		// In: The setup structure

typedef JGERR (JGFFUNC *pfnJgpCloseProto)(
		JGHANDLE SHandle);				// In: Show handle

typedef JGERR (JGFFUNC *pfnJgpSetEOFMarkProto)(
		JGHANDLE SHandle);				// In: Show handle

typedef JGERR (JGFFUNC *pfnJgpInputStreamProto)(
		JGHANDLE SHandle,				// In: Show Handle
		UINT8  JGHUGE *pARTStream,		// In: Pointer to the ART Stream
		UINT32 nBytes);					// In: Number of bytes being input

typedef JGERR (JGFFUNC *pfnJgpStartPlayProto)(
		JGHANDLE SHandle);				// In: Show Handle

typedef UINTW (JGFFUNC *pfnJgpResumePlayProto)(
		JGHANDLE SHandle);				// In: Show Handle

typedef UINTW (JGFFUNC *pfnJgpPausePlayProto)(
		JGHANDLE SHandle);				// In: Show Handle

typedef JGERR (JGFFUNC *pfnJgpStopPlayProto)(
		JGHANDLE SHandle);				// In: Show Handle

typedef JGERR (JGFFUNC *pfnJgpReleaseSoundProto)(
		JGHANDLE SHandle);				// In: Show Handle	

typedef JGERR (JGFFUNC *pfnJgpResumeSoundProto)(
		JGHANDLE SHandle);				// In: Show Handle	

typedef JGERR (JGFFUNC *pfnJgpSetPositionProto)(
		JGHANDLE SHandle,				// In: Show Handle
		UINT32 nPosition);				// In: Position

typedef JGERR (JGFFUNC *pfnJgpGetPositionProto)(
		JGHANDLE SHandle,				// In: Show Handle
		UINT32 JGFAR *pPosition);		// Out: Position

typedef JGERR (JGFFUNC *pfnJgpGetImageProto)(
		JGHANDLE SHandle,				// In: Show handle            
		JGP_IMAGE_REF JGFAR *phImg);	// Out: Handle to Image memory

typedef JGERR (JGFFUNC *pfnJgpGetMaskProto)(
		JGHANDLE SHandle,				// In: Show handle
		JGP_IMAGE_REF JGFAR *phImg);	// Out: Handle to Image memory

typedef JGERR (JGFFUNC *pfnJgpGetReportProto)(
		JGHANDLE SHandle,				// In:  Show Handle
		JGP_REPORT JGFAR *pReport);		// Out: Structure to receive the report

/* Function table.
 * This function table is used to access the functions exported by
 * the decompression library. The values are set by calls to
 * GetProcAddress.
 */
typedef struct tagJGFuncs {
	pfnJgpHeartBeatProto		pfnJgpHeartBeat;
	pfnJgpQueryStreamProto		pfnJgpQueryStream;
	pfnJgpDoTestProto			pfnJgpDoTest;
	pfnJgpOpenProto				pfnJgpOpen;
	pfnJgpCloseProto			pfnJgpClose;
	pfnJgpSetEOFMarkProto		pfnJgpSetEOFMark;
	pfnJgpInputStreamProto		pfnJgpInputStream;
	pfnJgpStartPlayProto		pfnJgpStartPlay;
	pfnJgpResumePlayProto		pfnJgpResumePlay;
	pfnJgpPausePlayProto		pfnJgpPausePlay;
	pfnJgpStopPlayProto			pfnJgpStopPlay;
	pfnJgpReleaseSoundProto		pfnJgpReleaseSound;
	pfnJgpResumeSoundProto		pfnJgpResumeSound;
	pfnJgpSetPositionProto		pfnJgpSetPosition;
	pfnJgpGetPositionProto		pfnJgpGetPosition;
	pfnJgpGetImageProto			pfnJgpGetImage;
	pfnJgpGetMaskProto			pfnJgpGetMask;
	pfnJgpGetReportProto		pfnJgpGetReport;
} JGFuncTable;

extern BYTE g_bJGJitState;

/* -----------------------------------------------------------------
 * Forward declarations
 * ----------------------------------------------------------------- */

static BOOL JGGetFunctionTable();

/* -----------------------------------------------------------------
 * Static declarations
 * ----------------------------------------------------------------- */

static JGFuncTable      ftJGDLL;
static HINSTANCE        hJGDLLModule        = NULL;
static JGHANDLE	        hLowColorContext    = NULL;
static JGHANDLE	        hMedColorContext    = NULL;
static JGHANDLE	        hHiColorContext     = NULL;
static JGHANDLE	        g_hActiveShowHandle = NULL;
static CCriticalSection g_csArt;

/* ------------------------------------------------------------
 * JGGetFunctionTable
 *
 * Loads the JG DLL if not already loaded and returns a
 * pointer to a function table with the accessible
 * functions.
 * ------------------------------------------------------------ */

static BOOL JGGetFunctionTable()
{
    BOOL fRetVal;
    
    g_csArt.Enter();

    fRetVal = FALSE;

    if (hJGDLLModule)
        fRetVal = TRUE;
    else if (g_bJGJitState == JIT_OK)
    {
        hJGDLLModule = LoadLibraryEx(_T(JG_MODULE_NAME), NULL, 0);
        if (hJGDLLModule)
        {
            ftJGDLL.pfnJgpHeartBeat =
                (pfnJgpHeartBeatProto) GetProcAddress(hJGDLLModule, strJgpHeartBeat);
            ftJGDLL.pfnJgpQueryStream =
                (pfnJgpQueryStreamProto) GetProcAddress(hJGDLLModule, strJgpQueryStream);
            ftJGDLL.pfnJgpDoTest =
                (pfnJgpDoTestProto) GetProcAddress(hJGDLLModule, strJgpDoTest);
            ftJGDLL.pfnJgpOpen =
                (pfnJgpOpenProto) GetProcAddress(hJGDLLModule, strJgpOpen);
            ftJGDLL.pfnJgpClose =
                (pfnJgpCloseProto) GetProcAddress(hJGDLLModule, strJgpClose);
            ftJGDLL.pfnJgpSetEOFMark =
                (pfnJgpSetEOFMarkProto) GetProcAddress(hJGDLLModule, strJgpSetEOFMark);
            ftJGDLL.pfnJgpInputStream =
                (pfnJgpInputStreamProto) GetProcAddress(hJGDLLModule, strJgpInputStream);
            ftJGDLL.pfnJgpStartPlay =
                (pfnJgpStartPlayProto) GetProcAddress(hJGDLLModule, strJgpStartPlay);
            ftJGDLL.pfnJgpResumePlay =
                (pfnJgpResumePlayProto) GetProcAddress(hJGDLLModule, strJgpResumePlay);
            ftJGDLL.pfnJgpPausePlay =
                (pfnJgpPausePlayProto) GetProcAddress(hJGDLLModule, strJgpPausePlay);
            ftJGDLL.pfnJgpStopPlay =
                (pfnJgpStopPlayProto) GetProcAddress(hJGDLLModule, strJgpStopPlay);
            ftJGDLL.pfnJgpReleaseSound =
                (pfnJgpReleaseSoundProto) GetProcAddress(hJGDLLModule, strJgpReleaseSound);
            ftJGDLL.pfnJgpResumeSound =
                (pfnJgpResumeSoundProto) GetProcAddress(hJGDLLModule, strJgpResumeSound);
            ftJGDLL.pfnJgpSetPosition =
                (pfnJgpSetPositionProto) GetProcAddress(hJGDLLModule, strJgpSetPosition);          
			ftJGDLL.pfnJgpGetPosition =
                (pfnJgpGetPositionProto) GetProcAddress(hJGDLLModule, strJgpGetPosition);
            ftJGDLL.pfnJgpGetImage =
                (pfnJgpGetImageProto) GetProcAddress(hJGDLLModule, strJgpGetImage);
			ftJGDLL.pfnJgpGetMask =
                (pfnJgpGetMaskProto) GetProcAddress(hJGDLLModule, strJgpGetMask);
			ftJGDLL.pfnJgpGetReport =
                (pfnJgpGetReportProto) GetProcAddress(hJGDLLModule, strJgpGetReport);

            if (ftJGDLL.pfnJgpHeartBeat &&
				ftJGDLL.pfnJgpQueryStream &&
				ftJGDLL.pfnJgpDoTest &&
                ftJGDLL.pfnJgpOpen &&
                ftJGDLL.pfnJgpClose &&
                ftJGDLL.pfnJgpSetEOFMark &&
                ftJGDLL.pfnJgpInputStream &&
                ftJGDLL.pfnJgpStartPlay &&
                ftJGDLL.pfnJgpResumePlay &&				
				ftJGDLL.pfnJgpPausePlay &&
                ftJGDLL.pfnJgpStopPlay &&
                ftJGDLL.pfnJgpReleaseSound &&
                ftJGDLL.pfnJgpResumeSound &&
                ftJGDLL.pfnJgpSetPosition &&
                ftJGDLL.pfnJgpGetPosition &&
                ftJGDLL.pfnJgpGetImage &&
                ftJGDLL.pfnJgpGetMask &&
                ftJGDLL.pfnJgpGetReport)
            {
                fRetVal = TRUE;
            }
            else
            {
                memset((BYTE *)&ftJGDLL, 0, sizeof(ftJGDLL));
                FreeLibrary(hJGDLLModule);
                hJGDLLModule = NULL;
            }

            g_bJGJitState = JIT_DONT_ASK;
        }
        else
        {
            g_bJGJitState = JIT_NEED_JIT;
        }
    }

    g_csArt.Leave();

    return fRetVal;
}

/* -----------------------------------------------------------
 * s_EnterCriticalSection
 *
 * This function enables a critical section.
 * ----------------------------------------------------------- */
static void s_EnterCriticalSection(
	JGPCRSEC JGPTR pCrSec)		// In: Critical section	variable
{
	#if defined(WIN32)
		CRITICAL_SECTION JGPTR pCs;
		pCs = (CRITICAL_SECTION JGPTR)pCrSec;
		InitializeCriticalSection(pCs);
		EnterCriticalSection(pCs);
	#else 
		pCrSec=pCrSec;
	#endif
}

/* -----------------------------------------------------------
 * s_LeaveCriticalSection
 *
 * This function disables a critical section.
 * ----------------------------------------------------------- */
static void s_LeaveCriticalSection(
	JGPCRSEC JGPTR pCrSec)		// In: Critical section	variable
{
	#if defined(WIN32)
		CRITICAL_SECTION JGPTR pCs;
		pCs = (CRITICAL_SECTION JGPTR)pCrSec;
		LeaveCriticalSection(pCs);
		DeleteCriticalSection(pCs);
	#else 
		pCrSec=pCrSec;
	#endif
}

/* -----------------------------------------------------------
 * BuildContext
 *
 * Build the decompression context for specified color depth.
 * ----------------------------------------------------------- */

static void BuildContext(int ColorDepth, JGHANDLE JGFAR *hShowHandle)
{
    JGP_SETUP			jgInit;
	static JGP_TEST		jgTest;
#ifdef BIG_ENDIAN
    JG_RGBX				palBG = {0, 255, 255, 255};
#else
    JG_RGBX				palBG = {255, 255, 255, 0};
#endif
    JGHANDLE *          pTheContext = NULL;
    PALETTEENTRY JGPTR	pPal;
	HGLOBAL				hPal = NULL;
	int					cPalColors, i;
	static BOOL			fDoneTest = FALSE;

    *hShowHandle = NULL;

	if (!fDoneTest)
	{
		memset(&jgTest, 0, sizeof(JGP_TEST));
		jgTest.Size = sizeof(jgTest);

		ftJGDLL.pfnJgpDoTest(&jgTest);
		fDoneTest = TRUE;
	}

    switch(ColorDepth)
    {
        case 4:
            pTheContext = &hLowColorContext;
            break;
        case 8:
            pTheContext = &hMedColorContext;
            break;
        default:
            pTheContext = &hHiColorContext;
            break;
    }

    memset(&jgInit, 0, sizeof(JGP_SETUP));
    jgInit.Size = sizeof(jgInit);

    /* Set up the JG initialization structure for this environment */
	jgInit.ColorDepth		= ColorDepth;
	jgInit.InhibitImage		= FALSE;
	jgInit.InhibitAudio		= jgTest.CanDoAudio ? FALSE : TRUE; 
	jgInit.InhibitMIDI		= jgTest.CanDoMIDI ? FALSE : TRUE;
	jgInit.InhibitDither	= FALSE;
	jgInit.InhibitSplash	= FALSE;
	jgInit.AudioMode		= JGP_AUDIO_DEFAULT;
	jgInit.CreateMask		= TRUE;
	jgInit.ScaleImage		= JGP_SCALE_NONE;
	jgInit.GammaAdjust		= JGP_GAMMA_NONE;
	jgInit.PaletteMode		= JGP_PALETTE_INPUT;
	jgInit.IndexOverride	= JGP_OVERRIDE_NONE;		

	/* If there is an existing handle, reuse it */
	if (!(*pTheContext))
		jgInit.OldHandle = NULL;
	else
		jgInit.OldHandle = *pTheContext;

	/* Set the background color to white for use with the mask */
    jgInit.BackgroundColor = palBG;

    switch(ColorDepth)
    {
        case 4:
            cPalColors = 16;
            
			hPal = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, cPalColors * 4);
		    if (!hPal)
            {
                return;
            }

			pPal = (PALETTEENTRY JGPTR) GlobalLock(hPal);	
		    if (!pPal)
		    {
                GlobalFree(hPal);
				return;
			}
				
			for (i = 0; i < cPalColors; i++)
		    {
				pPal[i] = g_peVga[i];
		    }			                

            jgInit.DefaultPalette = hPal;
            jgInit.PaletteSize = cPalColors;
            break;
        case 8:
 			cPalColors = g_lpHalftone.wCnt;
			
			hPal = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, cPalColors * 4);
		    if (!hPal)
            {
                return;
            }

            pPal = (PALETTEENTRY JGPTR) GlobalLock(hPal);	
		    if (!pPal)
		    {
				GlobalFree(hPal);
                return;
			}
				
			for (i = 0; i < cPalColors; i++)
		    {
				pPal[i] = g_lpHalftone.ape[i];
		    }			                

			jgInit.DefaultPalette = hPal;
            jgInit.PaletteSize = cPalColors;        
            break;
        default:
            jgInit.ColorDepth = 24;
            jgInit.DefaultPalette = NULL;
            jgInit.PaletteSize = 0;
            break;
    }
		
    if (ftJGDLL.pfnJgpOpen(hShowHandle, &jgInit) != JGP_SUCCESS)
    {
        *hShowHandle = NULL;
    }

	if (hPal)
    {
		GlobalUnlock(hPal);
		GlobalFree(hPal);
    }

    return;
}

/* ------------------------------------------------------------
 * _GetImageandMask
 *
 * Get new pixels and copy them to the displayed bitmap. If the  
 * bitmap does not yet exist, we'll create it.
 * ------------------------------------------------------------ */

void _GetImageandMask(JGHANDLE hJGInstance, CImgBitsDIB **ppibd,
                     LONG _yHei, LONG _colorMode)      
{
    BITMAPINFO *    pbmi;
    HGLOBAL         hDib;
    HGLOBAL         hMask;
    BYTE *          pbSrcDib;
    BYTE *          pbDstDib;
    BYTE *          pbSrcMask;
    BYTE *          pbDstMask;

    // Get the handle to the image dib
    if ((ftJGDLL.pfnJgpGetImage(hJGInstance, &hDib) == 0) && (hDib != NULL))
    {
        pbmi = (BITMAPINFO *)GlobalLock(hDib);
        if (!pbmi)
        {
            Assert("hDib is non-null, yet GlobalLock failed " && pbmi);
            return;
        }

        switch (_colorMode)
        {
            case 4:
                pbSrcDib = (BYTE *)pbmi + JG_BMI_SIZE_4;
                break;
            case 8:
                pbSrcDib = (BYTE *)pbmi + JG_BMI_SIZE_8;
                break;
            default:
                pbSrcDib = (BYTE *)pbmi + JG_BMI_SIZE_24;
                break;
        }

        // If we already have an image bitmap, just update it with the new pixels
        if (*ppibd)
        {
            pbDstDib = (BYTE *)(*ppibd)->GetBits();
            if (pbSrcDib && pbDstDib)
            {
                Assert(_yHei <= (*ppibd)->Height());
                memcpy(pbDstDib, pbSrcDib, (*ppibd)->CbLine() * min(_yHei, (*ppibd)->Height()));
            }
        }
        else // Create new image bitmap and copy pixels into it
        {
            (*ppibd) = new CImgBitsDIB();
            if (*ppibd &&
                !!(*ppibd)->AllocDIB((_colorMode <= 8) ? _colorMode : 24, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight, NULL, 0, -1, FALSE))
            {
                delete (*ppibd); // out of memory
                *ppibd = NULL;
            }
            
            if (*ppibd)
            {
                pbDstDib = (BYTE *)(*ppibd)->GetBits();
                if (pbSrcDib && pbDstDib)
                {
                    Assert(_yHei <= (*ppibd)->Height());
                    memcpy(pbDstDib, pbSrcDib, (*ppibd)->CbLine() * min(_yHei, (*ppibd)->Height()));
                }
            }
        }
        
        // Get the handle to the mask dib (if there is a mask)
        if ((ftJGDLL.pfnJgpGetMask(hJGInstance, &hMask) == 0) && (hMask != NULL))
        {
            pbmi = (BITMAPINFO *)GlobalLock(hMask);
            if (!pbmi)
            {
                Assert("hMask is non-null, yet GlobalLock failed " && pbmi);
                return;
            }
            pbSrcMask = (BYTE *)pbmi + JG_BMI_SIZE_1;

            // Ensure mask and update it with the new pixels
            if (*ppibd && pbSrcMask)
            {
                if (!(*ppibd)->GetMaskBits())
                {
                    (*ppibd)->AllocMask();
                }

                pbDstMask = (BYTE *)(*ppibd)->GetMaskBits();
                
                if (pbDstMask)
                {
                    Assert(_yHei <= (*ppibd)->Height());
                    memcpy(pbDstMask, pbSrcMask, (*ppibd)->CbLineMask() * min(_yHei, (*ppibd)->Height()));
                }
            }
        }
    }
}

/* ------------------------------------------------------------
 * GetArtReport
 *
 * Come here on timer message.  Trigger a heartbeat and get a 
 * report to see if we need to update the image.
 * ------------------------------------------------------------ */

BOOL CArtPlayer::GetArtReport(CImgBitsDIB **ppibd, LONG _yHei, LONG _colorMode)
{
    JGP_REPORT  jgUpdateInfo;
    JGHANDLE    hShowHandle;
    JGERR       iResult;
    JGPCRSEC    CrSec;

    if (!_dwShowHandle)
        return FALSE;

    if (!_fInPlayer)
    {
        s_EnterCriticalSection(&CrSec);
        _fInPlayer = TRUE;
        s_LeaveCriticalSection(&CrSec);

        memset(&jgUpdateInfo, 0, sizeof(JGP_REPORT));
        jgUpdateInfo.Size = sizeof(jgUpdateInfo);

        hShowHandle = (JGHANDLE) _dwShowHandle;

        iResult = ftJGDLL.pfnJgpHeartBeat(hShowHandle);
        iResult = ftJGDLL.pfnJgpGetReport(hShowHandle, &jgUpdateInfo);

        // Update our report flags
   	    _fPlaying = jgUpdateInfo.IsPlaying;
	    _fPaused = jgUpdateInfo.IsPaused;
        _fIsDone = jgUpdateInfo.IsDone;
        _ulCurrentTime = jgUpdateInfo.CurrentTime;
        _ulAvailPlayTime = jgUpdateInfo.AvailPlayTime;

        _fUpdateImage = jgUpdateInfo.UpdateImage;
        _rcUpdateRect.top = jgUpdateInfo.UpdateRect.top;
        _rcUpdateRect.left = jgUpdateInfo.UpdateRect.left;
        _rcUpdateRect.bottom = jgUpdateInfo.UpdateRect.bottom;
        _rcUpdateRect.right = jgUpdateInfo.UpdateRect.right;

        // If there are new pixels available, get them into the displayed dib
        if (jgUpdateInfo.UpdateImage)
        {
            _GetImageandMask(hShowHandle, ppibd, _yHei, _colorMode);
        }

        s_EnterCriticalSection(&CrSec);
        _fInPlayer = FALSE;
        s_LeaveCriticalSection(&CrSec);

        return (_fUpdateImage);
    }    

    return (FALSE);
}

/* ------------------------------------------------------------
 * QueryPlayState
 *
 * Check the play state to enable/disable menu options. 
 * ------------------------------------------------------------ */

BOOL CArtPlayer::QueryPlayState(int iCommand)
{
    BOOL bReturn = FALSE;

    switch (iCommand)
    {
        case IDM_IMGARTPLAY:
            if ((!_fPlaying) || _fPaused)
                bReturn = TRUE;  
            break;
        case IDM_IMGARTSTOP:
            if (_fPlaying && (!_fPaused))
                bReturn = TRUE;  
            break;
        case IDM_IMGARTREWIND:
            if (_ulCurrentTime > 0L)
                bReturn = TRUE;  
            break;
    }
    return (bReturn);
}

/* ------------------------------------------------------------
 * DoPlayCommand
 *
 * Execute a play command. 
 * ------------------------------------------------------------ */

void CArtPlayer::DoPlayCommand(int iCommand)
{
    JGERR iResult;
    JGPCRSEC CrSec;

    if (_fInPlayer)
        return;

    switch (iCommand)
    {
        case IDM_IMGARTPLAY:
            if (_dwShowHandle)
            {
                s_EnterCriticalSection(&CrSec);
                _fInPlayer = TRUE;
                s_LeaveCriticalSection(&CrSec);    
                
                // We only need to worry about ART with sound
                if (_fHasSound)
                {
                    // Release sound hardware from the active handle
                    if ((g_hActiveShowHandle != NULL) &&
                        (g_hActiveShowHandle != (JGHANDLE) _dwShowHandle))
                    {
                        if (_fDynamicImages)
                        {
                            iResult = ftJGDLL.pfnJgpReleaseSound(g_hActiveShowHandle);
                            if (iResult == JGP_SUCCESS)
                                g_hActiveShowHandle = NULL;

                            // Assign sound hardware to the new active handle
                            iResult = ftJGDLL.pfnJgpResumeSound((JGHANDLE) _dwShowHandle);
                            if (iResult == JGP_SUCCESS)
                                g_hActiveShowHandle = (JGHANDLE) _dwShowHandle;
                        }
                        else
                        {   // Stop the previous if it is sound and picture
                            iResult = ftJGDLL.pfnJgpStopPlay(g_hActiveShowHandle);
                            if (iResult == JGP_SUCCESS)
                            {
                                g_hActiveShowHandle = NULL;
                            }
                        }
                    }
                }	
                // Start the playback    
                iResult = ftJGDLL.pfnJgpStartPlay((JGHANDLE) _dwShowHandle);
                if (iResult == JGP_SUCCESS)
                {
                    _fPlaying = TRUE;
                    _fPaused = FALSE;

                    if ((g_hActiveShowHandle == NULL) && _fHasSound)
                        g_hActiveShowHandle = (JGHANDLE) _dwShowHandle;
                }

                s_EnterCriticalSection(&CrSec);
                _fInPlayer = FALSE;
                s_LeaveCriticalSection(&CrSec);
            }
            break;
        case IDM_IMGARTSTOP:
            if (_dwShowHandle)
            {
                iResult = ftJGDLL.pfnJgpStopPlay((JGHANDLE) _dwShowHandle);
                if (iResult == JGP_SUCCESS)
                {
                    _fPlaying = FALSE;
                    _fPaused = FALSE;
                }
            }
            break;
        case IDM_IMGARTREWIND:
            if (_dwShowHandle)
            {
                ftJGDLL.pfnJgpSetPosition((JGHANDLE) _dwShowHandle, 0L);
                _ulCurrentTime = 0L;
            }
            break;
    }
}

/* ------------------------------------------------------------
 * ~CArtPlayer
 *
 * Stop the Show, close the handle and clean-up our data 
 * structure.
 * ------------------------------------------------------------ */

CArtPlayer::~CArtPlayer()
{
    JGERR           iResult;
    JGHANDLE        hShowHandle;
    JGP_IMAGE_REF   hDib;
    JGP_IMAGE_REF   hMask;

    if (_dwShowHandle)
    {
        hShowHandle = (JGHANDLE) _dwShowHandle;

        // Stop playback
        iResult = ftJGDLL.pfnJgpStopPlay(hShowHandle);

        // Remove the active handle tag
        if (g_hActiveShowHandle == hShowHandle)
            g_hActiveShowHandle = NULL;

        // Free up the dib memory allocated by the player
        if (ftJGDLL.pfnJgpGetImage(hShowHandle, &hDib) == 0)
        {
            if ((ftJGDLL.pfnJgpGetMask(hShowHandle, &hMask) == 0) && (hMask != NULL))
                GlobalFree(hMask);
            
            GlobalFree(hDib);
        }

        // Close the show handle
        iResult = ftJGDLL.pfnJgpClose(hShowHandle);
        _dwShowHandle = 0;
    }
}

/* -----------------------------------------------------------
 * CImgTaskArt
 * ----------------------------------------------------------- */

class CImgTaskArt : public CImgTask
{
    typedef CImgTask super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskArt))

	~CImgTaskArt();

	// CImgTask methods

	virtual void Decode(BOOL *pfNonProgressive);

    // CImgTaskArt methods

	BOOL			FindImageHeightWidth(BYTE *buf, BYTE *sizeInfoBuffer, long cBufBytes,
										long *lBufferSize, int  *height, int  *width);
    BOOL            DecompressArtImage();
    virtual void	BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags);

    // Data members

    UINT            _ulCoversImg;
    HANDLE          _hDib;
    HANDLE          _hMask;
};

CImgTaskArt::~CImgTaskArt()
{
    if (!_pArtPlayer)
    {
        if (_hDib)
        {
            GlobalUnlock(_hDib);
            GlobalFree(_hDib);
        }

        if (_hMask)
        {
            GlobalUnlock(_hMask);
            GlobalFree(_hMask);
        }
    }
}

/* ------------------------------------------------------------
 * FindImageHeightWidth
 *
 * From the JG Stream find the Height and Width of the original
 * Image. Return TRUE if Height And Width found.
 * ------------------------------------------------------------ */

BOOL
CImgTaskArt::FindImageHeightWidth(BYTE *buf,
								  BYTE *sizeInfoBuffer,
								  long cBufBytes,
								  long *lBufferSize,
								  int  *height,
								  int  *width)
{
    JGP_STREAM      jgStreamInfo;
    CArtPlayer *    pArtPlayerNew;
    long            lBytesToCopy;

    memset(&jgStreamInfo, 0, sizeof(JGP_STREAM));
    jgStreamInfo.Size = sizeof(jgStreamInfo);

    lBytesToCopy = cBufBytes;
    if (((*lBufferSize) + lBytesToCopy) > JG_SIZE_INFO_BUFFER)
        lBytesToCopy = JG_SIZE_INFO_BUFFER - (*lBufferSize);

    if (lBytesToCopy > 0L)
    {
        memcpy(&sizeInfoBuffer[(*lBufferSize)], buf, cBufBytes);
        *lBufferSize += lBytesToCopy;
    }

    if (ftJGDLL.pfnJgpQueryStream(sizeInfoBuffer, *(lBufferSize), &jgStreamInfo) == JGP_SUCCESS)
    {
	    ////////////////////////////////////////////////////
		// Get the image height and width
		*height = jgStreamInfo.Dimensions.bottom;
        *width  = jgStreamInfo.Dimensions.right;

	    ////////////////////////////////////////////////////
		// See if this stream requires a heartbeat  
	    if (jgStreamInfo.Attributes & JGP_ISTEMPORAL)
		{
            // We need to store Show info, so clear some space
            pArtPlayerNew = new CArtPlayer;
            if (pArtPlayerNew == NULL)
            {
                XX_DMsg(DBG_IMAGE, ("not enough memory for ART Show data\n"));
            }
            else
            {
                _pArtPlayer = pArtPlayerNew;
                _pArtPlayer->_fInPlayer = FALSE; 
                _pArtPlayer->_fTemporalART = TRUE;
    	        _pArtPlayer->_fDynamicImages = (jgStreamInfo.Attributes & JGP_HASDYNAMICIMAGES);
                _pArtPlayer->_uiUpdateRate = jgStreamInfo.UpdateRate;

                ////////////////////////////////////////////////////
		        // See if this stream has sound  
	            if ((jgStreamInfo.Attributes & JGP_HASAUDIO) ||
	    	        (jgStreamInfo.Attributes & JGP_HASMIDI))
		        {
	    	        _pArtPlayer->_fHasSound = TRUE;
	            }
	        }
        }

        XX_DMsg(DBG_IMAGE, ("ART: Image size found h=%d w=%d\n", *height, *width));
        return TRUE;
    }
    
    return FALSE;
}

void
CImgTaskArt::Decode(BOOL *pfNonProgressive)
{
    DecompressArtImage();
}

/* --------------------------------------------------------------------------------------
 * DecompressArtImage
 *
 * This function does the actual decompression of the image
 * stream.
 *
 * The function returns TRUE if an image was succesfully
 * decoded otherwise FALSE.
 *
 * unsigned char **image,          out - Image data
 * unsigned char **mask,           out - pointer to BITMAPINFO structure & DIB for mask
 * HGLOBAL       *hDIB,            out - Handle to memory allocated for image
 * HGLOBAL       *hMask,           out - Handle to memory allocated for mask
 * int           ColorDepth        in  - Color depth to decompress image at.
 * ------------------------------------------------------------------------------------ */

BOOL CImgTaskArt::DecompressArtImage()
{
    CArtPlayer *        pStaticArtPlayer = NULL;
    CArtPlayer *        pArtPlayer = NULL;
    JGPCRSEC            CrSec;
    JGP_REPORT			jgUpdateInfo;
    JGHANDLE			hJGInstance;
    JGERR               iResult;
    unsigned char		buf[SIZE_DATA_BUFFER];
    unsigned long		cBufBytes;
    int					height, width;
    BOOL				fNewPixelsReady = FALSE;
    BOOL				fGotSizeInfo = FALSE;
    BYTE				sizeInfoBuffer[JG_SIZE_INFO_BUFFER];
    long				lBufferSize = 0L;
    int					yBottom;

    _ulCoversImg = IMGBITS_PARTIAL;

    /* STEP 1 - INIT */
    /* ------------- */

    /* Build a context for the JG Library to use for decompression */
    hJGInstance = NULL;
    BuildContext(_colorMode, &hJGInstance);
    if (!hJGInstance)
        return FALSE;

    /* Image Size Info */
    lBufferSize = 0L;
    memset(sizeInfoBuffer, 0, JG_SIZE_INFO_BUFFER);

    /* Used for progressive draw */
    memset(&jgUpdateInfo, 0, sizeof(JGP_REPORT));
    jgUpdateInfo.Size = sizeof(jgUpdateInfo);


    /* STEP 2 - DECOMPRESS */
    /* ------------------- */

    for(;;)
    {
        if (pArtPlayer)
        {
            if (pArtPlayer->_fInPlayer)
                continue;
            else
            {
                s_EnterCriticalSection(&CrSec);
                pArtPlayer->_fInPlayer = TRUE;
                s_LeaveCriticalSection(&CrSec);    
            }

            Read(buf, SIZE_DATA_BUFFER, &cBufBytes);
            if (cBufBytes == 0)
            {
                s_EnterCriticalSection(&CrSec);
                pArtPlayer->_fInPlayer = FALSE;
                s_LeaveCriticalSection(&CrSec);
                break;
            }

            if (ftJGDLL.pfnJgpInputStream(hJGInstance, buf, cBufBytes) != JGP_SUCCESS)
            {
                s_EnterCriticalSection(&CrSec);
                pArtPlayer->_fInPlayer = FALSE;
                s_LeaveCriticalSection(&CrSec);
                break;
            }

            s_EnterCriticalSection(&CrSec);
            pArtPlayer->_fInPlayer = FALSE;
            s_LeaveCriticalSection(&CrSec);
        }
        else
        {
            Read(buf, SIZE_DATA_BUFFER, &cBufBytes);
            if (cBufBytes == 0)
                break;

            if (ftJGDLL.pfnJgpInputStream(hJGInstance, buf, cBufBytes) != JGP_SUCCESS)
                break;
        }

        /* ---- Find image Height and Width ---- */
        if (!fGotSizeInfo)
        {
            fGotSizeInfo = FindImageHeightWidth(buf, sizeInfoBuffer,
                                                cBufBytes, &lBufferSize,
                                                &height, &width);
            if (fGotSizeInfo)
            {
                _xWid = width;
                _yHei = height;
                OnSize(_xWid, _yHei, _lTrans);

                /* ---- Save temporal Show handle ---- */     
                /* ---- Start the timer and playback ---- */     
                if (_pArtPlayer)
                {
                    _pArtPlayer->_dwShowHandle = (DWORD_PTR) hJGInstance;
                    pArtPlayer = _pArtPlayer;

                    OnAnim();
                    _pArtPlayer->DoPlayCommand(IDM_IMGARTPLAY);
                }
                else // Must be a static ART image
                {
                    pStaticArtPlayer = new CArtPlayer;
                    if (pStaticArtPlayer == NULL)
                        break;                   
                    pStaticArtPlayer->_dwShowHandle = (DWORD_PTR) hJGInstance;
                    pStaticArtPlayer->_fInPlayer = FALSE;
                    pArtPlayer = pStaticArtPlayer;
                }
            }
        }

        /* ---- See if we have new pixels ---- */
        if (fGotSizeInfo)
        {
            fNewPixelsReady = pArtPlayer->GetArtReport(
                    (CImgBitsDIB **)&_pImgBits, _yHei, _colorMode);
        }

        /* ---- Progressive draw ---- */
        if (fNewPixelsReady)
        {
            yBottom = pArtPlayer->_rcUpdateRect.bottom;

            if (yBottom == _yHei - 1)
                yBottom++;

            if ((_ulCoversImg == IMGBITS_PARTIAL) &&
                ((yBottom < _yBot) || (yBottom == _yHei)))
                _ulCoversImg = IMGBITS_TOTAL;

            _yBot = yBottom;
            OnProg(FALSE, _ulCoversImg, FALSE, _yBot);
        }

    } /* endfor */

    OnProg(TRUE, _ulCoversImg, FALSE, _yBot);

    if (_ulCoversImg == IMGBITS_TOTAL || _yBot + 1 >= _yHei)
        _ySrcBot = -1;
    else if (_yBot >= 31)
        _ySrcBot = _yBot + 1;

    /* STEP 3 - CLEANUP */
    /* ---------------- */

	// There is no more data, set the EOF mark
    if (pArtPlayer && !pArtPlayer->_fInPlayer)
    {
        s_EnterCriticalSection(&CrSec);
        pArtPlayer->_fInPlayer = TRUE;
        s_LeaveCriticalSection(&CrSec);    

	    ftJGDLL.pfnJgpSetEOFMark(hJGInstance);

        s_EnterCriticalSection(&CrSec);
        pArtPlayer->_fInPlayer = FALSE;
        s_LeaveCriticalSection(&CrSec);
    }

    // Close the show handle if it is not needed (i.e. static ART). 
    if (!_pArtPlayer)
    {
        // Get the image dib(s) before we close
        // the show so they can be destroyed
        if (ftJGDLL.pfnJgpGetImage(hJGInstance, &_hDib) == 0)
            if (ftJGDLL.pfnJgpGetMask(hJGInstance, &_hMask))
                iResult = ftJGDLL.pfnJgpClose(hJGInstance);

        hJGInstance = NULL;
        delete pStaticArtPlayer;
    }

    return TRUE; 
}

CImgTask * NewImgTaskArt()
{
    return(JGGetFunctionTable() ? new CImgTaskArt : NULL);
}

/* ------------------------------------------------------------
 *
 * Render the specified part of the ART image to the specified
 * location at the specified size.
 *
 * ------------------------------------------------------------ */
void
CImgTaskArt::BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags)
{
    int ySrcBot = (_ulCoversImg == IMGBITS_TOTAL) ? _yHei : _yBot + 1;

    if (_pImgBits)
    {
        ((CImgBitsDIB *)_pImgBits)->SetValidLines(ySrcBot);

        _pImgBits->StretchBlt(hdc, prcDst, prcSrc, dwRop, dwFlags);
    }
}
