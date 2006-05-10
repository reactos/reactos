#ifndef _CALLOUT_
#define _CALLOUT_

#include <internal/ob.h>

typedef struct _W32_CALLOUT_DATA
{
    PKWIN32_PROCESS_CALLOUT W32ProcessCallout;
    PKWIN32_THREAD_CALLOUT W32ThreadCallout;
    OB_OPEN_METHOD DesktopOpen;
    OB_DELETE_METHOD DesktopDelete;
    OB_DELETE_METHOD WinStaDelete;
    OB_ROS_PARSE_METHOD WinStaParse;
    OB_OPEN_METHOD WinStaOpen;
    OB_ROS_FIND_METHOD WinStaFind;
    OB_ROS_CREATE_METHOD DesktopCreate;
} W32_CALLOUT_DATA, *PW32_CALLOUT_DATA;

#endif
