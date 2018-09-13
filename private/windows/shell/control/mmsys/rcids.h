//****************************************************************************
//
//  Module:     MMSE.DLL
//  File:       rcids.h
//  Content:    This file contains all the constant declaration for the
//              MMSE resources.
//  History:
//      06/1994    -By-    Vij Rajarajan (VijR)
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************


//*****************************************************************************
// Icon ID number section
//*****************************************************************************

#include <cphelp.h>

#define ID_BASE_START                   0x1000
                                                         
#define ID_BASE_CONTROL                 (ID_BASE_START + 0x0500)    // 0x0100 - 0x0400 reserved
#define ID_BASE_STRINGS                 (ID_BASE_START + 0x0600)
#define ID_BASE_FILEPROP_STRINGS        (ID_BASE_START + 0x0700)
#define ID_BASE_EVENTS_STRINGS          (ID_BASE_START + 0x0800)
#define ID_BASE_MIDI_STRINGS            (ID_BASE_START + 0x0900)
#define ID_SIMPLE_PROP                  (ID_BASE_START + 0x0A00)
#define ID_ADVANCED_PROP                (ID_BASE_START + 0x1000)
#define ID_MIDI_PROP                    (ID_BASE_START + 0x1100)
#define ID_FILE_DETAILS                 (ID_BASE_START + 0x1200)
#define ID_EVENTS_START                 (ID_BASE_START + 0x1300)

#define IDC_STATIC                      -1

/////////////////////////////////////////////////////////////////////
// ICONS
/////////////////////////////////////////////////////////////////////
#define IDI_MMICON                      3004    //DO NOT CHANGE THIS ID. Other files depend on this number
#define IDI_IDFICON                     0x1101
#define IDI_WAVE                        0x1102
#define IDI_MIDI                        0x1103
#define IDI_MIXER                       0x1104
#define IDI_AUX                         0x1105
#define IDI_MCI                         0x1106
#define IDI_ICM                         0x1107
#define IDI_ACM                         0x1108
#define IDI_JOYSTICK                    0x1109
#define IDI_SOUNDSCHEMES                0x110A
#define IDI_AUDIO                       0x110B
#define IDI_VIDEO                       0x110C
#define IDI_CDAUDIO                     0x110D
#define IDI_PROGRAM                     0x110E
#define IDI_MSACM                       0x110F
#define IDI_PLAYBACK                    0x1110
#define IDI_RECORD                      0x1111
#define IDI_DWAVE                       0x1112
#define IDI_DMIDI                       0x1113
#define IDI_DVIDEO                      0x1114
#define IDI_INSTRUMENT                  0x1115
#define IDI_CHANNEL                     0x1116
#define IDI_BLANK                       0x1117
#define IDI_SELECTED_IDF                0x1118
#define IDI_NONSEL_IDF                  0x1119
#define IDI_MICROPHONE                  0x111A
#define IDI_SPEAKERICON                 0x111B
#define IDI_RECORDICON                  0x111C
#define IDI_MUSICICON                   0x111D
#define IDI_VOLUME                      0x111E
#define IDI_VOLUP                       0x111F
#define IDI_VOLDOWN                     0x1120
#define IDI_VOLMUTE                     0x1121
#define IDI_VOLMUTEUP                   0x1122
#define IDI_VOLMUTEDOWN                 0x1123
#define IDI_VOLTRANS                    0x1124
#define IDI_GS                          0x1125
#define IDI_MIDIKEY                     0x1126

////////////////////////////////////////////////////////////////////
//BITMAPS
////////////////////////////////////////////////////////////////////

#define IDB_PLAY                        0x1200
#define IDB_STOP                        0x1201
#define IDB_WIZBMP                      0x1202
#define IDB_MONITOR                     0x1203
#define IDB_HEADPHONES                  0x1204
#define IDB_STEREODESKTOP               0x1205
#define IDB_MONOLAPTOP                  0x1206
#define IDB_STEREOLAPTOP                0x1207
#define IDB_STEREOMONITOR               0x1208
#define IDB_STEREOCPU                   0x1209
#define IDB_MOUNTEDSTEREO               0x120A
#define IDB_STEREOKEYBOARD              0x120B
#define IDB_QUADRAPHONIC                0x120C
#define IDB_SURROUND                    0x120D
#define IDB_SURROUND_5_1                0x120E
#define IDB_ROLAND                      0x120F


////////////////////////////////////////////////////////////////////
// DIALOGS
////////////////////////////////////////////////////////////////////

#define CDDLG                           0x1300
#define VIDEODLG                        0x1301
#define AUDIODLG                        0x1302
#define EVENTSDLG                       0x1303
#define DLG_DEV_PROP                    0x1304
#define DLG_CPL_MSACM                   0x1305
#define DLG_ABOUT_MSACM                 0x1306
#define DLG_PRIORITY_SET                0x1307
#define DLG_WAVDEV_PROP                 0x1308
#define DLG_ACMDEV_PROP                 0x1309
#define DLG_FILE_DETAILS                0x130A
#define DLG_MESSAGE_BOX                 0x130B
#define SOUNDDIALOG                     0x130C
#define SAVESCHEMEDLG                   0x130D
#define PREVIEW_DLG                     0x130E
#define IDD_CPL_MIDI                    0x130F
#define IDD_MIDICHANGE                  0x1310
#define IDD_MIDICONFIG                  0x1311
#define IDD_SAVENAME                    0x1312
#define IDD_CPL_MIDI2                   0x1313
#define IDD_MIDICLASS_GEN               0x1314
#define IDD_DEVICE_DETAIL               0x1315
#define IDD_INSTRUMENT_GEN              0x1316
#define IDD_INSTRUMENT_DETAIL           0x1317
#define IDD_MIDIWIZ01                   0x1318
#define IDD_MIDIWIZ02                   0x1319
#define IDD_MIDIWIZ03                   0x131A
#define IDD_MIDIWIZ04                   0x131B
#define ADVVIDEODLG                     0x131C
#define ID_ADVVIDEO_COMPAT              0x131D
#define ID_VIDEO_ADVANCED               0x131E
#define IDD_ROLAND                      0x131F
#define BROWSEDLGTEMPLATE               0x1320
#define IDD_SPEAKERS                    0x1321
#define IDD_PLAYBACKPERF                0x1322
#define IDD_CAPTUREPERF                 0x1323
#define DLG_PP_DRIVERSETTINGS           0x1324
#define IDD_SPEECH                      0x1325
#define HWDLG                           0x1326
#define DM_ADVDLG                       0x1327
#define DM_CDDLG                        0x1328

