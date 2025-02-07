#ifndef _RAPPS_H
#define _RAPPS_H

#if DBG && !defined(_DEBUG)
#define _DEBUG // CORE-17505
#endif

#include "defines.h"

#include "dialogs.h"
#include "appinfo.h"
#include "appdb.h"
#include "misc.h"
#include "configparser.h"

extern LONG g_Busy;
extern bool g_PendingInstalledViewRefresh;

#define WM_NOTIFY_OPERATIONCOMPLETED (WM_APP + 0)
#define WM_NOTIFY_INSTALLERFINISHED  (WM_APP + 1)

#define MAINWINDOWCLASSNAME L"ROSAPPMGR2"
#define MAINWINDOWMUTEX szWindowClass
#define UPDATEDBMUTEX ( MAINWINDOWCLASSNAME L":UpDB" )

struct CUpdateDatabaseMutex : public CScopedMutex
{
    CUpdateDatabaseMutex() : CScopedMutex(UPDATEDBMUTEX, 1000 * 60 * 10, FALSE) { };
};

#endif /* _RAPPS_H */
