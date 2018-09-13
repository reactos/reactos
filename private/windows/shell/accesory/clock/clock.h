/****************************************************************************/
/*                                                                          */
/*  CLOCK.H -                                                               */
/*                                                                          */
/*      Windows Clock Include File                                          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*       Touched by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*       Revision remarks by Diane K. Oh ext #15201                         */
/*       This file has been changed to comply with the Unicode standard     */
/*       Following is a quick overview of what I have done.                 */
/*                                                                          */
/*       Was               Changed it into    Remark                        */
/*       ===               ===============    ======                        */
/*       CHAR              TCHAR              if it refers to text          */
/*       LPCHAR & LPSTR    LPTSTR             if it refers to text          */
/*       PSTR & NPSTR      LPTSTR             if it refers to text          */
/*                                                                          */
/****************************************************************************/

/*--------------------------------------------------------------------------*/
/*  Typedefs and Structures                                                 */
/*--------------------------------------------------------------------------*/

typedef struct tagTIME
{
    int     hour;   /* 0 - 11 hours for analog clock */
    int     hour12; /* 12 hour format */
    int     hour24; /* 24 hour format */
    int     minute;
    int     second;
    int     ampm;   /* 0 - AM , 1 - PM */
} TIME;

typedef struct tagDATE
{
    int     day;
    int     month;
    int     year;
} xDATE;

typedef struct tagCLOCKDISPSTRUCT
{
    /* Clock display format for main window/icon outut */
    /* either  IDM_ANALOG, or IDM_DIGITAL */
    WORD    wFormat;

    /* flags */
    BOOL    bIconic, bNoSeconds, bNoTitle, bTopMost, bNoDate;

    /* X and Y offset within client area of window
     * or icon where digital clock will be displayed */
    int nPosY, nPosHr, nPosSep1, nPosMin, nPosSep2, nPosSec, nPosAMPM;
    int nSizeChar, nSizeSep, nSizeY, nSizeAMPM;
    int nPosDateX, nPosDateY, nSizeDateX, nSizeDateY;

    /* size of shadow offset, in pixels. If 0, no shadow */
    WORD    wShdwOff;

    /* handle to offscreen bitmap for fast painting of shadowed digits */
    HBITMAP hBitmap;

    /* buffer to hold the win.ini international indicators
     * for 1159, and 2359 AM/PM 12 hour time format.
     * szAMPM[0] holds AM, szAMPM[1] holds PM indicator */
#define MAX_AMPM_LEN    10
    TCHAR   szAMPM[2][MAX_AMPM_LEN];
    int     nMaxAMPMLen;
    WORD    wAMPMPosition;

    /* intl time format (like DOS) 0 - 12 hour, 1 - 24 hour */
    WORD    wTimeFormat, wTimeLZero;

#define MAX_DATE_LEN    80
    TCHAR   szDateFmt[MAX_DATE_LEN];
    TCHAR   szDate[MAX_DATE_LEN];
    int     nDateLen;

#define MAX_TIME_LEN    80
    TCHAR   szTimeFmt[MAX_TIME_LEN];
    int     nTimeLen;

    /* intl time seperator character */
#define MAX_TIME_SEP    5
    TCHAR   szTimeSep[MAX_TIME_SEP];
} CLOCKDISPSTRUCT, *PCLOCKDISPSTRUCT;


/*--------------------------------------------------------------------------*/
/*  Function Templates                                                      */
/*--------------------------------------------------------------------------*/

void NEAR GetTime  (TIME *);
void NEAR ConvTime (TIME *);
void NEAR GetDate  (xDATE *);

void NEAR PASCAL PrepareSavedWindow (LPTSTR, PRECT);
void NEAR PASCAL ParseSavedWindow   (LPTSTR, PRECT);
void NEAR PASCAL PrepareSavedFlags  (LPTSTR, PCLOCKDISPSTRUCT);
void NEAR PASCAL ParseSavedFlags    (LPTSTR, PCLOCKDISPSTRUCT);

LONG FAR PASCAL ClockWndProc (HWND, WORD, WORD, LONG);


/*--------------------------------------------------------------------------*/
/*  Constants                                                               */
/*--------------------------------------------------------------------------*/

    /* Main Menu ID defines */

#define IDM_ANALOG       1
#define IDM_DIGITAL      2
#define IDM_SETFONT      3
#define IDM_ABOUT        4
#define IDM_TOPMOST      5  /* actually in system menu */
#define IDM_NOTITLE      6
#define IDM_SECONDS      7
#define IDM_DATE         8
#define IDM_UTC          9

/* Temp ID for dialogs. */
#define ID_JUNK     0xCACC
#define ID_DATA     99

    /* String Resource definitions */

#define IDS_APPNAME      2
#define IDS_TOOMANY      3
#define IDS_FONTFILE     4
#define IDS_TOPMOST      5

#define IDS_FONTCHOICE  22
#define IDS_USNAME      23
#define IDS_INIFILE     24

#define IDD_FONT        100

#define HOURSCALE       65
#define MINUTESCALE     80
#define HHAND           TRUE
#define MHAND           FALSE
#define SECONDSCALE     80
#define MAXBLOBWIDTH    25
#define BUFLEN          30
#define REPAINT         0
#define HANDPAINT       1

#define UPDATE                  0
#define REDRAW                  1

#define OPEN_TLEN               450      /* < half second */
#define ICON_TLEN               20000    /* 20 seconds    */

#ifndef HWND_TOPMOST
#define HWND_TOPMOST ((HWND)-1)
#endif


#ifndef HWND_NOTOPMOST
#define HWND_NOTOPMOST ((HWND)-2)
#endif

