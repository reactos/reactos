#ifndef __resource_h__
#define __resource_h__

#define IDC_STATIC              -1


// The Summary tab

//The function SetReadOnly in propdlg.c depends on
//the specific ordering of these ID's

#define IDD_SUMMARY             2100
#define IDD_SUMMARY_TITLE       2101
#define IDD_SUMMARY_SUBJECT     2102
#define IDD_SUMMARY_AUTHOR      2103
#define IDD_SUMMARY_KEYWORDS    2104
#define IDD_SUMMARY_COMMENTS    2105
#define IDD_SUMMARY_TEMPLATE    2106

#define IDD_SUMMARY_APPLY       2108
#define IDD_EXCEPTION			2110

  // The Statistics tab
#define IDD_STATISTICS                  2200
#define IDD_STATISTICS_CREATED          2201
#define IDD_STATISTICS_ACCESSED         2202
#define IDD_STATISTICS_CHANGED          2203
#define IDD_STATISTICS_LASTPRINT        2204
#define IDD_STATISTICS_LASTSAVEBY       2205
#define IDD_STATISTICS_REVISION         2206
#define IDD_STATISTICS_TOTALEDIT        2207
#define IDD_STATISTICS_LISTBOX          2208


//Strings
#define SZ_NOINFO						3000
#define SZ_PASSWORD						3001
#define SZ_READONLY						3002

#if 0
#define iszBYTES		 4000	
#define iszPAGES         4001
#define iszPARA          4002
#define iszLINES         4003
#define iszWORDS         4004
#define iszCHARS         4005
#define iszSLIDES        4006
#define iszNOTES         4007
#define iszHIDDENSLIDES  4008
#define iszMMCLIPS       4009
#define iszFORMAT        4010
#endif


#endif // __resource_h__
