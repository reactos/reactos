//{{NO_DEPENDENCIES}}
// App Studio generated include file.
// Used by WIZ.RC
//

#define DLG_APPLIST             100
#define DLG_INSTUNINSTALL       101

#define DLG_BROWSE              200
#define DLG_PICKFOLDER          201
#define DLG_GETTITLE            202
#define DLG_PICKICON            203
#define DLG_WHICHCFG            204
#define DLG_RMOPTIONS           205

#define DLG_SETUP               300
#define DLG_SETUPBROWSE         301

#ifndef DOWNLEVEL_PLATFORM
#ifdef WINNT       
#define DLG_CHGUSR                      302
#define DLG_CHGUSR_UNINSTALL            303
#define DLG_CHGUSRFINISH                304
#define DLG_CHGUSRFINISH_PREV           305
#endif // WINNT
#endif // DOWNLEVEL_PLATFORM

#define DLG_UNCUNINSTALLBROWSE  306


#define DLG_DELITEM             400

#define IDC_BUTTONSETUPFROMLIST 1000
#define IDC_BUTTONSETUP         1001
#define IDC_CMDSTATIC           1002
#define IDC_COMMAND             1003
#define IDC_BROWSE              1004
#define IDC_SEARCHNAME          1005
#define IDC_SEARCHICON          1006
#define IDC_FROMLIST            1009
#define IDC_FROMDISK            1010
#define IDC_APPLIST             1011
#define IDC_SETUPMSG            1012
#ifndef DOWNLEVEL_PLATFORM
#ifdef WINNT       
#define IDC_RADIO1                      1001
#define IDC_RADIO2                      1002
#endif // WINNT
#endif // DOWNLEVEL_PLATFORM

#define IDB_CURCFG              1015    // Button group
#define IDB_CLEANCFG            1016    // end button group

#define IDC_WIZBMP              1019
#define IDC_TITLE               1020
#define IDC_ICONLIST            1021
#define IDC_FOLDERTREE          1022
#define IDC_OPTIONLIST          1023
#define IDC_OPTIONTIP           1024

#define IDC_REGISTERED_APPS     1025
#define IDC_MODIFYUNINSTALL     1026

#define IDC_BASEBUTTONS         1060
#define IDC_MODIFY              IDC_BASEBUTTONS
#define IDC_REPAIR              IDC_BASEBUTTONS + 1
#define IDC_UNINSTALL           IDC_BASEBUTTONS + 2

#define IDC_NEWFOLDER           1027
#define IDC_DELFOLDER           1028

#define IDC_DELETEITEM          1030
#define IDC_TEXT                1031

#define IDC_INSTINSTR           1040
#define IDC_INSTICON            1041
#define IDC_UNINSTINSTR         1042
#define IDC_UNINSTICON          1043

#define IDC_FORCEX86ENV         1045

#define IDC_NETINSTINSTR        1050
#define IDC_NETINSTICON         1051

#define IDS_UNINSTINSTR         1065
#define IDS_UNINSTINSTR_NEW     1066
#define IDS_UNINSTINSTR_LEGACY  1067

//#define IDC_

#define IDC_STATIC              -1

//
//  Icons
//
#define IDI_CPLICON             1500
#define IDI_LISTINST            1501
#define IDI_DISKINST            1502
#define IDI_UNINSTALL           1503
#define IDI_FOLDEROPT           1504


#define IDS_NAME                2001
#define IDS_INFO                2002
#define IDS_BADPATHMSG          2003
#define IDS_SETUPPRGNAMES       2004
#define IDS_HAVESETUPPRG        2005
#define IDS_NOSETUPPRG          2006
#define IDS_TSHAVESETUPPRG      2007

#define IDS_INSERTDISK          2008
#define IDS_SEARCHING           2009
#define IDS_EXTENSIONS          2010
#define IDS_BADSETUP            2011
#define IDS_DUPLINK             2012
#define IDS_BROWSEEXT           2014
#define IDS_BROWSEFILTER        2015
#define IDS_BROWSETITLE         2016
#define IDS_NOCOPYENV           2017
#define IDS_SETCMD              2018
#define IDS_SPECIALCASE         2019
#define IDS_BROWSEFILTERMSI     2020

#define IDS_DEFBOOTDIR          2021
#define IDS_VMCLOSED            2022
#define IDS_VMSTILLALIVE        2023
#define IDS_GENERICNAME         2024
#define IDS_CHGPROPCLOSED       2025
#define IDS_CHGPROPSTILLALIVE   2026
#define IDS_NOSHORTCUT          2027

#define IDS_SETUPAPPNAMES       2028

#define IDS_UNINSTALL_ERROR         2030
#define IDS_UNINSTALL_FAILED        2031
#define IDS_OK                      2032
#define IDS_1APPWARNTITLE           2033
#define IDS_CANTDELETE              2035
#define IDS_NEWFOLDERSHORT          2036
#define IDS_NEWFOLDERLONG           2037
#define IDS_NONESEL                 2038
#define IDS_NOSUPPORT1              2040
#define IDS_NOSUPPORT2              2041
#define IDS_UNINSTALL_UNCUNACCESSIBLE 2042
#define IDS_CONFIGURE_FAILED        2043
#define IDS_CANT_REMOVE_FROM_REGISTRY 2044

#define IDS_CSHIGHSTRS              2050
#define IDS_CSLOWSTR                2051
#define IDS_AEHIGHSTRS              2052
#define IDS_MOUSEENV                2053
#define IDS_MOUSETSRS               2054
#define IDS_MOUSEDRVS               2055
#define IDS_LOADHIGH                2056
#define IDS_DEVHIGH                 2057
#define IDS_MODNAME                 2060

#define IDS_FOLDEROPT_NAME          2070
#define IDS_FOLDEROPT_INFO          2071
#define IDS_FILEFOLDERBROWSE_TITLE  2072

#ifndef DOWNLEVEL_PLATFORM
#ifdef WINNT       
#define IDS_CHGUSROPT               2081
#define IDS_CHGUSRINSTALL           2082
#define IDS_CHGUSREXECUTE           2083
#define IDS_CHGUSRTITLE             2084
#define IDS_GETINI_FAILED           2085
#define IDS_SETINI_FAILED           2086
#define IDS_CHGUSRUNINSTALL         2087
#define IDS_CHGUSRUNEXECUTE         2088
#define IDS_CHGUSRUNINSTALLMSG		2089
#define IDS_QUERYVALUE_FAILED       2090
#define IDS_CHGUSRFINISH		    2091
#define IDS_CHGUSRFINISH_PREV		2092
#endif // WINNT
#endif // DOWNLEVEL_PLATFORM


#define IDS_INSTALL_ERROR_GENERIC   2100
#define IDS_INSTALL_TRANSFORMCONFLICTS      2101
#define IDS_UNINSTALL_ERROR_GENERIC   2102
#define IDS_MODIFY_ERROR_GENERIC   2103
#define IDS_REPAIR_ERROR_GENERIC   2104

#define IDB_INSTALLBMP          5000
#define IDB_SHORTCUTBMP         5001
#define IDB_DOSCONFIG           5002
#define IDB_LEGACYINSTALLBMP    5003

#define IDB_CHECKSTATES         5100
