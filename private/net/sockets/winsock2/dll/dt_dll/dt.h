/*++
  
  Copyright (c) 1995 Intel Corp
  
  Module Name:
  
    dt.h
  
  Abstract:
  
    Header file containing definitions, function prototypes, and other
    stuff for internal use of the debug/trace dll.
  
  Author:
    
    Michael A. Grafton 
  
--*/


#define NO_OUTPUT       0
#define FILE_ONLY       1
#define WINDOW_ONLY     2
#define FILE_AND_WINDOW 3
#define DEBUGGER 4
#define EC_CHILD 1
#define TEXT_LEN 2048
#define MAX_FP   128

// resource constants
#define IDD_DIALOG1                     101
#define IDC_RADIO4                      1007
#define IDC_RADIO5                      1008
#define IDC_RADIO6                      1009
#define IDC_RADIO7                      1010
#define IDC_RADIO8                      1011
#define IDC_CHECK                       1020
#define IDC_STATIC                      -1

// structure to hold init data
typedef struct _INITDATA {

    SYSTEMTIME LocalTime; 
    DWORD      TID;
    DWORD      PID;   

} INITDATA, *PINITDATA;

BOOL
DTTextOut(
    IN HWND   WindowHandle,
    IN HANDLE FileHandle,
    IN char   *String,
    IN DWORD  Style);

extern HWND   DebugWindow;               
extern HANDLE LogFileHandle;             
extern DWORD  OutputStyle;
extern char   Buffer[TEXT_LEN];