////////////////////////////////////////////////////////////////////
// MENUS
////////////////////////////////////////////////////////////////////

#define POPUP_TREE_CONTEXT              0x1400
#define MMSE_YES                        0x1401
#define MMSE_NO                         0x1402
#define MMSE_CANCEL                     0x1403
#define MMSE_OK                         0x1404
#define MMSE_YESNO                      0x1405
#define MMSE_YESNOCANCEL                0x1406
#define MMSE_TEXT                       0x1407



////////////////////////////////////////////////////////////////////
// STRINGS
////////////////////////////////////////////////////////////////////

#define IDS_EVENTSNAME                  (ID_BASE_STRINGS + 0x0000)
#define IDS_EVENTSINFO                  (ID_BASE_STRINGS + 0x0001)
#define IDS_AUDIOPROPERTIES             (ID_BASE_STRINGS + 0x0002)
#define IDS_ADVAUDIOTITLE               (ID_BASE_STRINGS + 0x0003)
#define IDS_ABOUT_TITLE                 (ID_BASE_STRINGS + 0x0004)
#define IDS_ABOUT_VERSION               (ID_BASE_STRINGS + 0x0005)
#define IDS_PRIORITY_FROMTO             (ID_BASE_STRINGS + 0x0006)
#define IDS_TXT_DISABLED                (ID_BASE_STRINGS + 0x0007)
#define IDS_CUSTOMIZE                   (ID_BASE_STRINGS + 0x0008)
#define IDS_HIGHQUALITY                 (ID_BASE_STRINGS + 0x0009)
#define IDS_MEDIUMQUALITY               (ID_BASE_STRINGS + 0x000A)
#define IDS_LOWQUALITY                  (ID_BASE_STRINGS + 0x000B)
#define IDS_1QSCREENSIZE                (ID_BASE_STRINGS + 0x000C)
#define IDS_2QSCREENSIZE                (ID_BASE_STRINGS + 0x000D)
#define IDS_3QSCREENSIZE                (ID_BASE_STRINGS + 0x000E)
#define IDS_VIDEOMAXIMIZED              (ID_BASE_STRINGS + 0x000F)
#define IDS_NOAUDIOPLAY                 (ID_BASE_STRINGS + 0x0010)
#define IDS_NOAUDIOREC                  (ID_BASE_STRINGS + 0x0011)
#define IDS_TRAYVOLLNK                  (ID_BASE_STRINGS + 0x0012)
#define IDS_NORMALSIZE                  (ID_BASE_STRINGS + 0x0013)
#define IDS_ZOOMEDSIZE                  (ID_BASE_STRINGS + 0x0014)
#define IDS_NOMIDIPLAY                  (ID_BASE_STRINGS + 0x0015)
#define IDS_REMOVEFAIL                  (ID_BASE_STRINGS + 0x0016)
#define IDS_SYSREMOVE                   (ID_BASE_STRINGS + 0x0017)
#define IDS_REMOVED                     (ID_BASE_STRINGS + 0x0018)
#define IDS_RESTART                     (ID_BASE_STRINGS + 0x0019)
#define IDS_DELETE                      (ID_BASE_STRINGS + 0x001A)
#define IDS_MMDRIVERS                   (ID_BASE_STRINGS + 0x001B)
#define IDS_GENERAL                     (ID_BASE_STRINGS + 0x001C)
#define IDS_NOPROP                      (ID_BASE_STRINGS + 0x001D)
#define IDS_DEVDISABLEDOK               (ID_BASE_STRINGS + 0x001E)
#define IDS_DEVDISABLED                 (ID_BASE_STRINGS + 0x001F)
#define IDS_REMOVEWARN                  (ID_BASE_STRINGS + 0x0020)
#define IDS_REINSTALL                   (ID_BASE_STRINGS + 0x0021)
#define IDS_ADVANCED                    (ID_BASE_STRINGS + 0x0022)
#define IDS_DEVENABLEDOK                (ID_BASE_STRINGS + 0x0023)
#define IDS_DEVENABLEDNOTOK             (ID_BASE_STRINGS + 0x0024)
#define IDS_CHANGESAVED                 (ID_BASE_STRINGS + 0x0025)
#define IDS_COLLAPSE                    (ID_BASE_STRINGS + 0x0026)
#define IDS_DEVFRIENDLYYESNO            (ID_BASE_STRINGS + 0x0027)
#define IDS_FRIENDLYWARNING             (ID_BASE_STRINGS + 0x0028)
#define IDS_FRIENDLYNAME                (ID_BASE_STRINGS + 0x0029)
#define IDS_INSTRFRIENDLYYESNO          (ID_BASE_STRINGS + 0x002A)
#define IDS_NODEVS                      (ID_BASE_STRINGS + 0x002B)
#define IDS_NONE                        (ID_BASE_STRINGS + 0x002C)
#define IDS_ACMREMOVEFAIL               (ID_BASE_STRINGS + 0x002D)
#define IDS_AUDIOFOR                    (ID_BASE_STRINGS + 0x002E)
#define IDS_MIDIFOR                     (ID_BASE_STRINGS + 0x002F)
#define IDS_MIXERFOR                    (ID_BASE_STRINGS + 0x0030)
#define IDS_AUXFOR                      (ID_BASE_STRINGS + 0x0031)
#define IDS_ENABLEAUDIO                 (ID_BASE_STRINGS + 0x0032)
#define IDS_ENABLEMIDI                  (ID_BASE_STRINGS + 0x0033)
#define IDS_ENABLEMIXER                 (ID_BASE_STRINGS + 0x0034)
#define IDS_ENABLEAUX                   (ID_BASE_STRINGS + 0x0035)
#define IDS_ENABLEMCI                   (ID_BASE_STRINGS + 0x0036)
#define IDS_ENABLEACM                   (ID_BASE_STRINGS + 0x0037)
#define IDS_ENABLEICM                   (ID_BASE_STRINGS + 0x0038)
#define IDS_ENABLECAP                   (ID_BASE_STRINGS + 0x0039)
#define IDS_ENABLEJOY                   (ID_BASE_STRINGS + 0x003A)
#define IDS_DISABLEAUDIO                (ID_BASE_STRINGS + 0x003B)
#define IDS_DISABLEMIDI                 (ID_BASE_STRINGS + 0x003C)
#define IDS_DISABLEMIXER                (ID_BASE_STRINGS + 0x003D)
#define IDS_DISABLEAUX                  (ID_BASE_STRINGS + 0x003E)
#define IDS_DISABLEMCI                  (ID_BASE_STRINGS + 0x003F)
#define IDS_DISABLEACM                  (ID_BASE_STRINGS + 0x0040)
#define IDS_DISABLEICM                  (ID_BASE_STRINGS + 0x0041)
#define IDS_DISABLECAP                  (ID_BASE_STRINGS + 0x0042)
#define IDS_DISABLEJOY                  (ID_BASE_STRINGS + 0x0043)
#define IDS_CANTLOADACM                 (ID_BASE_STRINGS + 0x0044)
#define IDS_REMOVEHARDWAREWARN          (ID_BASE_STRINGS + 0x0045)
#define IDS_AUDIO                       (ID_BASE_STRINGS + 0x0046)
#define IDS_MIDI                        (ID_BASE_STRINGS + 0x0047)
#define IDS_MIXER                       (ID_BASE_STRINGS + 0x0048)
#define IDS_AUX                         (ID_BASE_STRINGS + 0x0049)
#define IDS_ACM                         (ID_BASE_STRINGS + 0x004A)
#define IDS_ICM                         (ID_BASE_STRINGS + 0x004B)
#define IDS_CAP                         (ID_BASE_STRINGS + 0x004C)
#define IDS_JOY                         (ID_BASE_STRINGS + 0x004D)
#define IDS_SYSREMOVEINFO               (ID_BASE_STRINGS + 0x004E)
#define IDS_REMOVEPNPWARN               (ID_BASE_STRINGS + 0x004F)
#define IDS_REMOVEMULTIPORTMIDI         (ID_BASE_STRINGS + 0x0050)
#define IDS_DISABLE                     (ID_BASE_STRINGS + 0x0051)
#define IDS_DISABLEMULTIPORTMIDI        (ID_BASE_STRINGS + 0x0052)
#define IDS_ENABLE                      (ID_BASE_STRINGS + 0x0053)
#define IDS_ENABLEMULTIPORTMIDI         (ID_BASE_STRINGS + 0x0054)
#define IDS_DEVENABLEDNODRIVER          (ID_BASE_STRINGS + 0x0055)
#define IDS_PNPPROBLEM                  (ID_BASE_STRINGS + 0x0056)
#define IDS_NOSNDVOL                    (ID_BASE_STRINGS + 0x0057)
#define IDS_RESTART_NOSOUND             (ID_BASE_STRINGS + 0x0058)
#define IDS_SPEECH_NAME                 (ID_BASE_STRINGS + 0x0059)
#define IDS_DESCRIPTION                 (ID_BASE_STRINGS + 0x005A)
#define IDS_CAPTION                     (ID_BASE_STRINGS + 0x005B)
#define IDS_E_INSTALL                   (ID_BASE_STRINGS + 0x005C)
#define IDS_E_ENGINE                    (ID_BASE_STRINGS + 0x005D)
#define IDS_MM_HEADER                   (ID_BASE_STRINGS + 0x005E)
#define IDS_AUX_HEADER                  (ID_BASE_STRINGS + 0x005F)
#define IDS_MIDI_HEADER                 (ID_BASE_STRINGS + 0x0060)
#define IDS_MIXER_HEADER                (ID_BASE_STRINGS + 0x0061)
#define IDS_WAVE_HEADER                 (ID_BASE_STRINGS + 0x0062)
#define IDS_MCI_HEADER                  (ID_BASE_STRINGS + 0x0063)
#define IDS_ACM_HEADER                  (ID_BASE_STRINGS + 0x0064)
#define IDS_ICM_HEADER                  (ID_BASE_STRINGS + 0x0065)
#define IDS_OTHER_HEADER                (ID_BASE_STRINGS + 0x0066)
#define IDS_VIDCAP_HEADER               (ID_BASE_STRINGS + 0x0067)
#define IDS_JOYSTICK_HEADER             (ID_BASE_STRINGS + 0x0068)
#define IDS_AUDIO_TAB                   (ID_BASE_STRINGS + 0x0069)
#define IDS_VIDEO_TAB                   (ID_BASE_STRINGS + 0x006A)
#define IDS_CDAUDIO_TAB                 (ID_BASE_STRINGS + 0x006B)
#define IDS_MIDI_TAB                    (ID_BASE_STRINGS + 0x006C)
#define IDS_VIDEO                       (ID_BASE_STRINGS + 0x006D)
#define IDS_CDMUSIC                     (ID_BASE_STRINGS + 0x006E)
#define IDS_AUDHW1                      (ID_BASE_STRINGS + 0x006F)
#define IDS_AUDHW2                      (ID_BASE_STRINGS + 0x0070)
#define IDS_AUDHW3                      (ID_BASE_STRINGS + 0x0071)
#define IDS_AUDHW4                      (ID_BASE_STRINGS + 0x0072)
#define IDS_SRCQUALITY1                 (ID_BASE_STRINGS + 0x0073)
#define IDS_SRCQUALITY2                 (ID_BASE_STRINGS + 0x0074)
#define IDS_SRCQUALITY3                 (ID_BASE_STRINGS + 0x0075)
#define IDS_SPEAKER1                    (ID_BASE_STRINGS + 0x0076)
#define IDS_SPEAKER2                    (ID_BASE_STRINGS + 0x0077)
#define IDS_SPEAKER3                    (ID_BASE_STRINGS + 0x0078)
#define IDS_SPEAKER4                    (ID_BASE_STRINGS + 0x0079)
#define IDS_SPEAKER5                    (ID_BASE_STRINGS + 0x007A)
#define IDS_SPEAKER6                    (ID_BASE_STRINGS + 0x007B)
#define IDS_SPEAKER7                    (ID_BASE_STRINGS + 0x007C)
#define IDS_SPEAKER8                    (ID_BASE_STRINGS + 0x007D)
#define IDS_SPEAKER9                    (ID_BASE_STRINGS + 0x007E)
#define IDS_SPEAKER10                   (ID_BASE_STRINGS + 0x007F)
#define IDS_SPEAKER11                   (ID_BASE_STRINGS + 0x0080)
#define IDS_ERROR_TITLE                 (ID_BASE_STRINGS + 0x0081)
#define IDS_ERROR_NOSNDVOL              (ID_BASE_STRINGS + 0x0082)
#define IDS_ERROR_NOMIXER               (ID_BASE_STRINGS + 0x0083)
#define IDS_USEANYDEVICE                (ID_BASE_STRINGS + 0x0084)
#define IDS_ERROR_DSPRIVS               (ID_BASE_STRINGS + 0x0085)
#define IDS_ERROR_DSGENERAL             (ID_BASE_STRINGS + 0x0086)
#define IDS_E_MICWIZARD                 (ID_BASE_STRINGS + 0x0087)
#define IDS_HARDWARE                    (ID_BASE_STRINGS + 0x0088)
#define IDS_MMNAME                      (ID_BASE_STRINGS + 0x0089)
#define IDS_MMINFO                      (ID_BASE_STRINGS + 0x008A)
#define IDS_MMHELP                      (ID_BASE_STRINGS + 0x008B)

