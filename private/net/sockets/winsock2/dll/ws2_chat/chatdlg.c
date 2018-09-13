/*++

Copyright (c) 1995 Intel Corp

Module Name:

    dialog.c

Abstract:

    Contains dialog-box procedures for the WinSock2 Chat sample
    application.  See ws2_chat.rc for the actual resource script.

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





BOOL APIENTRY
ChooseFamilyDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WParam,
    IN LPARAM LParam)
/*++

Routine Description:

    Callback function that processes messages sent to the 'Choose
    Family' dialog box.

Implementation 

    This dialog box is used to query the user as to which supported
    protocol he wishes to use for the chat connection he wishes to
    make.  The available protocols are listed in the IDC_FAM_LB
    listbox via the FillInFamilies function in socket.c.

    It is expected that this dialog box gets the WM_INITDIALOG
    message with a LParam that is a pointer to the CONNDATA structure
    for the impending connection.  The result of this function is that
    the ProtocolInfo field of the ConnData struct is filled with a
    pointer to the protocol information structure the user chose in
    the listbox.  

Arguments:

    DialogWindow - Supplies handle of the dialog box.

    Message - Supplies the message identifier

    WParam - Supplies the first message parameter

    LParam - Supplies the second message parameter

Return Value:

    TRUE -- This function handled the message.

    FALSE -- This function did not handle the message.

--*/
{
    int              LBIndex;         // index of the item chosen
    static PCONNDATA ConnData = NULL; // connection-specific data

    switch (Message) {

    case WM_INITDIALOG:
        
        // Get the parameter and fill in the listbox with the
        // available protocols.
        ConnData = (PCONNDATA)LParam;
        assert(ConnData != NULL);
        if (!FillInFamilies(DialogWindow, IDC_FAM_LB)) {
            MessageBox(DialogWindow, "FillInFamilies failed.", "Error.",
                        MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            EndDialog(DialogWindow, FALSE);
        }
        
        return(TRUE);

    case WM_COMMAND:

        switch (WParam) {
            
        case IDOK:
            
            // Get the index of the listbox item the user has chosen.
            LBIndex = (int)SendMessage(GetDlgItem(DialogWindow, IDC_FAM_LB),
                                  LB_GETCURSEL, 0, 0);
            if (LBIndex == LB_ERR) {

                // Nothing was selected.
                MessageBox(DialogWindow, "Choose an item, or hit cancel.",
                           "No Selection?", 
                           MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                return(TRUE);
            }
            
            // Get the protocol associated with the listbox item the
            // user selected.
            ConnData->ProtocolInfo = GetProtoFromIndex(LBIndex);
            assert(ConnData->ProtocolInfo != NULL);
            EndDialog(DialogWindow, TRUE);
            return(TRUE);
                        
        case IDCANCEL:

            // Kill the dialog box.
            EndDialog(DialogWindow, FALSE);
            return(TRUE);
                        
        default:

            return(FALSE);
        } // switch (WParam)
                
    default:

        return(FALSE);
    } // switch (Message)

} // ChooseFamilyDlgProc()





BOOL APIENTRY
InetConnDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WParam,
    IN LPARAM LParam)

/*++

Routine Description:

    Callback function that processes messages sent to the internet
    connection info dialog box.

Implementation

    This dialog box gets two strings from the user: the internet
    address and port to which she wants to make a chat connection.
    When the user clicks on the OK button, these two strings are
    converted into a sockaddr_in structure and a pointer to this
    structure is saved in the connection window's CONNDATA structure.

    NOTE: No name resolution takes place here!  The address edit
    control expects up to 15 characters, and this string is expected
    to be in.dotted.decimal.notation.  When RNR is available, this is
    where it would be called upon to provide the appropriate
    sockaddr_in structure.  Actually, this dialog box would be
    replaced with a protocol independent name dialog box; RNR would do
    all the rest...

Arguments:

    DialogWindow - Supplies handle of the dialog box.

    Message - Supplies the message identifier

    WParam - Supplies the first message parameter

    LParam - Supplies the second message parameter

Return Value:

    TRUE -- This function handled the message.

    FALSE -- This function did not handle the message; or the message
    was WM_INITDIALOG and we set the focus.

--*/

{
    char               PortText[INET_PORT_LEN + 1];   // holds port string
    char               AddressText[INET_ADDR_LEN + 1];// holds addr string
    static PCONNDATA   ConnData = NULL;               // conn-specific data
    struct sockaddr_in *SockAddrInet;                 // Inet socket address
    u_long             InetAddr;                      // Inet-style address
    u_short            InetPort;                      // Inet-style port
    int                Port;                          // intermediate var.
    BOOL               PortStringTranslated;          // for GetDlgItemInt
    struct hostent *host;

    switch (Message) {

    case WM_INITDIALOG:

        // Get the connection data pointer, initialize the dialog box.
        ConnData = (PCONNDATA)LParam;
        assert(ConnData != NULL);
        SendMessage(GetDlgItem(DialogWindow, IDC_INET_ADDRESS), 
                    EM_LIMITTEXT, (WPARAM)INET_ADDR_LEN, 0);
        SendMessage(GetDlgItem(DialogWindow, IDC_INET_PORT), 
                    EM_LIMITTEXT, (WPARAM)INET_PORT_LEN, 0);
        wsprintf(PortText,"%d", INET_DEFAULT_PORT);
        SendMessage(GetDlgItem(DialogWindow, IDC_INET_PORT),
                    WM_SETTEXT, 0, (LPARAM)PortText);
        SetFocus(GetDlgItem(DialogWindow, IDC_INET_ADDRESS));
        return(FALSE);
        
    case WM_COMMAND:

        switch (WParam) {

        case IDOK:
            
            // The user has pressed the 'OK' button.  Extract the
            // internet address into a buffer and check the value the
            // user typed for the port.
            GetDlgItemText(DialogWindow, 
                           IDC_INET_ADDRESS, 
                           AddressText, 
                           INET_ADDR_LEN);
            strcpy(ConnData->PeerAddress, AddressText);
            Port = (int)GetDlgItemInt(DialogWindow, 
                                      IDC_INET_PORT, 
                                      &PortStringTranslated, 
                                      TRUE);
            if ((Port < 0) || (Port > 65535) || !PortStringTranslated) {
              MessageBox(DialogWindow,
                         "Please choose a port between 0 and 65535.",
                         "Bad Port.",
                         MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                return(TRUE);
            }
                        
            // Convert the address string to an unsigned long, and
            // convert the port integer to an unsigned short.  If all
            // goes well, fill in the fields and end the dialog box.
            // If not, inform the user and *don't* kill the dialog box.
            InetAddr = inet_addr(AddressText);
            if (InetAddr == INADDR_NONE) {
                host = gethostbyname(AddressText);
                if (host) {
                    InetAddr = *((u_long *)host->h_addr_list[0]);
                }
            }
            InetPort = (u_short)Port;
            if ((InetAddr != INADDR_NONE) && (InetAddr != 0)) {

                // Allocate memory for an internet-style socket address;
                // point ConnData->SockAddr at this memory, and
                // use a local pointer to a struct sockaddr_in to
                // reference the fields. 
                ConnData->RemoteSockAddr.len = sizeof(struct sockaddr_in);
                ConnData->RemoteSockAddr.buf = 
                  malloc(ConnData->RemoteSockAddr.len);
                if (ConnData->RemoteSockAddr.buf == NULL) {
                    ChatSysError("malloc()",
                                 "InetConnDlgProc()",
                                 TRUE);
                }

                SockAddrInet = 
                  (struct sockaddr_in *)ConnData->RemoteSockAddr.buf;
                SockAddrInet->sin_family = AF_INET;
                SockAddrInet->sin_port = InetPort;
                SockAddrInet->sin_addr.S_un.S_addr = InetAddr;
                EndDialog(DialogWindow, TRUE);
                return(TRUE);

            } 
            
            MessageBox(DialogWindow, 
                       "Invalid Internet address. Try again or cancel.",
                       "Error", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            return(TRUE);
            
        case IDCANCEL:
                        
            // The user has pressed cancel.  Kill the dialog box.
            EndDialog(DialogWindow, FALSE);
            return(TRUE);
            
        default:

            break;
        } // switch (WParam)

        break;
    } // switch (Message)

    return(FALSE);
} // InetConnDlgProc()





BOOL APIENTRY
DefaultConnDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WParam,
    IN LPARAM LParam)
/*++

Routine Description:

    Callback function that processes messages sent to the default
    connection info dialog box.

Implementation

    This dialog box is the default when chat doesn't recognize a
    particular address family.  The user enters actual hex digits and
    the hex value is stored in the SocketAddress associated with the
    connection. Obviously, this requires the user to know how to
    interpret the structure of the socket address for this address
    family.  

Arguments:

    DialogWindow - Supplies handle of the dialog box.

    Message - Supplies the message identifier

    WParam - Supplies the first message parameter

    LParam - Supplies the second message parameter

Return Value:

    TRUE -- This function handled the message.

    FALSE -- This function did not handle the message.

--*/
{

    static PCONNDATA ConnData = NULL;                // connection data
    static int       AddrLen;                        // length of addresses
    char             AddrText[MAX_SOCKADDR_LEN * 2]; // address string

    switch (Message) {

    case WM_INITDIALOG:
        
        // Get the parameter and figure out how long addresses for
        // this protocol may be.
        ConnData = (PCONNDATA)LParam;
        assert(ConnData != NULL);

        AddrLen = ConnData->ProtocolInfo->iMaxSockAddr;
        
        if (AddrLen > MAX_SOCKADDR_LEN) {
            MessageBox(DialogWindow,
                       "Sorry, socket addresses are too big. Aborting.",
                       "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            EndDialog(DialogWindow, FALSE);
        }

        // Limit the amount of text in the edit control to twice
        // AddrLen, i.e. 2 hex characters per byte.
        SendMessage(GetDlgItem(DialogWindow, IDC_ADDRESS), EM_LIMITTEXT, 
                    (WPARAM)(AddrLen * 2), 0);
        return(TRUE);

    case WM_COMMAND:

        switch (WParam) {
            
        case IDOK:
            
            // The user clicked 'OK'.  Get the text from the edit
            // control, and convert the hex string into bytes and put
            // the bytes into the socket address for this connection.
            GetDlgItemText(DialogWindow, IDC_ADDRESS, AddrText, 
                           MAX_SOCKADDR_LEN * 2);
            ConnData->RemoteSockAddr.len = AddrLen;
            ConnData->RemoteSockAddr.buf = 
              malloc(ConnData->RemoteSockAddr.len);
            if (ConnData->RemoteSockAddr.buf == NULL) {
                ChatSysError("malloc()",
                             "DefaultConnDlgProc()",
                             TRUE);
            }

            if (TranslateHex(ConnData->RemoteSockAddr.buf,
                             ConnData->RemoteSockAddr.len,
                             AddrText, 
                             DialogWindow)) {
                EndDialog(DialogWindow, TRUE);
            }
            return(TRUE);
                        
        case IDCANCEL:

            // Kill the dialog box.
            EndDialog(DialogWindow, FALSE);
            return(TRUE);
                        
        default:

            return(FALSE);
        } // switch (WParam)
                
    default:

        return(FALSE);
    } // switch (Message)

} // DefaultConnDlgProc() 





BOOL APIENTRY
NameAndSubjectDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WParam,
    IN LPARAM LParam)
/*++

Routine Description:

    Callback function that processes messages sent to the name and
    subject dialog box.

Implementation

    This dialog box is brought up in MakeConnection (see socket.c)
    only if the protocol for the connection to be made supports
    connection-time data transfer.  The user fills in the two fields
    and this data is packed the WSABUF which is referenced through a
    pointer passed in during WM_INITDIALOG processing.

Arguments:

    DialogWindow - Supplies handle of the dialog box.

    Message - Supplies the message identifier

    WParam - Supplies the first message parameter

    LParam - Supplies the second message parameter

Return Value:

    TRUE -- This function handled the message.

    FALSE -- This function did not handle the message; or the message
    was WM_INITDIALOG and we set the focus.

--*/
{

    static LPWSABUF CallerBuffer;            // caller user data
    char            NameText[NAME_LEN + 1];  // name string
    char            SubjectText[SUB_LEN + 1];// subject string

    switch (Message) {

    case WM_INITDIALOG:
        
        // Get the parameter and initialize the dialog box.
        CallerBuffer = (LPWSABUF)LParam;
        assert(CallerBuffer != NULL);

        SendMessage(GetDlgItem(DialogWindow, IDC_CALLERNAME), 
                    EM_LIMITTEXT, (WPARAM)NAME_LEN, 0);
        SendMessage(GetDlgItem(DialogWindow, IDC_SUBJECT), 
                    EM_LIMITTEXT, (WPARAM)SUB_LEN, 0);
        SetFocus(GetDlgItem(DialogWindow, IDC_CALLERNAME));
        return(FALSE);

    case WM_COMMAND:

        switch (WParam) {
            
        case IDOK:
            
            // Get the strings the user has typed.
            GetDlgItemText(DialogWindow, 
                           IDC_CALLERNAME, 
                           NameText, 
                           NAME_LEN);
            GetDlgItemText(DialogWindow, 
                           IDC_SUBJECT, 
                           SubjectText, 
                           SUB_LEN);
            
            CallerBuffer->len = strlen(NameText) + strlen(SubjectText) + 2;
            CallerBuffer->buf = (char *)malloc(CallerBuffer->len);
            if (CallerBuffer->buf == NULL) {
                ChatSysError("malloc()",
                             "NameAndSubjectDlgProc()",
                             TRUE);
            }

            if (!PackTwoStrings(CallerBuffer->buf, 
                                CallerBuffer->len, 
                                NameText, 
                                SubjectText)) {
                MessageBox(DialogWindow, "PackTwoStrings failed. Aborting.", 
                           "Error.", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                EndDialog(DialogWindow, FALSE);
            }
                
            EndDialog(DialogWindow, TRUE);
            return(TRUE);
                        
        case IDCANCEL:

            // Kill the dialog box.
            EndDialog(DialogWindow, FALSE);
            return(TRUE);
                        
        default:

            return(FALSE);
        } // switch (WParam)
                
    default:

        return(FALSE);
    } // switch (Message)

} // NameAndSubjectDlgProc()





BOOL APIENTRY
AcceptConnectionDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WParam,
    IN LPARAM LParam)

/*++

Routine Description:

    Callback function that processes messages sent to the
    AcceptConnection dialog box.  Get's the callee's name and copies
    it into the CalleeBuffer, which is passed in as a parameter to the
    WM_INITDIALOG message.

Arguments:

    DialogWindow - Supplies handle of the AcceptConnection dialog box

    Message - Supplies the message identifier

    WParam - Supplies the first message parameter

    LParam - Supplies the second message parameter

Return Value:

    TRUE -- This function handled the message.

    FALSE -- This function did not handle the message; or the message
    was WM_INITDIALOG and we set the focus.

--*/

{
    static LPWSABUF CalleeData; // callee user data

    switch (Message) {

    case WM_INITDIALOG:

        CalleeData = (LPWSABUF)LParam;
        assert(CalleeData != NULL);

        // Determine how much room there is in the user data buffer.
        // Limit the text to NAME_LEN characters, or less if we
        // don't have room for that many.
        if (CalleeData->len < (NAME_LEN + 1)) {
            SendMessage(GetDlgItem(DialogWindow, IDC_CALLERNAME), 
                        EM_LIMITTEXT, (WPARAM)(CalleeData->len - 1),
                        0);
        } else {
            SendMessage(GetDlgItem(DialogWindow, IDC_CALLERNAME), 
                        EM_LIMITTEXT, (WPARAM)NAME_LEN, 0);
        }
        SetFocus(GetDlgItem(DialogWindow, IDC_CALLEENAME));
        return(FALSE);
        
    case WM_COMMAND:

        switch (WParam) {

        case IDOK:

            // Get the name from the control and but it in CalleeData.
            GetDlgItemText(DialogWindow, 
                           IDC_CALLEENAME, 
                           CalleeData->buf,
                           NAME_LEN);
            CalleeData->len = strlen(CalleeData->buf + 1);
            EndDialog(DialogWindow, TRUE);
            return(TRUE);
           
        case IDCANCEL:

            // Kill the dialog box
            EndDialog(DialogWindow, FALSE);
            return(TRUE);
            
        default:

            break;
        
        }
 
        break;
    } // switch (Message)

    return(FALSE);
} // AcceptConnectionDlgProc()





BOOL APIENTRY
InetListenPortDlgProc(
    IN HWND DialogWindow,
    IN UINT Message,
    IN WPARAM WParam,
    IN LPARAM LParam)
/*++

Routine Description:

    Callback function that processes messages sent to the
    listening port dialog box.  

Arguments:

    DialogWindow - Supplies handle of the AcceptConnection dialog box

    Message - Supplies the message identifier

    WParam - Supplies the first message parameter

    LParam - Supplies the second message parameter

Return Value:

    TRUE -- This function handled the message.

    FALSE -- This function did not handle the message; or the message
    was WM_INITDIALOG and we set the focus.

--*/
{

    static struct sockaddr_in *SockAddrInet;             // Inet sockaddress
    int                       Port;                      // Inet port
    BOOL                      PortStringTranslated;      // for GetDlgItemInt
    char                      PortText[INET_PORT_LEN+1]; // Inet port string

    switch (Message) {

    case WM_INITDIALOG:

        // Get the parameter and initialize the dialog box.
        SockAddrInet = (struct sockaddr_in *)LParam;
        assert(SockAddrInet != NULL);
        SendMessage(GetDlgItem(DialogWindow, IDC_LISTEN_PORT),
                    EM_LIMITTEXT, (WPARAM)INET_PORT_LEN, 0);
        wsprintf(PortText,"%d", INET_DEFAULT_PORT);
        SendMessage(GetDlgItem(DialogWindow, IDC_LISTEN_PORT),
                    WM_SETTEXT, 0, (LPARAM)PortText);
        SetFocus(GetDlgItem(DialogWindow, IDC_LISTEN_PORT));
        return(FALSE);
        
    case WM_COMMAND:

        switch (WParam) {

        case IDOK:
            
            // Get the port.
            Port = (int)GetDlgItemInt(DialogWindow, 
                                      IDC_LISTEN_PORT, 
                                      &PortStringTranslated, 
                                      TRUE);
            if ((Port < 0) || (Port > 65535) || !PortStringTranslated) {
                MessageBox(DialogWindow,
                           "Please choose a port between 0 and 65535.",
                           "Bad Port.", 
                           MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                return(TRUE);
            }
            
            SockAddrInet->sin_port = (u_short)Port;
            EndDialog(DialogWindow, TRUE);
            return(TRUE);
            
        case IDCANCEL:

            // Kill the dialog box.
            EndDialog(DialogWindow, FALSE);
            return(TRUE);
            
        default:
            break;
        }
        break;
    
    } // switch (Message)

    return(FALSE); 
} // InetListenPortDlgProc()
