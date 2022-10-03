/*
 * PROJECT:     ReactOS Event Log Viewer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Event Details Control.
 * COPYRIGHT:   Copyright 2007 Marc Piulachs <marc.piulachs@codexchange.net>
 *              Copyright 2008-2016 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2016-2022 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef _EVTDETCTL_H_
#define _EVTDETCTL_H_

/* Optional structure passed by pointer
 * as LPARAM to CreateEventDetailsCtrl() */
typedef struct _EVENTDETAIL_INFO
{
    PEVENTLOGFILTER EventLogFilter;
    INT iEventItem;
} EVENTDETAIL_INFO, *PEVENTDETAIL_INFO;

#define EVT_SETFILTER   (WM_APP + 2)
#define EVT_DISPLAY     (WM_APP + 3)

HWND
CreateEventDetailsCtrl(HINSTANCE hInstance,
                       HWND hParentWnd,
                       LPARAM lParam);

#endif /* _EVTDETCTL_H_ */
