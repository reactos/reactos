/****************************************************************************

Task State : MCIAVI has a separate background task for every open
instance of mciavi. The task handle and task state are stored in
the per-instance data structure.  The task can be in one of four
states.

   TASKABORT : Set by the AVI task when it fails to open the requested
   file during initialisation.


N.B.  TASKINIT is no longer used
   TASKINIT : This is the initial task state set when the
   instance data structure is initialized in mwOpenDevice()
   before the actual task is created by mmTaskCreate().
   After the task is created, mwOpenDevice() waits until
   the task state changes to TASKIDLE before returning
   success so that the background task is definitely initialized
   after an open call.

   TASKIDLE : The task sets the state to TASKIDLE and blocks whenever
   there is nothing to do. When the task wakes, the state is either
   TASKCLOSE if the instance is being closed or else TASKBUSY
   if  the task is to begin  recording or playback of the file.

   TASKCLOSE : mwCloseDevice() stops playback or recording which forces
   the task state to TASKIDLE and then sets the state to TASKCLOSE and
   wakes the task so that the task will destroy itself.

   TASKSTARTING: The task is in this state when it is initializing for
   playback, but hasn't started yet.  This is used so that the calling
   task can wait for play to start before returning "no error" as the
   result of an MCI play command.

   TASKCUEING: The task is in this state when it is reading extra
   records and buffering up audio before actually starting to play.

   TASKPAUSED: The task is in this state while it is paused.
   TASKPLAYING: The task is in this state during playback

****************************************************************************/

#define TASKABORT               0
//#define TASKBEINGCREATED	1
//#define TASKINIT		2
#define TASKIDLE		3
#define TASKSTARTING		4
#define TASKCUEING		5
#define TASKPLAYING		6
#define TASKPAUSED		7
#define TASKCLOSE		8
//				9
//				10
//#define TASKREADINDEX		11
//#define TASKRELOAD		12



// inter-thread requests - from user to worker
#define AVI_CLOSE		1
#define AVI_PLAY		2
#define AVI_STOP		3
#define AVI_REALIZE		4
#define AVI_UPDATE		5
#define AVI_PAUSE		6
#define AVI_CUE			7
#define AVI_SEEK		8
#define AVI_WINDOW		9
#define AVI_SETSPEED		10
#define AVI_MUTE		11
#define AVI_SETVOLUME		12
#define AVI_AUDIOSTREAM		13
#define AVI_VIDEOSTREAM		14
#define AVI_PUT			15
#define AVI_PALETTE		16
#define AVI_RESUME		17
#define AVI_GETVOLUME		18
#define AVI_WAVESTEAL		19
#define AVI_WAVERETURN		20
#define AVI_PALETTECOLOR	21


/* A function back in device.c */
void NEAR PASCAL ShowStage(NPMCIGRAPHIC npMCI);

//
//  call this to RTL to AVIFile.
//
BOOL FAR InitAVIFile(NPMCIGRAPHIC npMCI);
BOOL FAR FreeAVIFile(NPMCIGRAPHIC npMCI);

/* Functions in avitask.c */
void FAR PASCAL _LOADDS mciaviTask(DWORD_PTR dwInst);
void FAR PASCAL mciaviTaskCleanup(NPMCIGRAPHIC npMCI);

/* Functions in aviplay.c */
UINT NEAR PASCAL mciaviPlayFile(NPMCIGRAPHIC npMCI, BOOL bSetEvent);

/* Functions in avidraw.c */
/* !!! Should this be externally visible? */
BOOL NEAR PASCAL DoStreamUpdate(NPMCIGRAPHIC npMCI, BOOL fPaint);
void NEAR PASCAL StreamInvalidate(NPMCIGRAPHIC npMCI, LPRECT prc);

UINT NEAR PASCAL PrepareDC(NPMCIGRAPHIC npMCI);
void NEAR PASCAL UnprepareDC(NPMCIGRAPHIC npMCI);

BOOL FAR PASCAL DrawBegin(NPMCIGRAPHIC npMCI, BOOL FAR *fRestart);
void NEAR PASCAL DrawEnd(NPMCIGRAPHIC npMCI);

BOOL NEAR PASCAL DisplayVideoFrame(NPMCIGRAPHIC npMCI, BOOL fHurryUp);
BOOL NEAR PASCAL ProcessPaletteChange(NPMCIGRAPHIC npMCI, DWORD cksize);


/* Functions in avisound.c */
BOOL NEAR PASCAL PlayRecordAudio(NPMCIGRAPHIC npMCI, BOOL FAR *pfHurryUp,
				    BOOL FAR *pfPlayedAudio);
