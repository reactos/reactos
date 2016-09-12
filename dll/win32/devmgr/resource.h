#pragma once

#define IDC_STATIC          -1

#define IDS_APPNAME         10

#define IDI_MAIN_ICON       20
#define IDB_ROOT_IMAGE      21
#define IDB_TOOLBAR         22

/* windows */
#define IDC_TREEVIEW        30
#define IDC_TOOLBAR         31
#define IDC_STATUSBAR       32
#define IDR_MAINMENU        33
#define IDR_POPUP           34

/* Actions */
#define IDC_PROPERTIES      40
#define IDC_SCAN_HARDWARE   41
#define IDC_ENABLE_DRV      42
#define IDC_DISABLE_DRV     43
#define IDC_UPDATE_DRV      44
#define IDC_UNINSTALL_DRV   45
#define IDC_ADD_HARDWARE    46

/* Menu items */
#define IDC_ACTIONMENU      50
#define IDC_ABOUT           51
#define IDC_EXIT            52


/* view menu */
#define IDC_DEVBYTYPE       60
#define IDC_DEVBYCONN       61
#define IDC_RESBYTYPE       62
#define IDC_RESBYCONN       63
#define IDC_SHOWHIDDEN      64


/* tooltips */
#define IDS_TOOLTIP_PROPERTIES  70
#define IDS_TOOLTIP_SCAN        71
#define IDS_TOOLTIP_ENABLE      72
#define IDS_TOOLTIP_DISABLE     73
#define IDS_TOOLTIP_UPDATE      74
#define IDS_TOOLTIP_UNINSTALL   75

/* General strings */
#define IDS_CONFIRM_DISABLE     80
#define IDS_CONFIRM_UNINSTALL   81

/* Menu strings */
#define IDS_MENU_UPDATE         90
#define IDS_MENU_ENABLE         91
#define IDS_MENU_DISABLE        92
#define IDS_MENU_UNINSTALL      93
#define IDS_MENU_SCAN           94
#define IDS_MENU_ADD            95
#define IDS_MENU_PROPERTIES     96


/* menu hints */
#define IDS_HINT_BLANK          100
#define IDS_HINT_PROPERTIES     101
#define IDS_HINT_SCAN           102
#define IDS_HINT_ENABLE         103
#define IDS_HINT_DISABLE        104
#define IDS_HINT_UPDATE         105
#define IDS_HINT_UNINSTALL      106
#define IDS_HINT_ADD            107
#define IDS_HINT_ABOUT          108
#define IDS_HINT_EXIT           109

#define IDS_HINT_DEV_BY_TYPE    120
#define IDS_HINT_DEV_BY_CONN    121
#define IDS_HINT_RES_BY_TYPE    123
#define IDS_HINT_RES_BY_CONN    124
#define IDS_HINT_SHOW_HIDDEN    125

/* system menu hints */
#define IDS_HINT_SYS_RESTORE    130
#define IDS_HINT_SYS_MOVE       131
#define IDS_HINT_SYS_SIZE       132
#define IDS_HINT_SYS_MINIMIZE   133
#define IDS_HINT_SYS_MAXIMIZE   134
#define IDS_HINT_SYS_CLOSE      135








#define IDI_DEVMGR 255

#define IDS_NAME                  0x100
#define IDS_TYPE                  0x101
#define IDS_MANUFACTURER          0x102
#define IDS_LOCATION              0x103
#define IDS_STATUS                0x104
#define IDS_UNKNOWN               0x105
#define IDS_LOCATIONSTR           0x106
#define IDS_DEVCODE               0x107
#define IDS_DEVCODE2              0x108
#define IDS_ENABLEDEVICE          0x109
#define IDS_DISABLEDEVICE         0x10A
#define IDS_UNKNOWNDEVICE         0x10B
#define IDS_NODRIVERLOADED        0x10C
#define IDS_DEVONPARENT           0x10D
#define IDS_TROUBLESHOOTDEV       0x10E
#define IDS_ENABLEDEV             0x10F
#define IDS_PROPERTIES            0x110
#define IDS_UPDATEDRV             0x111
#define IDS_REINSTALLDRV          0x112
#define IDS_REBOOT                0x113
#define IDS_NOTAVAILABLE          0x114
#define IDS_NOTDIGITALLYSIGNED    0x115
#define IDS_NODRIVERS             0x116
#define IDS_RESOURCE_COLUMN       0x117
#define IDS_SETTING_COLUMN        0x118
#define IDS_RESOURCE_MEMORY_RANGE 0x119
#define IDS_RESOURCE_INTERRUPT    0x11A
#define IDS_RESOURCE_DMA          0x11B
#define IDS_RESOURCE_PORT         0x11C

