//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

#define IDH_NEWDEVICEAPPLET             301
#define IDB_BANNERBMP                   305
#define IDB_WATERBMP                    306

#define IDI_NEWDEVICEICON               602
#define IDI_INTERNET                    603
#define IDI_FOLDER                      604

#define IDD_NEWDEVWIZ_INTRO             10125
#define IDD_NEWDEVWIZ_SELECTCLASS       10126
#define IDD_NEWDEVWIZ_SELECTDEVICE      10127
#define IDD_WIZARDEXT_PRESELECT         10128
#define IDD_WIZARDEXT_SELECT            IDD_DYNAWIZ_SELECTCLASS_PAGE
                   // setupapi contains IDD_DYNAWIZ_SELECTDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE        IDD_DYNAWIZ_ANALYZEDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE_END    10129
#define IDD_NEWDEVWIZ_ANALYZEDEV        10130

#define IDD_WIZARDEXT_POSTANALYZE       10131
#define IDD_WIZARDEXT_POSTANALYZE_END   10132
#define IDD_NEWDEVWIZ_INSTALLDEV        10133

#define IDD_WIZARDEXT_FINISHINSTALL     10134
#define IDD_WIZARDEXT_FINISHINSTALL_END 10135
#define IDD_NEWDEVWIZ_FINISH            10136

#define IDD_DRVUPD                      10151
#define IDD_DRVUPD_SEARCH               10152
#define IDD_DRVUPD_SEARCHING            10153
#define IDD_NEWDEVWIZ_PNPENUM           10154
#define IDD_LISTDRIVERS                 10155
#define DLG_DEVINSTALL                  10156


#define IDC_NDW_TEXT                    1002
#define IDC_NDW_PICKCLASS_CLASSLIST     1007
#define IDC_NDW_PICKCLASS_HWTYPES       1008
#define IDC_NDW_DESCRIPTION             1009
#define IDC_NDW_INSTRUCTIONS            1010
#define IDC_NDW_DISPLAYRESOURCE         1011
#define IDC_NDW_CONFLICTHELP            1012
#define IDC_NDW_DISABLEDEVICE           1013
#define IDC_CLASSICON                   1014
#define IDC_RESTART                     1016
#define IDC_DRVUPD_DRVDESC              1018
#define IDC_DRVUPD_DRVMSG1              1019
#define IDC_DRVUPD_DRVMSG2              1020
#define IDC_DRVUPD_SEARCH               1021
#define IDC_DRVUPD_SELECTDRIVER         1022
#define IDC_SEARCHOPTION_FLOPPY         1024
#define IDC_SEARCHOPTION_CDROM          1025
#define IDC_SEARCHOPTION_INTERNET       1026
#define IDC_SEARCHOPTION_OTHER          1027

#define IDC_SEARCHICON                  1028
#define IDC_SEARCHNAME                  1029
#define IDC_FINISHINSTALL               1039
#define IDC_CANCELINSTALL               1040
#define IDC_LISTDRIVERS                 1041
#define IDC_LISTDRIVERS_TEXT            1042
#define IDC_LISTDRIVERS_LISTVIEW        1043
#define IDC_INTRO_MSG1                  1044
#define IDC_INTRO_MSG2                  1045
#define IDC_FINISH_MSG1                 1046

#define IDS_UNKNOWN                     2000
#define IDS_NDW_NOTADMIN                2001
#define IDS_NEWDEVICENAME               2002

#define IDS_NEEDREBOOT                  2009
#define IDS_NDW_PICKCLASS1              2027
#define IDS_NDW_ANALYZEERR1             2034
#define IDS_NDW_ANALYZEERR2             2035
#define IDS_NDW_ANALYZEERR3             2036
#define IDS_UPDATEDEVICE                2039
#define IDS_FOUNDDEVICE                 2040
#define IDS_NDW_ERRORFIN1               2041
#define IDS_NDW_ERRORFIN1_PNP           2043
#define IDS_NDW_NORMALFINISH2           2046
#define IDS_NDW_NORMALFINISH1           2047
#define IDS_INSTALL_PROBLEM             2048
#define IDS_INSTALL_PROBLEM_PNP         2049
#define IDS_NDW_STDCFG1                 2050
#define IDS_NDW_STDCFG2                 2051

#define IDS_DRVUPD                       2066
#define IDS_DRVUPD_INFO                  2067
#define IDS_DRVUPD_SEARCH                2068
#define IDS_DRVUPD_SEARCH_INFO           2069
#define IDS_DRVUPD_SEARCHING             2070
#define IDS_DRVUPD_SEARCHING_INFO        2071
#define IDS_NEWDEVWIZ_SELECTCLASS        2072
#define IDS_NEWDEVWIZ_SELECTCLASS_INFO   2073
#define IDS_NEWDEVWIZ_SELECTDEVICE       2074
#define IDS_NEWDEVWIZ_SELECTDEVICE_INFO  2075
#define IDS_NEWDEVWIZ_ANALYZEDEV         2076
#define IDS_NEWDEVWIZ_ANALYZEDEV_INFO    2077
#define IDS_NEWDEVWIZ_INSTALLDEV         2078
#define IDS_NEWDEVWIZ_INSTALLDEV_INFO    2079
#define IDS_LISTDRIVERS                  2082
#define IDS_LISTDRIVERS_INFO             2083
#define IDS_DRIVERDESC                   2084
#define IDS_DRIVERPROVIDER               2085
#define IDS_DRIVERMFG                    2086
#define IDS_DRIVERINF                    2087
#define IDS_DRIVER_BEST                  2088
#define IDS_DRIVER_CURR                  2089
#define IDS_DRIVER_BESTCURR              2090
#define IDS_NEWDRIVER_WELCOME1           2091
#define IDS_NEWDRIVER_WELCOME            2092
#define IDS_DRVUPD_WELCOME               2093
#define IDS_DRVUPD_WELCOME1              2094
#define IDS_DRIVER_SEARCH                2095
#define IDS_DRIVER_SEARCH1               2096
#define IDS_DRIVER_SEARCH2               2097
#define IDS_INTERNET_HOST                2098
#define IDS_DEFAULT_INTERNET_HOST        2099
#define IDS_DRVUPD_WAIT                  2100
#define IDS_DRVUPD_INSTALLING            2101
#define IDS_DRVUPD_FOUND                 2102
#define IDS_DRVUPD_BETTER                2103
#define IDS_DRVUPD_NOTFOUND              2104
#define IDS_FOUNDNEW_FOUND               2105
#define IDS_FOUNDNEW_NOTFOUND            2106
#define IDS_DEFAULTINF                   2107
#define IDS_UNKNOWNDEVICE                2108
#define IDS_SEARCHING_WAIT               2109
#define IDS_SEARCHING_RESULTS            2110
#define IDS_DRVUPD_LOCATION              2111
#define IDS_INTRO_MSG1_UPGRADE           2112
#define IDS_INTRO_MSG1_NEW               2113
#define IDS_INTRO_MSG2_UPGRADE           2114
#define IDS_INTRO_MSG2_NEW               2115
#define IDS_FINISH_MSG1_UPGRADE          2116
#define IDS_FINISH_MSG1_NEW              2117
#define IDS_DRVUPD_CURRENT               2118
#define IDS_NDW_CURRENTFINISH            2119
#define IDS_SEARCHING                    2120
#define IDS_NEWDEVICE_REBOOT             2121
#define IDS_NEWSEARCH                    2122

#define IDC_STATIC                      -1
