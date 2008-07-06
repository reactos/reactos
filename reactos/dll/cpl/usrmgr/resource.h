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

#define IDD_USER_GENERAL               310
#define IDC_USER_GENERAL_NAME          311
#define IDC_USER_GENERAL_FULL_NAME     312
#define IDC_USER_GENERAL_DESCRIPTION   313
#define IDC_USER_GENERAL_FORCE_CHANGE  314
#define IDC_USER_GENERAL_CANNOT_CHANGE 315
#define IDC_USER_GENERAL_NEVER_EXPIRES 316
#define IDC_USER_GENERAL_DISABLED      317
#define IDC_USER_GENERAL_LOCKED        318

#define IDD_GROUP_GENERAL              340
#define IDC_GROUP_GENERAL_NAME         341
#define IDC_GROUP_GENERAL_DESCRIPTION  342
#define IDC_GROUP_GENERAL_MEMBERS      343
#define IDC_GROUP_GENERAL_ADD          344
#define IDC_GROUP_GENERAL_REMOVE       345

#define IDD_CHANGE_PASSWORD         350
#define IDC_EDIT_PASSWORD1          351
#define IDC_EDIT_PASSWORD2          352


#define IDD_USER_NEW                360
#define IDC_USER_NEW_NAME           361
#define IDC_USER_NEW_FULL_NAME      362
#define IDC_USER_NEW_DESCRIPTION    363
#define IDC_USER_NEW_PASSWORD1      364
#define IDC_USER_NEW_PASSWORD2      365
#define IDC_USER_NEW_FORCE_CHANGE   366
#define IDC_USER_NEW_CANNOT_CHANGE  367
#define IDC_USER_NEW_NEVER_EXPIRES  368
#define IDC_USER_NEW_DISABLED       369

#define IDD_USER_MEMBERSHIP          370
#define IDC_USER_MEMBERSHIP_LIST     371
#define IDC_USER_MEMBERSHIP_ADD      372
#define IDC_USER_MEMBERSHIP_REMOVE   373

#define IDD_USER_PROFILE             380
#define IDC_USER_PROFILE_PATH        381
#define IDC_USER_PROFILE_SCRIPT      382
#define IDC_USER_PROFILE_LOCAL       383
#define IDC_USER_PROFILE_REMOTE      384
#define IDC_USER_PROFILE_LOCAL_PATH  385
#define IDC_USER_PROFILE_DRIVE       386
#define IDC_USER_PROFILE_REMOTE_PATH 387

#define IDD_GROUP_NEW                390
#define IDC_GROUP_NEW_NAME           391
#define IDC_GROUP_NEW_DESCRIPTION    392


#define IDD_USER_ADD_MEMBERSHIP      400
#define IDC_USER_ADD_MEMBERSHIP_LIST 401

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