////////////////////////////////////////////////////////////////////
// FILE PROPERTY STRINGS
////////////////////////////////////////////////////////////////////

#define IDS_FOURCC_IARL                 (ID_BASE_FILEPROP_STRINGS + 0x0000)
#define IDS_FOURCC_IART                 (ID_BASE_FILEPROP_STRINGS + 0x0001)
#define IDS_FOURCC_ICMS                 (ID_BASE_FILEPROP_STRINGS + 0x0002)
#define IDS_FOURCC_ICMT                 (ID_BASE_FILEPROP_STRINGS + 0x0003)
#define IDS_FOURCC_ICOP                 (ID_BASE_FILEPROP_STRINGS + 0x0004)
#define IDS_FOURCC_ICRD                 (ID_BASE_FILEPROP_STRINGS + 0x0005)
#define IDS_FOURCC_ICRP                 (ID_BASE_FILEPROP_STRINGS + 0x0006)
#define IDS_FOURCC_IDIM                 (ID_BASE_FILEPROP_STRINGS + 0x0007)
#define IDS_FOURCC_IDPI                 (ID_BASE_FILEPROP_STRINGS + 0x0008)
#define IDS_FOURCC_IENG                 (ID_BASE_FILEPROP_STRINGS + 0x0009)
#define IDS_FOURCC_IGNR                 (ID_BASE_FILEPROP_STRINGS + 0x000A)
#define IDS_FOURCC_IKEY                 (ID_BASE_FILEPROP_STRINGS + 0x000B)
#define IDS_FOURCC_ILGT                 (ID_BASE_FILEPROP_STRINGS + 0x000C)
#define IDS_FOURCC_IMED                 (ID_BASE_FILEPROP_STRINGS + 0x000D)
#define IDS_FOURCC_INAM                 (ID_BASE_FILEPROP_STRINGS + 0x000E)
#define IDS_FOURCC_IPLT                 (ID_BASE_FILEPROP_STRINGS + 0x000F)
#define IDS_FOURCC_IPRD                 (ID_BASE_FILEPROP_STRINGS + 0x0010)
#define IDS_FOURCC_ISBJ                 (ID_BASE_FILEPROP_STRINGS + 0x0011)
#define IDS_FOURCC_ISFT                 (ID_BASE_FILEPROP_STRINGS + 0x0012)
#define IDS_FOURCC_ISHP                 (ID_BASE_FILEPROP_STRINGS + 0x0013)
#define IDS_FOURCC_ISRC                 (ID_BASE_FILEPROP_STRINGS + 0x0014)
#define IDS_FOURCC_ISRF                 (ID_BASE_FILEPROP_STRINGS + 0x0015)
#define IDS_FOURCC_ITCH                 (ID_BASE_FILEPROP_STRINGS + 0x0016)
#define IDS_FOURCC_DISP                 (ID_BASE_FILEPROP_STRINGS + 0x0017)
#define IDS_NOCOPYRIGHT                 (ID_BASE_FILEPROP_STRINGS + 0x0018)
#define IDS_UNKFORMAT                   (ID_BASE_FILEPROP_STRINGS + 0x0019)
#define IDS_BADFILE                     (ID_BASE_FILEPROP_STRINGS + 0x001A)
#define IDS_MINFMT                      (ID_BASE_FILEPROP_STRINGS + 0x001B)
#define IDS_SECFMT                      (ID_BASE_FILEPROP_STRINGS + 0x001C)
#define IDS_DETAILS                     (ID_BASE_FILEPROP_STRINGS + 0x001D)
#define IDS_PREVIEW                     (ID_BASE_FILEPROP_STRINGS + 0x001E)
#define IDS_PREVIEWOF                   (ID_BASE_FILEPROP_STRINGS + 0x001F)
#define IDS_GOODFORMAT                  (ID_BASE_FILEPROP_STRINGS + 0x0020)
#define IDS_BADFORMAT                   (ID_BASE_FILEPROP_STRINGS + 0x0021)
#define IDS_UNCOMPRESSED                (ID_BASE_FILEPROP_STRINGS + 0x0022)
#define IDS_FORMAT_PCM                  (ID_BASE_FILEPROP_STRINGS + 0x0023)
#define IDS_FORMAT_ADPCM                (ID_BASE_FILEPROP_STRINGS + 0x0024)
#define IDS_FORMAT_IBM_CVSD             (ID_BASE_FILEPROP_STRINGS + 0x0025)
#define IDS_FORMAT_ALAW                 (ID_BASE_FILEPROP_STRINGS + 0x0026)
#define IDS_FORMAT_MULAW                (ID_BASE_FILEPROP_STRINGS + 0x0027)
#define IDS_FORMAT_OKI_ADPCM            (ID_BASE_FILEPROP_STRINGS + 0x0028)
#define IDS_FORMAT_IMA_ADPCM            (ID_BASE_FILEPROP_STRINGS + 0x0029)
#define IDS_FORMAT_MEDIASPACE_ADPCM     (ID_BASE_FILEPROP_STRINGS + 0x002A)
#define IDS_FORMAT_SIERRA_ADPCM         (ID_BASE_FILEPROP_STRINGS + 0x002B)
#define IDS_FORMAT_G723_ADPCM           (ID_BASE_FILEPROP_STRINGS + 0x002C)
#define IDS_FORMAT_DIGISTD              (ID_BASE_FILEPROP_STRINGS + 0x002D)
#define IDS_FORMAT_DIGIFIX              (ID_BASE_FILEPROP_STRINGS + 0x002E)
#define IDS_FORMAT_YAMAHA_ADPCM         (ID_BASE_FILEPROP_STRINGS + 0x002F)
#define IDS_FORMAT_SONARC               (ID_BASE_FILEPROP_STRINGS + 0x0030)
#define IDS_FORMAT_DSPGROUP_TRUESPEECH  (ID_BASE_FILEPROP_STRINGS + 0x0031)
#define IDS_FORMAT_ECHOSC1              (ID_BASE_FILEPROP_STRINGS + 0x0032)
#define IDS_FORMAT_AUDIOFILE_AF36       (ID_BASE_FILEPROP_STRINGS + 0x0033)
#define IDS_FORMAT_APTX                 (ID_BASE_FILEPROP_STRINGS + 0x0034)
#define IDS_FORMAT_AUDIOFILE_AF10       (ID_BASE_FILEPROP_STRINGS + 0x0035)
#define IDS_FORMAT_DOLBY_AC2            (ID_BASE_FILEPROP_STRINGS + 0x0036)
#define IDS_FORMAT_GSM610               (ID_BASE_FILEPROP_STRINGS + 0x0037)
#define IDS_FORMAT_G721_ADPCM           (ID_BASE_FILEPROP_STRINGS + 0x0038)
#define IDS_FORMAT_CREATIVE_ADPCM       (ID_BASE_FILEPROP_STRINGS + 0x0039)


