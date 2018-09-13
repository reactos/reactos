#ifndef __UIHELP_H
#define __UIHELP_H

const TCHAR STR_DSKQUOUI_HELPFILE_HTML[]  = TEXT("DSKQUOUI.CHM > windefault");
const TCHAR STR_DSKQUOUI_HTMLHELP_TOPIC[] = TEXT("nt_diskquota_overview.htm");
const TCHAR STR_DSKQUOUI_HELPFILE[]       = TEXT("DSKQUOUI.HLP");

#define IDH_QUOTA_FIRST  (0x00000000)

//
//-----------------------------------------------------------------------------
// Volume property page IDD_PROPPAGE_VOLQUOTA
//-----------------------------------------------------------------------------
//
// The colors of the traffic light indicate the state of the volume's
// quota system.  
//
//  RED     = Quotas are not enabled on the volume.
//  YELLOW  = Quota information is being rebuilt on the volume.  Quotas
//            are not active.
//  GREEN   = Quotas are enabled on the volume.
//  
#define IDH_TRAFFIC_LIGHT           (IDH_QUOTA_FIRST +  0)
//
// Describes the status of the volume's quota system.
//
#define IDH_TXT_QUOTA_STATUS        (IDH_QUOTA_FIRST +  1)
//
// Check this to enable quotas on the volume.  Uncheck it to disable quotas.
//
#define IDH_CBX_ENABLE_QUOTA        (IDH_QUOTA_FIRST +  2)
//
// Check this to deny users disk space when they exceed their quota
// limit on the volume.
//
#define IDH_CBX_DENY_LIMIT          (IDH_QUOTA_FIRST +  4)
//
// Check this to automatically assign unlimited quota to new volume users.
//
#define IDH_RBN_DEF_NO_LIMIT        (IDH_QUOTA_FIRST +  5)
//
// Check this to automatically assign a quota limit to new volume users.
//
#define IDH_RBN_DEF_LIMIT           (IDH_QUOTA_FIRST +  6)
//
// Enter a quota limit to be automatically assigned to new volume users.
// For example: to assign 20 megabytes, enter 20 and select "MB" in the
// drop-down list.
//
#define IDH_EDIT_DEF_LIMIT          (IDH_QUOTA_FIRST +  7)
//
// Enter a quota warning threshold to be automatically assigned to new volume 
// users.  For example: to assign 18 megabytes, enter 18 and select "MB" in the
// drop-down list.
//
#define IDH_EDIT_DEF_THRESHOLD      (IDH_QUOTA_FIRST +  8)
//
// Select a unit of storage to apply to the quota limit value.  
// For example:  to assign 20 megabytes, enter 20 in the edit box and select
// "MB" in the drop-down list.
//
#define IDH_CMB_DEF_LIMIT           (IDH_QUOTA_FIRST +  9)
//
// Select a unit of storage to apply to the quota warning threshold value.  
// For example:  to assign 18 megabytes, enter 18 in the edit box and select
// "MB" in the drop-down list.
//
#define IDH_CMB_DEF_THRESHOLD       (IDH_QUOTA_FIRST + 10)
//
// Displays per-user quota information for the volume.
//
#define IDH_BTN_DETAILS             (IDH_QUOTA_FIRST + 11)
//
// Opens the Window NT Event Viewer.
//
#define IDH_BTN_EVENTLOG            (IDH_QUOTA_FIRST + 12)
//
// These items control how Windows NT responds when users exceed their
// warning threshold or quota limit values.
//
#define IDH_GRP_ACTIONS             (IDH_QUOTA_FIRST + 13)
//
// These items define default quota values automatically applied to new users 
// of the volume.
//
#define IDH_GRP_DEFAULTS            (IDH_QUOTA_FIRST + 14)

