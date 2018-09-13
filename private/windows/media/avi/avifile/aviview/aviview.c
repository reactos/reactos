#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <commdlg.h>
#include <vfw.h>
#define CLIPSTUFF
#ifdef CLIPSTUFF
#include <vfw.h>
#endif

#include <msacm.h>

#define TEST_FINDSAMPLE

#ifndef streamtypeTEXT
    #pragma message("streamtypeTEXT is not defined in AVIFMT.H")
    #define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')
#endif
#include <memory.h>
#include "aviview.h"
#include "audplay.h"

#define GlobalSizePtr(lp)   GlobalSize(GlobalPtrHandle(lp))

#ifndef WIN32
extern LONG FAR PASCAL muldiv32(LONG, LONG, LONG);
#endif

extern BOOL RegisterObjects(void);
extern void RevokeObjects(void);

TCHAR gachFilter[512] = TEXT("");

#define FIXCC(fcc)  if (fcc == 0)       fcc = mmioFOURCC('N', 'o', 'n', 'e'); \
                    if (fcc == BI_RLE8) fcc = mmioFOURCC('R', 'l', 'e', '8');

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
typedef LONG (FAR PASCAL *LPWNDPROC)(HWND, UINT, WPARAM, LPARAM); // pointer to a window procedure

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
static  TCHAR        gszAppName[]=TEXT("AVIView");

static  HANDLE      ghInstApp;
static  HWND        ghwndApp;
static  HACCEL	    ghAccel;

#define SCROLLRANGE  10000

#define MAXNUMSTREAMS   50
int                 giCurrentStream;            // current stream;
PAVIFILE	    gpfile;			// the current file
int                 gaiStreamTop[MAXNUMSTREAMS];
PAVISTREAM          gapavi[MAXNUMSTREAMS];	// the current streams
PGETFRAME	    gapgf[MAXNUMSTREAMS];	// data for decompressing
						// video
HDRAWDIB	    ghdd[MAXNUMSTREAMS];	// drawdib handles
HIC		    ghic[MAXNUMSTREAMS];	// experimental: installable
						// draw handlers for non-video
						// streams
int		    gcpavi;			// # of streams

BOOL		    gfPlaying = FALSE;		// Are we playing right now?
LONG		    glPlayStartTime;		// When did we start playing?
LONG 		    glPlayStartPos;		// From what position?

PAVISTREAM          gpaviAudio;                 // 1st audio stream found
PAVISTREAM          gpaviVideo;                 // 1st video stream found

#define             gfVideoFound (gpaviVideo != NULL)
#define             gfAudioFound (gpaviAudio != NULL)

LONG                timeStart;			// cached start, end, length
LONG                timeEnd;
LONG                timeLength;
LONG		    timehscroll;	// how much arrows scroll HORZ bar
LONG		    vertSBLen;
LONG		    vertHeight;


DWORD		    gdwMicroSecPerPixel = 1000L;	// !!! x-stretch

TCHAR                gachFileName[MAX_PATH] = TEXT("");
TCHAR                gachSaveFileName[MAX_PATH] = TEXT("");
UINT		    gwZoom = 2;
AVICOMPRESSOPTIONS  gaAVIOptions[MAXNUMSTREAMS];
LPAVICOMPRESSOPTIONS  galpAVIOptions[MAXNUMSTREAMS];

HFONT               hfontApp;
TEXTMETRIC          tm;
				// !!! constants for painting
            #define VSPACE  8	// no one will ever know what this means :-(
            #define HSPACE  4	// space between frames
            #define TSPACE  (tm.tmHeight) // space for text area about each stream
            #define AUDIOVSPACE  64	// height of an audio stream
            #define FRAME_BORDER 2

void SaveSmall(PAVISTREAM ps, LPTSTR lpFilename);

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

#define GetScrollTime(hwnd) \
    (timeStart + muldiv32(GetScrollPos(hwnd, SB_HORZ), timeLength, SCROLLRANGE))

#define SetScrollTime(hwnd, time) SetScrollPos(hwnd, SB_HORZ, \
    (int)muldiv32((time) - timeStart, SCROLLRANGE, timeLength), TRUE)

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

LONG FAR PASCAL _export AppWndProc (HWND hwnd, unsigned uiMessage, WPARAM wParam, LPARAM lParam);
int  ErrMsg (LPTSTR sz,...);
BOOL fDialog(int id,HWND hwnd,FARPROC fpfn);

LONG NEAR PASCAL AppCommand(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam);

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HCURSOR hcurSave;
int     fWait = 0;

void StartWait()
{
    if (fWait++ == 0)
    {
        SetCursor(LoadCursor(NULL,IDC_WAIT));
    }
}

void EndWait()
{
    if (--fWait == 0)
    {
        SetCursor(LoadCursor(NULL,IDC_ARROW));
        InvalidateRect(ghwndApp, NULL, TRUE);
    }
}

BOOL WinYield()
{
    MSG msg;
    BOOL fAbort=FALSE;

    while(fWait > 0 && PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
	if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
            fAbort = TRUE;
	if (msg.message == WM_SYSCOMMAND && (msg.wParam & 0xFFF0) == SC_CLOSE)
	    fAbort = TRUE;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    return fAbort;
}


/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

//
// When we load a file or zoom changes, we re-set the scrollbars
//
void FixScrollbars(HWND hwnd)
{
    AVISTREAMINFO     avis;
    LONG		r, lHeight = 0;
    UINT		w;
    int			i;
    RECT		rc;

    //
    // Walk through all streams and determine how many pixels it will take to
    // draw it.
    //
    for (i = 0; i < gcpavi; i++) {

        AVIStreamInfo(gapavi[i], &avis, sizeof(avis));

        if (avis.fccType == streamtypeVIDEO) {

	    //
	    // Set the horizontal scrollbar scale to show every frame
	    // of the first video stream exactly once
	    //
	    if (gapavi[i] == gpaviVideo) {
		w = (avis.rcFrame.right - avis.rcFrame.left) * gwZoom / 4 +
								    HSPACE;
		r = (LONG)(avis.dwRate / avis.dwScale);
		gdwMicroSecPerPixel = muldiv32(1000000, 1, w * r);
		timehscroll = 1000 / r;	// msec per frame
	    }

	    lHeight +=	TSPACE + TSPACE / 2 + TSPACE +
			(avis.rcFrame.bottom - avis.rcFrame.top) * gwZoom / 4;
	} else if (avis.fccType == streamtypeAUDIO) {
	    lHeight += TSPACE + AUDIOVSPACE * gwZoom / 4;

	} else if (avis.fccType == streamtypeTEXT) {
	    lHeight += TSPACE + TSPACE;
	    ghic[i] = ICDrawOpen(avis.fccType, avis.fccHandler, NULL);
	}

	//
	// Every stream has this much space
	//
	lHeight += TSPACE + TSPACE + TSPACE + VSPACE;
    }

    //
    // Set vertical scrollbar for scrolling the visible area
    //
    GetClientRect(hwnd, &rc);
    vertHeight = lHeight;	// total height in pixels of entire display

    //
    // We won't fit in the window... need scrollbars
    //
    if (lHeight > rc.bottom) {
	vertSBLen = lHeight - rc.bottom;
	SetScrollRange(hwnd, SB_VERT, 0, (int)vertSBLen, TRUE);
	SetScrollPos(hwnd, SB_VERT, 0, TRUE);

    //
    // We will fit in the window!  No scrollbars necessary
    //
    } else {
	vertSBLen = 0;
	SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);
    }
}

