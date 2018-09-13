//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by compex.rc
//
#ifndef _WINDOWS_
   #include <windef.h>
   #include <winbase.h>
   #include <wingdi.h>
   #include <winuser.h>
   #include <commctrl.h>
#endif

   // Make sure we have the appropriate flags for Windows95 look and feel
#ifndef DS_3DLOOK
    #define DS_3DLOOK          0x04L
#endif

#ifndef DS_MODALFRAME
   #define DS_MODALFRAME   0x80L
#endif

#ifndef DS_CONTEXTHELP
   #define DS_CONTEXTHELP  0x2000L
#endif

#define IDC_STATIC                      -1

#define ICON_DATACLEN                   100

#define IDD_COMP_VIEW                   101
#define IDD_COMP_SETTINGS               102


// #define IDS_PROPBAGNAME                          999
// #define IDS_SYSTEMDATACLEANER                   1000
#define IDS_OLDFILESINROOT_W95		        1001
#define IDS_OLDFILESINROOT_DISP_W95		1002
#define IDS_OLDFILESINROOT_DESC_W95		1003
#define IDS_OLDFILESINROOT_FILE_W95		1004
// #define IDS_TEMPFILES				1005
#define IDS_TEMPFILES_DISP			1006
#define IDS_TEMPFILES_DESC			1007
// #define IDS_TEMPFILES_FILE			1008
// #define IDS_SETUPFILES				1009
#define IDS_SETUPFILES_DISP			1010
#define IDS_SETUPFILES_DESC			1011
// #define IDS_SETUPFILES_FILE			1012
// #define IDS_SETUPFILES_PROCESS			1013
// #define IDS_UNINSTALLFILES			1014
#define IDS_UNINSTALLFILES_DISP	                1015
#define IDS_UNINSTALLFILES_DESC			1016
// #define IDS_UNINSTALLFILES_FILE			1017
// #define IDS_UNINSTALLFILES_CLEN			1018
// #define IDS_OLDFILESINROOT_FOLDER_W95           1019
// #define IDS_OLDFILESINROOT_NT                   1020
#define IDS_OLDFILESINROOT_DISP_NT		1021
#define IDS_OLDFILESINROOT_DESC_NT		1022
// #define IDS_OLDFILESINROOT_FILE_NT		1023
// #define IDS_OLDFILESINROOT_FOLDER_NT            1024
#define IDC_COMP_LIST                           1000
#define IDS_COMPCLEANER                         1025
#define IDC_SPIN1                               1001
#define IDS_COMPCLEANER_DISP                    1026
#define IDC_EDIT1                               1002
#define IDS_COMPCLEANER_DESC                    1027
#define IDC_COMP_SPIN                           1003
#define IDS_COMPCLEANER_BUTTON                  1028
#define IDC_COMP_EDIT                           1004
#define IDC_COMP_DESC                           1005
#define IDC_COMP_DAYS                           1006
#define IDC_VIEW                                1007
#define IDC_SPIN_DESC                           1008
#define IDC_COMP_DIV                            1009
#define IDS_INDEXERFILES_DISP			1041
#define IDS_INDEXERFILES_DESC			1042

// #define IDS_INDEXCLEANER                        1046

#define ID_ICON_CONTENTINDEX			2000
#define ID_ICON_COMPRESS                        2001
#define ID_ICON_CHKDSK                          2002

#ifdef UNICODE
#define IDS_OLDFILESINROOT_DISP         IDS_OLDFILESINROOT_DISP_NT
#define IDS_OLDFILESINROOT_DESC         IDS_OLDFILESINROOT_DESC_NT
#else
#define IDS_OLDFILESINROOT_DISP         IDS_OLDFILESINROOT_DISP_W95
#define IDS_OLDFILESINROOT_DESC         IDS_OLDFILESINROOT_DESC_W95
#endif
// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        102
#define _APS_NEXT_COMMAND_VALUE         40000
#define _APS_NEXT_CONTROL_VALUE         1010
#define _APS_NEXT_SYMED_VALUE           100
#endif
#endif
