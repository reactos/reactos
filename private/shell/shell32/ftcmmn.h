#ifndef FTCMMN_H
#define FTCMMN_H

#define WM_CTRL_SETFOCUS WM_USER + 1

#define MAX_EXT                 MAX_PATH
#define MAX_PROGID              MAX_PATH
#define MAX_PROGIDDESCR         200 //no reason
#define MAX_APPFRIENDLYNAME     260
#define MAX_ACTION	    		63
#define MAX_ACTIONDESCR			200 //no reason
#define MAX_ACTIONCMDLINE       (MAX_PATH * 2) //no reason
#define MAX_ACTIONAPPL			MAX_PATH
#define MAX_ACTIONDDEAPPNOTRUN	MAX_PATH
#define MAX_ACTIONDDEMSG		200 //no reason
#define MAX_ACTIONTOPIC			200 //no reason
#define MAX_MIME				50 //no reason
#define MAX_ICONLOCATION		MAX_PATH + 10 // + 10 for comma and index
#define MAX_FTMAX				MAX_EXT

#define BROWSEHACK_DONTINPLACENAVIGATE     0x00000008

typedef struct tagPROGIDACTION
{
    BOOL  fNew;
    BOOL  fUseDDE;
    TCHAR szAction[MAX_ACTION];
    TCHAR szOldAction[MAX_ACTION];
    TCHAR szCmdLine[MAX_ACTIONCMDLINE];
    TCHAR szDDEMsg[MAX_ACTIONDDEMSG];
    TCHAR szDDEApplication[MAX_ACTIONAPPL];
    TCHAR szDDEAppNotRunning[MAX_ACTIONDDEAPPNOTRUN];
    TCHAR szDDETopic[MAX_ACTIONTOPIC];
}PROGIDACTION;

#endif //FTCMMN_H