//
// Initialize the streams of a loaded file -- the compression options, the
// DrawDIB handles, and the scroll bars
//
void InitStreams(HWND hwnd)
{
    AVISTREAMINFO     avis;
    LONG	lTemp;
    int		i;

    //
    // Start with bogus times
    //
    timeStart = 0x7FFFFFFF;
    timeEnd   = 0;

    //
    // Walk through and init all streams loaded
    //
    for (i = 0; i < gcpavi; i++) {

        AVIStreamInfo(gapavi[i], &avis, sizeof(avis));

	//
	// Save and SaveOptions code takes a pointer to our compression opts
	//
	galpAVIOptions[i] = &gaAVIOptions[i];

	//
	// clear options structure to zeroes
	//
	_fmemset(galpAVIOptions[i], 0, sizeof(AVICOMPRESSOPTIONS));

	//
 	// Initialize the compression options to some default stuff
	// !!! Pick something better
	//
	galpAVIOptions[i]->fccType = avis.fccType;

	switch(avis.fccType) {

	    case streamtypeVIDEO:		
		galpAVIOptions[i]->dwFlags = AVICOMPRESSF_VALID |
			AVICOMPRESSF_KEYFRAMES | AVICOMPRESSF_DATARATE;
		galpAVIOptions[i]->fccHandler = 0;
		galpAVIOptions[i]->dwQuality = (DWORD)ICQUALITY_DEFAULT;
		galpAVIOptions[i]->dwKeyFrameEvery = 7;	// !!! ask compressor?
		galpAVIOptions[i]->dwBytesPerSecond = 60000;
		break;

	    case streamtypeAUDIO:
		galpAVIOptions[i]->dwFlags |= AVICOMPRESSF_VALID;
		galpAVIOptions[i]->dwInterleaveEvery = 5;
		acmMetrics(NULL,
			      ACM_METRIC_MAX_SIZE_FORMAT,
			      (LPVOID) &galpAVIOptions[i]->cbFormat);

		galpAVIOptions[i]->lpFormat =
			GlobalAllocPtr(GHND, galpAVIOptions[i]->cbFormat);

		lTemp = galpAVIOptions[i]->cbFormat;
		// Use current format as default format
		AVIStreamReadFormat(gapavi[i], 0,
				    galpAVIOptions[i]->lpFormat,
				    &lTemp);
		break;

	    default:
		break;
	}

	//
	// We're finding the earliest and latest start and end points for
	// our scrollbar.
	//
        timeStart = min(timeStart, AVIStreamStartTime(gapavi[i]));
        timeEnd   = max(timeEnd, AVIStreamEndTime(gapavi[i]));

	//
	// Initialize video streams for getting decompressed frames to display
	//
        if (avis.fccType == streamtypeVIDEO) {

#if 1
	    gapgf[i] = AVIStreamGetFrameOpen(gapavi[i], NULL);
#else
	    // Alternate code for testing AVIStreamGetFrameOpen
	    BITMAPINFOHEADER bih;

	    bih.biSize = sizeof(bih);
	    bih.biClrUsed = 256;
	    bih.biBitCount = 8;
	    bih.biPlanes = 1;
	    bih.biWidth = 0;
	    bih.biHeight = 0;
	    bih.biCompression = BI_RGB;
	    bih.biSizeImage = 0;
	    bih.biClrImportant = 0;

	    gapgf[i] = AVIStreamGetFrameOpen(gapavi[i], &bih);
#endif

	    if (gapgf[i] == NULL)
		continue;
	
	    ghdd[i] = DrawDibOpen();
	    // !!! DrawDibBegin?
	
	    if (gpaviVideo == NULL) {

		//
		// Remember the first video stream --- treat it specially
		//
                gpaviVideo = gapavi[i];
	    }

	} else if (avis.fccType == streamtypeAUDIO) {

	    //
	    // Remember the first audio stream --- treat it specially
	    //
	    if (gpaviAudio == NULL)
	        gpaviAudio = gapavi[i];

	}

    }

    timeLength = timeEnd - timeStart;

    SetScrollRange(hwnd, SB_HORZ, 0, SCROLLRANGE, TRUE);
    SetScrollTime(hwnd, timeStart);

    FixScrollbars(hwnd);
}

//
// Update the window title to reflect what's loaded
//
void FixWindowTitle(HWND hwnd)
{
    TCHAR ach[80];

    wsprintf(ach, TEXT("%s %s"),
            (LPTSTR)gszAppName,
            (LPTSTR)gachFileName);

    SetWindowText(hwnd, ach);

    InvalidateRect(hwnd, NULL, TRUE);
}

void FreeDrawStuff(HWND hwnd)
{
    int	i;

    aviaudioStop();

    for (i = 0; i < gcpavi; i++) {
	if (gapgf[i]) {
	    AVIStreamGetFrameClose(gapgf[i]);
	    gapgf[i] = NULL;
	}
	if (ghdd[i]) {
	    DrawDibClose(ghdd[i]);
	    ghdd[i] = 0;
	}
	if (ghic[i]) {
	    ICClose(ghic[i]);
	    ghic[i] = 0;
	}
    }
    SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
    gpaviVideo = gpaviAudio = NULL;
}

void FreeAvi(HWND hwnd)
{
    int	i;

    FreeDrawStuff(hwnd);

    for (i = 0; i < gcpavi; i++) {
	AVIStreamClose(gapavi[i]);
	if (galpAVIOptions[i]->lpFormat) {
	    GlobalFreePtr(galpAVIOptions[i]->lpFormat);
	}
    }
    if (gpfile)
	AVIFileClose(gpfile);

    gpfile = NULL;
    gcpavi = 0;
    giCurrentStream = 0;
}

void InitBall(HWND hwnd)
{
    PAVISTREAM FAR PASCAL NewBall(void);

    FreeAvi(hwnd);

    gapavi[0] = NewBall();

    if (gapavi[0])
	gcpavi = 1;

    lstrcpy(gachFileName, TEXT("BALL"));
    InitStreams(hwnd);
    FixWindowTitle(hwnd);
}


void InsertAVIFile(PAVIFILE pfile, HWND hwnd, LPTSTR lpszFile)
{
    int		i;

    for (i = gcpavi; i < MAXNUMSTREAMS; i++) {
	gapavi[i] = NULL;
	
	if (AVIFileGetStream(pfile, &gapavi[i], 0L, i - gcpavi) != AVIERR_OK)
	    break;

	if (gapavi[i] == NULL)
	    break;
    }

    if (gcpavi == i)
    {
        ErrMsg(TEXT("Unable to open %s"), lpszFile);
	if (pfile)
	    AVIFileClose(pfile);
	goto exit;
    }

    gcpavi = i;

    if (gpfile) {
	AVIFileClose(pfile);
    } else
	gpfile = pfile;

exit:
    InitStreams(hwnd);
    FixWindowTitle(hwnd);
}


void InitAvi(HWND hwnd, LPTSTR szFile, UINT wMenu)
{
    HRESULT	hr;
    PAVIFILE	pfile;

    hr = AVIFileOpen(&pfile, szFile, OF_SHARE_DENY_WRITE, 0L);

    if (hr != 0)
    {
        ErrMsg(TEXT("Unable to open %s"), szFile);
        return;
    }

    if (wMenu == MENU_OPEN)
	FreeAvi(hwnd);
    else
	FreeDrawStuff(hwnd);

    InsertAVIFile(pfile, hwnd, szFile);
}