////////////////////////////////////////////////////////////////////
// EVENT STRINGS
////////////////////////////////////////////////////////////////////

#define IDS_BROWSEFORSOUND              (ID_BASE_EVENTS_STRINGS + 0x0000)
#define IDS_UNKNOWN                     (ID_BASE_EVENTS_STRINGS + 0x0001)
#define IDS_REMOVESCHEME                (ID_BASE_EVENTS_STRINGS + 0x0002)
#define IDS_CHANGESCHEME                (ID_BASE_EVENTS_STRINGS + 0x0003)
#define IDS_SOUND                       (ID_BASE_EVENTS_STRINGS + 0x0004)
#define IDS_WINDOWSDEFAULT              (ID_BASE_EVENTS_STRINGS + 0x0005)
#define IDS_ERRORFILEPLAY               (ID_BASE_EVENTS_STRINGS + 0x0006)
#define IDS_ERRORUNKNOWNPLAY            (ID_BASE_EVENTS_STRINGS + 0x0007)
#define IDS_ERRORFORMATPLAY             (ID_BASE_EVENTS_STRINGS + 0x0008)
#define IDS_ERRORPLAY                   (ID_BASE_EVENTS_STRINGS + 0x0009)
#define IDS_NOWAVEDEV                   (ID_BASE_EVENTS_STRINGS + 0x000A)
#define IDS_NODESC                      (ID_BASE_EVENTS_STRINGS + 0x000B)
#define IDS_SAVESCHEME                  (ID_BASE_EVENTS_STRINGS + 0x000C)
#define IDS_CONFIRMREMOVE               (ID_BASE_EVENTS_STRINGS + 0x000D)
#define IDS_NOOVERWRITEDEFAULT          (ID_BASE_EVENTS_STRINGS + 0x000E)
#define IDS_SAVECHANGE                  (ID_BASE_EVENTS_STRINGS + 0x000F)
#define IDS_OVERWRITESCHEME             (ID_BASE_EVENTS_STRINGS + 0x0010)
#define IDS_ERRORDEVBUSY                (ID_BASE_EVENTS_STRINGS + 0x0011)
#define IDS_NONECHOSEN                  (ID_BASE_EVENTS_STRINGS + 0x0012)
#define IDS_DEFAULTAPP                  (ID_BASE_EVENTS_STRINGS + 0x0013)
#define IDS_INVALIDFILE                 (ID_BASE_EVENTS_STRINGS + 0x0014)
#define IDS_NULLCHAR                    (ID_BASE_EVENTS_STRINGS + 0x0015)
#define IDS_WAVFILES                    (ID_BASE_EVENTS_STRINGS + 0x0016)
#define IDS_NOSNDFILE                   (ID_BASE_EVENTS_STRINGS + 0x0017)
#define IDS_NOSNDFILETITLE              (ID_BASE_EVENTS_STRINGS + 0x0018)
#define IDS_OK                          (ID_BASE_EVENTS_STRINGS + 0x0019)
#define IDS_INVALIDFILEQUERY            (ID_BASE_EVENTS_STRINGS + 0x001A)
#define IDS_ISNOTSNDFILE                (ID_BASE_EVENTS_STRINGS + 0x001B)
#define IDS_SCHEMENOTSAVED              (ID_BASE_EVENTS_STRINGS + 0x001C)
#define IDS_PREVSCHEME                  (ID_BASE_EVENTS_STRINGS + 0x001D)