#define IDS_DEV_NO_PROBLEM                 0x200
#define IDS_DEV_NOT_CONFIGURED             0x201
#define IDS_DEV_DEVLOADER_FAILED           0x202
#define IDS_DEV_DEVLOADER_FAILED2          0x203
#define IDS_DEV_OUT_OF_MEMORY              0x204
#define IDS_DEV_ENTRY_IS_WRONG_TYPE        0x205
#define IDS_DEV_LACKED_ARBITRATOR          0x206
#define IDS_DEV_BOOT_CONFIG_CONFLICT       0x207
#define IDS_DEV_FAILED_FILTER              0x208
#define IDS_DEV_DEVLOADER_NOT_FOUND        0x209
#define IDS_DEV_DEVLOADER_NOT_FOUND2       0x20A
#define IDS_DEV_DEVLOADER_NOT_FOUND3       0x20B
#define IDS_DEV_INVALID_DATA               0x20C
#define IDS_DEV_INVALID_DATA2              0x20D
#define IDS_DEV_FAILED_START               0x20E
#define IDS_DEV_LIAR                       0x20F
#define IDS_DEV_NORMAL_CONFLICT            0x210
#define IDS_DEV_NOT_VERIFIED               0x211
#define IDS_DEV_NEED_RESTART               0x212
#define IDS_DEV_REENUMERATION              0x213
#define IDS_DEV_PARTIAL_LOG_CONF           0x214
#define IDS_DEV_UNKNOWN_RESOURCE           0x215
#define IDS_DEV_REINSTALL                  0x216
#define IDS_DEV_REGISTRY                   0x217
#define IDS_DEV_WILL_BE_REMOVED            0x218
#define IDS_DEV_DISABLED                   0x219
#define IDS_DEV_DISABLED2                  0x21A
#define IDS_DEV_DEVLOADER_NOT_READY        0x21B
#define IDS_DEV_DEVLOADER_NOT_READY2       0x21C
#define IDS_DEV_DEVLOADER_NOT_READY3       0x21D
#define IDS_DEV_DEVICE_NOT_THERE           0x21E
#define IDS_DEV_MOVED                      0x21F
#define IDS_DEV_TOO_EARLY                  0x220
#define IDS_DEV_NO_VALID_LOG_CONF          0x221
#define IDS_DEV_FAILED_INSTALL             0x222
#define IDS_DEV_HARDWARE_DISABLED          0x223
#define IDS_DEV_CANT_SHARE_IRQ             0x224
#define IDS_DEV_FAILED_ADD                 0x225
#define IDS_DEV_DISABLED_SERVICE           0x226
#define IDS_DEV_TRANSLATION_FAILED         0x227
#define IDS_DEV_NO_SOFTCONFIG              0x228
#define IDS_DEV_BIOS_TABLE                 0x229
#define IDS_DEV_IRQ_TRANSLATION_FAILED     0x22A
#define IDS_DEV_FAILED_DRIVER_ENTRY        0x22B
#define IDS_DEV_DRIVER_FAILED_PRIOR_UNLOAD 0x22C
#define IDS_DEV_DRIVER_FAILED_LOAD         0x22D
#define IDS_DEV_DRIVER_SERVICE_KEY_INVALID 0x22E
#define IDS_DEV_LEGACY_SERVICE_NO_DEVICES  0x22F
#define IDS_DEV_DUPLICATE_DEVICE           0x230
#define IDS_DEV_FAILED_POST_START          0x231
#define IDS_DEV_HALTED                     0x232
#define IDS_DEV_PHANTOM                    0x233
#define IDS_DEV_SYSTEM_SHUTDOWN            0x234
#define IDS_DEV_HELD_FOR_EJECT             0x235
#define IDS_DEV_DRIVER_BLOCKED             0x236
#define IDS_DEV_REGISTRY_TOO_LARGE         0x237
#define IDS_DEV_SETPROPERTIES_FAILED       0x238