/*----------------------------------------------------------------------------*\
|   AppInit( hInst, hPrev)						       |
|									       |
|   Description:							       |
|	This is called when the application is first loaded into	       |
|	memory.  It performs all initialization that doesn't need to be done   |
|	once per instance.						       |
|									       |
|   Arguments:								       |
|	hInstance	instance handle of current instance		       |
|	hPrev		instance handle of previous instance		       |
|									       |
|   Returns:								       |
|	TRUE if successful, FALSE if not				       |
|									       |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HINSTANCE hInst, HINSTANCE hPrev, int sw,LPSTR szCmdLine)
{
    WNDCLASS cls;
    int      dx,dy;

    /* Save instance handle for DialogBoxs */
    ghInstApp = hInst;

    ghAccel = LoadAccelerators(hInst, MAKEINTATOM(ID_APP));

    if (szCmdLine && szCmdLine[0]) {
#ifdef UNICODE
	// convert to unicode
	lstrcpy(gachFileName, GetCommandLine());
#else
    	lstrcpy(gachFileName, szCmdLine);
#endif
    }

    if (!hPrev) {
	/*
	 *  Register a class for the main application window
	 */
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = LoadIcon(hInst,MAKEINTATOM(ID_APP));
        cls.lpszMenuName   = MAKEINTATOM(ID_APP);
        cls.lpszClassName  = MAKEINTATOM(ID_APP);
        cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance      = hInst;
        cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        cls.lpfnWndProc    = (LPWNDPROC)AppWndProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        if (!RegisterClass(&cls))
	    return FALSE;
    }

    AVIStreamInit();
    RegisterObjects();

    {HDC hdc;
    hfontApp = GetStockObject(ANSI_VAR_FONT);
    hdc = GetDC(NULL);
    SelectObject(hdc, hfontApp);
    GetTextMetrics(hdc, &tm);
    ReleaseDC(NULL, hdc);
    }

    dx = GetSystemMetrics (SM_CXSCREEN);
    dy = GetSystemMetrics (SM_CYSCREEN);

    ghwndApp =
	CreateWindowEx(
#ifdef BIDI
	    WS_EX_BIDI_SCROLL  | WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
			    0,
#endif
			    MAKEINTATOM(ID_APP),    // Class name
                            gszAppName,             // Caption
                            WS_OVERLAPPEDWINDOW,    // Style bits
                            CW_USEDEFAULT, 0,       // Position
                            320,300,                // Size
                            (HWND)NULL,             // Parent window (no parent)
                            (HMENU)NULL,            // use class menu
                            (HANDLE)hInst,          // handle to window instance
                            (LPSTR)NULL             // no params to pass on
                           );
    ShowWindow(ghwndApp,sw);

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )			       |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|	hInst		instance handle of this instance of the app	       |
|	hPrev		instance handle of previous instance, NULL if first    |
|       szCmdLine       ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    MSG     msg;

    /* Call initialization procedure */
    if (!AppInit(hInst,hPrev,sw,szCmdLine))
        return FALSE;

    /*
     * Polling messages from event queue
     */
    for (;;)
    {
        while (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                return msg.wParam;

	    if (TranslateAccelerator(ghwndApp, ghAccel, &msg))
		continue;
	
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

	//
	// If we have no messages to dispatch, we do our background task...
	// If we're playing a file, we set the scroll bar to show the video
	// frames corresponding with the current playing audio sample
	//
        if (gfPlaying) {
	    LONG    l;

	    //
	    // Use the audio clock to tell how long we've been playing.  To
	    // maintain sync, it's important we use this clock.
	    //
	    l = aviaudioTime(); 	// returns -1 if no audio playing

	    //
	    // If we can't use the audio clock to tell us how long we've been
	    // playing, calculate it ourself
	    //
	    if (l == -1)
		l = timeGetTime() - glPlayStartTime + glPlayStartPos;

	    if (l != GetScrollTime(ghwndApp)) {
	        if (l < timeStart)	// make sure number isn't out of bounds
		    l = timeStart;
	        if (l > timeEnd)	// looks like we're all done!
		    gfPlaying = FALSE;
		SetScrollTime(ghwndApp, l);
		InvalidateRect(ghwndApp, NULL, FALSE);
		UpdateWindow(ghwndApp);
		continue;
	    }
	}
	
	WaitMessage();
    }

    return msg.wParam;
}

typedef BYTE _huge * HPBYTE;
typedef int _huge *  HPINT;

void PaintAudio(HDC hdc, PRECT prc, PAVISTREAM pavi, LONG lStart, LONG lLen)
{
    PCMWAVEFORMAT wf;
    int i;
    int x,y;
    int w,h;
    BYTE b;
    HBRUSH hbr;
    RECT rc = *prc;
    LONG    lBytes;
    LONG    l, lLenOrig = lLen;
    LONG    lWaveBeginTime = AVIStreamStartTime(pavi);
    LONG    lWaveEndTime   = AVIStreamEndTime(pavi);

    static LPVOID lpAudio = NULL;

    // We've been told to draw some times that don't exist
    if (lStart < lWaveBeginTime) {
	lLen -= lWaveBeginTime - lStart;
	lStart = lWaveBeginTime;
	rc.left = rc.right - (int)muldiv32(rc.right - rc.left, lLen, lLenOrig);
    }

    if (lStart + lLen > lWaveEndTime) {
	lLenOrig = lLen;
	lLen = lWaveEndTime - lStart;
	rc.right = rc.left + (int)muldiv32(rc.right - rc.left, lLen, lLenOrig);
    }

    /* Now change and work with samples, not time. */
    l = lStart;
    lStart = AVIStreamTimeToSample(pavi, lStart);
    lLen = AVIStreamTimeToSample(pavi, l + lLen) - lStart;

    l = sizeof(wf);
    AVIStreamReadFormat(pavi, lStart, &wf, &l);
    if (!l)
        return;

    w = rc.right - rc.left;
    h = rc.bottom - rc.top;

    if (rc.left > prc->left) {
        SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));
	PatBlt(hdc, prc->left, rc.top, rc.left - prc->left,
						rc.bottom - rc.top, PATCOPY);
    }

    if (rc.right < prc->right) {
        SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));
	PatBlt(hdc, rc.right, rc.top, prc->right - rc.right,
						rc.bottom - rc.top, PATCOPY);
    }

#ifdef WIN32
#define BACKBRUSH  (RGB(0, 0, 0))
#define MONOBRUSH  (RGB(0,255,0))
#define LEFTBRUSH  (RGB(0,0,255))
#define RIGHTBRUSH (RGB(0,255,0))
#define HPOSBRUSH  (RGB(255,0,0))
#else
#define BACKBRUSH  (GetSysColor(COLOR_3DFACE))
#define MONOBRUSH  (GetSysColor(COLOR_3DSHADOW))
#define LEFTBRUSH  (RGB(0,0,255))
#define RIGHTBRUSH (RGB(0,255,0))
#define HPOSBRUSH  (RGB(255,0,0))
#endif

    hbr = SelectObject(hdc, CreateSolidBrush(BACKBRUSH));
    PatBlt(hdc, rc.left, rc.top, w, h, PATCOPY);
    DeleteObject(SelectObject(hdc, hbr));

    //
    // !!! we can only paint PCM data
    //
    if (wf.wf.wFormatTag != WAVE_FORMAT_PCM)
        return;

    lBytes = lLen * wf.wf.nChannels * wf.wBitsPerSample / 8;

    if (!lpAudio)
        lpAudio = GlobalAllocPtr (GHND, lBytes);
    else if ((LONG)GlobalSizePtr(lpAudio) < lBytes)
        lpAudio = GlobalReAllocPtr(lpAudio, lBytes, GMEM_MOVEABLE);

    if (!lpAudio)
        return;

    AVIStreamRead(pavi, lStart, lLen, lpAudio, lBytes, NULL, &l);

    if (l != lLen)
        ; // FatalAppExit(0, "BLAH!!!");

    lLen = l;

    if (lLen == 0)
	return;

