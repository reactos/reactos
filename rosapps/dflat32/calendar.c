/* ------------- calendar.c ------------- */
#include "dflat.h"

#ifndef TURBOC

#define CALHEIGHT 17
#define CALWIDTH 33

static int DyMo[] = {31,28,31,30,31,30,31,31,30,31,30,31};
static struct tm ttm;
static int dys[42];
static DFWINDOW Cwnd;

static void FixDate(void)
{
    /* ---- adjust Feb for leap year ---- */
    DyMo[1] = (ttm.tm_year % 4) ? 28 : 29;
    ttm.tm_mday = min(ttm.tm_mday, DyMo[ttm.tm_mon]);
}

/* ---- build calendar dates array ---- */
static void BuildDateArray(void)
{
    int offset, dy = 0;
    memset(dys, 0, sizeof dys);
    FixDate();
    /* ----- compute the weekday for the 1st ----- */
    offset = ((ttm.tm_mday-1) - ttm.tm_wday) % 7;
    if (offset < 0)
        offset += 7;
    if (offset)
        offset = (offset - 7) * -1;
    /* ----- build the dates into the array ---- */
    for (dy = 1; dy <= DyMo[ttm.tm_mon]; dy++)
        dys[offset++] = dy;
}

static void CreateWindowMsg(DFWINDOW wnd)
{
    int x, y;
    DrawBox(wnd, 1, 2, CALHEIGHT-4, CALWIDTH-4);
    for (x = 5; x < CALWIDTH-4; x += 4)
        DrawVector(wnd, x, 2, CALHEIGHT-4, FALSE);
    for (y = 4; y < CALHEIGHT-3; y+=2)
        DrawVector(wnd, 1, y, CALWIDTH-4, TRUE);
}

static void DisplayDates(DFWINDOW wnd)
{
    int week, day;
    char dyln[10];
    int offset;
    char banner[CALWIDTH-1];
    char banner1[30];

    SetStandardColor(wnd);
    PutWindowLine(wnd, "Sun Mon Tue Wed Thu Fri Sat", 2, 1);
    memset(banner, ' ', CALWIDTH-2);
    strftime(banner1, 16, "%B, %Y", &ttm);
    offset = (CALWIDTH-2 - strlen(banner1)) / 2;
    strcpy(banner+offset, banner1);
    strcat(banner, "    ");
    PutWindowLine(wnd, banner, 0, 0);
    BuildDateArray();
    for (week = 0; week < 6; week++)    {
        for (day = 0; day < 7; day++)    {
            int dy = dys[week*7+day];
            if (dy == 0)
                strcpy(dyln, "   ");
            else    {
                if (dy == ttm.tm_mday)
                    sprintf(dyln, "%c%c%c%2d %c",
                        CHANGECOLOR,
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

static int KeyboardMsg(DFWINDOW wnd, PARAM p1)
{
    switch ((int)p1)    {
        case PGUP:
            if (ttm.tm_mon == 0)    {
                ttm.tm_mon = 12;
                ttm.tm_year--;
            }
            ttm.tm_mon--;
            FixDate();
            mktime(&ttm);
            DisplayDates(wnd);
            return TRUE;
        case PGDN:
            ttm.tm_mon++;
            if (ttm.tm_mon == 12)    {
                ttm.tm_mon = 0;
                ttm.tm_year++;
            }
            FixDate();
            mktime(&ttm);
            DisplayDates(wnd);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

static int CalendarProc(DFWINDOW wnd,DFMESSAGE msg,
                                PARAM p1,PARAM p2)
{
    switch (msg)    {
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
        case DFM_COMMAND:
            if ((int)p1 == ID_HELP)    {
                DisplayHelp(wnd, "Calendar");
                return TRUE;
            }
            break;
        case CLOSE_WINDOW:
            Cwnd = NULL;
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

void Calendar(DFWINDOW pwnd)
{
    if (Cwnd == NULL)    {
        time_t tim = time(NULL);
        ttm = *localtime(&tim);
        Cwnd = DfCreateWindow(PICTUREBOX,
                    "Calendar",
                    -1, -1, CALHEIGHT, CALWIDTH,
                    NULL, pwnd, CalendarProc,
                    SHADOW     |
                    MINMAXBOX  |
                    CONTROLBOX |
                    MOVEABLE   |
                    HASBORDER
        );
    }
    DfSendMessage(Cwnd, SETFOCUS, TRUE, 0);
}

#endif
