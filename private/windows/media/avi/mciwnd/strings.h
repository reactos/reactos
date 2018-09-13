/****************************************************************************/
/*		            Please internationalize me			    */
/****************************************************************************/

// Make sure these don't conflict with other MSVIDEO numbers!
#define MCIWND_BASE	333

#define IDS_MCIFILES 	(MCIWND_BASE + 0)
#define IDS_ALLFILES 	(MCIWND_BASE + 1)
#define IDS_MCIERROR 	(MCIWND_BASE + 2)
#define IDS_NODEVICE 	(MCIWND_BASE + 3)
#define IDS_HALFSIZE 	(MCIWND_BASE + 4)
#define IDS_NORMALSIZE  (MCIWND_BASE + 5)
#define IDS_DOUBLESIZE 	(MCIWND_BASE + 6)
#define IDS_PLAY 	(MCIWND_BASE + 7)
#define IDS_STOP 	(MCIWND_BASE + 8)
#define IDS_RECORD 	(MCIWND_BASE + 9)
#define IDS_EJECT 	(MCIWND_BASE + 10)
#define IDS_CLOSE 	(MCIWND_BASE + 11)
#define IDS_NEW 	(MCIWND_BASE + 12)
#define IDS_VIEW 	(MCIWND_BASE + 13)
#define IDS_VOLUME 	(MCIWND_BASE + 14)
#define IDS_SPEED 	(MCIWND_BASE + 15)
#define IDS_OPEN 	(MCIWND_BASE + 16)
#define IDS_SAVE 	(MCIWND_BASE + 17)
#define IDS_CONFIGURE 	(MCIWND_BASE + 18)
#define IDS_COMMAND 	(MCIWND_BASE + 19)
#define IDS_COPY 	(MCIWND_BASE + 20)

#ifndef CHICAGO
#define IDS_TT_PLAY     0x806       // same as MCI_PLAY
#define IDS_TT_STOP     0x808       // same as MCI_STOP
#define IDS_TT_RECORD   0x80F       // same as MCI_RECORD
#define IDS_TT_EJECT    108         // same as IDM_MCIEJECT
#define IDS_TT_MENU     107         // same as IDM_MENU
#endif


/****************************************************************************/
/*		       Please don't internationalize me			    */
/****************************************************************************/

#define DLG_MCICOMMAND  942
#define IDC_MCICOMMAND  10
#define IDC_RESULT      11
#define MPLAYERICON	943
#define IDBMP_TOOLBAR   959

SZCODE	szNULL[] = TEXT("");
SZCODEA szNULLA[] = "";
SZCODE  szMCIExtensions[] = TEXT("MCI Extensions");
SZCODE  szPutDest[] = TEXT("put destination at %d %d %d %d");
SZCODE  szPutSource[] = TEXT("put source at %d %d %d %d");
SZCODE  szSetFormatTMSF[] = TEXT("set time format tmsf");
SZCODE  szSetFormatMS[] = TEXT("set time format ms");
SZCODE  szSetFormatFrames[] = TEXT("set time format frames");
SZCODE  szSetFormat[] = TEXT("set time format %s");
SZCODE  szStatusFormat[] = TEXT("status time format");
SZCODEA  szStatusFormatA[] = "status time format";
SZCODE  szStatusNumTracks[] = TEXT("status number of tracks");
SZCODE  szStatusPosTrack[] = TEXT("status position track %d");
SZCODE  szStatusVolume[] = TEXT("status volume");
SZCODE  szStatusSpeed[] = TEXT("status speed");
SZCODE  szSetSpeed[] = TEXT("set speed %d");
SZCODE  szSetVolume[] = TEXT("setaudio volume to %d");
SZCODE  szSysInfo[] = TEXT("sysinfo installname");
SZCODEA  szSysInfoA[] = "sysinfo installname";
SZCODE  szSetPalette[] = TEXT("setvideo palette handle to %d");
SZCODE  szMDIClient[] = TEXT("MDIClient");
SZCODE  szSave[] = TEXT("save \"%s\"");
SZCODE  szNew[] = TEXT("open new type %s alias %d wait");
SZCODE  szOpenShareable[] = TEXT("open \"%s\" alias %d wait shareable");
SZCODE  szOpen[] = TEXT("open \"%s\" alias %d wait");
SZCODE  szOpenAVI[] = TEXT("open \"%s\" alias %d wait type AVIVideo");
SZCODE  szWindowHandle[] = TEXT("window handle %u");
SZCODE  szStatusPalette[] = TEXT("status palette handle");
SZCODE  szConfigureTest[] = TEXT("configure test");
SZCODE  szConfigure[] = TEXT("configure");
SZCODE  szSetSpeed1000Test[] = TEXT("set speed 1000 test");
SZCODE  szSetSpeed500Test[] = TEXT("set speed 500 test");
SZCODE  szSetSpeedTest[] = TEXT("set speed %d test");
SZCODE  szSetVolume0Test[] = TEXT("setaudio volume to 0 test");
SZCODE  szSetVolumeTest[] = TEXT("setaudio volume to %d test");
SZCODE  szStatusMode[] = TEXT("status mode");
SZCODEA  szStatusModeA[] = "status mode";
SZCODE  szPlay[] = TEXT("play %s");
SZCODE  szPlayReverse[] = TEXT("play reverse %s");
SZCODE  szPlayFrom[] = TEXT("play from %ld");
SZCODE  szPlayTo[] = TEXT("play to %ld");
SZCODE  szRepeat[] = TEXT("repeat");
SZCODE  szSetDoorOpen[] = TEXT("set door open");
SZCODE  szStep[] = TEXT("step by %ld");
SZCODE  szSeek[] = TEXT("seek to %ld");
SZCODE  szClose[] = TEXT("close");
SZCODE  szStatusPosition[] = TEXT("status position");
SZCODEA  szStatusPositionA[] = "status position";
SZCODE  szStatusStart[] = TEXT("status start position");
SZCODE  szPlayFullscreenReverse[] = TEXT("play fullscreen reverse %s");
SZCODE  szPlayFullscreen[] = TEXT("play fullscreen %s");
SZCODE  szSmallFonts[] = TEXT("small fonts");
SZCODE  szStatusForward[] = TEXT("status forward");
SZCODE	szInterface[] = TEXT("AVIVideo!@%ld");

static SZCODEA  szDebug[] = "debug";