#define MulDiv(a,b,c) (UINT)((DWORD)(UINT)(a) * (DWORD)(UINT)(b) / (UINT)(c))

    // !!! Flickers less painting it NOW or LATER?
    // First show the current position as a bar
    hbr = SelectObject(hdc, CreateSolidBrush(HPOSBRUSH));
    PatBlt(hdc, prc->right / 2, prc->top, 1, prc->bottom - prc->top, PATCOPY);
    DeleteObject(SelectObject(hdc, hbr));

    // Mono
    if (wf.wf.nChannels == 1) {

        hbr = SelectObject(hdc, CreateSolidBrush(MONOBRUSH));
        y = rc.top + h/2;
        PatBlt(hdc, rc.left, y, w, 1, PATCOPY);

        if (wf.wBitsPerSample == 8) {
            for (x=0; x<w; x++) {
                b = *((HPBYTE)lpAudio + muldiv32(x,lLen,w));

                if (b > 0x80) {
                    i = y - MulDiv(b-0x80,(h/2),128);
                    PatBlt(hdc, rc.left+x, i, 1, y-i, PATCOPY);
                }
                else {
                    i = y + MulDiv(0x80-b,(h/2),128);
                    PatBlt(hdc, rc.left+x, y, 1, i-y, PATCOPY);
                }
            }
        }
        else if (wf.wBitsPerSample == 16) {
            for (x=0; x<w; x++) {
                i = *((HPINT)lpAudio + muldiv32(x,lLen,w));
                if (i > 0) {
                   i = y - (int) ((LONG)i * (h/2) / 32768);
                   PatBlt(hdc, rc.left+x, i, 1, y-i, PATCOPY);
                }
                else {
                   i = (int) ((LONG)i * (h/2) / 32768);
                   PatBlt(hdc, rc.left+x, y, 1, -i, PATCOPY);
                }
            }
        }
        DeleteObject(SelectObject(hdc, hbr));
    } // endif mono

    // Stereo
    else if (wf.wf.nChannels == 2) {

        if (wf.wBitsPerSample == 8) {

            // Left channel
            hbr = SelectObject(hdc, CreateSolidBrush(LEFTBRUSH));
            y = rc.top + h/4;
            PatBlt(hdc, rc.left, y, w, 1, PATCOPY);

            for (x=0; x<w; x++) {
                b = *((HPBYTE)lpAudio + muldiv32(x,lLen,w) * 2);

                if (b > 0x80) {
                    i = y - MulDiv(b-0x80,(h/4),128);
                    PatBlt(hdc, rc.left+x, i, 1, y-i, PATCOPY);
                }
                else {
                    i = y + MulDiv(0x80-b,(h/4),128);
                    PatBlt(hdc, rc.left+x, y, 1, i-y, PATCOPY);
                }
            }

            // Right channel
            DeleteObject(SelectObject(hdc, hbr));
            hbr = SelectObject(hdc, CreateSolidBrush(RIGHTBRUSH));
            y = rc.top + h * 3 / 4;
            PatBlt(hdc, rc.left, y, w, 1, PATCOPY);

            for (x=0; x<w; x++) {
                b = *((HPBYTE)lpAudio + muldiv32(x,lLen,w) * 2 + 1);

                if (b > 0x80) {
                    i = y - MulDiv(b-0x80,(h/4),128);
                    PatBlt(hdc, rc.left+x, i, 1, y-i, PATCOPY);
                }
                else {
                    i = y + MulDiv(0x80-b,(h/4),128);
                    PatBlt(hdc, rc.left+x, y, 1, i-y, PATCOPY);
                }
            }
            DeleteObject(SelectObject(hdc, hbr));
        }

        else if (wf.wBitsPerSample == 16) {

            // Left channel
            hbr = SelectObject(hdc, CreateSolidBrush(LEFTBRUSH));
            y = rc.top + h/4;
            PatBlt(hdc, rc.left, y, w, 1, PATCOPY);

            for (x=0; x<w; x++) {
                i = *((HPINT)lpAudio + muldiv32(x,lLen,w) * 2);
                if (i > 0) {
                    i = y - (int) ((LONG)i * (h/4) / 32768);
                    PatBlt(hdc, rc.left+x, i, 1, y-i, PATCOPY);
                }
                else {
                    i = (int) ((LONG)i * (h/4) / 32768);
                    PatBlt(hdc, rc.left+x, y, 1, -i, PATCOPY);
                }
            }

            // Right channel
            DeleteObject(SelectObject(hdc, hbr));

            hbr = SelectObject(hdc, CreateSolidBrush(RIGHTBRUSH));
            y = rc.top + h * 3 / 4;
            PatBlt(hdc, rc.left, y, w, 1, PATCOPY);

            for (x=0; x<w; x++) {
                i = *((HPINT)lpAudio + muldiv32(x,lLen,w) * 2 + 1);
                if (i > 0) {
                   i = y - (int) ((LONG)i * (h/4) / 32768);
                   PatBlt(hdc, rc.left+x, i, 1, y-i, PATCOPY);
                }
                else {
                   i = (int) ((LONG)i * (h/4) / 32768);
                   PatBlt(hdc, rc.left+x, y, 1, -i, PATCOPY);
                }
            }
            DeleteObject(SelectObject(hdc, hbr));
        }
    } // endif stereo
}

