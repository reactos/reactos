/*
 *  WinMM (winmm.h) : Common internal header
 *
 *  [8-18-2003] AG: Created
 */


#include <windows.h>


// These are the memory-mapped file names used
#define FM_WINMM_GENERAL_INFO   "WINMM_000"
#define FM_MIDI_IN_DEV_INFO     "WINMM_001"
#define FM_MIDI_IN_HANDLE_INFO  "WINMM_002"
#define FM_MIDI_OUT_DEV_INFO    "WINMM_003"
#define FM_MIDI_OUT_HANDLE_INFO "WINMM_004"
#define FM_WAVE_IN_DEV_INFO     "WINMM_005"
#define FM_WAVE_IN_HANDLE_INFO  "WINMM_006"
#define FM_WAVE_OUT_DEV_INFO    "WINMM_007"
#define FM_WAVE_OUT_HANDLE_INFO "WINMM_008"



typedef struct  // WINMM_000
{
    UINT MidiInDeviceCount;
    UINT MidiInHandleCount;
    UINT MidiOutDeviceCount;
    UINT MidiOutHandleCount;
    UINT WaveInDeviceCount;
    UINT WaveInHandleCount;
    UINT WaveOutDeviceCount;
    UINT WaveOutHandleCount;
} WinMMGeneralInfo, *LPWinMMGeneralInfo;


typedef struct  // WINMM_003
{
    BOOL IsOpen;    // Correct?
} MidiOutDeviceInfo, *LPMidiOutDeviceInfo;

typedef struct  // WINMM_004
{
    UINT DeviceID;  // Needs to be first
    BOOL IsOpen;
} MidiOutHandleInfo, *LPMidiOutHandleInfo;



// Initialization routines
void mi_Init();
void mo_Init();
