/* ------------ barchart.c ----------- */
#include "dflat.h"

#define BCHEIGHT 12
#define BCWIDTH 44
#define COLWIDTH 4

static DFWINDOW Bwnd;

/* ------- project schedule array ------- */
static struct ProjChart {
    char *prj;
    int start, stop;
} projs[] = {
    {"Center St", 0,3},
    {"City Hall", 0,5},
    {"Rt 395   ", 1,4},
    {"Sky Condo", 2,3},
    {"Out Hs   ", 0,4},
    {"Bk Palace", 1,5}
};

static char *Title =  "              PROJECT SCHEDULE";
static char *Months = "           Jan Feb Mar Apr May Jun";

static int BarChartProc(DFWINDOW wnd, DFMESSAGE msg,
                                    PARAM p1, PARAM p2)
{
    switch (msg)    {
        case DFM_COMMAND:
            if ((int)p1 == ID_HELP)    {
                DisplayHelp(wnd, "BarChart");
                return TRUE;
            }
            break;
        case CLOSE_WINDOW:
            Bwnd = NULL;
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

void BarChart(DFWINDOW pwnd)
{
    int pct = sizeof projs / sizeof(struct ProjChart);
    int i;

    if (Bwnd == NULL)    {
        Bwnd = DfCreateWindow(PICTUREBOX,
                    "BarChart",
                    -1, -1, BCHEIGHT, BCWIDTH,
                    NULL, pwnd, BarChartProc,
                    SHADOW     |
                    CONTROLBOX |
                    MOVEABLE   |
                    HASBORDER
        );
        DfSendMessage(Bwnd, ADDTEXT, (PARAM) Title, 0);
        DfSendMessage(Bwnd, ADDTEXT, (PARAM) "", 0);
        for (i = 0; i < pct; i++)    {
            DfSendMessage(Bwnd,ADDTEXT,(PARAM)projs[i].prj,0);
            DrawBar(Bwnd, SOLIDBAR+(i%4),
                11+projs[i].start*COLWIDTH, 2+i,
                (1 + projs[i].stop-projs[i].start) * COLWIDTH,
                TRUE);
        }
        DfSendMessage(Bwnd, ADDTEXT, (PARAM) "", 0);
        DfSendMessage(Bwnd, ADDTEXT, (PARAM) Months, 0);
        DrawBox(Bwnd, 10, 1, pct+2, 25);
    }
    DfSendMessage(Bwnd, SETFOCUS, TRUE, 0);
}