////////////////////////////////////////////////////////////////////
// MIDI STRINGS
////////////////////////////////////////////////////////////////////

#define IDS_MMPROP                      (ID_BASE_MIDI_STRINGS + 0x0000)
#define IDS_NOCHAN                      (ID_BASE_MIDI_STRINGS + 0x0001)
#define IDS_CHANPLURAL                  (ID_BASE_MIDI_STRINGS + 0x0002)
#define IDS_CHANSINGULAR                (ID_BASE_MIDI_STRINGS + 0x0003)
#define IDS_MIDIDETAILS                 (ID_BASE_MIDI_STRINGS + 0x0004)
#define IDS_MIDI_DEV_AND_INST           (ID_BASE_MIDI_STRINGS + 0x0005)
#define IDS_UNSPECIFIED                 (ID_BASE_MIDI_STRINGS + 0x0006)
#define IDS_DEFAULT_SCHEME_NAME         (ID_BASE_MIDI_STRINGS + 0x0007)
#define IDS_RUNONCEWIZLABEL             (ID_BASE_MIDI_STRINGS + 0x0008)
#define IDS_DEF_DEFINITION              (ID_BASE_MIDI_STRINGS + 0x0009)
#define IDS_DEF_INSTRNAME               (ID_BASE_MIDI_STRINGS + 0x000A)
#define IDS_QUERY_DELETESCHEME          (ID_BASE_MIDI_STRINGS + 0x000B)
#define IDS_DEF_CAPTION                 (ID_BASE_MIDI_STRINGS + 0x000C)
#define IDS_QUERY_OVERSCHEME            (ID_BASE_MIDI_STRINGS + 0x000D)
#define IDS_WIZNAME                     (ID_BASE_MIDI_STRINGS + 0x000E)
#define IDS_IDFFILES                    (ID_BASE_MIDI_STRINGS + 0x000F)
#define IDS_IDF_CAPTION                 (ID_BASE_MIDI_STRINGS + 0x0010)
#define IDS_QUERY_OVERIDF               (ID_BASE_MIDI_STRINGS + 0x0011)
#define IDS_MAPPER_BUSY                 (ID_BASE_MIDI_STRINGS + 0x0012)