//-----------------------------------------------------------------------------
// User property page IDD_PROPPAGE_USERQUOTA
//-----------------------------------------------------------------------------
//
// Show the account name for the volume user.
//
#define IDH_TXT_USERNAME            (IDH_QUOTA_FIRST + 15)
//
// The number of bytes occupied by the user's data on the volume.
//
#define IDH_TXT_SPACEUSED           (IDH_QUOTA_FIRST + 16)
//
// The number of bytes available to the user on the volume.
//
#define IDH_TXT_SPACEREMAINING      (IDH_QUOTA_FIRST + 17)
//
// Indicates if the user's disk usage is under the warning threshold, over 
// the warning threshold or over the quota limit.
//
#define IDH_ICON_USERSTATUS         (IDH_QUOTA_FIRST + 18)
//
// These items define quota warning threshold and limit values for the user.
//
#define IDH_GRP_SETTINGS            (IDH_QUOTA_FIRST + 19)
//
// Check this to assign unlimited quota to the user.
//
#define IDH_RBN_USER_NOLIMIT        (IDH_QUOTA_FIRST + 20)
//
// Check this to assign a quota warning threshold and limit value to the user.
//
#define IDH_RBN_USER_LIMIT          (IDH_QUOTA_FIRST + 21)
//
// Enter a quota warning threshold to be assigned to the user. For example: 
// to assign 18 megabytes, enter 18 and select "MB" in the drop-down list.
//
// BUGBUG:  This could be a duplicate of IDH_EDIT_DEF_THRESHOLD
//
#define IDH_EDIT_USER_THRESHOLD     (IDH_QUOTA_FIRST + 22)
//
// Enter a quota limit value to be assigned to the user. For example: 
// to assign 20 megabytes, enter 20 and select "MB" in the drop-down list.
//
// BUGBUG:  This could be a duplicate of IDH_EDIT_DEF_LIMIT
//
#define IDH_EDIT_USER_LIMIT         (IDH_QUOTA_FIRST + 23)
//
// Select a unit of storage to apply to the quota limit value.  
// For example:  to assign 20 megabytes, enter 20 in the edit box and select
// "MB" in the drop-down list.
//
// BUGBUG:  This could be a duplicate of IDH_CMB_DEF_THRESHOLD
//
#define IDH_CMB_USER_LIMIT          (IDH_QUOTA_FIRST + 24)
//
// Select a unit of storage to apply to the quota warning threshold value.  
// For example:  to assign 18 megabytes, enter 18 in the edit box and select
// "MB" in the drop-down list.
//
// BUGBUG:  This could be a duplicate of IDH_CMB_DEF_THRESHOLD
//
#define IDH_CMB_USER_THRESHOLD      (IDH_QUOTA_FIRST + 25)
//
// Show the domain/folder name for the volume user.
//
#define IDH_EDIT_DOMAINNAME         (IDH_QUOTA_FIRST + 26)

//-----------------------------------------------------------------------------
// File Owner dialog IDD_OWNERSANDFILES
//-----------------------------------------------------------------------------
//
// Select the owner of files to display in the list.
//
#define IDH_CMB_OWNERDLG_OWNERS     (IDH_QUOTA_FIRST + 27)
//
// List of files owned by selected user.
//
#define IDH_LV_OWNERDLG             (IDH_QUOTA_FIRST + 28)
//
// Permanently deletes from disk those files selected in the list.
//
#define IDH_BTN_OWNERDLG_DELETE     (IDH_QUOTA_FIRST + 29)
//
// Moves files selected in the list to another location.
//
#define IDH_BTN_OWNERDLG_MOVETO     (IDH_QUOTA_FIRST + 30)
//
// Transfers ownership of selected files to the logged-on user.
//
#define IDH_BTN_OWNERDLG_TAKE       (IDH_QUOTA_FIRST + 31)
//
// Displays a dialog for choosing a destination folder.
//
#define IDH_BTN_OWNERDLG_BROWSE     (IDH_QUOTA_FIRST + 32)
//
// Enter path to a destination folder.
//
#define IDH_EDIT_OWNERDLG_MOVETO    (IDH_QUOTA_FIRST + 33)

//-----------------------------------------------------------------------------
// Volume quota policy prop page IDD_PROPPAGE_POLICY
// Most of the controls in this dialog are duplicates of IDD_PROPPAGE_VOLQUOTA.
// Help for the additional controls is listed here.
//-----------------------------------------------------------------------------
//
// Check this to apply quota policy only to drives with non-removable media.
//
#define IDH_RBN_POLICY_FIXED        (IDH_QUOTA_FIRST + 34)
//
// Check this to apply quota policy to all NTFS storage volumes, including those
// with removable media.
//
#define IDH_RBN_POLICY_REMOVABLE    (IDH_QUOTA_FIRST + 35)
//
//-----------------------------------------------------------------------------
// These IDs are for controls on the volume prop page.  They were
// added late in the project and I didn't reserve any ranges for
// each page like I should have [brianau - 11/27/98]
//-----------------------------------------------------------------------------
//
// Check this to generate an event log entry when a user's disk space usage
// exceeds their assigned disk quota warning level.
//
#define IDH_CBX_LOG_OVERWARNING     (IDH_QUOTA_FIRST + 36)
//
// Check this to generate an event log entry when a user's disk space usage
// exceeds their assigned disk quota limit.
//
#define IDH_CBX_LOG_OVERLIMIT       (IDH_QUOTA_FIRST + 37)

#endif