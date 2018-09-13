/*++

Copyright (c) 1995 Intel Corp

Module Name:

    ws2_chat.h

Abstract:

    This module defines the data structures and constants
    for the WinSock 2 sample chat application.

Author:

    Dan Chou & Michael Grafton

--*/

#ifndef _WS2_CHAT_H
#define _WS2_CHAT_H

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include <winsock2.h>
#include "queue.h"

// turn off "unreferenced formal parameter" warning
#pragma warning(disable: 4100)



//
// Manifest Constants
//


// UI/MDI
#define MAIN_MENU_POS         1
#define CLIENT_WINDOW_ID      1

#define EC_SEND_CHILD         0
#define EC_RECV_CHILD         1

#define IDM_CONNECT           10
#define IDM_CLOSE             12
#define IDM_EXIT              13
#define IDM_TILE              30
#define IDM_CASCADE           31
#define IDM_ARRANGE           32
#define IDM_CLOSEALL          33
#define IDM_CLEAR_SENDBUFFER  34
#define IDM_CLEAR_RECVBUFFER  35
#define IDM_FIRSTCHILD        100

// For some reason, 30k is the most characters you can have in an edit
// control at once.  This keeps us from going over.
#define MAX_EC_TEXT 25500

// resource constants
#define IDC_SUBJECT           1000
#define IDC_CALLEENAME        1001
#define IDC_CALLERNAME        1002
#define IDC_FAM_LB            1003
#define IDC_INET_ADDRESS      1004
#define IDC_INET_PORT         1005
#define IDC_ADDRESS           1006
#define IDC_LISTEN_PORT       1007
#define IDC_STATIC            -1

// data lengths
#define NAME_LEN              20
#define ADDR_LEN              40
#define SUB_LEN               80
#define MSG_LEN               200
#define TITLE_LEN             40
#define BUFFER_LENGTH         1024

// various stuff
#define MAX_SOCKADDR_LEN      512
#define MAX_LISTENING_SOCKETS 64
#define CONN_WND_EXTRA        8
#define GWL_CONNINFO          0
#define GWL_OLDEDITPROC       4
#define NO_MAX_MSG_SIZE       0xffffffff

// what kind of send?
#define OVERLAPPED_IO         0
#define NON_OVERLAPPED_IO     1

// chat return values
#define CHAT_OK               0
#define CHAT_ERROR            1
#define CHAT_CLOSED           2
#define CHAT_WOULD_BLOCK      3
#define CHAT_ABORTED          4

// for Internet connections
#define INET_ADDR_LEN         64
#define INET_PORT_LEN         5
#define INET_DEFAULT_PORT     9009

// user message values
#define USMSG_ACCEPT          WM_USER+1
#define USMSG_CONNECT         WM_USER+2
#define USMSG_TEXTOUT         WM_USER+3

// version of Winsock we need
#define VERSION_MAJOR         2
#define VERSION_MINOR         0

// quality of Service
#define TOKENRATE             100
#define QOS_UNSPECIFIED       -1



// 
// Types and Data Structures
//

typedef struct _CONNDATA CONNDATA, *PCONNDATA;
typedef struct _OUTPUT_REQUEST OUTPUT_REQUEST, *POUTPUT_REQUEST;

// structure created by the user interface and passed to the
// input/output thread -- tells the I/O thread what to send and how to
// send it.
struct _OUTPUT_REQUEST {
    int             Type;       // OVERLAPPED_IO or NON_OVERLAPPED_IO
    WSABUF          Buffer;     // holds the buffer (pointer) and size
    char            Character;  // for one-character ouput events
    PCONNDATA       ConnData;   // data for the associated connection
    LPWSAOVERLAPPED Overlapped; // NULL if not an overlapped send
};

// structure for storing data unique to each connection.
struct _CONNDATA {

                    // the socket for the connection
    SOCKET          Socket;    

                    // string containing the name of the user (only
                    // valid if user-data is supported by the protocol)
    char            PeerName[NAME_LEN + 1];       

                    // string containing the subject of the chat
                    // session, as entered by the calling entity
    char            Subject[SUB_LEN + 1];     

                    // string containing the (human-readable) address
                    // of the connected entity  
    char            PeerAddress[ADDR_LEN + 1]; 
                    
                    // handle to thread that takes care of network events
    HANDLE          IOThreadHandle;   

                    // handle to event signaled when a network event
                    // occurs 
    HANDLE          SocketEventObject;

                    // handle to event signaled when output is ready
                    // to be shipped 
    HANDLE          OutputEventObject;

                    // points to a queue used to hold output buffers
    PQUEUE          OutputQueue;

                    // handle to window associated with the connection
    HWND            ConnectionWindow;

                    // handle to the sending and receiving edit controls 
    HWND            SendWindow;
    HWND            RecvWindow;

                    // the remote socket address for the connection
    WSABUF          RemoteSockAddr;

                    // buffer used by the connecting entity to get
                    // user-data back when the  USMSG_CONNECT is
                    // received   
    WSABUF          CalleeBuffer;

                    // points to a protocol information struct which
                    // represents the protocol for this connection 
    LPWSAPROTOCOL_INFO ProtocolInfo;

                    // are we waiting for an FD_WRITE?
    BOOL            WriteOk;

                    // How many events the wait in the I/O thread is
                    // waiting on.  Once that thread starts, this will
                    // always be at least 2 -- the SocketEventObject
                    // and the OutputEventObject.
    DWORD           NumEvents;

                    // The array of events which the I/O thread is
                    // currently waiting on.  Contains NumEvents entries.
    WSAEVENT        EventArray[WSA_MAXIMUM_WAIT_EVENTS]; 

                    // An array of pointers to output request
                    // structures, indexed in parallel to EventArray,
                    // above. This allows us to associate the events
                    // with the output requests, so we can free the
                    // right memory when the event is signaled.
    POUTPUT_REQUEST OutReqArray[WSA_MAXIMUM_WAIT_EVENTS];

                    // The maximum message size we can send on this
                    // socket. This value is either an integer or the
                    // manifest constant NO_MAX_MSG_SIZE.
    DWORD            MaxMsgSize;
};

// structure to associate listening sockets with a protocol
// information struct. 
typedef struct _LISTENDATA {
    SOCKET             Socket;       // a listening socket
    LPWSAPROTOCOL_INFO ProtocolInfo; // the associated protocol info. struct
} LISTENDATA, *PLISTENDATA;




//
// Function prototypes for functions to be used outside of ws2_chat.c
//

void
OutputString(
    IN HWND RecvWindow,
    IN char *String);


BOOL
ExtractTwoStrings(
    IN  char *Buffer,
    OUT char *String1,
    IN  char Length1,
    OUT char *String2,
    IN  int  Length2);

BOOL
TranslateHex(
    OUT LPVOID Buffer, 
    IN  int    BufferLen,
    IN  char   *HexString,
    IN  HWND   WindowHandle);

BOOL
PackTwoStrings(
    OUT char *Buffer,
    IN int   BufferLen,
    IN char  *String1,
    IN char  *String2);

BOOL 
MakeRoom(
    IN HWND EditControl,
    IN int  HowMuch);

void 
ChatSysError(
    IN char *FailedFunction,
    IN char *InFunction,
    IN BOOL Fatal);

//
// Externally-Visible Variables
//

extern HANDLE  GlobalInstance;    // Identifies the instance of chat
extern char    ConnClassStr[];    // String to register window class
extern char    ChatClassStr[];    // String to register window class
extern HWND    GlobalFrameWindow; // Chat's main -- or frame -- window

#endif // _WS2_CHAT_
