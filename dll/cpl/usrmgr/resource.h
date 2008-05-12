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


/* Icons */
#define IDI_USRMGR_ICON                 40
#define IDI_USRMGR_ICON2                100 // Needed for theme compatibility with Windows.
#define IDI_USER                        41
#define IDI_LOCKED_USER                 42
#define IDI_GROUP                       43


#define IDD_USERS                       100
#define IDD_GROUPS                      101
#define IDD_EXTRA                       102

#define IDC_USERS_LIST                  200

#define IDC_GROUPS_LIST                 300

#define IDC_STATIC                      -1


/* Dialogs */

#define IDD_USER_GENERAL            310
#define IDC_USER_NAME               311
#define IDC_USER_FULLNAME           312
#define IDC_USER_DESCRIPTION        313
#define IDC_USER_PW_CHANGE          314
#define IDC_USER_PW_NOCHANGE        315
#define IDC_USER_PW_EXPIRE          316
#define IDC_USER_DEACTIVATE         317
#define IDC_USER_LOCK               318


#define IDD_CHANGE_PASSWORD         350
#define IDC_EDIT_PASSWORD1          351
#define IDC_EDIT_PASSWORD2          352


/* Strings */

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