////////////////////////////////////////////////////////////////////
// SOME CONTROL ID's
////////////////////////////////////////////////////////////////////

#define IDC_CD_TB_VOLUME                (ID_SIMPLE_PROP + 0x0000)
#define IDC_CD_CB_SELECT                (ID_SIMPLE_PROP + 0x0001)
#define IDC_VIDEO_INWINDOW              (ID_SIMPLE_PROP + 0x0002)
#define IDC_VIDEO_FULLSCREEN            (ID_SIMPLE_PROP + 0x0003)
#define IDC_VIDEO_CB_SIZE               (ID_SIMPLE_PROP + 0x0004)
#define IDC_SCREENSAMPLE                (ID_SIMPLE_PROP + 0x0005)
#define IDC_AUDIO_CB_PLAY               (ID_SIMPLE_PROP + 0x0006)
#define IDC_AUDIO_CB_REC                (ID_SIMPLE_PROP + 0x0007)
#define IDC_MUSIC_CB_PLAY               (ID_SIMPLE_PROP + 0x0008)
#define IDC_AUDIO_PREF                  (ID_SIMPLE_PROP + 0x0009)
#define IDC_TASKBAR_VOLUME              (ID_SIMPLE_PROP + 0x000A)
#define IDC_LAUNCH_SNDVOL               (ID_SIMPLE_PROP + 0x000B)
#define IDC_LAUNCH_RECVOL               (ID_SIMPLE_PROP + 0x000C)
#define IDC_LAUNCH_MUSICVOL             (ID_SIMPLE_PROP + 0x000D)
#define IDC_PLAYBACK_ADVSETUP           (ID_SIMPLE_PROP + 0x000E)
#define IDC_MASTERVOLUME                (ID_SIMPLE_PROP + 0x000F)
#define IDC_RECORD_ADVSETUP             (ID_SIMPLE_PROP + 0x0010)
#define IDC_MUSIC_ABOUT                 (ID_SIMPLE_PROP + 0x0011)
#define IDC_VOLUME_LOW                  (ID_SIMPLE_PROP + 0x0012)
#define IDC_VOLUME_HIGH                 (ID_SIMPLE_PROP + 0x0013)
#define ID_APPLY                        (ID_SIMPLE_PROP + 0x0014)
#define ID_INIT                         (ID_SIMPLE_PROP + 0x0015)
#define ID_REMOVE                       (ID_SIMPLE_PROP + 0x0016)
#define ID_REBUILD                      (ID_SIMPLE_PROP + 0x0017)
#define IDC_CDEN_DIGAUDIO               (ID_SIMPLE_PROP + 0x0018)
#define IDC_SOUND_FILES                 (ID_SIMPLE_PROP + 0x0019)
#define IDC_EVENT_TREE                  (ID_SIMPLE_PROP + 0x001A)
#define IDD_INPUT                       (ID_SIMPLE_PROP + 0x001B)
#define IDC_TTS_ABOUT                   (ID_SIMPLE_PROP + 0x001C)
#define IDC_SR_MIC                      (ID_SIMPLE_PROP + 0x001D)
#define IDC_SRLIST                      (ID_SIMPLE_PROP + 0x001E)
#define IDC_SR_GENERAL                  (ID_SIMPLE_PROP + 0x001F)
#define IDC_SR_ABOUT                    (ID_SIMPLE_PROP + 0x0020)
#define IDC_SR_TRAIN                    (ID_SIMPLE_PROP + 0x0021)
#define IDC_SR_PRONUNCIATION            (ID_SIMPLE_PROP + 0x0022)
#define IDC_TTSLIST                     (ID_SIMPLE_PROP + 0x0023)
#define IDC_TTS_TRANSLATE               (ID_SIMPLE_PROP + 0x0024)
#define IDC_TTS_GENERAL                 (ID_SIMPLE_PROP + 0x0025)
#define IDC_TTS_PRONUNCIATION           (ID_SIMPLE_PROP + 0x0026)
#define IDD_OUTPUT                      (ID_SIMPLE_PROP + 0x0027)

////////////////////////////////////////////////////////////////////
// ADVANCED PROPERTY CONTROLS
////////////////////////////////////////////////////////////////////

#define IDD_CPL_BTN_ABOUT               (ID_ADVANCED_PROP + 0x0000)
#define IDD_ABOUT_TXT_DESCRIPTION       (ID_ADVANCED_PROP + 0x0001)
#define IDD_ABOUT_TXT_VERSION           (ID_ADVANCED_PROP + 0x0002)
#define IDD_ABOUT_TXT_COPYRIGHT         (ID_ADVANCED_PROP + 0x0003)
#define IDD_ABOUT_TXT_LICENSING         (ID_ADVANCED_PROP + 0x0004)
#define IDD_ABOUT_TXT_FEATURES          (ID_ADVANCED_PROP + 0x0005)
#define IDD_PRIORITY_TXT_FROMTO         (ID_ADVANCED_PROP + 0x0006)
#define IDD_PRIORITY_COMBO_PRIORITY     (ID_ADVANCED_PROP + 0x0007)
#define IDD_ABOUT_ICON_DRIVER           (ID_ADVANCED_PROP + 0x0008)
#define IDC_DONOTMAP                    (ID_ADVANCED_PROP + 0x0009)
#define ID_TOGGLE                       (ID_ADVANCED_PROP + 0x000A)
#define ID_ADV_REMOVE                   (ID_ADVANCED_PROP + 0x000B)
#define IDC_ENABLE                      (ID_ADVANCED_PROP + 0x000C)
#define IDC_DISABLE                     (ID_ADVANCED_PROP + 0x000D)
#define IDC_DEV_ICON                    (ID_ADVANCED_PROP + 0x000E)
#define IDC_DEV_DESC                    (ID_ADVANCED_PROP + 0x000F)
#define IDC_DEV_STATUS                  (ID_ADVANCED_PROP + 0x0010)
#define ID_DEV_SETTINGS                 (ID_ADVANCED_PROP + 0x0011)
#define ID_WHATSTHIS                    (ID_ADVANCED_PROP + 0x0012)
#define IDC_ADV_TREE                    (ID_ADVANCED_PROP + 0x0013)
#define IDC_DEVICECLASS                 (ID_ADVANCED_PROP + 0x0014)



