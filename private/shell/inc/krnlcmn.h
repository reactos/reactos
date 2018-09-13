//---------------------------------------------------------------------------
//
//  KrnlCmn.h
//
//  Include file for common private krnl386/kernel32 APIs.
//
//---------------------------------------------------------------------------

//
//  idProcess can be one of:
//      0L                      --  for current process
//      MAKELONG(hTask, 0)      --  for process with task handle hTask
//      idProcess               --  for real PID
//
//  iIndex is:
//      extra DWORD if >= 0
//      kernel thing if negative
//

#define GPD_PPI                 0       // Going away
#define GPD_FLAGS               -4
#define GPD_PARENT              -8
#define GPD_STARTF_FLAGS        -12     // Can be changed
#define GPD_STARTF_POS          -16
#define GPD_STARTF_SIZE         -20
#define GPD_STARTF_SHOWCMD      -24
#define GPD_STARTF_HOTKEY       -28
#define GPD_STARTF_SHELLDATA    -32
#define	GPD_CURR_PROCESS_ID     -36
#define	GPD_CURR_THREAD_ID      -40
#define	GPD_EXP_WINVER          -44
#define GPD_HINST               -48
#define GPD_HUTSTATE		-52
#define GPD_COMPATFLAGS         -56

#define CW_USEDEFAULT_32    0x80000000
#define CW_USEDEFAULT_16    0x00008000

#ifdef WIN32
#define INDEX   LONG
#define OURAPI  APIENTRY
#else
#define INDEX   int
#define OURAPI  API

#define STARTF_USESHOWWINDOW    0x00000001
#define STARTF_USESIZE          0x00000002
#define STARTF_USEPOSITION      0x00000004
#define STARTF_FORCEONFEEDBACK  0x00000040
#define STARTF_FORCEOFFFEEDBACK 0x00000080
#define STARTF_USEHOTKEY        0x00000200  // ;4.0
#define STARTF_HASSHELLDATA     0x00000400  // ;Internal
#endif

DWORD OURAPI GetProcessDword(DWORD idProcess, INDEX iIndex);
BOOL  OURAPI SetProcessDword(DWORD idProcess, INDEX iIndex, DWORD dwValue);

// 
// For GPD_FLAGS
//
#define GPF_DEBUG_PROCESS   0x00000001
#define GPF_WIN16_PROCESS   0x00000008
#define GPF_DOS_PROCESS     0x00000010
#define GPF_CONSOLE_PROCESS 0x00000020
#define GPF_SERVICE_PROCESS 0x00000100


#undef OURAPI
#undef INDEX