BOOL NEAR PASCAL KeepPlayingAudio(NPMCIGRAPHIC npMCI);
BOOL NEAR PASCAL HandleAudioChunk(NPMCIGRAPHIC npMCI);

DWORD FAR PASCAL SetUpAudio(NPMCIGRAPHIC npMCI, BOOL fPlaying);
DWORD FAR PASCAL CleanUpAudio(NPMCIGRAPHIC npMCI);
void  FAR PASCAL BuildVolumeTable(NPMCIGRAPHIC npMCI);
BOOL  FAR PASCAL StealWaveDevice(NPMCIGRAPHIC npMCI);
BOOL  FAR PASCAL GiveWaveDevice(NPMCIGRAPHIC npMCI);

/* Functions in aviopen.c */
BOOL FAR PASCAL mciaviCloseFile(NPMCIGRAPHIC npMCI);

// now called on app thread
BOOL FAR PASCAL mciaviOpenFile(NPMCIGRAPHIC npMCI);
// called on worker thread to complete
BOOL NEAR PASCAL OpenFileInit(NPMCIGRAPHIC npMCI);


/* Messages used to control switching of audio between (and within)
 * applications.  These messages are POSTED, hence audio switching will
 * be asynchronous.  The timing depends on various factors: machine load,
 * video speed, etc..  We should probably do this via RegisterWindowMessage
 */
#define WM_AUDIO_ON  WM_USER+100
#define WM_AUDIO_OFF WM_USER+101

// messages sent to winproc thread - set up using RegisterWindowMessage
// in drvproc.c
#define AVIM_DESTROY		(WM_USER+103)
#define AVIM_SHOWSTAGE		(WM_USER+104)

//#define AVIM_DESTROY		(mAVIM_DESTROY)
//#define AVIM_SHOWSTAGE	(mAVIM_SHOWSTAGE)
//extern UINT mAVIM_DESTROY;
//extern UINT mAVIM_SHOWSTAGE;


// in hmemcpy.asm
#ifndef _WIN32
LPVOID FAR PASCAL MemCopy(LPVOID dest, LPVOID source, LONG count);
#else
#define MemCopy memmove
#endif // WIN16

#define GET_BYTE()		(*((BYTE _huge *) (npMCI->lp))++)
#ifdef _WIN32
#define GET_WORD()		(*((UNALIGNED WORD _huge *) (npMCI->lp))++)
#define GET_DWORD()		(*((UNALIGNED DWORD _huge *) (npMCI->lp))++)
#define PEEK_DWORD()		(*((UNALIGNED DWORD _huge *) (npMCI->lp)))
#else
#define GET_WORD()		(*((WORD _huge *) (npMCI->lp))++)
#define GET_DWORD()		(*((DWORD _huge *) (npMCI->lp))++)
#define PEEK_DWORD()		(*((DWORD _huge *) (npMCI->lp)))
#endif
#define SKIP_BYTES(nBytes)	((npMCI->lp) += (nBytes))

#define Now()		(timeGetTime())

void NEAR PASCAL aviTaskCheckRequests(NPMCIGRAPHIC npMCI);

BOOL FAR PASCAL ReadIndex(NPMCIGRAPHIC npMCI);

LONG NEAR PASCAL FindPrevKeyFrame(NPMCIGRAPHIC npMCI, STREAMINFO *psi, LONG lFrame);
LONG NEAR PASCAL FindNextKeyFrame(NPMCIGRAPHIC npMCI, STREAMINFO *psi, LONG lFrame);

//
// try to set the dest or source rect without stopping play.
// called both at stop time and at play time
//
// returns TRUE if stop needed, or else FALSE if all handled.
// lpdwErr is set to a non-zero error if any error occured (in which case
// FALSE will be returned.
//
BOOL TryPutRect(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPRECT lprc, LPDWORD lpdwErr);

// called on worker thread only
DWORD InternalSetVolume(NPMCIGRAPHIC npMCI, DWORD dwVolume);
DWORD InternalGetVolume(NPMCIGRAPHIC npMCI);
DWORD Internal_Update(NPMCIGRAPHIC npMCI, DWORD dwFlags, HDC hdc, LPRECT lprc);

// called on winproc or worker thread
DWORD InternalRealize(NPMCIGRAPHIC npMCI);
BOOL TryStreamUpdate(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    HDC hdc,
    LPRECT lprc
);


// called to release the synchronous portion of a command
void TaskReturns(NPMCIGRAPHIC npMCI, DWORD dwErr);

