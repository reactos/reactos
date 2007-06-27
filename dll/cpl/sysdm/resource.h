#ifndef __CPL_RESOURCE_H
#define __CPL_RESOURCE_H

#define IDC_STATIC  -1

#define IDI_CPLSYSTEM                   50
#define IDI_DEVMGR                      51
#define IDI_HARDPROF                    52
#define IDI_USERPROF                    53

#define IDB_ROSBMP                      55

#define IDS_CPLSYSTEMNAME               60
#define IDS_CPLSYSTEMDESCRIPTION        61

#define IDS_MEGABYTE                    62
#define IDS_GIGABYTE                    63
#define IDS_TERABYTE                    64
#define IDS_PETABYTE                    65

#define IDS_VARIABLE                    66
#define IDS_VALUE                       67

/* propsheet - general */
#define IDD_PROPPAGEGENERAL             100
#define IDC_MACHINELINE1                101
#define IDC_MACHINELINE2                102
#define IDC_MACHINELINE3                103
#define IDC_MACHINELINE4                104
#define IDC_MACHINELINE5                105
#define IDC_LICENCE                     106
#define IDC_ROSIMG                      107
#define IDC_ROSHOMEPAGE_LINK            108


/* propsheet - hardware */
#define IDD_PROPPAGEHARDWARE            300
#define IDC_HARDWARE_WIZARD	            207
#define IDC_HARDWARE_PROFILE            209
#define IDC_HARDWARE_DRIVER_SIGN        210
#define IDC_HARDWARE_DEVICE_MANAGER	    211


/* propsheet - advanced */
#define IDD_PROPPAGEADVANCED            400
#define IDC_ENVVAR                      401
#define IDC_STAREC                      402
#define IDC_PERFOR                      403
#define IDC_USERPROFILE                 404
#define IDC_ERRORREPORT                 405


/* user profiles */
#define IDD_USERPROFILE                 500
#define IDC_USERPROFILE_LIST            501
#define IDC_USERPROFILE_CHANGE          503
#define IDC_USERPROFILE_DELETE          504
#define IDC_USERPROFILE_COPY            505
#define IDC_USERACCOUNT_LINK            506


/* environment variables */
#define IDD_ENVIRONMENT_VARIABLES       600
#define IDC_USER_VARIABLE_LIST          601
#define IDC_USER_VARIABLE_NEW           602
#define IDC_USER_VARIABLE_EDIT          603
#define IDC_USER_VARIABLE_DELETE        604
#define IDC_SYSTEM_VARIABLE_LIST        605
#define IDC_SYSTEM_VARIABLE_NEW         606
#define IDC_SYSTEM_VARIABLE_EDIT        607
#define IDC_SYSTEM_VARIABLE_DELETE      608


/* edit environment variables */
#define IDD_EDIT_VARIABLE               700
#define IDC_VARIABLE_NAME               701
#define IDC_VARIABLE_VALUE              702


/* Virtual memory */
#define IDD_VIRTMEM                     900
#define IDC_PAGEFILELIST                901
#define IDC_DRIVEGROUP                  902
#define IDC_DRIVE                       903
#define IDC_SPACEAVAIL                  904
#define IDC_CUSTOM                      905
#define IDC_INITIALSIZE                 906
#define IDC_MAXSIZE                     907
#define IDC_SYSMANSIZE                  908
#define IDC_NOPAGEFILE                  909
#define IDC_SET                         910
#define IDC_TOTALGROUP                  911
#define IDC_MINIMUM                     912
#define IDC_RECOMMENDED                 913
#define IDC_CURRENT                     914


/* startup and recovery */
#define IDD_STARTUPRECOVERY             1000
#define IDC_STRECOSCOMBO                1001
#define IDC_STRECLIST                   1002
#define IDC_STRRECLISTEDIT              1003
#define IDC_STRRECLISTUPDWN             1004
#define IDC_STRRECREC                   1005
#define IDC_STRRECRECEDIT               1006
#define IDC_STRRECRECUPDWN              1007
#define IDC_STRRECEDIT                  1008
#define IDC_STRRECWRITEEVENT            1009
#define IDC_STRRECSENDALERT             1010
#define IDC_STRRECRESTART               1011
#define IDC_STRRECDEBUGCOMBO            1012
#define IDC_STRRECDUMPFILE              1013
#define IDC_STRRECOVERWRITE             1014


/* hardware profiles */
#define IDD_HARDWAREPROFILES            1100
#define IDC_HRDPROFLSTBOX               1102
#define IDC_HRDPROFUP                   1103
#define IDC_HRDPROFDWN                  1104
#define IDC_HRDPROFPROP                 1105
#define IDC_HRDPROFCOPY                 1106
#define IDC_HRDPROFRENAME               1107
#define IDC_HRDPROFDEL                  1108
#define IDC_HRDPROFWAIT                 1109
#define IDC_HRDPROFSELECT               1110
#define IDC_HRDPROFEDIT                 1111
#define IDC_HRDPROFUPDWN                1112


/* rename profile */
#define IDD_RENAMEPROFILE               1200
#define IDC_RENPROFEDITFROM             1201
#define IDC_RENPROFEDITTO               1202

/* licence */
#define IDD_LICENCE                     1500
#define IDC_LICENCEEDIT                 1501
#define RC_LICENSE                      1502
#define RTDATA                          1503


#endif /* __CPL_RESOURCE_H */