/*----------------------------------------------------------------------------*\
|   AppWndProc( hwnd, uiMessage, wParam, lParam )			       |
|                                                                              |
|   Description:                                                               |
|       The window proc for the app's main (tiled) window.  This processes all |
|       of the parent window's messages.                                       |
|                                                                              |
|   Arguments:                                                                 |
|	hwnd		window handle for the window			       |
|       uiMessage       message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       0 if processed, nonzero if ignored                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
LONG FAR PASCAL _export AppWndProc(hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    WPARAM     wParam;
    LPARAM     lParam;
{
    PAINTSTRUCT ps;
    BOOL        f;
    HDC         hdc;
    TCHAR        ach[200];
    int	        iFrameWidth;
    int		iLen;
    LONG        lFrame, lCurFrame;
    int		n;
    int         nFrames;
    LPBITMAPINFOHEADER lpbi;
    static      BYTE abFormat[1024];
    LONG        l;
    LONG	lTime;
    LONG        lSize = 0;
    LONG	lAudioStart;
    LONG	lAudioLen;
    RECT        rcFrame, rcC;
    int		yStreamTop;
    int         i;
    HBRUSH      hbr;
    RECT	rc;
    HRESULT	hr;

    switch (msg) {
        case WM_CREATE:
            if (gachFileName[0])
                InitAvi(hwnd, gachFileName, MENU_OPEN);
	    break;

        case WM_COMMAND:
            return AppCommand(hwnd,msg,wParam,lParam);

        case WM_INITMENU:
            f = gcpavi > 0;
            EnableMenuItem((HMENU)wParam, MENU_SAVE,   MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_SAVEAS, f ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_SAVERAW,f ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_OPTIONS,f ? MF_ENABLED : MF_GRAYED);

            f = gcpavi > 0;
            EnableMenuItem((HMENU)wParam, MENU_NEW,    f ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_ADD,    f ? MF_ENABLED : MF_GRAYED);
	
            EnableMenuItem((HMENU)wParam, MENU_COPY,   f ? MF_ENABLED : MF_GRAYED);

            EnableMenuItem((HMENU)wParam, MENU_PLAY_STREAM,f ? MF_ENABLED : MF_GRAYED);

	    f = gpfile != 0;
            EnableMenuItem((HMENU)wParam, MENU_CFILE,  f ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_PLAY_FILE,  f ? MF_ENABLED : MF_GRAYED);

	    f = TRUE;
	    EnableMenuItem((HMENU)wParam, MENU_PASTE,  f ? MF_ENABLED : MF_GRAYED);
	
            f = FALSE;
            EnableMenuItem((HMENU)wParam, MENU_CUT,    f ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_DELETE, f ? MF_ENABLED : MF_GRAYED);
	    EnableMenuItem((HMENU)wParam, MENU_MARK,   f ? MF_ENABLED : MF_GRAYED);
	
	    f = gfAudioFound;
            EnableMenuItem((HMENU)wParam, MENU_SAVEWAVE,f ? MF_ENABLED : MF_GRAYED);
	
	    f = gfVideoFound || gfAudioFound;
            EnableMenuItem((HMENU)wParam, MENU_PLAY,
			(f && !gfPlaying) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, MENU_STOP,
                        (f && gfPlaying) ? MF_ENABLED : MF_GRAYED);

	    CheckMenuItem((HMENU)wParam, MENU_ZOOMQUARTER,
		    (gwZoom == 1) ? MF_CHECKED : MF_UNCHECKED);
	    CheckMenuItem((HMENU)wParam, MENU_ZOOMHALF,
		    (gwZoom == 2) ? MF_CHECKED : MF_UNCHECKED);
	    CheckMenuItem((HMENU)wParam, MENU_ZOOM1,
		    (gwZoom == 4) ? MF_CHECKED : MF_UNCHECKED);
	    CheckMenuItem((HMENU)wParam, MENU_ZOOM2,
		    (gwZoom == 8) ? MF_CHECKED : MF_UNCHECKED);
	    CheckMenuItem((HMENU)wParam, MENU_ZOOM4,
		    (gwZoom == 16) ? MF_CHECKED : MF_UNCHECKED);
	    	
            break;

	case WM_NCHITTEST:
	    if (fWait)
	    {
		lParam = DefWindowProc(hwnd,msg,wParam,lParam);

		if (lParam == HTMENU)
		    lParam = HTCLIENT;

		return lParam;
	    }
	    break;

	case WM_SIZE:
	    // Set vertical scrollbar for scrolling streams
	    GetClientRect(hwnd, &rc);
	    if (vertHeight > rc.bottom) {
	        vertSBLen = vertHeight - rc.bottom;
	        SetScrollRange(hwnd, SB_VERT, 0, (int)vertSBLen, TRUE);
	    } else {
	        vertSBLen = 0;
	        SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);
	    }
	    break;

        case WM_SETCURSOR:
            if (fWait && LOWORD(lParam) == HTCLIENT)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                return TRUE;
            }
            break;

        case WM_DESTROY:
            FreeAvi(hwnd);
	    RevokeObjects();
	    AVIStreamExit();
	    PostQuitMessage(0);
	    break;

        case WM_CLOSE:
	    if (fWait)
		return 0;
            break;

	case WM_SYSCOMMAND:
	    switch (wParam & 0xFFF0) {
		case SC_KEYMENU:
		    if (fWait)	    // block keyboard access to menus if waiting
			return 0;
		    break;
	    }
	    break;

        case WM_PALETTECHANGED:
            if ((HWND)wParam == hwnd)
                break;

	case WM_QUERYNEWPALETTE:
            hdc = GetDC(hwnd);

            if (f = DrawDibRealize(ghdd[0], hdc, FALSE)) // !!! stream #
                InvalidateRect(hwnd,NULL,TRUE);

            ReleaseDC(hwnd,hdc);

            return f;
	
        case WM_ERASEBKGND:
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd,&ps);

            SelectObject(hdc, hfontApp);

            #define PRINT(sz) \
                (TextOut(hdc, TSPACE, yStreamTop, sz, lstrlen(sz)), \
                yStreamTop += tm.tmHeight+1)

            #define PF1(sz,a)                   (wsprintf(ach, sz, a), PRINT(ach))
            #define PF2(sz,a,b)                 (wsprintf(ach, sz, a, b), PRINT(ach))
            #define PF3(sz,a,b,c)               (wsprintf(ach, sz, a, b, c), PRINT(ach))
            #define PF4(sz,a,b,c,d)             (wsprintf(ach, sz, a, b, c, d), PRINT(ach))
            #define PF5(sz,a,b,c,d,e)           (wsprintf(ach, sz, a, b, c, d, e), PRINT(ach))
            #define PF6(sz,a,b,c,d,e,f)         (wsprintf(ach, sz, a, b, c, d, e, f), PRINT(ach))
            #define PF7(sz,a,b,c,d,e,f,g)       (wsprintf(ach, sz, a, b, c, d, e, f, g), PRINT(ach))
            #define PF8(sz,a,b,c,d,e,f,g,h)     (wsprintf(ach, sz, a, b, c, d, e, f, g, h), PRINT(ach))
            #define PF9(sz,a,b,c,d,e,f,g,h,i)   (wsprintf(ach, sz, a, b, c, d, e, f, g, h, i), PRINT(ach))

	    GetClientRect(hwnd, &rcC);

	    lTime = GetScrollTime(hwnd);
	    yStreamTop = -GetScrollPos(hwnd, SB_VERT);

	    for (i=0; i<gcpavi; i++) {
		AVISTREAMINFO		avis;
                BOOL                    fFirstFrame = TRUE;

                LONG                    lPos;

                LONG                    lNearKey;
                LONG                    lPrevKey;
                LONG                    lNextKey;

                LONG                    lNearAny;
                LONG                    lPrevAny;
                LONG                    lNextAny;

                LONG                    lNearFmt;
                LONG                    lPrevFmt;
                LONG                    lNextFmt;

		LONG			lEnd, lEndTime;

                // the idea is to allow stream select!
                gaiStreamTop[i] = yStreamTop;
			
                hr = AVIStreamInfo(gapavi[i], &avis, sizeof(avis));
                FIXCC(avis.fccHandler);
                FIXCC(avis.fccType);

		l = sizeof(abFormat);
                hr = AVIStreamReadFormat(gapavi[i],0, &abFormat, &l);

#ifdef WIN32
                PF7(TEXT("Stream%d [%4.4hs/%4.4hs] Start: %ld Length: %ld (%ld.%03ld sec)             "),
			    i,
                            (LPSTR)&avis.fccType,
                            (LPSTR)&avis.fccHandler,
			    AVIStreamStart(gapavi[i]),
                            AVIStreamLength(gapavi[i]),
                            AVIStreamLengthTime(gapavi[i]) / 1000,
			    AVIStreamLengthTime(gapavi[i]) % 1000);
#else
                PF7(TEXT("Stream%d [%4.4ls/%4.4ls] Start: %ld Length: %ld (%ld.%03ld sec)             "),
			    i,
                            (LPSTR)&avis.fccType,
                            (LPSTR)&avis.fccHandler,
			    AVIStreamStart(gapavi[i]),
                            AVIStreamLength(gapavi[i]),
                            AVIStreamLengthTime(gapavi[i]) / 1000,
			    AVIStreamLengthTime(gapavi[i]) % 1000);
#endif

                lPos = AVIStreamTimeToSample(gapavi[i], lTime);
                AVIStreamSampleSize(gapavi[i], lPos, &lSize);

                lNearKey = AVIStreamFindSample(gapavi[i], lPos, FIND_PREV|FIND_KEY);

#ifdef TEST_FINDSAMPLE
                lNearAny = AVIStreamFindSample(gapavi[i], lPos, FIND_PREV|FIND_ANY);
                lNearFmt = AVIStreamFindSample(gapavi[i], lPos, FIND_PREV|FIND_FORMAT);

                lPrevKey = AVIStreamFindSample(gapavi[i], lPos-1, FIND_PREV|FIND_KEY);
                lPrevAny = AVIStreamFindSample(gapavi[i], lPos-1, FIND_PREV|FIND_ANY);
                lPrevFmt = AVIStreamFindSample(gapavi[i], lPos-1, FIND_PREV|FIND_FORMAT);

                lNextKey = AVIStreamFindSample(gapavi[i], lPos+1, FIND_NEXT|FIND_KEY);
                lNextAny = AVIStreamFindSample(gapavi[i], lPos+1, FIND_NEXT|FIND_ANY);
                lNextFmt = AVIStreamFindSample(gapavi[i], lPos+1, FIND_NEXT|FIND_FORMAT);
#endif
                PF5(TEXT("Pos:%ld Time:%ld.%03ld sec Size:%ld bytes %s                                 "),
                        lPos, lTime/1000, lTime%1000, lSize,
                        (LPTSTR)(lPos == lNearKey ? TEXT("Key") : TEXT("")));

#ifdef TEST_FINDSAMPLE
                PF9(TEXT("PrevKey=%ld NearKey=%ld NextKey=%ld, PrevAny=%ld NearAny=%ld NextAny=%ld, PrevFmt=%ld NearFmt=%ld NextFmt=%ld                      "),
                    lPrevKey, lNearKey, lNextKey,
                    lPrevAny, lNearAny, lNextAny,
                    lPrevFmt, lNearFmt, lNextFmt);
#endif

                if (avis.fccType == streamtypeVIDEO) {

                    lpbi = (LPBITMAPINFOHEADER)abFormat;
                    FIXCC(lpbi->biCompression);

                    //
                    // display video format
                    //
                    //  Video: 160x120x8 (cram)
                    //
#ifdef WIN32
                    PF4(TEXT("Format: %dx%dx%d (%4.4hs)"),
                        (int)lpbi->biWidth,
                        (int)lpbi->biHeight,
                        (int)lpbi->biBitCount,
                        (LPSTR)&lpbi->biCompression);
#else
                    PF4(TEXT("Format: %dx%dx%d (%4.4s)"),
                        (int)lpbi->biWidth,
                        (int)lpbi->biHeight,
                        (int)lpbi->biBitCount,
                        (LPSTR)&lpbi->biCompression);
#endif

		    //
		    // Which frame belongs at this time?
		    //
		    lEndTime = AVIStreamEndTime(gapavi[i]);
		    if (lTime <= lEndTime)
		        lFrame = AVIStreamTimeToSample(gapavi[i], lTime);
		    else {	// we've scrolled past the end of this stream
		        lEnd = AVIStreamEnd(gapavi[i]);
		        lFrame = lEnd + AVIStreamTimeToSample(
				gapavi[i], lTime - lEndTime);
		    }

                    yStreamTop += TSPACE/2;

		    // how wide is each frame to paint?
		    iFrameWidth = (avis.rcFrame.right - avis.rcFrame.left) *
			gwZoom / 4 + HSPACE;

		    //
		    // how many frames can we fit on each half of the screen?
		    //
		    nFrames = (rcC.right - iFrameWidth) / (2 * iFrameWidth);
		    if (nFrames < 0)
		        nFrames = 0;    // at least draw *something*

		    // Step through all the frames we'll draw
		    for (n=-nFrames; n<=nFrames; n++)
		    {
			// !!! If this code didn't have awful rounding errors,
			// I wouldn't need to special case the first stream.
			if (gapavi[i] == gpaviVideo) {
			    // by definition, we know what frame we're drawing.
			    lCurFrame = lFrame + n;

			    // what time is it at that frame?
			    l = AVIStreamSampleToTime(gapavi[i], lCurFrame);
			} else {
			    // What time is it at this pixel?
			    l = lTime + muldiv32(n * iFrameWidth, gdwMicroSecPerPixel, 1000);

			    // Get the frame
			    lCurFrame = AVIStreamTimeToSample(gapavi[i], l);
			}
			
			// !!!
			// Could actually return an LPBI for invalid frames
			// so we better force it to NULL.
			//
			if (gapgf[i] && lCurFrame >= AVIStreamStart(gapavi[i]))
		            lpbi = AVIStreamGetFrame(gapgf[i], lCurFrame);
			else
			    lpbi = NULL;

		        // Location of the current frame
		        rcFrame.left   = rcC.right / 2 -
			    ((avis.rcFrame.right - avis.rcFrame.left) *
				gwZoom/4)/2+ (n * iFrameWidth);
		        rcFrame.top    = yStreamTop;
		        rcFrame.right  = rcFrame.left +
				(avis.rcFrame.right - avis.rcFrame.left)*gwZoom/4;
		        rcFrame.bottom = rcFrame.top +
				(avis.rcFrame.bottom - avis.rcFrame.top)*gwZoom/4;

		        //
		        // draw frame around current frame.
		        //
		        if (n == 0)
			    hbr = CreateSolidBrush(RGB(255,0,0));
		        else
			    hbr = CreateSolidBrush(RGB(255,255,255));

		        InflateRect(&rcFrame, 1, 1);
		        FrameRect(hdc, &rcFrame, hbr);
		        InflateRect(&rcFrame, -1, -1);
		        DeleteObject (hbr);

		        if (lpbi)
		        {
			    DrawDibDraw(ghdd[i],hdc,
			        rcFrame.left, rcFrame.top,
			        rcFrame.right - rcFrame.left,
			        rcFrame.bottom - rcFrame.top,
			        lpbi, NULL,
				0, 0, -1, -1,
				((gapavi[i] == gpaviVideo) && fFirstFrame) ?
					0 : DDF_BACKGROUNDPAL);

			    fFirstFrame = FALSE;

			    iLen = wsprintf(ach, TEXT("%ld %ld.%03lds"),
			        lCurFrame, l/1000, l%1000);
		        }
		        else
		        {
			    if (gapgf[i])
  			        SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
  			    else
  			        SelectObject(hdc,GetStockObject(LTGRAY_BRUSH));

			    PatBlt(hdc,
			        rcFrame.left, rcFrame.top,
			        rcFrame.right - rcFrame.left,
			        rcFrame.bottom - rcFrame.top,
			        PATCOPY);
			    iLen = 0;
			    ach[0] = TEXT('\0');
		        }

			rc.left = rcFrame.left;
			rc.right = rcFrame.right + HSPACE;
			rc.top = rcFrame.bottom + 2;
			rc.bottom = rc.top + TSPACE;
			ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE,
				   &rc, ach, iLen, NULL);
		    }
		
                    yStreamTop += TSPACE + avis.rcFrame.bottom*gwZoom/4;
		}
		else if (avis.fccType == streamtypeAUDIO)
                {
                    LPWAVEFORMAT pwf = (LPWAVEFORMAT)abFormat;
                    TCHAR *szFmt;

                    if (pwf->wFormatTag == 1) {  // PCM
                        if (pwf->nChannels == 1)
                            szFmt = TEXT("Format: Mono %dHz %dbit");
                        else
                            szFmt = TEXT("Format: Stereo %dHz %dbit");
                    }
                    else if (pwf->wFormatTag == 2) {  // ADPCM
                        if (pwf->nChannels == 1)
                            szFmt = TEXT("Format: ADPCM Mono %dHz %dbit");
                        else
                            szFmt = TEXT("Format: ADPCM Stereo %dHz %dbit");
                    }
                    else {
                        if (pwf->nChannels == 1)
                            szFmt = TEXT("Format: Compressed Mono %dHz %dbit");
                        else
                            szFmt = TEXT("Format: Compressed Stereo %dHz %dbit");
                    }

                    PF2(szFmt,(int)pwf->nSamplesPerSec,
                        (int)(pwf->nAvgBytesPerSec * 8 / pwf->nSamplesPerSec));

		    lAudioStart = lTime - muldiv32(rcC.right / 2,
						gdwMicroSecPerPixel, 1000);
		    lAudioLen = 2 * (lTime - lAudioStart);

                    // !!! Fix the SAMPLE field for short audio clips !!!
                    //PF2("Sample:%ld %ld                        ",
                    //            AVIStreamTimeToSample(gapavi[i], lAudioStart),
                    //            AVIStreamTimeToSample(gapavi[i], lAudioLen));

		    /* Make rectangle to draw audio into */
		    rc.left = rcC.left;
		    rc.right = rcC.right;
                    rc.top = yStreamTop;
		    rc.bottom = rc.top + AUDIOVSPACE * gwZoom / 4;

		    PaintAudio(hdc, &rc, gapavi[i], lAudioStart, lAudioLen);

                    yStreamTop += rc.bottom - rc.top;
		}
		else if (avis.fccType == streamtypeTEXT)
		{
		    LONG    lPos;
		    int	    iLeft;

		    lPos = AVIStreamTimeToSample(gapavi[i],
						 lTime -
						 muldiv32((rcC.right - rcC.left),
							gdwMicroSecPerPixel,
							1000));

		    if (lPos < 0)
			lPos = 0;

		    PatBlt(hdc, rcC.left, yStreamTop,
			   rcC.right - rcC.left, TSPACE + TSPACE,
			   WHITENESS);
		
                    while (lPos < AVIStreamEnd(gapavi[i]) - 1) {

			// What pixel is it at this time?
			iLeft = (rcC.right + rcC.left) / 2 +
				    (int) muldiv32(AVIStreamSampleToTime(gapavi[i], lPos) - lTime,
					     1000,  gdwMicroSecPerPixel);

			if (iLeft >= rcC.right)
			    break;

                        AVIStreamRead(gapavi[i], lPos, 1, ach, sizeof(ach), &l, NULL);

			if (l)
			    TextOut(hdc, iLeft, yStreamTop, ach, (int) l - 1);

			iLen = wsprintf(ach, TEXT("%ld"), lPos);
			TextOut(hdc, iLeft, yStreamTop + TSPACE, ach, iLen);

#if 0
			if (ghic[i]) {
			    ICDrawBegin(ghic[i],
					0,
					0,
					hwnd,
					hdc,
					iLeft,
					yStreamTop,
					100,
					50,
					NULL,
					0,
					0,
					-1,
					-1,
					1,
					1);
	
			    ICDraw(ghic[i], 0,
				   NULL, ach, l, lPos);
			    ICDrawEnd(ghic[i]);
			}
#endif		
			lPos += 1;
                    }

		    yStreamTop += TSPACE + TSPACE;
		}
		else
		{
		}

                yStreamTop += VSPACE;

		if (yStreamTop >= rcC.bottom)
		    break;
	    }

            EndPaint(hwnd,&ps);
            break;

	case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_UP:    PostMessage (hwnd,WM_VSCROLL,SB_LINEUP,0L);   break;
                case VK_DOWN:  PostMessage (hwnd,WM_VSCROLL,SB_LINEDOWN,0L); break;
                case VK_PRIOR: PostMessage (hwnd,WM_HSCROLL,SB_PAGEUP,0L);   break;
                case VK_NEXT:  PostMessage (hwnd,WM_HSCROLL,SB_PAGEDOWN,0L); break;
                case VK_HOME:  PostMessage (hwnd,WM_HSCROLL,SB_THUMBPOSITION,0L);     break;
                case VK_END:   PostMessage (hwnd,WM_HSCROLL,SB_THUMBPOSITION,0x7FFF); break;
                case VK_LEFT:  PostMessage (hwnd,WM_HSCROLL,SB_LINEUP,0L);   break;
                case VK_RIGHT: PostMessage (hwnd,WM_HSCROLL,SB_LINEDOWN,0L); break;
	    }
	    break;

        case WM_HSCROLL:
            l = GetScrollTime(hwnd);

            switch (GET_WM_HSCROLL_CODE(wParam, lParam)) {
                case SB_LINEDOWN:      l += timehscroll;  break;
                case SB_LINEUP:        l -= timehscroll;  break;
                case SB_PAGEDOWN:      l += timeLength/10; break;
                case SB_PAGEUP:        l -= timeLength/10; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
			l = GET_WM_HSCROLL_POS(wParam, lParam);
			l = timeStart + muldiv32(l, timeLength, SCROLLRANGE);
			break;
            }

	    if (l < timeStart)
		l = timeStart;

	    if (l > timeEnd)
		l = timeEnd;

	    if (l == GetScrollTime(hwnd))
		break;
	
	    SetScrollTime(hwnd, l);
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            break;

        case WM_VSCROLL:
            l = GetScrollPos(hwnd, SB_VERT);
	    GetClientRect(hwnd, &rc);

            switch (GET_WM_VSCROLL_CODE(wParam, lParam)) {
                case SB_LINEDOWN:      l += 10;  break;
                case SB_LINEUP:        l -= 10;  break;
                case SB_PAGEDOWN:      l += rc.bottom; break;
                case SB_PAGEUP:        l -= rc.bottom; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
		    l = GET_WM_VSCROLL_POS(wParam, lParam);
		    break;
            }

	    if (l < 0)
		l = 0;

	    if (l > vertSBLen)
		l = vertSBLen;

	    if (l == GetScrollPos(hwnd, SB_VERT))
		break;
	
	    SetScrollPos(hwnd, SB_VERT, (int)l, TRUE);
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;

	case MM_WOM_OPEN:
	case MM_WOM_DONE:
	case MM_WOM_CLOSE:
	    aviaudioMessage(hwnd, msg, wParam, lParam);
	    break;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

