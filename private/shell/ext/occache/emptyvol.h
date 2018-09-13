#ifndef __EMPTY_VOLUME_CACHE__
#define __EMPTY_VOLUME_CACHE__

#include "utils.h"

// Define node storing a path read from the registry in a singly-linked list.
typedef struct _TAG_CACHE_PATH_NODE CACHE_PATH_NODE;
typedef CACHE_PATH_NODE* LPCACHE_PATH_NODE;
struct _TAG_CACHE_PATH_NODE
{
    TCHAR szCachePath[MAX_PATH];

    LPCACHE_PATH_NODE pNext;
};

// Define node storing control handles in a singly-linked list.
typedef struct _TAG_CONTROL_HANDLE_NODE CONTROL_HANDLE_NODE;
typedef CONTROL_HANDLE_NODE* LPCONTROL_HANDLE_NODE;
struct _TAG_CONTROL_HANDLE_NODE
{
    HANDLE hControl;

    LPCONTROL_HANDLE_NODE pNext;
};

// Define node storing heads and tails of control handle lists in a 
// singly-linked list.
// There is a control handle list for each volume.
typedef struct _TAG_CONTROL_HANDLE_HEADER CONTROL_HANDLE_HEADER;
typedef CONTROL_HANDLE_HEADER* LPCONTROL_HANDLE_HEADER;
struct _TAG_CONTROL_HANDLE_HEADER
{
    DWORD dwSpaceUsed;
    int   nDriveNum;

    LPCONTROL_HANDLE_NODE pHandlesHead;
    LPCONTROL_HANDLE_NODE pHandlesTail;

    LPCONTROL_HANDLE_HEADER pNext;
};

// Handles to activeX controls are cached in memory during calls to
// GetSpaceUsed so that re-enumeration is not needed for subsequent
// calls to Purge.
//
// The structure for storing control handles for various volumes is:
//
//                                m_pControlsTail -+
//                                                 |
//                                                \|/
//   m_pControlsHead --> Header01 --> Header02 --> Header03 --> NULL
//                       |            |            |
//                       +- C:        +- D:        +- E:
//                       |            |            |
//                       +- Head01    +- Head02    +- Head03
//                       |            |            |
//                       +- Tail01    +- Tail02    +- Tail03
//
// where 
//      HeaderXX is of type CONTROL_HANDLE_HEADER, and
//      HeadXX and TailXX are of type LPCONTROL_HANDLE_NODE.
//
// HeadXX and TailXX are respectively head and tail pointers to a list
// of handles.  Those are handles to controls installed on the drive
// specified in HeaderXX.
//


#endif // __EMPTY_VOLUME_CACHE__
