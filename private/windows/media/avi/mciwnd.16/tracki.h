/*
	TrackBar
	
	All the useful information for a trackbar.
*/

typedef struct {
        HWND    hwnd;           // our window handle
        HDC     hdc;            // current DC

        LONG    lLogMin;        // Logical minimum
        LONG    lLogMax;        // Logical maximum
        LONG    lLogPos;        // Logical position

        LONG    lSelStart;      // Logical selection start
        LONG    lSelEnd;        // Logical selection end

        WORD    wThumbWidth;    // Width of the thumb
        WORD    wThumbHeight;   // Height of the thumb

        int     iSizePhys;      // Size of where thumb lives
        RECT    rc;             // track bar rect.

        RECT    Thumb;          // Rectangle we current thumb
        DWORD   dwDragPos;      // Logical position of mouse while dragging.

        WORD    Flags;          // Flags for our window
        int     Timer;          // Our timer.
        WORD    Cmd;            // The command we're repeating.

        int     nTics;          // number of ticks.
        PDWORD  pTics;          // the tick marks.

} TrackBar, *PTrackBar;

// Trackbar flags

#define TBF_NOTHUMB     0x0001  // No thumb because not wide enough.
#define TBF_SELECTION   0x0002  // a selection has been established (draw the range)

/*
	useful constants.
*/

#define REPEATTIME      500     // mouse auto repeat 1/2 of a second
#define TIMER_ID        1

#define	GWW_TRACKMEM		0 /* handle to track bar memory */
#define EXTRA_TB_BYTES          sizeof(PTrackBar) /* Total extra bytes.         */

/*
	Useful defines.
*/

#define TrackBarCreate(hwnd)    SetWindowWord(hwnd,GWW_TRACKMEM,(WORD)LocalAlloc(LPTR,sizeof(TrackBar)))
#define TrackBarDestroy(hwnd)   LocalFree((HLOCAL)GetWindowWord(hwnd,GWW_TRACKMEM))
#define TrackBarLock(hwnd)      (PTrackBar)GetWindowWord(hwnd,GWW_TRACKMEM)

/*
	Function Prototypes
*/

static void   NEAR PASCAL DoTrack(PTrackBar, int, DWORD);
static WORD   NEAR PASCAL WTrackType(PTrackBar, LONG);
static void   NEAR PASCAL TBTrackInit(PTrackBar, LONG);
static void   NEAR PASCAL TBTrackEnd(PTrackBar, LONG);
static void   NEAR PASCAL TBTrack(PTrackBar, LONG);
static void   NEAR PASCAL DrawThumb(PTrackBar);
static HBRUSH NEAR PASCAL SelectColorObjects(PTrackBar, BOOL);
static void   NEAR PASCAL SetTBCaretPos(PTrackBar);

extern DWORD FAR PASCAL lMulDiv32(DWORD, DWORD, DWORD);
