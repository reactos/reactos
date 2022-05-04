/*
 * TIMER.C - timer internal command.
 *
 * clone from 4nt timer command
 *
 * 20 Aug 1999
 *     started - Paolo Pantaleo <paolopan@freemail.it>
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_TIMER


#define NCS_NOT_SPECIFIED -1
#define NCS_ON 1
#define NCS_OFF 0

//print timer value
#define PT(format) PrintElapsedTime(GetTickCount()-cT,format)


//current timer Time (at wich started to count)
#define cT clksT[clk_n]

//current timer status
#define cS clksS[clk_n]


static VOID
PrintElapsedTime (DWORD time,INT format)
{
    DWORD h,m,s,ms;

    TRACE("PrintElapsedTime(%lu, %d)\n", time, format);

    switch (format)
    {
    case 0:
        ConOutResPrintf(STRING_TIMER_HELP1, time);
        break;

    case 1:
        ms = time % 1000;
        time /= 1000;
        s = time % 60;
        time /=60;
        m = time % 60;
        h = time / 60;
        ConOutResPrintf(STRING_TIMER_HELP2,
                        h, cTimeSeparator,
                        m, cTimeSeparator,
                        s, cDecimalSeparator, ms/10);
        break;
    }
}


INT CommandTimer (LPTSTR param)
{
    // all timers are kept
    static DWORD clksT[10];

    // timers status
    // set all the clocks off by default
    static BOOL clksS[10]={FALSE,FALSE,FALSE,FALSE,
        FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};

    // TRUE if /S in command line
    BOOL bS = FALSE;

    // avoid to set clk_n more than once
    BOOL bCanNSet = TRUE;

    INT NewClkStatus = NCS_NOT_SPECIFIED;

    // the clock number specified on the command line
    // 1 by default
    INT clk_n=1;

    // output format
    INT iFormat=1;


    // command line parsing variables
    INT argc;
    LPTSTR *p;

    INT i;

    if (_tcsncmp (param, _T("/?"), 2) == 0)
    {
        ConOutResPrintf(STRING_TIMER_HELP3, cTimeSeparator, cTimeSeparator, cDecimalSeparator);
        return 0;
    }

    nErrorLevel = 0;

    p = split (param, &argc, FALSE, FALSE);

    //read options
    for (i = 0; i < argc; i++)
    {
        //set timer on
        if (!(_tcsicmp(&p[i][0],_T("on")))  && NewClkStatus == NCS_NOT_SPECIFIED)
        {
            NewClkStatus = NCS_ON;
            continue;
        }

        //set timer off
        if (!(_tcsicmp(&p[i][0],_T("off"))) && NewClkStatus == NCS_NOT_SPECIFIED)
        {
            NewClkStatus = NCS_OFF;
            continue;
        }

        // other options
        if (p[i][0] == _T('/'))
        {
            // set timer number
            if (_istdigit(p[i][1]) && bCanNSet)
            {
                clk_n = p[i][1] - _T('0');
                bCanNSet = FALSE;
                continue;
            }

            // set s(plit) option
            if (_totupper(p[i][1]) == _T('S'))
            {
                bS = TRUE;
                continue;
            }

            // specify format
            if (_totupper(p[i][1]) == _T('F'))
            {
                iFormat = p[i][2] - _T('0');
                continue;
            }
        }
    }

    // do stuff (start/stop/read timer)
    if (NewClkStatus == NCS_ON)
    {
        cT=GetTickCount();
        cS=TRUE;

        ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
        ConOutPuts(GetTimeString());
        freep(p);
        return 0;
    }

    if (bS)
    {
        if (cS)
        {
            ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
            ConOutPrintf(_T("%s\n"), GetTimeString());
            PrintElapsedTime(GetTickCount()-cT, iFormat);
            freep(p);
            return 0;
        }

        cT=GetTickCount();
        cS=TRUE;
        ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
        ConOutPuts(GetTimeString());
        freep(p);
        return 0;
    }

    if (NewClkStatus == NCS_NOT_SPECIFIED)
    {
        if (cS)
        {
            cS=FALSE;
            ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
            ConOutPrintf(_T("%s\n"), GetTimeString());
            PrintElapsedTime(GetTickCount()-cT, iFormat);
            freep(p);
            return 0;
        }

        cT=GetTickCount();
        cS=TRUE;
        ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
        ConOutPuts(GetTimeString());
        freep(p);
        return 0;
    }


    if (NewClkStatus == NCS_OFF)
    {
        if (cS)
        {
            cS=FALSE;
            ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
            ConOutPrintf(_T("%s\n"), GetTimeString());
            PrintElapsedTime(GetTickCount()-cT, iFormat);
            freep(p);
            return 0;
        }
        ConOutResPrintf (STRING_TIMER_TIME,clk_n,cS?_T("ON"):_T("OFF"));
        ConOutPuts(GetTimeString());
        freep(p);
        return 0;
    }

    freep(p);
    return 0;
}

#endif /* INCLUDE_CMD_TIMER */
