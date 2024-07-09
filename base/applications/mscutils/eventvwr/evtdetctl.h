/*
 * PROJECT:     ReactOS Event Log Viewer
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Event Details Control
 * COPYRIGHT:   Marc Piulachs <marc.piulachs@codexchange.net>
 *              Eric Kohl
 *              Hermes Belusca-Maito
 */

#ifndef _EVTDETCTL_H_
#define _EVTDETCTL_H_

#define EVT_SETFILTER   (WM_APP + 2)
#define EVT_DISPLAY     (WM_APP + 3)

HWND
CreateEventDetailsCtrl(HINSTANCE hInstance,
                       HWND hParentWnd,
                       LPARAM lParam);

#endif /* _EVTDETCTL_H_ */
