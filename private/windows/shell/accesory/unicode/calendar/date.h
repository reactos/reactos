/* Typedefs and constants for use with date.c */

typedef struct {
    BYTE dayofweek;
    BYTE day;
    BYTE month;
    WORD year;
} DOSDATE;
typedef DOSDATE *PDOSDATE;

typedef struct {
    BYTE hundredths;
    BYTE seconds;
    BYTE minutes;
    BYTE hour;
} DOSTIME;
typedef DOSTIME *PDOSTIME;


typedef struct tagTIME
{
    TCHAR  chSep;             /* Separator character for date string */
    TCHAR  sz1159[6];         /* string for AM */
    TCHAR  sz2359[6];         /* string for PM */
    int    iTime;             /* time format */
    int    iLZero;            /* lead zero for hour */
} TIME;

#ifdef JAPAN
#define CCHTIMESZ      18
#else
#define CCHTIMESZ      12
#endif
                             /* Absolute maximum number of chars in a zero
                                terminated time string, taking into account
                                international formats.  4 plus space plus
                                6 CHAR AM/PM string plus 0 at end.
                              */

#define CCHDATEDISP    64    /* The number of characters in a zero
                                terminated ASCII date string.
                                30 is large enough for US style
                                strings, so 64 ought to do it for all
                                else.
                              */

#define CBMONTHARRAY   56    /* Number of bytes in the month array. was 49 */
#define CCHDAY         20    /* The longest day name. */
#define CCHMONTH       20    /* The longest month name. */
#define CCHYEAR        4     /* Chars in a year (e.g., 1985). */
#define SAMPLETIME     600   /* arbitrary time used for calculating
                                length of a time string in daymode */

#define MAX_SHORTFMT   12    /* max size of short date format string */
#define MAX_LONGFMT    64    /* max size of long date format string */

typedef struct tagDATE
{
    int    iDate;
    int    iLZero;            /* lead zero for hour */
    TCHAR  chSep;
    TCHAR  szShortFmt[MAX_SHORTFMT];
    TCHAR  szLongFmt[CCHDATEDISP];
} DATE;


#define GDS_SHORT       1
#define GDS_LONG        2
#define GDS_DAYOFWEEK   4
#define GDS_NODAY       8


#define IDS_DATESTRINGS    32736
#define IDS_MONTHS         IDS_DATESTRINGS
#define IDS_MONTHABBREVS   IDS_MONTHS + 12
#define IDS_DAYSOFWEEK     IDS_MONTHABBREVS + 12
#define IDS_DAYABBREVS     IDS_DAYSOFWEEK + 7
#define IDS_DAYLETTER      IDS_DAYABBREVS + 7

#define PD_ERRFORMAT       -1
#define PD_ERRSUBRANGE     -2
#define PD_ERRRANGE        -3


/* extern global variables */

extern WORD    iYearOffset;
extern TIME    Time;
extern DATE    Date;
extern HANDLE  hinstTimeDate;
extern WORD    cchTimeMax;
extern WORD    cchLongDateMax;
extern TCHAR  *rgszDayAbbrevs[];


/* function prototypes */

TCHAR * FAR APIENTRY Ascii2Int();
TCHAR * FAR APIENTRY Int2Ascii();
TCHAR * APIENTRY SkipDateSep();

VOID FAR APIENTRY InitTimeDate(HANDLE);
BOOL FAR APIENTRY ValidateDosDate(PDOSDATE);
INT FAR APIENTRY GetTimeString(PDOSTIME, TCHAR *);
INT FAR APIENTRY GetDateString(PDOSDATE, TCHAR *, WORD);
INT FAR APIENTRY GetMonthYear(PDOSDATE, TCHAR *);
INT FAR APIENTRY ParseTimeString(PDOSTIME, TCHAR *);
INT FAR APIENTRY ParseDateString(PDOSDATE, TCHAR *);
