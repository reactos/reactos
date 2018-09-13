#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Status Constants
#define CONNECTED       1
#define CONNECTING      2
#define REQUEST_OPENING 3
#define REQUEST_OPENED  4
#define LDG_STARTING    5
#define LDG_START       6
#define LDG_LDG         7
#define LDG_RDY         8 
#define LDG_DONE        9

// Priority Constants
#define LOW    1
#define MEDIUM 2
#define HIGH   3

#define BUF_SIZE 8192
#define BUF_NUM 16

#define URLMAX 4
#define TIMEOUT  60000

#define MAX_SCHEME_LENGTH 64

//MESSAGE ID'S
#define DOWNLOAD_DONE         WM_USER + 1
#define DOWNLOAD_OPEN_REQUEST WM_USER + 2
#define DOWNLOAD_SEND_REQUEST WM_USER + 3
#define DOWNLOAD_READ_FILE    WM_USER + 4


typedef struct
{
    TCHAR    *pBuf;      //Actual buffer to hold data
    DWORD    lNumRead;   //number of bytes read in buffer
    void     *pNext;     //Pointer to next buffer
} buffer;


typedef struct
{
    buffer   *pPool;         //Pointer to current available buffer in pool
    INT      iDownloads;     //number of current downloads
    DWORD    mainThreadId;   //thread ID for main thread
    CRITICAL_SECTION csInfo; //for critical section
} info;

typedef struct
{
    TCHAR    *pURLName;     //The name of the URL
    INT      iStatus;       //the url's status
    INT      iPriority;     //the url's priority
                            // LOW, MEDIUM, or HIGH
    buffer   *pStartBuffer; //first buffer to hold data
    buffer   *pCurBuffer;   //Current Buffer
    info     *pInfo;        //Pointer to pool of buffers
    void     *pNext;        //pointer to next element
    HINTERNET hInetCon;     //Internet connection
    HINTERNET hInetReq;     //Internet Request
    TCHAR    szRHost[INTERNET_MAX_HOST_NAME_LENGTH]; //from crackUrl
    TCHAR    szRPath[INTERNET_MAX_PATH_LENGTH];      //from crackUrl
    TCHAR    szRScheme[MAX_SCHEME_LENGTH];           //from crackUrl
    INTERNET_PORT nPort;                             //from crackUrl
    INTERNET_SCHEME nScheme;                         //from crackUrl
} outQ;

//
void callOpenRequest(outQ *pOutQ);
void callSendRequest(outQ *pOutQ);
void callReadFile(outQ *pOutQ);
BOOL getServerName(outQ *pOutQ);