////////////////////////////////////////////////////////////////////
// CONTROL IDS
////////////////////////////////////////////////////////////////////

#define IDC_IMAGEFRAME                  (ID_BASE_CONTROL + 0x0000)
#define IDD_DISPFRAME                   (ID_BASE_CONTROL + 0x0001)
#define IDD_FILENAME                    (ID_BASE_CONTROL + 0x0002)
#define IDD_CRLABEL                     (ID_BASE_CONTROL + 0x0003)
#define IDD_COPYRIGHT                   (ID_BASE_CONTROL + 0x0004)
#define IDD_LENLABEL                    (ID_BASE_CONTROL + 0x0005)
#define IDD_FILELEN                     (ID_BASE_CONTROL + 0x0006)
#define IDD_AUDIOFORMAT                 (ID_BASE_CONTROL + 0x0007)
#define IDD_AUDIOFORMATLABEL            (ID_BASE_CONTROL + 0x0008)
#define IDD_VIDEOFORMAT                 (ID_BASE_CONTROL + 0x0009)
#define IDD_VIDEOFORMATLABEL            (ID_BASE_CONTROL + 0x000A)
#define IDD_INFO_NAME                   (ID_BASE_CONTROL + 0x000B)
#define IDD_INFO_VALUE                  (ID_BASE_CONTROL + 0x000C)
#define IDD_DISP_ICON                   (ID_BASE_CONTROL + 0x000D)
#define IDC_DETAILSINFO_GRP             (ID_BASE_CONTROL + 0x000E)
#define IDC_ITEMSLABEL                  (ID_BASE_CONTROL + 0x000F)
#define IDC_DESCLABEL                   (ID_BASE_CONTROL + 0x0010)
#define IDD_MIDISEQUENCELABEL           (ID_BASE_CONTROL + 0x0011)
#define IDD_MIDISEQUENCENAME            (ID_BASE_CONTROL + 0x0012)
#define IDC_ACCELERATION                (ID_BASE_CONTROL + 0x0013)
#define IDC_SRCQUALITY                  (ID_BASE_CONTROL + 0x0014)
#define IDC_PREFERREDONLY               (ID_BASE_CONTROL + 0x0015)
#define IDC_HWMESSAGE                   (ID_BASE_CONTROL + 0x0016)
#define IDC_SRCMSG                      (ID_BASE_CONTROL + 0x0017)
#define IDC_DEFAULTS                    (ID_BASE_CONTROL + 0x0018)
#define IDC_SPEAKERCONFIG               (ID_BASE_CONTROL + 0x0019)
#define IDAPPLY                         (ID_BASE_CONTROL + 0x0020)


////////////////////////////////////////////////////////////////////
// MIDI CONTROL IDS
////////////////////////////////////////////////////////////////////

#define IDC_INSTRUMENTS                 (ID_MIDI_PROP + 0x0000)
#define IDC_SCHEMES                     (ID_MIDI_PROP + 0x0001)
#define IDL_CHANNELS                    (ID_MIDI_PROP + 0x0002)
#define IDE_SHOW_CHANNELS               (ID_MIDI_PROP + 0x0003)
#define IDE_SCHEMENAME                  (ID_MIDI_PROP + 0x0004)
#define IDB_DETAILS                     (ID_MIDI_PROP + 0x0005)
#define IDC_RADIO_SINGLE                (ID_MIDI_PROP + 0x0006)
#define IDC_RADIO_CUSTOM                (ID_MIDI_PROP + 0x0007)
#define IDB_CONFIGURE                   (ID_MIDI_PROP + 0x0008)
#define IDB_DELETE                      (ID_MIDI_PROP + 0x0009)
#define IDB_SAVE_AS                     (ID_MIDI_PROP + 0x000A)
#define IDB_CHANGE                      (ID_MIDI_PROP + 0x000B)
#define IDB_REMOVE                      (ID_MIDI_PROP + 0x000C)
#define IDE_ALIAS                       (ID_MIDI_PROP + 0x000D)
#define IDC_TYPES                       (ID_MIDI_PROP + 0x000E)
#define IDC_DEVICES                     (ID_MIDI_PROP + 0x000F)
#define IDL_INSTRUMENTS                 (ID_MIDI_PROP + 0x0010)
#define IDC_CLASS_ICON                  (ID_MIDI_PROP + 0x0011)
#define IDC_INSTRUMENT_LABEL            (ID_MIDI_PROP + 0x0012)
#define IDC_CLASS_LABEL                 (ID_MIDI_PROP + 0x0013)
#define IDC_DEVICE_TYPE                 (ID_MIDI_PROP + 0x0014)
#define IDC_MANUFACTURER                (ID_MIDI_PROP + 0x0015)
#define IDB_NEWTYPE                     (ID_MIDI_PROP + 0x0016)
#define IDB_ADDWIZ                      (ID_MIDI_PROP + 0x0017)
#define IDC_WIZBMP                      (ID_MIDI_PROP + 0x0018)
#define IDC_GROUPBOX                    (ID_MIDI_PROP + 0x0019)
#define IDC_SCHEMESLABEL                (ID_MIDI_PROP + 0x001A)
#define IDC_TEXT_1                      (ID_MIDI_PROP + 0x001B)
#define IDC_TEXT_2                      (ID_MIDI_PROP + 0x001C)
#define IDC_TEXT_3                      (ID_MIDI_PROP + 0x001D)
#define IDC_TEXT_4                      (ID_MIDI_PROP + 0x001E)
#define IDC_TEXT_5                      (ID_MIDI_PROP + 0x001F)
#define IDC_TEXT_6                      (ID_MIDI_PROP + 0x0020)
#define IDC_TEXT_7                      (ID_MIDI_PROP + 0x0021)
#define IDC_TEXT_8                      (ID_MIDI_PROP + 0x0022)
#define IDC_TEXT_9                      (ID_MIDI_PROP + 0x0023)
#define IDC_GROUPBOX_2                  (ID_MIDI_PROP + 0x0024)
#define IDC_ICON_1                      (ID_MIDI_PROP + 0x0025)
#define IDC_ICON_2                      (ID_MIDI_PROP + 0x0026)
#define IDE_TYPES                       (ID_MIDI_PROP + 0x0027)
#define IDC_TEXT_10                     (ID_MIDI_PROP + 0x0028)
#define IDC_TEXT_11                     (ID_MIDI_PROP + 0x0029)
#define IDC_TEXT_12                     (ID_MIDI_PROP + 0x002A)
#define IDC_TEXT_13                     (ID_MIDI_PROP + 0x002B)
#define IDC_TEXT_14                     (ID_MIDI_PROP + 0x002C)
#define IDC_TEXT_15                     (ID_MIDI_PROP + 0x002E)
#define IDC_TEXT_16                     (ID_MIDI_PROP + 0x002F)
#define IDC_TEXT_17                     (ID_MIDI_PROP + 0x0030)
#define IDC_TEXT_18                     (ID_MIDI_PROP + 0x0031)
#define IDC_TEXT_19                     (ID_MIDI_PROP + 0x0032)
#define IDC_TEXT_20                     (ID_MIDI_PROP + 0x0033)
#define IDC_TEXT_21                     (ID_MIDI_PROP + 0x0034)
#define IDC_TEXT_22                     (ID_MIDI_PROP + 0x0035)
#define IDC_TEXT_23                     (ID_MIDI_PROP + 0x0036)
#define IDC_TEXT_24                     (ID_MIDI_PROP + 0x0037)
#define IDC_TEXT_25                     (ID_MIDI_PROP + 0x0038)
#define IDC_TEXT_26                     (ID_MIDI_PROP + 0x0039)
#define IDC_TEXT_27                     (ID_MIDI_PROP + 0x003A)
#define IDC_TEXT_28                     (ID_MIDI_PROP + 0x003B)
#define IDC_TEXT_29                     (ID_MIDI_PROP + 0x003C)
#define IDC_TEXT_30                     (ID_MIDI_PROP + 0x003D)
#define IDC_TEXT_31                     (ID_MIDI_PROP + 0x003E)
#define IDC_TEXT_32                     (ID_MIDI_PROP + 0x003F)
#define IDC_ICON_3                      (ID_MIDI_PROP + 0x0040)
#define IDC_ICON_4                      (ID_MIDI_PROP + 0x0041)
#define IDC_ICON_5                      (ID_MIDI_PROP + 0x0042)
#define IDC_ICON_6                      (ID_MIDI_PROP + 0x0043)
#define IDC_ICON_7                      (ID_MIDI_PROP + 0x0044)
#define IDC_ICON_8                      (ID_MIDI_PROP + 0x0045)
#define IDC_ABOUTSYNTH                  (ID_MIDI_PROP + 0x0046)
#define IDC_LOGO_FRAME                  (ID_MIDI_PROP + 0x0047)
#define IDC_RENDERER                    (ID_MIDI_PROP + 0x0048)
#define IDC_GROUPBOX_3                  (ID_MIDI_PROP + 0x0049)


