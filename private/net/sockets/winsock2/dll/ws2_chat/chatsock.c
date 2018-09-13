/*++

Copyright (c) 1995 Intel Corp

Module Name:

    chatsock.c

Abstract:

    Socket-related functions for the WinSock2 Chat sample application.

Author:

    Dan Chou & Michael Grafton

--*/

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include <winsock2.h>
#include <stdlib.h>
#include <assert.h>
#include "ws2_chat.h"
#include "chatsock.h"
#include "chatdlg.h"



//
// Static Globals
//

// points to an array of WSAPROTOCOL_INFO structs
static LPWSAPROTOCOL_INFO InstalledProtocols = NULL; 

// number of WSAPROTOCOL_INFO structs in the InstalledProtocols buffer 
static int NumProtocols = 0;

// static array of sockets, one for each listening socket  
static LISTENDATA ListeningSockets[MAX_LISTENING_SOCKETS];

// number of meaningful entries in ListeningSockets
static int NumFound = 0;




// 
// Function Prototypes -- Internal Functions
//

DWORD
IOThreadFunc(
    IN LPVOID ParamPtr);

BOOL
HandleSocketEvent(
    IN OUT PCONNDATA ConnData);

BOOL
HandleOutputEvent(
    IN OUT PCONNDATA);

BOOL
HandleOtherEvent(
    IN     DWORD     WaitStatus,
    IN OUT PCONNDATA ConnData);

int
HandleEvents(
    IN PCONNDATA          ConnData,
    IN LPWSANETWORKEVENTS NetworkEvents);

BOOL
FillLocalAddress(
    IN LPVOID          SockAddr);

int
DoRecv(
    IN PCONNDATA ConnData);

int
DoOverlappedCallbackSend(
    IN POUTPUT_REQUEST OutReq,
    IN PCONNDATA       ConnData);

int
DoOverlappedEventSend(
    IN POUTPUT_REQUEST OutReq,
    IN PCONNDATA       ConnData);

int
DoSend(
    IN POUTPUT_REQUEST OutReq,
    IN PCONNDATA       ConnData);

void CALLBACK
SendCompFunc(
    IN DWORD           Error,
    IN DWORD           BytesTransferred,
    IN LPWSAOVERLAPPED OverlappedPtr,
    IN DWORD           Flags);

int CALLBACK
AcceptCondFunc(
    IN LPWSABUF    CallerId,
    IN LPWSABUF    CallerData,
    IN LPQOS       CallerSQos,
    IN LPQOS       CallerGQos,
    IN LPWSABUF    CalleeId,
    OUT LPWSABUF   CalleeData,
    OUT GROUP FAR  *Group,
    IN DWORD   CallbackData);

LPWSAPROTOCOL_INFO
GetProtoFromSocket(
    IN SOCKET Socket);

BOOL
GetMaxMsgSize(
    IN OUT PCONNDATA ConnData);




//
// Function Definitions
//


BOOL
InitWS2(void)
/*++

Routine Description:

    Calls WSAStartup, makes sure we have a good version of WinSock2

Arguments:

    None.

Return Value:

    TRUE - WinSock 2 DLL successfully started up

    FALSE - Error starting up WinSock 2 DLL.

--*/

