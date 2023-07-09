#ifndef _STARTIDS_H
#define _STARTIDS_H

#define IDM_FILERUN                 401
#define IDM_LOGOFF                  402
#define IDM_EJECTPC                 410
#define IDM_SETTINGSASSIST          411
#define IDM_TRAYPROPERTIES          413
#define IDM_UPDATEWIZARD            414
#define IDM_UPDATE_SEP              415

#ifdef WINNT // hydra specific ids
#define IDM_MU_DISCONNECT           5000
#define IDM_MU_SECURITY             5001
#endif

#define IDM_RECENT              501
#define IDM_FIND                502
#define IDM_HELPSEARCH          503
#define IDM_PROGRAMS            504
#define IDM_CONTROLS            505
#define IDM_EXITWIN             506
#define IDM_SETTINGS            508
#define IDM_PRINTERS            510
#define IDM_STARTMENU           511
#define IDM_MYCOMPUTER          512
#define IDM_PROGRAMSINIT        513
#define IDM_RECENTINIT          514
#define IDM_MYDOCUMENTS         516
#define IDM_MENU_FIND           520
#define TRAY_IDM_FINDFIRST      521  // this range
#define TRAY_IDM_FINDLAST       550  // is reserved for find command
#define IDM_NETCONNECT          557


// Orphans from IE401....
#ifdef FEATURE_BROWSEWEB
#define IDM_MENU_WEB            551
#endif

#define IDM_DESKTOPHTML_CUSTOMIZE   552
#define IDM_DESKTOPHTML_UPDATE      553
#define IDM_DESKTOPHTML_ONOFF       554
#define IDM_FOLDERPROPERTIES        555
#define IDM_ACTIVEDESKTOP_PROP      556
#define IDM_FAVORITES               507
#define IDM_SUSPEND                 409

#define IDM_CSC                 553

#endif

