/*--

Copyright (c) 1990  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    defines for resource file

Author:

    Mark Enstrom (marke) 30-Dec-1992

Revision History:

	Dan Almosnino (danalm) 20-Sep-1995

	Added TEXT_QRUN, Text String-Length choice items (IDM_S...), Font choice option

	Dan Almosnino (danalm) 17-Oct-1995

	Added RUN_BATCH			for batch mode execution
	Added IDM_TRANSPARENT 	for transparent background text option
--*/
#define IDR_ACCELERATOR2                106
#define ID_RUN               40001

#define ID_DC                   110
#define IDD_RESULTS             111
#define IDD_HELP				913
#define IDC_RESULTSLIST         112
#define IDM_SAVERESULTS         113
#define IDM_SHOW                114
#define IDR_USRBENCH_MENU       101
//#define IDUSERBENCH				122
#define IDM_EXIT                202
#define IDM_RUN                 205
#define IDM_QRUN1               206
#define IDM_QRUN2   			207
#define RUN_BATCH				208
#define SHOW_HELP				911
#define IDM_HELP				911
#define IDC_HELPLIST			912
#define IDM_S001	             221
#define IDM_S002	             222
#define IDM_S004	             223
#define IDM_S008	             224
#define IDM_S016	             225
#define IDM_S032	             226
#define IDM_S064	             227
#define IDM_S128	             228
#define IDM_SXXX	             240
#define IDM_FONT                250
#define IDM_TRANSPARENT			260
#define ID_TEST_START           400
/*
 * Menu IDs.
 */

#define IDM_DUMMY           1
#define IDM_ABOUT           150
//#define IDM_EXIT            151
#define IDM_ALL             152
#define IDM_VIEWRESULTS     153
//#define IDM_SAVERESULTS     154
#define IDM_PROFILEALL      155

//#define IDM_BENCHFIRST      1000

/*
 * Control IDs.
 */

//#define IDC_RESULTSLIST     100
#define ID_EDIT             0xCAC
#define ID2             134
#define ID256           135
#define IDWIDTH         100
#define IDHEIGHT        101
#define IDIN            102
#define IDCM            103
#define IDPELS          104

/*
 * Dialog IDs
 */

#define IDD_ABOUT       300
//#define IDD_RESULTS     301
#define CLEARBOX        302
#define CLEARBOXNOMENU  303
#define CLEARBOXNOFONT  304
#define EMPTY           305



