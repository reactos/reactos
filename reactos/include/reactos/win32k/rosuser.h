/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/rosuser.h
 * PURPOSE:         Win32 Shared USER Types for RosUser*
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#ifndef __WIN32K_ROSUSER_H
#define __WIN32K_ROSUSER_H

/* DEFINES *******************************************************************/

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

/* FUNCTIONS *****************************************************************/

#if 0
BOOL
NTAPI
RosUserEnumDisplayMonitors(
    HDC hdc,
    LPRECT rect,
    MONITORENUMPROC proc,
    LPARAM lp
);

BOOL
NTAPI
RosUserGetMonitorInfo(
    HMONITOR handle,
    LPMONITORINFO info
);
#endif

BOOL NTAPI 
RosUserGetCursorPos( LPPOINT pt );

BOOL NTAPI 
RosUserSetCursorPos( INT x, INT y );

BOOL NTAPI 
RosUserClipCursor( LPCRECT clip );

void NTAPI 
RosUserSetCursor( ICONINFO* IconInfo );

INT
APIENTRY
RosUserEnumDisplayMonitors(
   OPTIONAL OUT HMONITOR *hMonitorList,
   OPTIONAL OUT PRECTL monitorRectList,
   OPTIONAL IN DWORD listSize);

BOOL
APIENTRY
RosUserGetMonitorInfo(
   IN HMONITOR hMonitor,
   OUT LPMONITORINFO pMonitorInfo);

VOID NTAPI
RosUserConnectCsrss();

#endif /* __WIN32K_NTUSER_H */
