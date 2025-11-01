/* ------------- calendar.c ------------- */
#include "dflat.h"

#ifndef NOCALENDAR

#define CALHEIGHT 17
#define CALWIDTH  33

static int DyMo[]={31,28,31,30,31,30,31,31,30,31,30,31};
static struct tm ttm, ctm;
static int dys[42];
static WINDOW Cwnd;

#ifndef strftime
static char *nameOfMonth[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
#endif

/* Returns 1 if year (0-based, not 1900-based) is a leap year (longer) */
int isLeapYear(int year)
{
    if (!(year % 400))
        return 1;                       /* 2000 is a leap year */

    if (!(year % 100))
        return 0;                       /* 1900 and 2100 are not */

    if (!(year % 4))
        return 1;                       /* multiple of 4? Then it is a leap year */

    return 0;                           /* default: not a leap year */

}

static void FixDate(void)
{
    int i,j;

    /* Adjust Feb for leap year */
    DyMo[1]=isLeapYear(1900+ttm.tm_year) ? 29 : 28;

    /* Enforce ranges: 1..?? for mday, 0..11 for mon */
    while (ttm.tm_mday > DyMo[ttm.tm_mon])
        {
        ttm.tm_mday -= DyMo[ttm.tm_mon];
        ttm.tm_mon++;
        if (ttm.tm_mon > 11)
            {
            ttm.tm_mon=0;
            ttm.tm_year++;
            }

        }

    /* re-calculate yday in 0..??? range */
    ttm.tm_yday=0;
    i=0;
    while (i < ttm.tm_mon)
        {
        ttm.tm_yday += DyMo[i];
        i++;
        }

    ttm.tm_yday+=ttm.tm_mday-1;

    /* 1st of January of 1900 (tm_year base) was a Monday (wday=1) */
    i=0;
    j=1;
    if (ttm.tm_year >= 1980)
        {
        i=80;
        j=2;
        }

    while (i < ttm.tm_year)
        {
        j += isLeapYear(i+1900) ? 2 : 1; /* shift 1 or 2 days each year */
        if (j>6)
            j -= 7;

        i++;
        }

    ttm.tm_wday=(j + ttm.tm_yday) % 7;  /* "day of year" helps us! */

}

/* ---- build calendar dates array ---- */
static void BuildDateArray(void)
{
    int offset, dy=0;

    memset(dys, 0, sizeof dys);
    FixDate();

    /* Compute the weekday for the 1st */
    offset=((ttm.tm_mday-1) - ttm.tm_wday) % 7;
    if (offset < 0)
        offset += 7;

    if (offset)
        offset=(offset - 7) * -1;

    /* Build the dates into the array */
    for (dy=1; dy <= DyMo[ttm.tm_mon]; dy++)
        dys[offset++]=dy;

}

static void CreateWindowMsg(WINDOW wnd)
{
    int x, y;

    DrawBox(wnd, 1, 2, CALHEIGHT-4, CALWIDTH-4);
    for (x=5;x<CALWIDTH-4;x += 4)
        DrawVector(wnd, x, 2, CALHEIGHT-4, FALSE);

    for (y=4;y<CALHEIGHT-3;y+=2)
        DrawVector(wnd, 1, y, CALWIDTH-4, TRUE);
}

static void DisplayDates(WINDOW wnd)
{
    int week, day,offset;
    char dyln[10],banner[CALWIDTH-1],banner1[30];

    SetStandardColor(wnd);
    PutWindowLine(wnd, "Sun Mon Tue Wed Thu Fri Sat", 2, 1);
    memset(banner, ' ', CALWIDTH-2);

#ifndef strftime
    sprintf(banner1, "%s, %i", nameOfMonth[ttm.tm_mon], 1900+ttm.tm_year);
#else
    strftime(banner1, 16, "%B, %Y", &ttm);
#endif

    offset=(CALWIDTH-2 - strlen(banner1)) / 2;
    strcpy(banner+offset, banner1);
    strcat(banner, "    ");
    PutWindowLine(wnd, banner, 0, 0);
    BuildDateArray();
    for (week=0;week<6;week++)
        {
        for (day=0;day<7;day++)
            {
            int dy=dys[week*7+day];

            if (dy == 0)
                strcpy(dyln, "   ");
            else
                {
                /* If current day of current month then highlight */
                if ((dy == ctm.tm_mday) && (ctm.tm_mon == ttm.tm_mon))
                    sprintf(dyln, "%c%c%c%2d %c", CHANGECOLOR,
                        SelectForeground(wnd)+0x80,
                        SelectBackground(wnd)+0x80,
                        dy, RESETCOLOR);
                else
                    sprintf(dyln, "%2d ", dy);

                }
            SetStandardColor(wnd);
            PutWindowLine(wnd, dyln, 2 + day * 4, 3 + week*2);
            }

        }

}

static int KeyboardMsg(WINDOW wnd, PARAM p1)
{
    switch ((int)p1)
        {
        case PGUP:
            if (ttm.tm_mon == 0)
                {
                ttm.tm_mon=12;
                ttm.tm_year--;
                }

            ttm.tm_mon--;
            FixDate();
            DisplayDates(wnd);
            return TRUE;
        case PGDN:
            ttm.tm_mon++;
            if (ttm.tm_mon == 12)
                {
                ttm.tm_mon=0;
                ttm.tm_year++;
                }
            FixDate();
            DisplayDates(wnd);
            return TRUE;
        default:
            break;

        }

    return FALSE;
}

static int CalendarProc(WINDOW wnd,MESSAGE msg, PARAM p1,PARAM p2)
{
    switch (msg)
        {
        case CREATE_WINDOW:
            DefaultWndProc(wnd, msg, p1, p2);
            CreateWindowMsg(wnd);
            return TRUE;
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1))
                return TRUE;

            break;
        case PAINT:
            DefaultWndProc(wnd, msg, p1, p2);
            DisplayDates(wnd);
            return TRUE;
        case COMMAND:
            if ((int)p1 == ID_HELP)
                {
                DisplayHelp(wnd, "Calendar");
                return TRUE;
                }
            break;
        case CLOSE_WINDOW:
            Cwnd=NULL;
            break;
        default:
            break;

        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

void Calendar(WINDOW pwnd)
{
    if (Cwnd == NULL)
        {
        time_t tim=time(NULL);

        ttm=*localtime(&tim);
        ctm=ttm;                        /* Store current calendar day and month */
        Cwnd=CreateWindow(PICTUREBOX,"Calendar",-1,-1,CALHEIGHT,CALWIDTH,NULL,pwnd,CalendarProc,
                    SHADOW | MINMAXBOX | CONTROLBOX | MOVEABLE | HASBORDER);
        }

    SendMessage(Cwnd, SETFOCUS, TRUE, 0);

}

#endif
