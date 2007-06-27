#ifndef __CPL_USRMGR_RESOURCE_H__
#define __CPL_USRMGR_RESOURCE_H__

#include <commctrl.h>

/* metrics */
#define PROPSHEETWIDTH      246
#define PROPSHEETHEIGHT     228
#define PROPSHEETPADDING    6

#define SYSTEM_COLUMN       (18 * PROPSHEETPADDING)
#define LABELLINE(x)        (((PROPSHEETPADDING + 2) * x) + (x + 2))

#define ICONSIZE            16

/* ids */

#define IDC_USRMGR_ICON                   40
#define IDC_USRMGR_ICON2                  100 // Needed for theme compatability with Windows.

#define IDD_USERS                       100
#define IDD_GROUPS                      101
#define IDD_EXTRA                       102

#define IDC_USERS_LIST                  200

#define IDC_GROUPS_LIST                 300

#define IDC_STATIC                      -1


#define IDS_CPLNAME                 2000
#define IDS_CPLDESCRIPTION          2001

#define IDS_NAME                    2100
#define IDS_FULLNAME                2101
#define IDS_DESCRIPTION             2102

/* Menus */
#define IDM_POPUP_GROUP             120
#define IDM_GROUP_ADD_MEMBER        121
#define IDM_GROUP_NEW               122
#define IDM_GROUP_DELETE            123
#define IDM_GROUP_RENAME            124
#define IDM_GROUP_PROPERTIES        125

#define IDM_POPUP_USER              130
#define IDM_USER_CHANGE_PASSWORD    131
#define IDM_USER_NEW                132
#define IDM_USER_DELETE             133
#define IDM_USER_RENAME             134
#define IDM_USER_PROPERTIES         135

#endif /* __CPL_USRMGR_RESOURCE_H__ */

