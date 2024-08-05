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

#define WM_NOTIFY_OPERATIONCOMPLETED (WM_APP + 0)

#endif /* _RAPPS_H */
