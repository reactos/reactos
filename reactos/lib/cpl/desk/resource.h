#ifndef __CPL_RESOURCE_H
#define __CPL_RESOURCE_H

/* metrics */
#define PROPSHEETWIDTH  246
#define PROPSHEETHEIGHT 228
#define PROPSHEETPADDING        6
#define SYSTEM_COLUMN   (0 * PROPSHEETPADDING)
// this is not supported by the MS Resource compiler:
//#define LABELLINE(x)    0 //(((PROPSHEETPADDING + 2) * x) + (x + 2))

#define ICONSIZE        16

/* ids */

#define IDI_CPLSYSTEM	100

#define IDD_PROPPAGEBACKGROUND	100
#define IDD_PROPPAGESCREENSAVER	101
#define IDD_PROPPAGEAPPEARANCE	102
#define IDD_PROPPAGESETTINGS	103

#define IDS_CPLSYSTEMNAME	1001
#define IDS_CPLSYSTEMDESCRIPTION	2001

/* controls */
#define IDC_INSTALL 101
#define IDC_SOFTWARELIST       102
#define IDC_ADDREMOVE   103

#endif /* __CPL_RESOURCE_H */

/* EOF */