BOOL FAR PASCAL _export SaveCallback(int iProgress)
{
    TCHAR    ach[128];

    wsprintf(ach, TEXT("%s - Saving %s: %d%%"),
        (LPTSTR) gszAppName, (LPTSTR) gachSaveFileName, iProgress);

    SetWindowText(ghwndApp, ach);
    return WinYield();
}

LONG NEAR PASCAL AppCommand (hwnd, msg, wParam, lParam)
    HWND     hwnd;
    unsigned msg;
    WPARAM     wParam;
    LPARAM     lParam;
{
    OPENFILENAME ofn;
    TCHAR	achFilter[128];

    switch(GET_WM_COMMAND_ID(wParam, lParam))
    {
	case MENU_EXIT:
	    PostMessage(hwnd,WM_CLOSE,0,0L);
            break;

	// Set the compression options for each stream - pass an array of
	// streams and an array of compression options structures
        case MENU_OPTIONS:
            AVISaveOptions(hwnd, ICMF_CHOOSE_KEYFRAME | ICMF_CHOOSE_DATARATE
			| ICMF_CHOOSE_PREVIEW,
		gcpavi, gapavi, galpAVIOptions);
	    break;
	
        case MENU_SAVEAS:
	case MENU_SAVERAW:
	case MENU_SAVESMALL:

            gachSaveFileName[0] = 0;

            /* prompt user for file to open */
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.hInstance = NULL;
	    AVIBuildFilter(achFilter, sizeof(achFilter)/sizeof(TCHAR), TRUE);
            ofn.lpstrFilter = achFilter;
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter = 0;
            ofn.nFilterIndex = 0;
            ofn.lpstrFile = gachSaveFileName;
            ofn.nMaxFile = sizeof(gachSaveFileName)/sizeof(TCHAR);
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
	    if (GET_WM_COMMAND_ID(wParam, lParam) == MENU_SAVEAS)
                ofn.lpstrTitle = TEXT("Save AVI File");
	    else if (GET_WM_COMMAND_ID(wParam, lParam) == MENU_SAVERAW)
                ofn.lpstrTitle = TEXT("Save Raw");
	    else if (GET_WM_COMMAND_ID(wParam, lParam) == MENU_SAVESMALL)
                ofn.lpstrTitle = TEXT("Save Small");
	    else
                ofn.lpstrTitle = TEXT("Please give me a title");
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
			    OFN_OVERWRITEPROMPT;
            ofn.nFileOffset = 0;
            ofn.nFileExtension = 0;
            ofn.lpstrDefExt = TEXT("avi");
            ofn.lCustData = 0;
            ofn.lpfnHook = NULL;
            ofn.lpTemplateName = NULL;

            if (GetSaveFileName(&ofn))
            {
                FARPROC lpfn = MakeProcInstance(SaveCallback, ghInstApp);

		if (lpfn)
		{
		    DWORD	fccHandler[MAXNUMSTREAMS];
		    int		i;
		
		    StartWait();

		    for (i = 0; i < gcpavi; i++)
		        fccHandler[i] = galpAVIOptions[i]->fccHandler;

		    // We only want to save the first video stream
		    if (GET_WM_COMMAND_ID(wParam, lParam) == MENU_SAVESMALL) {
			SaveSmall(gpaviVideo, gachSaveFileName);
			EndWait();
			break;
		    }
		
		    // !!! This won't take away audio compression !!!
		    // We want to save raw -- don't use any video compression
		    if (GET_WM_COMMAND_ID(wParam, lParam) == MENU_SAVERAW)
		        for (i = 0; i < gcpavi; i++)
			    galpAVIOptions[i]->fccHandler = 0;


		    AVISaveV(gachSaveFileName,
			     NULL,
			     (AVISAVECALLBACK) lpfn,
			     gcpavi,
			     gapavi,
			     galpAVIOptions);
		    // !!! error check?

		    // Now put the video compressors back that we stole
		    for (i = 0; i < gcpavi; i++)
		        galpAVIOptions[i]->fccHandler = fccHandler[i];
		
		    EndWait();
		    FreeProcInstance(lpfn);
		    FixWindowTitle(hwnd);
		}
            }
	    break;

	case MENU_NEW:
	    FreeAvi(hwnd);
	    gachFileName[0] = TEXT('\0');
	    FixWindowTitle(hwnd);
	    break;
	
        case MENU_OPEN:
	case MENU_ADD:
            gachFileName[0] = 0;

            /* prompt user for file to open */
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwnd;
            ofn.hInstance = NULL;
	    if (wParam == MENU_ADD)
	    {
		ofn.lpstrTitle = TEXT("Merge With");
	    }
	    else
	    {
		ofn.lpstrTitle = TEXT("Open AVI");
	    }

	    if (gachFilter[0] == TEXT('\0'))
		AVIBuildFilter(gachFilter, sizeof(gachFilter)/sizeof(TCHAR), FALSE);
	
	    ofn.lpstrFilter = gachFilter;
            ofn.lpstrCustomFilter = NULL;
            ofn.nMaxCustFilter = 0;
            ofn.nFilterIndex = 0;
            ofn.lpstrFile = gachFileName;
            ofn.nMaxFile = sizeof(gachFileName)/sizeof(TCHAR);
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.nFileOffset = 0;
            ofn.nFileExtension = 0;
            ofn.lpstrDefExt = NULL;
            ofn.lCustData = 0;
            ofn.lpfnHook = NULL;
            ofn.lpTemplateName = NULL;

            if (GetOpenFileNamePreview(&ofn))
            {
		InitAvi(hwnd, gachFileName, wParam);
            }
	    break;

	case MENU_BALL:
	    InitBall(hwnd);
	    break;
	
	case MENU_ZOOMQUARTER:
	    gwZoom = 1;
	    FixScrollbars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
	    break;
	
	case MENU_ZOOMHALF:
	    gwZoom = 2;
	    FixScrollbars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
	    break;
	
	case MENU_ZOOM1:
	    gwZoom = 4;
	    FixScrollbars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
	    break;
	
	case MENU_ZOOM2:
	    gwZoom = 8;
	    FixScrollbars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
	    break;
	
	case MENU_ZOOM4:
	    gwZoom = 16;
	    FixScrollbars(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

	//
        // readly play the file, via MCI
	//
        case MENU_PLAY_STREAM:
        case MENU_PLAY_FILE:
            {
                TCHAR ach[80];
                HWND hwndMci = MCIWndCreate(NULL, ghInstApp,
                        MCIWNDF_SHOWNAME | MCIWNDF_SHOWMODE |
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        NULL);

                if (hwndMci) {
                    if (GET_WM_COMMAND_ID(wParam,lParam) == MENU_PLAY_FILE)
                        wsprintf(ach, TEXT("AVIVideo!@%ld"), gpfile);
                    else
                        wsprintf(ach, TEXT("AVIVideo!@%ld"), gapavi[giCurrentStream]);

                    MCIWndOpen(hwndMci, ach, 0);
                    MCIWndPlay(hwndMci);
                }
            }
	    break;

	//
	// Simulate playing the file.  We just play the 1st audio stream and let
	// our main message loop scroll the video by whenever it's bored.
	//
	case MENU_PLAY:
	    if (gfAudioFound)
	        aviaudioPlay(hwnd,
			 gpaviAudio,
			 AVIStreamTimeToSample(gpaviAudio, GetScrollTime(hwnd)),
			 AVIStreamEnd(gpaviAudio),
			 FALSE);
	    gfPlaying = TRUE;
	    glPlayStartTime = timeGetTime();
	    glPlayStartPos = GetScrollTime(hwnd);
	    break;

	//
	// Stop the play preview
	//
	case MENU_STOP:
	    if (gfAudioFound)
	        aviaudioStop();
	    gfPlaying = FALSE;
	    break;

#ifdef CLIPSTUFF
	case MENU_COPY:
	{
	    PAVIFILE	    pf;
	
	    AVIMakeFileFromStreams(&pf, gcpavi, gapavi);
	    AVIPutFileOnClipboard(pf);
	    AVIFileClose(pf);
	}
	    break;

	case MENU_CFILE:
	    AVIPutFileOnClipboard(gpfile);
	    break;
	
	case MENU_PASTE:
	{
	    PAVIFILE		pf  = NULL;

	    AVIGetFromClipboard(&pf);

	    if (pf) {
		DPF("Pasting file from clipboard....\n");
		FreeDrawStuff(hwnd);

		InsertAVIFile(pf, hwnd, TEXT("Clipboard"));
	    } else {
		DPF("Can't get file from clipboard....\n");
	    }
	    break;
	}
#endif
    }
    return 0L;
}

/*----------------------------------------------------------------------------*\
|   ErrMsg - Opens a Message box with a error message in it.  The user can     |
|            select the OK button to continue                                  |
\*----------------------------------------------------------------------------*/
int ErrMsg (LPTSTR sz,...)
{
    static TCHAR ach[2000];
    va_list va;

    va_start(va, sz);
    wvsprintf (ach,sz, va);
    va_end(va);
    MessageBox(NULL,ach,NULL,
#ifdef BIDI
		MB_RTL_READING |
#endif
    MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   fDialog(id,hwnd,fpfn)						       |
|									       |
|   Description:                                                               |
|	This function displays a dialog box and returns the exit code.	       |
|	the function passed will have a proc instance made for it.	       |
|									       |
|   Arguments:                                                                 |
|	id		resource id of dialog to display		       |
|	hwnd		parent window of dialog 			       |
|	fpfn		dialog message function 			       |
|                                                                              |
|   Returns:                                                                   |
|	exit code of dialog (what was passed to EndDialog)		       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL fDialog(int id,HWND hwnd,FARPROC fpfn)
{
    BOOL	f;
    HANDLE	hInst;

    hInst = GetWindowInstance(hwnd);
    fpfn  = MakeProcInstance(fpfn,hInst);
    f = DialogBox(hInst,MAKEINTRESOURCE(id),hwnd,fpfn);
    FreeProcInstance (fpfn);
    return f;
}

/*****************************************************************************
 *
 * dprintf() is called by the DPF macro if DEBUG is defined at compile time.
 *
 * The messages will be send to COM1: like any debug message. To
 * enable debug output, add the following to WIN.INI :
 *
 * [debug]
 * ICSAMPLE=1
 *
 ****************************************************************************/

#ifdef DEBUG

#define MODNAME "AVIVIEW"

static void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];

    static BOOL fDebug = -1;
    va_list va;

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", MODNAME, FALSE);

    if (!fDebug)
        return;

    lstrcpyA(ach, MODNAME ": ");
    va_start(va, szFormat);
    wvsprintfA(ach+lstrlenA(ach),szFormat, va);
    va_end(va);
//    lstrcat(ach, "\r\n");

    OutputDebugStringA(ach);
}

#endif


// !!! function that makes DIBs half as big
BOOL CrunchDIB(
    LPBITMAPINFOHEADER  lpbiSrc,    // BITMAPINFO of source
    LPVOID              lpSrc,      // input bits to crunch
    LPBITMAPINFOHEADER  lpbiDst,    // BITMAPINFO of dest
    LPVOID              lpDst);     // output bits to crunch


//
// Save a video stream into a new file, after calling CrunchDib on each frame...
//
void SaveSmall(PAVISTREAM ps, LPTSTR lpFilename)
{
    PAVIFILE		pf;
    PAVISTREAM		psSmall = NULL;
    HRESULT		hr;
    AVISTREAMINFO	strhdr;
    BITMAPINFOHEADER	bi;
    BITMAPINFOHEADER	biNew;
    LONG		l;
	
    LPVOID		lpOld = NULL;
    LPVOID		lpNew = NULL;

    AVIStreamFormatSize(ps, 0, &l);
    if (l > sizeof(bi))
	return;

    l = sizeof(bi);
    hr = AVIStreamReadFormat(ps, 0, &bi, &l);
    if (bi.biCompression != BI_RGB)
	return;

    hr = AVIStreamInfo(ps, &strhdr, sizeof(strhdr));

    hr = AVIFileOpen(&pf, lpFilename, OF_WRITE | OF_CREATE, NULL);
    if (hr != 0)
	return;

    biNew = bi;
    biNew.biWidth /= 2;
    biNew.biHeight /= 2;
    biNew.biSizeImage = ((((UINT)biNew.biBitCount * biNew.biWidth + 31)&~31) / 8) *
			biNew.biHeight;

    SetRect(&strhdr.rcFrame, 0, 0, (int) biNew.biWidth, (int) biNew.biHeight);

    hr = AVIFileCreateStream(pf, &psSmall, &strhdr);
    if (hr != 0) {
	goto exit;
    }

    hr = AVIStreamSetFormat(psSmall, 0, &biNew, sizeof(biNew));
    if (hr != 0) {
	goto exit;
    }

    lpOld = GlobalAllocPtr(GMEM_MOVEABLE, bi.biSizeImage);
    lpNew = GlobalAllocPtr(GMEM_MOVEABLE, biNew.biSizeImage);

    if (!lpOld || !lpNew) {
	goto exit;
    }

    for (l = AVIStreamStart(ps); l < AVIStreamEnd(ps); l++) {
	hr = AVIStreamRead(ps, l, 1, lpOld, bi.biSizeImage, NULL, NULL);
	// !!! error check
	
	CrunchDIB(&bi, lpOld, &biNew, lpNew);

	hr = AVIStreamWrite(psSmall, l, 1, lpNew, biNew.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	// !!! error check
    }

exit:
    if (lpOld)
	GlobalFreePtr(lpOld);
    if (lpNew)
	GlobalFreePtr(lpNew);
    if (psSmall)
	AVIStreamClose(psSmall);
    AVIFileClose(pf);
}