{
    int           Error;              // catches return value of WSAStartup
    WORD          VersionRequested;   // passed to WSAStartup
    WSADATA       WsaData;            // receives data from WSAStartup
    BOOL          ReturnValue = TRUE; // return value
    
    // Start WinSock 2.  If it fails, we don't need to call
    // WSACleanup().
    VersionRequested = MAKEWORD(VERSION_MAJOR, VERSION_MINOR); 
    Error = WSAStartup(VersionRequested, &WsaData);
    if (Error) {
        MessageBox(GlobalFrameWindow,
                   "Could not find high enough version of WinSock",
                   "Error", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;
    } else {

        // Now confirm that the WinSock 2 DLL supports the exact version
        // we want. If not, make sure to call WSACleanup().
        if (LOBYTE(WsaData.wVersion) != VERSION_MAJOR ||
            HIBYTE(WsaData.wVersion) != VERSION_MINOR) {
            MessageBox(GlobalFrameWindow,
                       "Could not find the correct version of WinSock",
                       "Error",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            WSACleanup();
            ReturnValue = FALSE;
        }
    }
    return(ReturnValue);
    
} // InitWS2()





BOOL
FindProtocols(void)
/*++

Routine Description:

    Finds out about all transport protocols installed on the local
    machine and saves the information into global variables.

Implementation:

    This function uses WSAEnumProtocols to find out about all
    installed protocols on the local machine.  It stores this
    information in two variables which are global to this file;
    InstalledProtocols is a pointer to a buffer of WSAPROTOCOL_INFO
    structs, while NumProtocols is the number of protocols in that
    buffer.  This function is the only function in the file allowed to
    touch these variables. 

Arguments:

    None.

Return Value:

    TRUE - Successfully initialized the protocol buffer.

    FALSE - Some kind of problem arose.  The user is informed of the
    error.

--*/
{
    
    DWORD BufferSize = 0;       // size of InstalledProtocols buffer
    char  MsgText[MSG_LEN];     // holds message strings

    // Call WSAEnumProtocols to figure out how big of a buffer we need.
    NumProtocols = WSAEnumProtocols(NULL,  
                                    NULL,  
                                    &BufferSize); 

    if ((NumProtocols != SOCKET_ERROR) && (WSAGetLastError() != WSAENOBUFS)) {
        // We're in trouble!!
        MessageBox(GlobalFrameWindow, "WSAEnumProtocols is broken.", "Error", 
                   MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        goto Fail;
    }
    
    // Allocate a buffer, call WSAEnumProtocols to get an array of
    // WSAPROTOCOL_INFO structs.
    InstalledProtocols = (LPWSAPROTOCOL_INFO)malloc(BufferSize);
    if (InstalledProtocols == NULL) {
        MessageBox(GlobalFrameWindow, "malloc failed.", "Error", 
                   MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        goto Fail;
    }
    NumProtocols = WSAEnumProtocols(NULL, 
                                    (LPVOID)InstalledProtocols,
                                    &BufferSize);
    if (NumProtocols == SOCKET_ERROR) {
        // uh-oh
        wsprintf(MsgText, "WSAEnumProtocols failed.  Error Code: %d",
                 WSAGetLastError());
        MessageBox(GlobalFrameWindow, MsgText, "Error", 
                   MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        goto Fail;
    }
    return(TRUE);

 Fail:

    WSACleanup();
    return(FALSE);

} // FindProtocols()





BOOL
FillLocalAddress(
    IN struct sockaddr *SockAddr)
/*++

Routine Description:

    Fills in the struct sockaddr with a local address appropriate for
    the address family, for use in a call to bind.

Arguments:

    SockAddr -- A pointer to a struct sockaddr which will be filled in
    appropriately, as determined by the particular address family.
    The address family field (sa_family) must already be filled into
    the structure before being passed into this function.

Return Value:

    TRUE -- FillLocalAddress recognized the address family and
    successfully filled in the struct sockaddr.  This structure is
    ready to be passed to a bind() call.

    FALSE -- FillLocalAddress didn't recognize the address family and
    didn't touch the struct sockaddr.

--*/
{

    struct sockaddr_in *SockAddrInet; // used to cast SockAddr to an
                                      // Internet-style address
    BOOL ReturnValue = TRUE;          // holds the return value

    switch (SockAddr->sa_family) {
        
    case AF_INET:
        
        // Cast the pointer so we can now access it's fields as a
        // struct sockaddr_in, the address structure for internet
        // addresses. 
        SockAddrInet = (struct sockaddr_in *)SockAddr;
        if (!DialogBoxParam(GlobalInstance, 
                            "InetListenPortDlg", 
                            GlobalFrameWindow, 
                            InetListenPortDlgProc,
                            (LPARAM)SockAddrInet)) {
            ReturnValue = FALSE;
        }
        break;

    default:
        
        ReturnValue = FALSE;
        break;
    }
    return(ReturnValue);

} // FillLocalAddress()





BOOL
ListenAll(void)

/*++

Routine Description:

    For each installed, connection-oriented protocol, this function
    creates a socket, binds to a local address, and listens on the
    created socket.  The socket is set up for Windows message
    notification for any connection attempts.

Arguments:

    None.

Return Value:

    TRUE - Successfully listened on all installed protocols.

    FALSE - Error listening on all installed protocols.  It is not an
    error if there are more connection-oriented protocols than
    MAX_LISTENING_SOCKETS installed; in this case we just ignore any
    extra protocols.

--*/

{
    struct sockaddr *SockAddr;        // holds socket addresses
    int             SockAddrLen;      // length, in bytes, of SockAddrLen
    LPWSAPROTOCOL_INFO COProtocolInfo;   // current protocol info to examine
    int             i;                // counting variable
    char            MsgText[MSG_LEN]; // holds message strings
    int             Error;            // holds return values
    int             Index;            // indexes into text messages
    
    // Find all protocols that support connection-oriented data
    // transfer; create a socket, bind to a local address and listen
    // for connections on that socket for each such protocol found.
    // Also fill in an entry in the ListeningSockets array.
    for (i = 0; i < NumProtocols; i++) {

        COProtocolInfo = &InstalledProtocols[i];
        assert(COProtocolInfo != NULL);
        if (UseProtocol(COProtocolInfo)) {

            // We've found a suitable protocol.  Create a socket, fill
            // in the next entry in ListeningSockets
            ListeningSockets[NumFound].Socket = WSASocket(0,              
                                                          0,              
                                                          0,              
                                                          COProtocolInfo, 
                                                          0,              
                                                          WSA_FLAG_OVERLAPPED);
            if (ListeningSockets[NumFound].Socket == INVALID_SOCKET) {
                MessageBox(GlobalFrameWindow, "Could not open a socket.", 
                           "Non-fatal error.", 
                           MB_OK | MB_SETFOREGROUND);
                continue;
            }
            ListeningSockets[NumFound].ProtocolInfo = COProtocolInfo;
            
            // Allocate a block of memory for the socket address and
            // zero it out.
            SockAddrLen = COProtocolInfo->iMaxSockAddr;
            SockAddr = (struct sockaddr *)malloc(SockAddrLen);
            if (!SockAddr) {
                ChatSysError("malloc()", 
                             "ListenAll()", 
                             TRUE);
            }
            memset((char *)SockAddr, 0, SockAddrLen);

            // Get a local address to bind the socket to
            SockAddr->sa_family = (u_short)COProtocolInfo->iAddressFamily;
            FillLocalAddress(SockAddr);       

            // Bind the socket to SockAddr.
            Error = bind(ListeningSockets[NumFound].Socket, 
                         SockAddr, 
                         SockAddrLen);
            if (Error == SOCKET_ERROR) {

                // bind() failed
                MessageBox(GlobalFrameWindow, "Could not bind the socket.",
                           "Non-fatal error", MB_OK | MB_SETFOREGROUND);
                free(SockAddr);
                continue;
            }
            free(SockAddr);

            // Set up the socket for windows message event notification.
            // Note that this call automatically puts the socket into
            // non-blocking mode, as if we had called WSAIoctl with
            // the FIONBIO flag.
            Error = WSAAsyncSelect(ListeningSockets[NumFound].Socket,
                                   GlobalFrameWindow, 
                                   USMSG_ACCEPT, 
                                   FD_ACCEPT);
            if (Error == SOCKET_ERROR) {
                MessageBox(GlobalFrameWindow, "Error: WSAAsyncSelect()", 
                           "Non-fatal error", MB_OK | MB_SETFOREGROUND);
                continue;
            }
            
            // Listen for incoming connection requests on the socket.
            Error = listen(ListeningSockets[NumFound].Socket,
                           SOMAXCONN);
            if (Error == SOCKET_ERROR) {
                MessageBox(GlobalFrameWindow, "Error: listen()", 
                           "Non-fatal error",
                           MB_OK | MB_SETFOREGROUND);
                continue;
            }
            
            // Looks good -- increase the count, check for overflow of
            // the max amount, re-iterate if we're ok.
            if (++NumFound == MAX_LISTENING_SOCKETS) {
                wsprintf(MsgText, 
                         "More than %d useable protocols. Ignoring extras.",
                         MAX_LISTENING_SOCKETS);
                MessageBox(GlobalFrameWindow, MsgText, "Alert", 
                           MB_OK | MB_SETFOREGROUND);
                break;
            } 
                    
        } // if (UseProtocol(COProtocolInfo)) 
    } // for(; ; ;)

    if (NumFound == 0) {

        Index = wsprintf(MsgText, 
                         "Couldn't find a suitable protocol ");
        Index += wsprintf(MsgText + Index, 
                          "and/or no listening sockets could be opened.\r\n");
        wsprintf(MsgText + Index, "Shall we continue anyway?");

        if (MessageBox(GlobalFrameWindow, MsgText, "No protocols.",
                       MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND) 
            == IDNO) {

            WSACleanup();
            return(FALSE);
        }
    }

    return(TRUE);

} // ListenAll()





BOOL
UseProtocol(
    IN LPWSAPROTOCOL_INFO Proto)
/*++

Routine Description:

    Returns true if Proto is suitable for use by Chat.

Arguments:

    Proto -- Points to a protocol information struct.

Return Value:

    TRUE -- Chat likes it.

    FALSE -- Get this chintzy protocol out of here!

--*/
{

    if (!(Proto->dwServiceFlags1 & XP1_CONNECTIONLESS) &&
        (Proto->dwServiceFlags1 & XP1_GUARANTEED_DELIVERY) &&
        (Proto->dwServiceFlags1 & XP1_GUARANTEED_ORDER)) {
        
        return(TRUE);
    
    } else {
        
        return(FALSE);
    }
} // UseProtocol





void 
CleanUpSockets(void)
/*++

Routine Description:

    This function closes all listening sockets.

Arguments:

    None.

Return Value:

    None.

--*/
{

    int i; // counting variable
    
    for (i = 0; i < NumFound; i++) {
        closesocket(ListeningSockets[i].Socket);
    }
} // CleanUpSockets()





int CALLBACK
AcceptCondFunc(
    IN LPWSABUF   CallerId,
    IN LPWSABUF   CallerData,
    IN LPQOS      CallerSQos,
    IN LPQOS      CallerGQos,
    IN LPWSABUF   CalleeId,
    OUT LPWSABUF  CalleeData,
    OUT GROUP FAR *Group,
    IN DWORD  CallbackData)
/*++

Routine Description:

    Condition function called when an incoming connection request is
    handled. 

Implementation:

    This function allows the user to accept or reject an incoming
    connection request after examining the caller's name and the
    subject of the call. If the call is accepted, and
    connection-time data transfer is supported by the particular
    protocol on which the connection is made, a dialog box comes up
    which prompts the user for his/her name.

Arguments:

    CallerId -- Supplies the address of the caller.

    CallerData -- Supplies the caller's user data.  Chat uses this
    parameter to send the caller's name and the subject of the call.

    CallerSQos -- Supplies the forward and backward QOS.

    CallerGQos -- Supplies the forward and backward flow specs for the
    socket group the caller is to create.  Not used by chat.

    CalleeId -- Supplies the local address.

    CalleeData -- Returns user data back to the caller (Chat uses this
    parameter to return the callee's name) 

    Group -- Returns the appropriate group action to take on the
    connecting socket.  Not used by chat.  Always returns NULL.

    CallbackData -- Supplies a pointer the CONNDATA structure
    associated with this connection.

Return Value:

    CF_ACCEPT - Accept the connection request from the caller.

    CF_REJECT - Reject the connection request from the caller.

--*/

{
    char          MsgText[MSG_LEN];         // build message string here   
    char          TitleText[TITLE_LEN + 1]; // build title string here
    PCONNDATA     ConnData;                 // connection-specific data
    int           Index;                    // index into MsgText
    int           ReturnValue = CF_ACCEPT;  // return value
    
    ConnData = (PCONNDATA)CallbackData;

    // CallerId contains the socket address of the connecting entity.
    // Copy this into the connection-specific data.
    ConnData->RemoteSockAddr.len = CallerId->len;
    ConnData->RemoteSockAddr.buf = malloc(ConnData->RemoteSockAddr.len);
    if (ConnData->RemoteSockAddr.buf == NULL) {
        ChatSysError("malloc()",
                     "AcceptCondFunc()",
                     TRUE);
    }
    memcpy((char *)ConnData->RemoteSockAddr.buf, (char *)CallerId->buf, 
           CallerId->len);
    
    // Translate the RemoteSockAddr into a human readable form, and
    // store it in ConnData->PeerAddress
    GetAddressString(ConnData->PeerAddress, 
                     ConnData->RemoteSockAddr.buf,
                     ConnData->RemoteSockAddr.len,
                     ConnData->ProtocolInfo);

    Index = wsprintf(MsgText, "Someone is attempting a chat connection.\r\n");

    if (CallerData != NULL) {
        
        // The connection request has come with some caller data.  Use
        // it to inform the user of who is trying to connect
        ExtractTwoStrings(CallerData->buf, 
                          ConnData->PeerName, 
                          NAME_LEN + 1, 
                          ConnData->Subject, 
                          SUB_LEN + 1);
        
        // Build the strings for the message box and the title of the
        // connection window
        Index += wsprintf(MsgText + Index,
                 "From: %s\r\nSubject: %s\r\n", ConnData->PeerName, 
                          ConnData->Subject);
        wsprintf(TitleText, "Connected to: %s @ %s", ConnData->PeerName, 
                 ConnData->PeerAddress);
        
    } else {
        
        // There is no caller data...build a string for the title.
        wsprintf(TitleText, "Connected to: %s", ConnData->PeerAddress);
    }

    // Continue building MsgText.
    Index += wsprintf(MsgText + Index, "Address: %s\r\n", 
                      ConnData->PeerAddress);          
    Index += wsprintf(MsgText + Index, "Would you like to accept it?");
    
    // Prompt the user to accept or reject the connection.
    //
    // ****NOTE****
    // This is NOT the right way to do this.  The application should
    // not hold up this thread by putting up this message box, or the
    // dialog box below.  As mentioned in the API spec, this function
    // should  return "as soon as possible", and clearly this is not
    // what's being done here.  Please look to future versions of chat
    // for a fix.  Thanks.
    if (MessageBox(ConnData->ConnectionWindow, MsgText,
                   "Connection Request", 
                   MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND) == IDYES) {
        
        // The user has accepted the connection request.
        SetWindowText(ConnData->ConnectionWindow, TitleText);
        if (CalleeData != NULL) {

            // We can try to pass user data back. Call up a dialog box
            // to get a name string and put it into CalleeData.
            if (!DialogBoxParam(GlobalInstance, 
                                "AcceptConnectionDlg",
                                ConnData->ConnectionWindow,
                                AcceptConnectionDlgProc, 
                                (LPARAM)CalleeData)) {
                
                CalleeData->len = 0;
            }
        }
        ReturnValue = CF_ACCEPT;

    } // if (MessageBox(...))
    else {

        // The user has rejected the connection request.
        ReturnValue = CF_REJECT;
    }
    return(ReturnValue);

} // AcceptCondFunc()





BOOL
GetAddressString(
    OUT char            *String,
    IN  LPVOID          SockAddr,
    IN  int             SockAddrLen,
    IN  LPWSAPROTOCOL_INFO ProtocolInfo)
/*++

Routine Description:

    This function translates the SockAddr into a human-readable
    string, if possible.  If chat doesn't recognize the protocol, then
    the string "(unknown)" is returned in String.

Arguments:

    String -- Returns the string representing the SockAddr in a
    human-readable form.

    SockAddr -- An address.

    SockAddrLen -- The length, in bytes, of SockAddr.

    ProtocolInfo -- Pointer to the protocol information structure.

Return Value:

    TRUE -- Chat recognized the protocol family and successfully
    translated the address.

    FALSE -- Chat did not recognize the protocol family and returned
    the string "(unknown)".

--*/
{

    char               *TempString;   // string returned by inet_ntoa
    struct sockaddr_in *SockAddrInet; // casts the address to a sockaddr_in
    BOOL               ReturnValue;   // holds the return value

    ReturnValue = TRUE;

    switch (ProtocolInfo->iAddressFamily) {

    case AF_INET:
        
        // It's an Internet-style address.
        SockAddrInet = (struct sockaddr_in *)SockAddr;
        TempString = inet_ntoa(SockAddrInet->sin_addr);
        if (TempString == NULL) {
            MessageBox(NULL, "inet_ntoa() failed.", "Error.", 
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        } else {
            strcpy(String, TempString);
        }
        ReturnValue = TRUE;
        break;

    default:

        strcpy(String, "(unknown)");
        ReturnValue = FALSE;
        break;
    }
    return(ReturnValue);

} // GetAddressString()





int
HandleEvents(
    IN PCONNDATA          ConnData,
    IN LPWSANETWORKEVENTS NetworkEvents)

/*++

Routine Description:

    Handles network events that may occur on a connected socket.
    The events handled by this function are FD_CLOSE, FD_READ, and
    FD_WRITE.  

Arguments:

    ConnData - Supplies a pointer to data for the connection on which
    the event happened.

    NetworkEvents - Supplies a WSANETWORKEVENT structure, which is a
    record of the network events that have occurred as well as any
    accompanying error-codes.

Return Value:

    CHAT_OK -- The network event was successfully handled.

    CHAT_ERROR -- Some kind of error occurred while handling the
    event, and the connection should be closed.

    CHAT_CLOSED -- The connection has been gracefully closed.

--*/

{
    int Result;                // holds the result of DoRecv
    int ReturnValue = CHAT_OK; // return value
    
    // The following three if statements all execute unless one gets
    // an error or closed socket, in which case we return immediately.
    if (NetworkEvents->lNetworkEvents & FD_READ) {
        
        // An FD_READ event has occurred on the connected socket.
        if (NetworkEvents->iErrorCode[FD_READ_BIT] == WSAENETDOWN) {

            // There is an error.
            ReturnValue = CHAT_ERROR;
            goto Done;

        } else {

            // Read data off the socket...
            Result = DoRecv(ConnData);
            if ((Result == CHAT_ERROR) || (Result == CHAT_CLOSED)) {
                ReturnValue = Result;
                goto Done;
            }
        }        
    } 

    if (NetworkEvents->lNetworkEvents & FD_WRITE) {
        
        // An FD_WRITE event has occurred on the connected socket.
        if (NetworkEvents->iErrorCode[FD_WRITE_BIT] == WSAENETDOWN) {

            // There is an error.
            ReturnValue = CHAT_ERROR;
            goto Done;

        } else {
            
            // Allow chat to send on this socket, and signal the
            // OuputEventObject in case there is pending output that is
            // not completed due to WSAEWOULDBLOCK.
            ConnData->WriteOk = TRUE;
            SetEvent(ConnData->OutputEventObject);
        }
    }

    if (NetworkEvents->lNetworkEvents & FD_CLOSE) {

        if (NetworkEvents->iErrorCode[FD_CLOSE_BIT] == 0) {

            // A graceful shutdown has occurred...
            ReturnValue = CHAT_CLOSED;
            goto Done;

        } else {
            
            // This is some other type of abortive close or failure...
            ReturnValue = CHAT_ABORTED;
            goto Done;
        }
        
    } 

 Done:
    return(ReturnValue);

} // HandleEvents()





DWORD
IOThreadFunc(
    IN LPVOID ParamPtr)

/*++

Routine Description:

    This routine is invoked as a separate thread to handle all input
    and output for a connection.  

Implementation:
    
    This thread sits in a loop, waiting for one of several things to
    occur.  These can be:

        1. The user interface thread has some input ready to be sent,
        and has signaled the ouput event object.

        2. WinSock 2 has indicated there is a network event associated
        with the socket.  

        3. Callback notification.  This can be via an event or via a
        queued callback function, depending on whether or not we've
        compiled with the CALLBACK_NOTIFICATION flag.
        
    The return value from the wait indicates what has
    happened, and a switch statement handles all the possible cases.
    When there is an error or the connection is being closed, the loop
    is broken and the thread will exit and the connection window will
    be closed (and sent a WM_DESTROY message).

Arguments:

    ParamPtr - Supplies a pointer to a ConnData structure that holds
    data specific to this connection.

Return Value:

    0 - Always returns 0.  The return value is not needed.

--*/
{
    char             MsgText[MSG_LEN]; // holds message strings
    DWORD            WaitStatus;       // holds return value of the wait
    PCONNDATA        ConnData;         // connection-specific data
    BOOL             KeepGoing = TRUE; // keep processing output requests?
    BOOL             Forever = TRUE;   // constant to avoid warning
    
    ConnData =  (PCONNDATA)ParamPtr;

    // Initialize the EventArray.  The only two events in it, for now,
    // are the Socket and Ouput events; if this is the event
    // notification version of chat, then there will be an additional
    // event for each overlapped send waiting for a completion
    // notification. 
    ConnData->NumEvents = 2;
    ConnData->EventArray[0] = ConnData->SocketEventObject;
    ConnData->EventArray[1] = ConnData->OutputEventObject;

    // Initialize the array of output requests, which is indexed in
    // parallel to the above event array.  When an event is signaled,
    // we can figure out which output request and overlapped
    // structures to free by indexing into this array.
    // These first two entries should never be referenced, because
    // their parallel entries in EventArray (see above) are for the
    // permanent Socket and Output event objects.
    ConnData->OutReqArray[0] = ConnData->OutReqArray[1] = NULL;

    while (Forever) {

        // Wait for an event (or a queued callback function) to wake
        // us up.  This is an alertable wait state (fAlertable == TRUE).
        WaitStatus = WSAWaitForMultipleEvents(ConnData->NumEvents,
                                              ConnData->EventArray,
                                              FALSE,        // fWaitAll
                                              WSA_INFINITE, // dwTimeout
                                              TRUE);        // fAlertable
        
        // Determine why we woke up and act accordingly.  Note that
        // breaking out of the switch causes us to break out of the
        // while loop as well.  When we don't want to break out of the
        // loop, case statements end with the continue statement.
        switch (WaitStatus) {
            
        case WSA_WAIT_FAILED:

            // A fatal error.  Pop up a message box and break out of
            // the while loop to end the thread.
            
            wsprintf(MsgText, 
                     "WSAWaitForMultipleEvents() failed.  Error code: %d", 
                     WSAGetLastError());
            MessageBox(ConnData->ConnectionWindow, MsgText, "Fatal Error",
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            break;

        case WAIT_IO_COMPLETION:
            
            // An I/O completion routine has been executed.  Cleanup
            // has already occurred in SendCompFunc, so there is
            // nothing left to do.  Just reiterate through the loop.
            
            continue;
            
        case WSA_WAIT_EVENT_0:
            
            // The SocketEventObject has been signaled.  Handle it in
            // a separate function.  Break out of the thread if
            // HandleSocketEvent returns FALSE, indicating error.
            if (HandleSocketEvent(ConnData)) {
                continue;
            } else {
                break;
            }
            
        // Please note: WSA_WAIT_EVENT_[1,2,3] are defined in
        // chatsock.h, not winsock2.h like WSA_WAIT_EVENT_0

        case WSA_WAIT_EVENT_1:

            // The OuputEventObject has been signaled.  Handle it in a
            // separate function, and break out of the thread if it
            // returns FALSE
            if (HandleOutputEvent(ConnData)) {
                continue;
            } else {
                break;
            }                

        default:

            // Some other event has been signaled.  Handle it in a
            // separate function, and break out of the thread if it
            // returns FALSE.
            if (HandleOtherEvent(WaitStatus, ConnData)) {
                continue;
            } else {
                break;
            }

        } // switch (WaitStatus)

        // Break out of the while loop.
        break;
    
    } // while (1)
    
    // Thread is ending because the connection was closed or an error
    // occurred 
    PostMessage(ConnData->ConnectionWindow, 
                WM_CLOSE, 
                0, 
                0);
    return(0);

} // IOThreadFunc()





BOOL
HandleSocketEvent(
    IN OUT PCONNDATA ConnData)
/*++

Routine Description:

    Handles the case in IOThreadFunc where the thread is woken up by a
    signal to the socket event object.

Arguments:

    ConnData - Supplies a pointer to data for the connection on which
    the event happened.
    
Return Value:

    TRUE -- Chat successfully handled the event and the thread should
    continue on.

    FALSE -- An error occurred and Chat should kill the thread and the
    connection. 

--*/
{
    int              Result;           // holds return values
    WSANETWORKEVENTS NetworkEvents;    // tells us what events happened
    char             MsgText[MSG_LEN]; // holds text strings
    BOOL             ReturnValue;      // holds the return value

    // Find out what happened and act accordingly.
    Result = WSAEnumNetworkEvents(ConnData->Socket, 
                                  ConnData->SocketEventObject,
                                  &NetworkEvents);
    if (Result == SOCKET_ERROR) {
        
        // Handle the fatal error.
        wsprintf(MsgText,
                 "WSAEnumNetworkEvents failed.  Error code: %d", 
                 Result);
        MessageBox(GlobalFrameWindow, MsgText, "Fatal Error",
                   MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;
        
    } else {
        
        // Handle all of the network events on the given socket
        Result = HandleEvents(ConnData, &NetworkEvents);
        
        if (Result == CHAT_CLOSED) {
            
            // HandleEvents() has determined that the remote party
            // has terminated the connection.  Inform the user, and
            // return. 
            if (ConnData->PeerName[0] != 0) {
                wsprintf(MsgText, 
                         "%s @ %s has terminated the connection.", 
                         ConnData->PeerName, ConnData->PeerAddress);
            } else {
                wsprintf(MsgText, 
                         "The party at %s has terminated the connection.",
                         ConnData->PeerAddress);
            }
            MessageBox(ConnData->ConnectionWindow, MsgText, "Sorry!", 
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            ReturnValue = FALSE;
            
        } else if (Result == CHAT_ABORTED) {

            // HandleEvents has determined that the connection has
            // been aborted due to an undetermined error.
            MessageBox(ConnData->ConnectionWindow, 
                       "The connection has been broken.",
                       "Connection aborted.", 
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            ReturnValue = FALSE;
            
        } else if (Result == CHAT_ERROR) {
            
            // HandleEvents() has returned an error.  Inform the
            // user and break to exit the thread and kill the window.
            MessageBox(ConnData->ConnectionWindow,
                       "An unidentified network or system error occurred.",
                       "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            ReturnValue = FALSE;
            
        } else if (Result == CHAT_OK) {
            
            // Inform the caller that everything went fine.
            ReturnValue = TRUE;
            
        
        } else {
            
            // This case should only occur if there is a
            // programming error.  Break out of the while loop to
            // kill the thread, the window, and therefore the
            // connection. 
            MessageBox(ConnData->ConnectionWindow,
                       "HandleEvents() returned an unexpected value.",
                       "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            ReturnValue = FALSE;
        }
    } 
    
    return(ReturnValue);

} // HandleSocketEvent()





BOOL
HandleOutputEvent(
    IN OUT PCONNDATA ConnData)
/*++

Routine Description:

    Handles the case in IOThreadFunc where the thread is woken up by a
    signal to the output event object.

Arguments:

    ConnData - Supplies a pointer to data for the connection on which
    the event happened.
    
Return Value:

    TRUE -- Chat successfully handled the event and the thread should
    continue on.

    FALSE -- An error occurred and Chat should kill the thread and the
    connection. 

--*/
{
    BOOL            ReturnValue; // holds the return value
    BOOL            KeepGoing;   // keep pulling requests off the queue?
    POUTPUT_REQUEST OutReq;      // pointer to the output request
    int             Result;      // holds results of functions

    ReturnValue = TRUE;

    // First we check to see if a previous send attempt failed with
    // WSAEWOULDBLOCK; if so, we don't have to bother trying to send
    // data until we get an FD_WRITE network event, so just return
    // with TRUE to indicate that the thread should wait again.
    if (ConnData->WriteOk) {
        
        // This loop pulls output requests off the queue and hands
        // them to DoSend.
        KeepGoing = TRUE;
        while (KeepGoing) {
            
            OutReq = (POUTPUT_REQUEST)QRemove(ConnData->OutputQueue);
            if (OutReq == NULL) {
                
                // Nothing is left on the queue.
                KeepGoing = FALSE;
                ReturnValue = TRUE;

            } else {

                // Do the output
                Result = DoSend(OutReq, ConnData);
                if (Result == CHAT_WOULD_BLOCK) {
                        
                    // The send would have blocked; we need to
                    // requeue the output request and wait for an
                    // FD_WRITE network event.
                    KeepGoing = FALSE;
                    QInsertAtHead(ConnData->OutputQueue, (LPVOID)OutReq);
                    ReturnValue = TRUE;
                    
                } else if (Result == CHAT_ERROR) {
                        
                    // An error occurred in DoSend.  Set KeepGoing to
                    // FALSE in order to get us out of the loop.
                    KeepGoing = FALSE;
                    ReturnValue = FALSE;
                    MessageBox(ConnData->ConnectionWindow,
                               "Error sending data.",
                               "Error.", 
                               MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                    
                } else {
                    
                    // DoSend returned CHAT_OK.  Keep looping.
                    continue;
                }                        
            }
        } // while (KeepGoing)
    } // if (ConnData->WriteOk)

    return(ReturnValue);

} // HandleOutputEvent()





BOOL
HandleOtherEvent(
    IN     DWORD     WaitStatus,
    IN OUT PCONNDATA ConnData)
/*++

Routine Description:

    Handles the case in IOThreadFunc where the thread is woken up by
    an event that is either an overlapped I/O event or was not
    specified in the event array (which is an error!).

Arguments:

    WaitStatus -- Supplies the value returned by
    WSAWaitForMultipleEvents. 

    ConnData - Supplies a pointer to data for the connection on which
    the event happened.
    
Return Value:

    TRUE -- Chat successfully handled the event and the thread should
    continue on.

    FALSE -- An error occurred and Chat should kill the thread and the
    connection. 

--*/
{
    POUTPUT_REQUEST OutReq;           // points to an output request
    BOOL            ReturnValue;      // holds the return value
    DWORD           Count;            // counting variable
    char            MsgText[MSG_LEN]; // holds message strings

    // First do a sanity check to make sure the index returned is
    // within the bounds of what WE think is the event array. 
    if ((WaitStatus >= WSA_WAIT_EVENT_0) &&
        (WaitStatus <= (WSA_WAIT_EVENT_0  + ConnData->NumEvents - 1))) {

        // Free the data buffer, the overlapped structure, the output
        // request itself, and the event 
        OutReq = ConnData->OutReqArray[WaitStatus - WSA_WAIT_EVENT_0];
        free(OutReq->Buffer.buf);
        free(OutReq->Overlapped);
        free(OutReq);
        CloseHandle(ConnData->EventArray[WaitStatus - WSA_WAIT_EVENT_0]);
        
        // Update all our event and output request arrays to
        // reflect that the overlapped send has completed.
        ConnData->NumEvents--;
        for (Count = (WaitStatus - WSA_WAIT_EVENT_0); 
             Count < ConnData->NumEvents;
             Count++) {
            ConnData->EventArray[Count] = ConnData->EventArray[Count + 1];
            ConnData->OutReqArray[Count] = ConnData->OutReqArray[Count + 1];
        }
        ReturnValue = TRUE;

    } else {
        
        // WSAWaitForMultipleEvents returned an unexpected
        // value... 
        wsprintf(MsgText, "WSAWaitForMultipleEvents() returned %d.",
                 WaitStatus);
        MessageBox(GlobalFrameWindow, MsgText, "Error.", 
                   MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;
    }

    return(ReturnValue);

} // HandleOtherEvent()





void
HandleAcceptMessage(
    IN HWND   ConnectionWindow,
    IN SOCKET Socket,
    IN LPARAM LParam)
/*++

Routine Description:

    Handles the reception of a USMSG_ACCEPT message, which indicates a
    connection attempt is incoming.

Implementation:

    This function lets the user decide whether to accept the
    connection; if he or she does, the function  initializes
    connection-specific data, calls WSAEventSelect to register
    interest in certain network events, and starts a network event
    handling thread.

Arguments:

    ConnectionWindow -- Handle to the connection window associated
    with this connection request.

    Socket -- Contains the handle to the listening socket to which the
    connection request has been made. 

    LParam -- The LParam that was delivered with the USMSG_ACCEPT
    message; contains the error code.

Return Value:

    None.

--*/
{

    int       Error;            // gets error code if necessary
    char      MsgText[MSG_LEN]; // holds message strings
    DWORD     ThreadId;         // needed for CreateThread
    PCONNDATA ConnData;         // connection-specific data
        
    ConnData = GetConnData(ConnectionWindow);

    Error = WSAGETSELECTERROR(LParam);

    // Check to see if there was an error on the connection attempt.
    if (Error) {
        
        // Some kind of error occurred.
        if (Error == WSAENETDOWN) {
            MessageBox(ConnectionWindow,
                       "The network is down!", "Uh-oh", 
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        } else {
            MessageBox(ConnectionWindow,
                       "Unknown error on FD_ACCEPT", "Error",
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        }
        goto Fail;
    }
    
    // Get a pointer to the associated protocol information structure
    // for the socket we are listening on (Socket). This will be the
    // same as the protocol info for the new socket.  Note that we
    // could have allocated a new buffer and called getsockopt with
    // the SO_PROTOCOL_INFO option.  But since we've already allocated
    // the memory, this way is more efficient.
    ConnData->ProtocolInfo = GetProtoFromSocket(Socket);
    assert(ConnData->ProtocolInfo != NULL);
        
    // Accept the connection (this calls AcceptCondFunc, of course,
    // before actually accepting the connection).
    {
        struct sockaddr address;
        int address_len;
       
        ConnData->Socket = WSAAccept(Socket, 
                                     &address, 
                                     &address_len, 
                                     NULL,
                                     (DWORD)NULL);
//         ConnData->Socket = WSAAccept(Socket, 
//                                      NULL, 
//                                      NULL, 
//                                      AcceptCondFunc,
//                                      (DWORD)ConnData);
    }
    
    if (ConnData->Socket == INVALID_SOCKET) {
        
        // WSAAccept failed -- inform the user and that's all.
        Error = WSAGetLastError();
        if (Error != WSAECONNREFUSED) {

            // An unexpected error code.
            wsprintf(MsgText, "WSAAccept failed.  Error code: %d",
                     Error);
            MessageBox(ConnectionWindow, MsgText, "Error",
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);  
            goto Fail;
        
        } else {
            
            // AcceptCondFunc returned CF_REJECT...
            MessageBox(GlobalFrameWindow, 
                       "The connection attempt has been refused.",
                       "Connection refused.", MB_OK | MB_SETFOREGROUND);
            goto Fail;
        }
    } 
    
    // Put Connection in Event Object Notification Mode.
    WSAEventSelect(ConnData->Socket,
                   ConnData->SocketEventObject,
                   FD_READ | FD_WRITE | FD_CLOSE);

    // Determine the maximum message size, if any.
    if (!GetMaxMsgSize(ConnData)) {
        goto Fail;
    }
    
    // Start the I/O thread, and save the thread handle.
    ConnData->IOThreadHandle = 
      CreateThread(NULL, 
                   0, 
                   (LPTHREAD_START_ROUTINE)IOThreadFunc, 
                   ConnData, 
                   0,
                   &ThreadId);
    if (ConnData->IOThreadHandle == NULL) {
        ChatSysError("CreateThread()",
                     "HandleAcceptMessage()",
                     TRUE);
    }

    return;

 Fail:

    DestroyWindow(ConnectionWindow);
    return;

} // HandleAcceptMessage()





void
HandleConnectMessage(
    IN HWND   ConnectionWindow,
    IN LPARAM LParam)
/*++

Routine Description:

    Handles the reception of a USMSG_CONNECT message, which indicates
    a connection attempt is complete (though not necessarily
    successful). 

Implementation:

    As with HandleAcceptMessages, this function initializes
    connection-specific data, calls WSAEventSelect to register
    interest in certain network events, and starts a network event
    handling thread. 

Arguments:

    ConnectionWindow -- Handle to the connection window associated
    with this connection request.

    LParam -- Contains the LParam that was a parameter of the
    USMSG_CONNECT message.

Return Value:

    None.

--*/
{

    PCONNDATA ConnData;               // connection-specific data
    char      TitleText[TITLE_LEN];   // text buffer for window title
    DWORD     ThreadId;               // needed for CreateThread()
    int       Error;                  // holds error codes
    
    Error = WSAGETSELECTERROR(LParam);

    // Check to see if there was an error on the connection attempt.
    if (Error) {

        // Some kind of error occurred.
        if (Error == WSAECONNREFUSED) {

            MessageBox(ConnectionWindow,
                       "Your connection attempt has been refused",
                       "Connection Refused",
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        } else {

            MessageBox(ConnectionWindow,
                       "Couldn't connect",
                       "Error",
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        }

        goto Fail;

    } 
    
    // Connection has been accepted.  Change the title of the
    // connection window to reflect who the user has connected to
    ConnData = GetConnData(ConnectionWindow);
    
    if (ConnData->CalleeBuffer.len != 0) {
        
        // The callee buffer contains the connection-time data
        // sent by the callee. -- a string containing the name of
        // the callee. 
        wsprintf(TitleText, "Connected To: %s @ %s",
                 ConnData->CalleeBuffer.buf, ConnData->PeerAddress);
    } else {
        
        // ConnData->PeerAddress just contains the address that
        // the user typed in before the connection attempt.
        wsprintf(TitleText, "Connected To: %s", ConnData->PeerAddress);
    }
    SetWindowText(ConnectionWindow, TitleText);
    
    // Put Connection in Event Object Notification Mode.
    WSAEventSelect(ConnData->Socket,
                   ConnData->SocketEventObject,
                   FD_READ | FD_WRITE | FD_CLOSE);

    // Determine the maximum message size, if any.
    if (!GetMaxMsgSize(ConnData)) {
        goto Fail;
    }
        
    // Start the I/O thread, and save the thread handle.
    ConnData->IOThreadHandle = 
      CreateThread(NULL, 
                   0, 
                   (LPTHREAD_START_ROUTINE)IOThreadFunc, 
                   ConnData,
                   0, 
                   &ThreadId);
    if (ConnData->IOThreadHandle == NULL) {
        ChatSysError("CreateThread()",
                     "HandleConnectMessage()",
                     TRUE);
    }

    return;
    
 Fail:
    
    DestroyWindow(ConnectionWindow);
    return;
    
} // HandleConnectMessage()





BOOL
GetMaxMsgSize(
    IN OUT PCONNDATA ConnData)
/*++

Routine Description:

    Determines the maximum message size (if any) of a connected
    socket.  The connection must already be established, i.e. the
    socket must be bound to a local address, for this function to
    work.  Fills the correct value into a field of the
    connection-specific data.

Arguments:

    ConnData -- Connection data for a connected socket. 

Return Value:

    TRUE -- The maximum message size was succesfully determined and
    stored in ConnData->MaxMsgSize.

    FALSE -- There was an error calling getsockopt to get the maximum
    message size.

--*/
{
    BOOL ReturnValue = TRUE;       // return value
    int  DwordLen = sizeof(DWORD); // sizeof a DWORD!
    int  Error;                    // return value of getsockopt

    if ((ConnData->ProtocolInfo->dwMessageSize == 0) ||
        (ConnData->ProtocolInfo->dwMessageSize == 0xffffffff)) {
        
        // Either the protocol isn't message-oriented, or there is no
        // maximum message size.
        ConnData->MaxMsgSize = NO_MAX_MSG_SIZE;
        
    } else {
        
        // There is a maximum message size.  Note it.
        if (ConnData->MaxMsgSize == 0x1) {

            // The actual maximum message size was not stored in the
            // protocol information structure -- rather, we need to
            // get it using getsockopt().
            Error = getsockopt(ConnData->Socket,
                               SOL_SOCKET,
                               SO_MAX_MSG_SIZE,
                               (char *)&ConnData->MaxMsgSize,
                               &DwordLen);
            if (Error) {
                ReturnValue = FALSE;
            }

        } // if (ConnData->MaxMsgSize == 0x1)
        else {
            
            // The message size is stored in the protocol information
            // structure.  Use it.
            ConnData->MaxMsgSize = ConnData->ProtocolInfo->dwMessageSize;
        }
            

    } // else
    
    return(ReturnValue);
    
} // GetMaxMsgSize()





BOOL
IsSendable(
    char Char)
/*++

Routine Description:

    Determines whether a certain character value is ok to send over
    the socket to the far end.  Eliminates most non-printable
    characters except for newline and backspace.

Arguments:

    Char -- Supplies the character to be tested.

Return Value:

    TRUE -- The character should be sent as is.

    FALSE -- The character should not be sent.

--*/
{

    if (isprint(Char) || (Char == '\b') || (Char == '\r')) {
        return(TRUE);
    } else {
        return(FALSE);
    }

} // IsSendable()





int
DoRecv(
    IN PCONNDATA ConnData)

/*++

Routine Description:

    Receives as much available data as possible up to the size of the
    receive buffer-1 (BUFFER_LENGTH-1).  A single byte is reserved at
    the end of the receive buffer to append a terminating NULL.

Arguments:

    ConnData -- Points to the data for the connection that is ready to
    receive data.

Return Value:

    CHAT_OK -- Received data was successfully sent to the receive edit
           control.

    CHAT_ERROR -- Error receiving data.

    CHAT_CLOSED -- The socket was gracefully closed.

--*/

{
    DWORD  NumBytes;               // stores how many bytes we received
    int    Error;                  // gets error values
    int    Result;                 // gets return value from WSARecv
    char   Buf[BUFFER_LENGTH];     // buffer to receive data
    int    ReturnValue = CHAT_OK;  // returnValue
    WSABUF RecvBuffer;             // WSABuf to pass to WSARecv
    DWORD  Flags = 0;              // flags for WSARecv
    
    RecvBuffer.buf = Buf;
    RecvBuffer.len = BUFFER_LENGTH - 1;

    // Do the receive
    Result = WSARecv(ConnData->Socket,
                     &RecvBuffer,
                     1,
                     &NumBytes,
                     &Flags,
                     NULL,
                     NULL);

    // Check for errors.
    if (Result == SOCKET_ERROR) {

        Error = WSAGetLastError();
        
        switch (Error) {

        case WSAENETRESET:  // flow through
        case WSAECONNRESET:

            // The remote party has reset the connection.
            ReturnValue = CHAT_CLOSED;
            goto Done;

        case WSAEWOULDBLOCK:
            
            // No data received; return to wait for another read event.
            ReturnValue = CHAT_OK;

        default:

            // Some other error...hit the panic button.
            ReturnValue = CHAT_ERROR;
            goto Done;
        }

    }

    // Append a NULL to the text buffer.
    RecvBuffer.buf[NumBytes] = '\0';

    // Output the received text into the receive edit control.
    OutputString(ConnData->RecvWindow, RecvBuffer.buf);
 
 Done:
    return(ReturnValue);

} // DoRecv()





int
DoOverlappedCallbackSend(
    IN POUTPUT_REQUEST OutReq,
    IN PCONNDATA       ConnData)

/*++

Routine Description:

    Sends a buffer of data over a connected socket using overlapped
    I/O with callback function completion notification.

Implementation:

    This function just does an overlapped send, giving WinSock 2 a
    completion function which will be called when the operation has
    finished, and within which cleanup will occur.

Arguments:

    OutReq -- Points to a OUTPUT_REQUEST structure, which specifies an
    output request.

    ConnData -- Supplies a pointer to a CONNDATA structure, which
    identifies a Chat connection. 

Return Value:

    CHAT_WOULD_BLOCK -- The send operation failed with WSAEWOULDBLOCK.

    CHAT_ERROR -- A unexpected error occurred.
    
    CHAT_OK -- The send was successfully initiated. WinSock 2 will
    execute the callback function when the send has completed.

--*/

{
    int             Size = 0;      // how many bytes we send
    int             Error;         // return value of WSASend
    int             Errno;         // result of WSAGetLastError
    LPWSABUF        Buffers;       // points to an array of WSABUFs
    DWORD           BytesSent;     // needed in WSASend
    int             ReturnValue;   // the return value

    ReturnValue = CHAT_OK;
        
    // Allocate an OVERLAPPED structure.
    OutReq->Overlapped = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
    if (OutReq->Overlapped == NULL) {
        ChatSysError("malloc()",
                     "DoOverlappedCallbackSend()",
                     TRUE);
    }
    Buffers = &OutReq->Buffer;
    
    // The hEvent field of the Overlapped structure is not used
    // for callback notification...thus we can use it any way we
    // want to pass information to the callback routine.  We use
    // it to store a pointer to the OutReq associated with this
    // overlapped send operation, so the callback function can
    // free the memory.
    OutReq->Overlapped->hEvent = (WSAEVENT)OutReq;
    
    Error = WSASend(ConnData->Socket, 
                    Buffers, 
                    1, 
                    &BytesSent, 
                    0, 
                    OutReq->Overlapped,
                    SendCompFunc);
    
    if (Error == SOCKET_ERROR) {
        
        // There is an error...
        Errno = WSAGetLastError();
        if (Errno == WSAEWOULDBLOCK) {
            
            // WSAEWOULDBLOCK means we have to wait for an FD_WRITE
            // before we can send. 
            ConnData->WriteOk = FALSE;
            ReturnValue = CHAT_WOULD_BLOCK;
            
        } else if (Errno == WSA_IO_PENDING) {
            
            // Overlapped send successfully initiated.
            ReturnValue = CHAT_OK;
        } 
        else {
            
            // An unexpected error occurred. 
            ReturnValue = CHAT_ERROR;
        }
        
    }

    // No error -- the I/O request was completed immediately...
    return(ReturnValue);

} // DoOverlappedCallbackSend()





int
DoOverlappedEventSend(
    IN POUTPUT_REQUEST OutReq,
    IN PCONNDATA       ConnData)

/*++

Routine Description:

    Sends a buffer of data over a connected socket using overlapped
    I/O with event notification.

Implementation:

    If the overlapped send is succesfully initiated, this function
    increases the event count and adds an entry to the event array of
    the associated connection.  During IOThreadFunc, the thread will
    thus wait on the new event as well, and will perform the
    appropriate cleanup action when awoken.

Arguments:

    OutReq -- Points to a OUTPUT_REQUEST structure, which specifies an
    output request.

    ConnData -- Supplies a pointer to a CONNDATA structure, which
    identifies a Chat connection. 

Return Value:

    CHAT_WOULD_BLOCK -- The send operation failed with WSAEWOULDBLOCK.

    CHAT_ERROR -- A unexpected error occurred.
    
    CHAT_OK -- The send was successfully initiated and the new event
    placed in the event array; it will be signaled by WinSock 2 when
    the send operation has completed.

--*/

{
    int             Size = 0;      // how many bytes we send
    int             Error;         // return value of WSASend
    int             Errno;         // result of WSAGetLastError
    LPWSABUF        Buffers;       // points to an array of WSABUFs
    DWORD           BytesSent;     // needed in WSASend
    int             ReturnValue;   // the return value

    ReturnValue = CHAT_OK;
        
    // Allocate an OVERLAPPED structure.
    
    // Note that the pointer to an overlapped structure is kept in
    // OutReq.  We use an array of OutReq pointers that is parallel to
    // the event array.  With that one pointer, then, we can find the
    // associated Overlapped structure and free it, and then free the
    // OutReq structure itself. 
    OutReq->Overlapped = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
    if (OutReq->Overlapped == NULL) {
        ChatSysError("malloc()",
                     "DoOverlappedEventSend()",
                     TRUE);
    }
    Buffers = &OutReq->Buffer;
    
    OutReq->Overlapped->hEvent = 
      (WSAEVENT)CreateEvent(NULL,  
                            FALSE, 
                            FALSE, 
                            NULL); 
    if (OutReq->Overlapped->hEvent == NULL) {
        ChatSysError("CreateEvent()",
                     "DoOverlappedEventSend()",
                     TRUE);
    }

    // Do the send.
    Error = WSASend(ConnData->Socket, 
                    Buffers, 
                    1, 
                    &BytesSent, 
                    0, 
                    OutReq->Overlapped,
                    NULL);
        
    if (Error == SOCKET_ERROR) {

        // There is an error...
        Errno = WSAGetLastError();
        if (Errno == WSAEWOULDBLOCK) {
            
            // WSAEWOULDBLOCK means we have to wait for an FD_WRITE
            // before we can send. 
            ConnData->WriteOk = FALSE;
            ReturnValue = CHAT_WOULD_BLOCK;
            
        } else if (Errno == WSA_IO_PENDING) {
            
            // Overlapped send successfully initiated.
            // Increase the event count, and update the event and
            // output request arrays to hold the entries for this
            // overlapped send.
            ConnData->NumEvents++;
            if (ConnData->NumEvents > WSA_MAXIMUM_WAIT_EVENTS) {
                
                // We're trying to wait on too many events at
                // once.  It's very unlikely this could happen --
                // you'd have to get about 62 overlapped sends, in
                // row, before one of them is completed and
                // signaled.  So just return CHAT_ERROR which will
                // break the connection.
                ReturnValue = CHAT_ERROR;
                
            } else {
                    
                ConnData->EventArray[ConnData->NumEvents - 1] = 
                  OutReq->Overlapped->hEvent;
                
                ConnData->OutReqArray[ConnData->NumEvents - 1] = OutReq;
                ReturnValue = CHAT_OK;
            }
        } // else if (...)
        else {
            
            // An unexpected error occurred. 
            ReturnValue = CHAT_ERROR;
        }
        
    } // if (Error == SOCKET_ERROR)

    // No error -- the I/O request was completed immediately...
    ReturnValue = CHAT_OK;
    
    return(ReturnValue);
    
} // DoOverlappedEventSend()





int
DoSend(
    IN POUTPUT_REQUEST OutReq,
    IN PCONNDATA       ConnData)

/*++

Routine Description:

    Sends a buffer of data over a connected socket.

Implementation:

    This function uses the information contained in the output request
    structure to determine whether to use overlapped or non-overlapped
    I/O.  If using overlapped I/O, it calls an appropriate function
    depending on whether the symbol CALLBACK_NOTIFICATION has been
    defined. Otherwise, the non-overlapped send in performed
    immediately.  

Arguments:

    OutReq -- Points to a OUTPUT_REQUEST structure, which specifies an
    output request.

    ConnData -- Supplies a pointer to a CONNDATA structure, which
    identifies a Chat connection. 

Return Value:

    CHAT_WOULD_BLOCK -- The send operation failed with WSAEWOULDBLOCK.

    CHAT_ERROR -- A unexpected error occurred.
    
    CHAT_OK -- The send was successfully initiated.  If the send was
    overlapped, either a callback function is forthcoming or another
    event was placed in the ConnData->EventArray for IOThreadFunc to
    wait on until it's signaled by WinSock 2.

--*/

{
    int             Size = 0;      // how many bytes we send
    int             Error;         // return value of WSASend
    int             Errno;         // result of WSAGetLastError
    WSABUF          Buffers[1];    // points to the data to be sent
    DWORD           BytesSent;     // needed in WSASend
    int             ReturnValue;   // the return value
        
    if (OutReq->Type == NON_OVERLAPPED_IO) {

        // Do a non-overlapped send by setting lpOverlapped and
        // lpCompletionRoutine to NULL.
        Buffers[0].len = OutReq->Buffer.len;
        Buffers[0].buf = OutReq->Buffer.buf;
        Error = WSASend(ConnData->Socket, 
                        Buffers, 
                        1, 
                        &BytesSent, 
                        0, 
                        NULL,
                        NULL);
                
        if (Error == SOCKET_ERROR) {

            // There's an error...
            Errno = WSAGetLastError();
            if (Errno == WSAEWOULDBLOCK) {

                //  WSAEWOULDBLOCK means we have to wait for an FD_WRITE
                //  before we can send.
                ConnData->WriteOk = FALSE;
                ReturnValue = CHAT_WOULD_BLOCK;

            } else {
                
                // It's an unexpected error.
                ReturnValue = CHAT_ERROR;
            }
        } else {

            // There's no error.
            ReturnValue = CHAT_OK;
            free(OutReq);
        }
        
    } else if (OutReq->Type == OVERLAPPED_IO) {
        
#ifdef CALLBACK_NOTIFICATION
        
        ReturnValue = DoOverlappedCallbackSend(OutReq, ConnData);
        
#else
        
        ReturnValue = DoOverlappedEventSend(OutReq, ConnData);

#endif
        
    } else {

        // Unknown output type...
        MessageBox(ConnData->ConnectionWindow,
                   "Unknown output type given to IOThread. Aborting.",
                   "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = CHAT_ERROR;
    } 

    return(ReturnValue);

} // DoSend()





void CALLBACK
SendCompFunc(
    IN DWORD           Error,
    IN DWORD           BytesTransferred,
    IN LPWSAOVERLAPPED Overlapped,
    IN DWORD           Flags)
/*++

Routine Description:

    Completion routine called after a successfully initiated overlapped
    send operation completes; cleans up data structures associated
    with the send.

Arguments:

    Error -- Supplies the completion status for the overlapped
    operation. 

    BytesTransferred -- Supplies the actual number of bytes sent.

    Overlapped -- Supplies a pointer to a WSAOVERLAPPED structure.
    The hEvent field in the structure contains a pointer to the
    OUTPUT_REQUEST structure associated with this send operation.

    Flags -- Not yet used.
    
Return Value:

    None

--*/

{
    POUTPUT_REQUEST OutReq; // The output request
    
    OutReq = (POUTPUT_REQUEST)(Overlapped->hEvent);
        
    if (Error) {
        MessageBox(NULL,
                   "Error during overlapped send.",
                   "Error",
                   MB_OK | MB_SETFOREGROUND);
    }

    // Free the data buffer, the output request structure,  and the
    // overlapped I/O structure. 
    free(OutReq->Buffer.buf);
    free(OutReq);
    free(Overlapped);
    
} // SendCompFunc()





PCONNDATA
GetConnData(
    IN HWND ConnectionWindow
    )

/*++

Routine Description:

    Retrieves a pointer to the PCONNDATA data structure associated
    with a connection window

Arguments:

    ConnectionWindow - Supplies the handle to a connection window.

Return Value:

    Returns the PCONNDATA associated with the given connection window 
    handle.

--*/

{
    return((PCONNDATA)GetWindowLong(ConnectionWindow, GWL_CONNINFO));
    
} // GetConnData()





BOOL
MakeConnection(
    IN HWND ConnectionWindow)

/*++

Routine Description:

    Initiates a call to another instance of chat.
    1) Creates a socket
    2) Set the socket up for windows message notification of FD_CONNECT
       events.
    3) Allocate a buffer for caller name & subject, and fill it in.
    4) Allocate a buffer to receive callee name.
    5) Set up quality of service for the connection.
    6) Attempt a connection

Arguments:

    ConnectionWindow - Handle to the window that receives
    notification when an FD_CONNECT event occurs. 

Return Value:

    TRUE - A connection attempt was successfully initiated.

    FALSE - Error occurred while attempting to initiate a connection.

--*/

{
    int             ConnectStatus;    // the return value of WSAConnect
    int             Error;            // gets error values
    BOOL            ReturnValue;      // holds the return value
    PCONNDATA       ConnData;         // connection-specific data
    LPWSABUF        CallerBuffer;     // user data we will send
    LPWSABUF        CalleeBuffer;     // user data we will receive
    QOS             QualityOfService; // QOS structure, used by WSAConnect
    LPFLOWSPEC      FlowSpec;         // dummy pointer for code readability
    char            MsgText[MSG_LEN]; // holds message strings
    BOOL            QOSSupported;     // does the protocol support QOS?
    BOOL            CTDTSupported;    // support for conn-time data xfer?
    struct sockaddr *SockAddr;        // socket address for WSAConnect
    int             SockAddrLen;      // the length of the above
    
    ReturnValue = TRUE;
    ConnData = GetConnData(ConnectionWindow);
    QOSSupported = (ConnData->ProtocolInfo->dwServiceFlags1 & 
                    XP1_QOS_SUPPORTED);
    CTDTSupported = (ConnData->ProtocolInfo->dwServiceFlags1 & 
                      XP1_CONNECT_DATA);
    if (CTDTSupported) {
        
        // CallerBuffer and CallerBuffer->buf (allocated within the
        // NameAndSubject dialog box procedure) both get freed by the
        // end of this function.  CalleeBuffer.buf gets freed upon
        // connection shutdown -- it needs to be valid after this
        // function has exited.
        CallerBuffer = (LPWSABUF)malloc(sizeof(WSABUF));
        if (CallerBuffer == NULL) {
            ChatSysError("malloc()",
                         "MakeConnection()",
                         TRUE);
        }
        CalleeBuffer = &ConnData->CalleeBuffer;

        // The connection-time data sent back when/if this connection
        // is accepted will just contain the callee's name.
        CalleeBuffer->len = NAME_LEN + 1;
        CalleeBuffer->buf = (char *)malloc(CalleeBuffer->len);
        if (CallerBuffer->buf == NULL) {
            ChatSysError("malloc()",
                         "MakeConnection()",
                         TRUE);
        }
        
        // Prompt the user for his name and the subject of the call.
        // a pointer to the CallerBuffer is passed to the dialog box
        // procedure and the data is packed into this memory.
        if (!DialogBoxParam(GlobalInstance, 
                            "NameAndSubjectDlg", 
                            ConnectionWindow, 
                            NameAndSubjectDlgProc,
                            (LPARAM)CallerBuffer)) {
            ReturnValue = FALSE;
            goto Done;
        }
    } // if (CTDTSupported)

    // Create a socket for this connection.
    ConnData->Socket = WSASocket(0, 
                                 0, 
                                 0, 
                                 ConnData->ProtocolInfo, 
                                 0,
                                 WSA_FLAG_OVERLAPPED);
    
    if (ConnData->Socket == INVALID_SOCKET) {

        MessageBox(ConnectionWindow, "Could not open a socket.", "Error", 
                   MB_OK | MB_SETFOREGROUND);
        ReturnValue = FALSE;
        goto Done;

    } else {

        // Set up socket for windows message event notification.
        WSAAsyncSelect(ConnData->Socket, 
                       ConnectionWindow, 
                       USMSG_CONNECT, 
                       FD_CONNECT);
    }
    
    if (QOSSupported) {

        // Set  up  quality  of  service  info  we  want.   Do  not include any
        // provider-specific information.
        QualityOfService.ProviderSpecific.len = 0;
        QualityOfService.ProviderSpecific.buf = NULL;
        
        // FlowSpec is used only to make this code more readable.
        FlowSpec = & QualityOfService.SendingFlowspec;
        FlowSpec->TokenRate = TOKENRATE;
        FlowSpec->TokenBucketSize = QOS_UNSPECIFIED;
        FlowSpec->PeakBandwidth = QOS_UNSPECIFIED;
        FlowSpec->Latency = QOS_UNSPECIFIED;
        FlowSpec->DelayVariation = QOS_UNSPECIFIED;
        FlowSpec->LevelOfGuarantee = BestEffortService;
        FlowSpec->CostOfCall = 0;
        FlowSpec->NetworkAvailability = 1;
            
        // again, the extra FlowSpec variable is for readability
        FlowSpec = & QualityOfService.ReceivingFlowspec;
        FlowSpec->TokenRate = TOKENRATE;
        FlowSpec->TokenBucketSize = QOS_UNSPECIFIED;
        FlowSpec->PeakBandwidth = QOS_UNSPECIFIED;
        FlowSpec->Latency = QOS_UNSPECIFIED;
        FlowSpec->DelayVariation = QOS_UNSPECIFIED;
        FlowSpec->LevelOfGuarantee = BestEffortService;
        FlowSpec->CostOfCall = 0;
        FlowSpec->NetworkAvailability = 1;
    }

    // Reminder: WinSock 2 requires that we pass a struct sockaddr *
    // to WSAConnect; however, the service provider is free to
    // interpret the pointer as an arbitrary chunk of data of size
    // SockAddrLen. 
    SockAddr = (struct sockaddr *)ConnData->RemoteSockAddr.buf;
    SockAddrLen = ConnData->RemoteSockAddr.len;

    // Try to connect; depending on whether QOS or Connection Time
    // Data Transfer are supported, pass the appropriate data.
    if (QOSSupported && CTDTSupported) {
        
        // Both are supported...
        ConnectStatus = WSAConnect(ConnData->Socket,
                                   SockAddr,
                                   SockAddrLen,
                                   CallerBuffer,
                                   CalleeBuffer,
                                   &QualityOfService,
                                   NULL);
    } else if (!QOSSupported && CTDTSupported) {
        
        // Only CTDT is supported...
        ConnectStatus = WSAConnect(ConnData->Socket,
                                   SockAddr,
                                   SockAddrLen,
                                   CallerBuffer,
                                   CalleeBuffer,
                                   NULL,
                                   NULL);
        
    } else if (QOSSupported && !CTDTSupported) {
        
        // Only QOS is supported...
        ConnectStatus = WSAConnect(ConnData->Socket,
                                   SockAddr,
                                   SockAddrLen,
                                   NULL,
                                   NULL,
                                   &QualityOfService,
                                   NULL);
        
    } else {
        
        // Neither is supported...
        ConnectStatus = WSAConnect(ConnData->Socket,
                                   SockAddr,
                                   SockAddrLen,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
    }

    // Check for errors.
    if (ConnectStatus == SOCKET_ERROR) {

        Error = WSAGetLastError();
        if (Error != WSAEWOULDBLOCK) {
            wsprintf(MsgText, "WSAConnect failed.  Error code: %d",
                     Error);
            MessageBox(ConnectionWindow, MsgText, "Error", 
                       MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            ReturnValue = FALSE;
        }      
      
    } else {

        MessageBox(ConnectionWindow, 
                   "WSAConnect should have returned SOCKET_ERROR.",
                   "Error", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;
    }

 Done:
    
    // Free allocated memory, except CalleeBuffer which will be
    // freed when connection window is destroyed.
    if (CTDTSupported) {
        free(CallerBuffer->buf);
        free(CallerBuffer);
    }
    return(ReturnValue);

} // MakeConnection()





BOOL
FillInFamilies(
    IN HWND  DialogWindow,
    IN DWORD FamilyLB)
/*++

Routine Description:

    Fills in the listbox with a string for each protocol on which Chat
    has a listening socket.  

Implementation:

    For each socket in the ListeningSockets array, this function:

        1. Gets the associated protocol information structure. 
        
        2. If Chat recognizes the address family, it prints it in a
        string; if not, it prints "unknown".

        3. In the second half of the string, prints out the
        human-readable string contained in the protocol information.

        4. Sends the string to be an entry in the listbox identified
        by FamilyLB.

Arguments:

    DialogWindow -- Window handle for the dialog box.

    FamilyLB -- Integer identifier for a listbox in the dialog box. 

Return Value:

    TRUE - All messages were successfully sent to the listbox.

    FALSE - An LB_ADDSTRING message failed.

--*/

{

    int     i;                 // counting variable
    LRESULT Result;            // result of SendMessage calls
    char    LBString[MSG_LEN]; // string to send to the listbox
    int     Offset;             // index into the string

    // Iterate through all members of the ListeningSockets array.
    for (i = 0; i < NumFound; i++){

		// We are prepending a number to the list box string to be displayed
		// since the list box does us the favor of alphabetizing the 
		// the sting and we are depending on the list entries appearing in the
        // list box in the order that we added them to the list.

		Offset = wsprintf(LBString, "%03i",i);
         
        switch (ListeningSockets[i].ProtocolInfo->iAddressFamily) {
           
        case AF_INET:
            
            Offset += wsprintf(LBString+ Offset, "AF_INET/");
            wsprintf(LBString + Offset, "%s", 
                     ListeningSockets[i].ProtocolInfo->szProtocol);
            Result = SendMessage(GetDlgItem(DialogWindow, FamilyLB),
                                 LB_ADDSTRING, 0, (LPARAM)LBString);
            break;

        default:
            
            Offset += wsprintf(LBString+ Offset, "(unknown)/");
            wsprintf(LBString + Offset, "%s",
                     ListeningSockets[i].ProtocolInfo->szProtocol);
            Result = SendMessage(GetDlgItem(DialogWindow, FamilyLB),
                                 LB_ADDSTRING, 0, (LPARAM)LBString);
            break;

        }
        if ((Result == LB_ERR) || (Result == LB_ERRSPACE)) {
            return(FALSE);
        }
    }
    return(TRUE);

} // FillInFamilies()





LPWSAPROTOCOL_INFO
GetProtoFromIndex(
    IN int LBIndex)
/*++

Routine Description:

    Takes an index into the ChooseFamily dialog box and returns the
    protocol associated with that index. This works because when the
    dialog box is set up, the strings representing the protocols are
    added to the listbox in the order they fall in ListeningSockets. 

Arguments:

    LBIndex -- The index of the user's selection.

Return Value:

    NULL -- The index doesn't correspond to an element in the
    ListeningSockets array.

    LPWSAPROTOCOL_INFO -- A pointer to the protocol information struct
    corresponding to LBIndex.

--*/
{
    if (LBIndex > (NumFound - 1)) {
        return(NULL);
    }    
    return(ListeningSockets[LBIndex].ProtocolInfo);

} // GetProtoFromIndex()





LPWSAPROTOCOL_INFO
GetProtoFromSocket(
    IN SOCKET Socket)
/*++

Routine Description:

    Searches the ListeningSockets array for the first entry with a
    Socket field equal to the Socket parameter.  When found, it
    returns a pointer to the associated protocol information
    structure.  If not found, return NULL.

Arguments:

    Socket -- The socket who's associated WSAPROTOCOL_INFO struct we are
    looking for. 

Return Value:

    NULL -- The socket was not found in the ListeningSockets array.

    LPWSAPROTOCOL_INFO -- A pointer to the protocol information struct
    for the socket.

--*/
{

    int i; // counting variable

    for (i = 0; i < NumFound; i++) {
        if (ListeningSockets[i].Socket == Socket) {
            return(ListeningSockets[i].ProtocolInfo);
        }
    }
    return(NULL);

} // GetProtoFromSocket()





void
CleanupConnection(
    IN PCONNDATA ConnData)
/*++

Routine Description:

    Frees all memory and objects allocated for this connection.

Arguments:

    ConnData -- Pointer to the connection-specific data structure.

Return Value:

    None.

--*/
{

    // Clean up connection-specific data.  To keep this code
    // readable, we ignore any errors.
    if (ConnData->SocketEventObject != NULL) {
        CloseHandle(ConnData->SocketEventObject);
        ConnData->SocketEventObject = NULL;
    }
    if (ConnData->OutputEventObject != NULL) {
        CloseHandle(ConnData->SocketEventObject);
        ConnData->SocketEventObject = NULL;
    }
    if (ConnData->OutputQueue != NULL) {
        QFree(ConnData->OutputQueue);
        ConnData->OutputQueue = NULL;
    }
    if (ConnData->IOThreadHandle != NULL) {
        CloseHandle(ConnData->IOThreadHandle);
        ConnData->IOThreadHandle = NULL;
    }
    if (ConnData->Socket != INVALID_SOCKET) {
        closesocket(ConnData->Socket);
    }
    if (ConnData->RemoteSockAddr.buf != NULL) {
        free(ConnData->RemoteSockAddr.buf);
        ConnData->RemoteSockAddr.buf = NULL;
    }
    if (ConnData->CalleeBuffer.buf != NULL) {
        free(ConnData->CalleeBuffer.buf);
        ConnData->CalleeBuffer.buf = NULL;
    }
    free(ConnData);

} // CleanupConnection()