////////////////////////////////////////////////////////////////////
// EVENTS IDS
////////////////////////////////////////////////////////////////////

#define ID_SCHEMENAME                   (ID_EVENTS_START + 0x0000)
#define LB_INSTALLED                    (ID_EVENTS_START + 0x0001)
#define ID_PLAY                         (ID_EVENTS_START + 0x0002)
#define ID_DISPFRAME                    (ID_EVENTS_START + 0x0003)
#define ID_REMOVE_SCHEME                (ID_EVENTS_START + 0x0004)
#define ID_SAVE_SCHEME                  (ID_EVENTS_START + 0x0005)
#define CB_SCHEMES                      (ID_EVENTS_START + 0x0006)
#define ID_INSTALL                      (ID_EVENTS_START + 0x0007)
#define ID_CONFIGURE                    (ID_EVENTS_START + 0x0008)
#define ID_STOP                         (ID_EVENTS_START + 0x0009)
#define ID_DETAILS                      (ID_EVENTS_START + 0x000A)
#define ID_BROWSE                       (ID_EVENTS_START + 0x000B)
#define IDC_STATIC_PREVIEW              (ID_EVENTS_START + 0x000C)
#define IDC_STATIC_NAME                 (ID_EVENTS_START + 0x000D)
#define IDC_SOUNDGRP                    (ID_EVENTS_START + 0x000E)
#define ID_DESC                         (ID_EVENTS_START + 0x000F)
#define ID_SAVEAS_HELP                  (ID_EVENTS_START + 0x0010)
#define ID_MUTE                         (ID_EVENTS_START + 0x0011)


////////////////////////////////////////////////////////////////////
// mmioFOURCC
////////////////////////////////////////////////////////////////////

#define FOURCC_INFO mmioFOURCC('I','N','F','O')
#define FOURCC_DISP mmioFOURCC('D','I','S','P')
#define FOURCC_IARL mmioFOURCC('I','A','R','L')
#define FOURCC_IART mmioFOURCC('I','A','R','T')
#define FOURCC_ICMS mmioFOURCC('I','C','M','S')
#define FOURCC_ICMT mmioFOURCC('I','C','M','T')
#define FOURCC_ICOP mmioFOURCC('I','C','O','P')
#define FOURCC_ICRD mmioFOURCC('I','C','R','D')
#define FOURCC_ICRP mmioFOURCC('I','C','R','P')
#define FOURCC_IDIM mmioFOURCC('I','D','I','M')
#define FOURCC_IDPI mmioFOURCC('I','D','P','I')
#define FOURCC_IENG mmioFOURCC('I','E','N','G')
#define FOURCC_IGNR mmioFOURCC('I','G','N','R')
#define FOURCC_IKEY mmioFOURCC('I','K','E','Y')
#define FOURCC_ILGT mmioFOURCC('I','L','G','T')
#define FOURCC_IMED mmioFOURCC('I','M','E','D')
#define FOURCC_INAM mmioFOURCC('I','N','A','M')
#define FOURCC_IPLT mmioFOURCC('I','P','L','T')
#define FOURCC_IPRD mmioFOURCC('I','P','R','D')
#define FOURCC_ISBJ mmioFOURCC('I','S','B','J')
#define FOURCC_ISFT mmioFOURCC('I','S','F','T')
#define FOURCC_ISHP mmioFOURCC('I','S','H','P')
#define FOURCC_ISRC mmioFOURCC('I','S','R','C')
#define FOURCC_ISRF mmioFOURCC('I','S','R','F')
#define FOURCC_ITCH mmioFOURCC('I','T','C','H')
#define FOURCC_VIDC mmioFOURCC('V','I','D','C')

#define mmioWAVE    mmioFOURCC('W','A','V','E')
#define mmioFMT     mmioFOURCC('f','m','t',' ')
#define mmioDATA    mmioFOURCC('d','a','t','a')

