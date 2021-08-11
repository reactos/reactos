/*
 * PROJECT:         ReactOS On-Screen Keyboard
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         On-Screen Keyboard resource identifiers
 * COPYRIGHT:       Denis ROBERT
 */

#define IDI_SOUNDCLICK                      50

#define IDC_LED_NUM                         100
#define IDC_LED_CAPS                        101
#define IDC_LED_SCROLL                      102
#define IDC_SHOWWARNINGCHECK                103

#define IDI_OSK                             200
#define IDI_BACK                            201
#define IDI_TAB                             202
#define IDI_CAPS_LOCK                       203
#define IDI_RETURN                          204
#define IDI_SHIFT                           205
#define IDI_REACTOS                         206
#define IDI_MENU                            207
#define IDI_HOME                            208
#define IDI_PG_UP                           209
#define IDI_PG_DOWN                         210
#define IDI_LEFT                            211
#define IDI_TOP                             212
#define IDI_RIGHT                           213
#define IDI_BOTTOM                          214

#define IDR_OSK_MENU                        600
#define IDM_EXIT                            601
#define IDM_STANDARD_KB                     602
#define IDM_ENHANCED_KB                     603
#define IDM_REG_LAYOUT                      604
#define IDM_BLOCK_LAYOUT                    605
#define IDM_101_KEYS                        606
#define IDM_102_KEYS                        607
#define IDM_106_KEYS                        608
#define IDM_ON_TOP                          609
#define IDM_CLICK_SOUND                     610
#define IDM_TYPE_MODE                       611
#define IDM_FONT                            612
#define IDM_HELP_TOPICS                     613
#define IDM_ABOUT                           614

#define IDD_WARNINGDIALOG_OSK               215

#define IDS_OSK                             500
#define IDS_AUTHORS                         501

#define IDS_NUMLOCK                         502
#define IDS_CAPSLOCK                        503
#define IDS_SCROLLLOCK                      504

#define IDS_ESCAPE                          505
#define IDS_PRN                             506
#define IDS_STOP                            507 /* SCROLL LOCK */
#define IDS_ATTN                            508 /* PAUSE KEY */
#define IDS_INSERT                          509
#define IDS_NUMLOCKKEY                      510
#define IDS_DELETE                          511
#define IDS_END                             512
#define IDS_CTRL                            513
#define IDS_LEFTALT                         514
#define IDS_RIGHTALT                        515

#define IDC_STATIC                          -1


/* Scan codes by key number */
/* + 0x0100 to avoid a conflict with IDCANCEL = 2 */
/* + 0x0200 if extended key */

#define SCAN_CODE_1     0x0129
#define SCAN_CODE_2     0x0102
#define SCAN_CODE_3     0x0103
#define SCAN_CODE_4     0x0104
#define SCAN_CODE_5     0x0105
#define SCAN_CODE_6     0x0106
#define SCAN_CODE_7     0x0107
#define SCAN_CODE_8     0x0108
#define SCAN_CODE_9     0x0109
#define SCAN_CODE_10    0x010A
#define SCAN_CODE_11    0x010B
#define SCAN_CODE_12    0x010C
#define SCAN_CODE_13    0x010D
#define SCAN_CODE_15    0x010E
#define SCAN_CODE_16    0x010F
#define SCAN_CODE_17    0x0110
#define SCAN_CODE_18    0x0111
#define SCAN_CODE_19    0x0112
#define SCAN_CODE_20    0x0113
#define SCAN_CODE_21    0x0114
#define SCAN_CODE_22    0x0115
#define SCAN_CODE_23    0x0116
#define SCAN_CODE_24    0x0117
#define SCAN_CODE_25    0x0118
#define SCAN_CODE_26    0x0119
#define SCAN_CODE_27    0x011A
#define SCAN_CODE_28    0x011B
#define SCAN_CODE_29    0x012B
#define SCAN_CODE_30    0x013A
#define SCAN_CODE_31    0x011E
#define SCAN_CODE_32    0x011F
#define SCAN_CODE_33    0x0120
#define SCAN_CODE_34    0x0121
#define SCAN_CODE_35    0x0122
#define SCAN_CODE_36    0x0123
#define SCAN_CODE_37    0x0124
#define SCAN_CODE_38    0x0125
#define SCAN_CODE_39    0x0126
#define SCAN_CODE_40    0x0127
#define SCAN_CODE_41    0x0128
#define SCAN_CODE_42    0x012B
#define SCAN_CODE_43    0x011C
#define SCAN_CODE_44    0x012A
#define SCAN_CODE_45    0x0156
#define SCAN_CODE_46    0x012C
#define SCAN_CODE_47    0x012D
#define SCAN_CODE_48    0x012E
#define SCAN_CODE_49    0x012F
#define SCAN_CODE_50    0x0130
#define SCAN_CODE_51    0x0131
#define SCAN_CODE_52    0x0132
#define SCAN_CODE_53    0x0133
#define SCAN_CODE_54    0x0134
#define SCAN_CODE_55    0x0135
#define SCAN_CODE_57    0x0136
#define SCAN_CODE_58    0x011D
#define SCAN_CODE_60    0x0138
#define SCAN_CODE_61    0x0139
#define SCAN_CODE_62    0x0338
#define SCAN_CODE_64    0x031D
#define SCAN_CODE_75    0x0352
#define SCAN_CODE_76    0x0353
#define SCAN_CODE_79    0x034B
#define SCAN_CODE_80    0x0347
#define SCAN_CODE_81    0x034F
#define SCAN_CODE_83    0x0348
#define SCAN_CODE_84    0x0350
#define SCAN_CODE_85    0x0349
#define SCAN_CODE_86    0x0351
#define SCAN_CODE_89    0x034D
#define SCAN_CODE_90    0x0145
#define SCAN_CODE_91    0x0147
#define SCAN_CODE_92    0x014B
#define SCAN_CODE_93    0x014F
#define SCAN_CODE_95    0x0335
#define SCAN_CODE_96    0x0148
#define SCAN_CODE_97    0x014C
#define SCAN_CODE_98    0x0150
#define SCAN_CODE_99    0x0152
#define SCAN_CODE_100   0x0137
#define SCAN_CODE_101   0x0149
#define SCAN_CODE_102   0x014D
#define SCAN_CODE_103   0x0151
#define SCAN_CODE_104   0x0153
#define SCAN_CODE_105   0x014A
#define SCAN_CODE_106   0x014E
#define SCAN_CODE_108   0x031C
#define SCAN_CODE_110   0x0101
#define SCAN_CODE_112   0x013B
#define SCAN_CODE_113   0x013C
#define SCAN_CODE_114   0x013D
#define SCAN_CODE_115   0x013E
#define SCAN_CODE_116   0x013F
#define SCAN_CODE_117   0x0140
#define SCAN_CODE_118   0x0141
#define SCAN_CODE_119   0x0142
#define SCAN_CODE_120   0x0143
#define SCAN_CODE_121   0x0144
#define SCAN_CODE_122   0x0157
#define SCAN_CODE_123   0x0158
#define SCAN_CODE_124   0x032A
#define SCAN_CODE_125   0x0146
#define SCAN_CODE_126   0x071D

#define SCAN_CODE_127   0x035B  // Left ROS
#define SCAN_CODE_128   0x035C  // Right ROS
#define SCAN_CODE_129   0x035D  // Applications

/* EOF */