#define IDS_PROP_DEVICEID           0x300
#define IDS_PROP_HARDWAREIDS        0x301
#define IDS_PROP_COMPATIBLEIDS      0x302
#define IDS_PROP_MATCHINGDEVICEID   0x303
#define IDS_PROP_SERVICE            0x304
#define IDS_PROP_ENUMERATOR         0x305
#define IDS_PROP_CAPABILITIES       0x306
#define IDS_PROP_DEVNODEFLAGS       0x307
#define IDS_PROP_CONFIGFLAGS        0x308
#define IDS_PROP_CSCONFIGFLAGS      0x309
#define IDS_PROP_EJECTIONRELATIONS  0x30A
#define IDS_PROP_REMOVALRELATIONS   0x30B
#define IDS_PROP_BUSRELATIONS       0x30C
#define IDS_PROP_DEVUPPERFILTERS    0x30D
#define IDS_PROP_DEVLOWERFILTERS    0x30E
#define IDS_PROP_CLASSUPPERFILTERS  0x30F
#define IDS_PROP_CLASSLOWERFILTERS  0x310
#define IDS_PROP_CLASSINSTALLER     0x311
#define IDS_PROP_CLASSCOINSTALLER   0x312
#define IDS_PROP_DEVICECOINSTALLER  0x313
#define IDS_PROP_FIRMWAREREVISION   0x314
#define IDS_PROP_CURRENTPOWERSTATE  0x315
#define IDS_PROP_POWERCAPABILITIES  0x316
#define IDS_PROP_POWERSTATEMAPPINGS 0x317

#define IDD_HARDWARE        0x400
#define IDD_DEVICEGENERAL   0x401
#define IDD_DEVICEDRIVER    0x402
#define IDD_DEVICERESOURCES 0x403
#define IDD_DRIVERDETAILS   0x404
#define IDD_DEVICEDETAILS   0x405
#define IDD_DEVICEPOWER     0x406

#define IDC_DEVICON          0x57B
#define IDC_DEVNAME          0x57C
#define IDC_DEVTYPE          0x57D
#define IDC_DEVMANUFACTURER  0x57E
#define IDC_DEVLOCATION      0x57F
#define IDC_DEVSTATUSGROUP   0x580
#define IDC_DEVSTATUS        0x581
#define IDC_DEVUSAGE         0x582
#define IDC_DEVICES          0x583
#define IDC_LV_DEVICES       0x584
#define IDC_PROPERTIESGROUP  0x585
#define IDC_MANUFACTURER     0x587
#define IDC_LOCATION         0x588
#define IDC_STATUS           0x586
#define IDC_TROUBLESHOOT     0x589
#define IDC_PROPERTIES2      0x58A
#define IDC_DEVUSAGELABEL    0x58B
#define IDC_DEVPROBLEM       0x58C
#define IDC_DRVPROVIDER      0x58D
#define IDC_DRVDATE          0x58E
#define IDC_DRVVERSION       0x58F
#define IDC_DIGITALSIGNER    0x590
#define IDC_DRIVERDETAILS    0x591
#define IDC_DRIVERFILES      0x592
#define IDC_FILEPROVIDER     0x593
#define IDC_FILEVERSION      0x594
#define IDC_FILECOPYRIGHT    0x595
#define IDC_DETAILSPROPNAME  0x596
#define IDC_DETAILSPROPVALUE 0x597
#define IDC_UPDATEDRIVER     0x598
#define IDC_DRIVERRESOURCES  0x599
