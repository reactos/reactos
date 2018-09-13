/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    protocol.cxx

Abstract:

    Contains functions to negotiate data connections with, send commands to and
    receive data from, the FTP server

    Contents:
        Command
        I_Command
        NegotiateDataConnection
        GetReply
        ReceiveFtpResponse
        AbortTransfer
        (SendCommand)
        (I_SendCommand)
        (I_AttemptDataNegotiation)
        (I_NegotiateDataConnection)

Author:

    Heath Hunnicutt (t-hheath) 21-Jun-1994

Environment:

    Win32 user-level DLL

Revision History:

--*/

#include <wininetp.h>
#include "ftpapih.h"

//
// private prototypes
//

PRIVATE
DWORD
__cdecl
SendCommand(
    IN ICSocket * s,
    IN LPCSTR lpszFormat,
    IN ...
    );

PRIVATE
DWORD
I_SendCommand(
    IN ICSocket * s,
    IN LPCSTR lpszFormat,
    va_list arglist
    );

PRIVATE
DWORD
I_AttemptDataNegotiation(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN va_list arglist
    );

PRIVATE
DWORD
I_NegotiateDataConnection(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN va_list arglist,
    IN BOOL bOverridePassive
    );

//
// functions
//


DWORD
Command(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN ...
    )

/*++

Routine Description:

    Sends a command to the FTP server on the control connection and optionally
    sets up a data connection. Wrapper for I_Command()

Arguments:

    lpSessionInfo       - pointer to FTP_SESSION_INFO describing FTP server and
                          our connection to it

    fExpectResponse     - TRUE if we need a data connection

    dwFlags             - controlling how to transfer data between the client
                          and server data connection, and how to set up data
                          connection

    prcResponse         - pointer to response code returned from server

    lpszCommandFormat   - pointer to command string

    ...                 - optional arguments for command string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "Command",
                "%#x, %B, %#x, %#x, %q",
                lpSessionInfo,
                fExpectResponse,
                dwFlags,
                prcResponse,
                lpszCommandFormat
                ));

    DWORD error;
    va_list arglist;

    va_start(arglist, lpszCommandFormat);

    error = I_Command(lpSessionInfo,
                      fExpectResponse,
                      dwFlags,
                      prcResponse,
                      lpszCommandFormat,
                      arglist
                      );

    va_end(arglist);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
I_Command(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN va_list arglist
    )

/*++

Routine Description:

    Sends a command to the FTP server, and if requested, negotiates a data
    connection

Arguments:

    lpSessionInfo       - pointer to FTP_SESSION_INFO describing the FTP server
                          and our connection to it

    fExpectResponse     - TRUE if we need a data connection

    dwFlags             - controlling how to transfer data between the client
                          and server data connection, and how to set up data
                          connection

    prcResponse         - pointer to returned server response

    lpszCommandFormat   - command to send to server

    arglist             - optional arguments for lpszCommandFormat

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "I_Command",
                "%#x, %B, %#x, %#x, %q, %#x",
                lpSessionInfo,
                fExpectResponse,
                dwFlags,
                prcResponse,
                lpszCommandFormat,
                arglist
                ));

    DWORD error;

    INET_ASSERT(lpSessionInfo != NULL);
    INET_ASSERT(prcResponse != NULL);
    INET_ASSERT(lpszCommandFormat != NULL);
    INET_ASSERT(   ((dwFlags & FTP_TRANSFER_TYPE_MASK) == FTP_TRANSFER_TYPE_UNKNOWN)
                || ((dwFlags & FTP_TRANSFER_TYPE_MASK) == FTP_TRANSFER_TYPE_ASCII)
                || ((dwFlags & FTP_TRANSFER_TYPE_MASK) == FTP_TRANSFER_TYPE_BINARY)
                );

    //
    // if the control socket is not valid, return "connection dropped"
    //

    if (!lpSessionInfo->socketControl->IsValid()) {
        error = ERROR_FTP_DROPPED;
        goto quit;
    }

    //
    // there can only be one data connection established for each FTP session
    //

    if (lpSessionInfo->socketData->IsValid()) {
        error = ERROR_FTP_TRANSFER_IN_PROGRESS;
        goto quit;
    }

    //
    // if we need a data connection then send the connection set-up commands
    // and issue the command
    //

    if (fExpectResponse) {
        error = I_AttemptDataNegotiation(lpSessionInfo,
                                         dwFlags,
                                         prcResponse,
                                         lpszCommandFormat,
                                         arglist
                                         );
        if (error == ERROR_SUCCESS) {

            //
            // check the server response for failure
            //

            if ((prcResponse->Major != FTP_RESPONSE_PRELIMINARY)
            && (prcResponse->Major != FTP_RESPONSE_COMPLETE)) {
                error = ERROR_INTERNET_EXTENDED_ERROR;
            }
        }
    } else {

        //
        // no data connection required, just send the command and check any
        // response from the server for a failure indication
        //

        error = I_SendCommand(lpSessionInfo->socketControl,
                              lpszCommandFormat,
                              arglist
                              );
        if (error == ERROR_SUCCESS) {
            error = GetReply(lpSessionInfo, prcResponse);
            if (error == ERROR_SUCCESS) {
                if ((prcResponse->Major != FTP_RESPONSE_COMPLETE)
                && (prcResponse->Major != FTP_RESPONSE_CONTINUE)
                && (prcResponse->Major != FTP_RESPONSE_PRELIMINARY)) {
                    error = ERROR_INTERNET_EXTENDED_ERROR;
                }
            }
        }
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
NegotiateDataConnection(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN ...
    )

/*++

Routine Description:

    Sets up a data connection between this client and the FTP server. Wrapper
    for I_AttemptDataNegotiation()

Arguments:

    lpSessionInfo       - pointer to FTP_SESSION_INFO describing the FTP server
                          and our connection to it

    dwFlags             - controlling how to transfer data between the client
                          and server data connection, and how to set up data
                          connection

    prcResponse         - pointer to returned server response code

    lpszCommandFormat   - command string to send

    ...                 - optional arguments for lpszCommandFormat

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error
                    error returned by Windows sockets

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "NegotiateDataConnection",
                "%#x, %#x, %#x, %q",
                lpSessionInfo,
                dwFlags,
                prcResponse,
                lpszCommandFormat
                ));

    va_list arglist;

    va_start(arglist, lpszCommandFormat);

    DWORD error = I_AttemptDataNegotiation(lpSessionInfo,
                                           dwFlags,
                                           prcResponse,
                                           lpszCommandFormat,
                                           arglist
                                           );

    va_end(arglist);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
GetReply(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    OUT FTP_RESPONSE_CODE *prcResponse
    )

/*++

Routine Description:

    Gets the response code from the server. The response text is stored in the
    per-thread last response text field, and the FTP response code is parsed
    off the start of the text and returned in prcResponse

Arguments:

    lpSessionInfo   - pointer to FTP_SESSION_INFO for which to get response text

    prcResponse     - pointer to the response code structure

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "GetReply",
                "%#x, %#x",
                lpSessionInfo,
                prcResponse
                ));

    PCHAR pchReplyBuffer;
    DWORD cchBufferLength;
    DWORD error;

    INET_ASSERT(prcResponse != NULL);

    //
    // set up a default error code
    //

    prcResponse->Major = FTP_RESPONSE_PERMANENT_FAILURE;
    prcResponse->Minor = 0;
    prcResponse->Detail = 0;
    prcResponse->Status = 0;

    pchReplyBuffer = NULL;

    //
    // receive the last response on the control socket
    //

    error = ReceiveFtpResponse(lpSessionInfo->socketControl,
                               (LPVOID *)&pchReplyBuffer,
                               &cchBufferLength,
                               TRUE,
                               prcResponse
                               );
    if (error == ERROR_SUCCESS) {

        INET_ASSERT(pchReplyBuffer != NULL);

        //
        // append this response text to that already stored in this thread's
        // data object
        //

        InternetSetLastError(0,
                             pchReplyBuffer,
                             cchBufferLength,
                             SLE_APPEND | SLE_ZERO_TERMINATE
                             );

        //
        // extract status code
        //

        LPSTR lpszError = pchReplyBuffer;

        ExtractInt(&lpszError, 0, &prcResponse->Status);

        DEBUG_PRINT(PROTOCOL, INFO, ("FTP status = %d\n", prcResponse->Status));

    } else {

        INET_ASSERT(pchReplyBuffer == NULL);

    }

    //
    // finished with the buffer
    //

    if (pchReplyBuffer != NULL) {
        pchReplyBuffer = (PCHAR)FREE_MEMORY((HLOCAL)pchReplyBuffer);

        INET_ASSERT(pchReplyBuffer == NULL);

    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ReceiveFtpResponse(
    IN ICSocket * Socket,
    OUT LPVOID * lpBuffer,
    OUT LPDWORD lpdwBufferLength,
    IN BOOL fEndOfLineCheck,
    IN FTP_RESPONSE_CODE * prcResponse
    )

/*++

Routine Description:

    Receives data from the FTP server. Optionally parses it for end-of-line
    sequence

    This function is typically going to receive one or more lines of response
    data, i.e. a small amount of data. Also typically, we will not normally
    expect to get EOF(connection) because we are usually reading the control
    connection

Arguments:

    Socket              - socket on which to receive data

    lpBuffer            - pointer to pointer to returned data buffer

    lpdwBufferLength    - pointer to returned length of (returned) data buffer

    fEndOfLineCheck     - TRUE if we are to perform an end-of-line check

    prcResponse         - pointer to returned server response code

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "ReceiveFtpResponse",
                "%#x, %#x, %#x, %B, %#x",
                Socket,
                lpBuffer,
                lpdwBufferLength,
                fEndOfLineCheck,
                prcResponse
                ));

    INET_ASSERT(lpBuffer != NULL);
    INET_ASSERT(lpdwBufferLength != NULL);

    if (fEndOfLineCheck) {

        INET_ASSERT(prcResponse != NULL);

    }

    LPSTR pchBuffer;
    DWORD bufferLength;
    DWORD bufferLeft;
    DWORD bytesReceived;
    int idxPosInLine;
    BOOL fLastLineDigitsSeen;
    BOOL fAtEOL;
    int nNumericReply;
    DWORD error;

    //
    // initialize variables (for SocketReceive())
    //

    pchBuffer = NULL;
    bufferLength = 0;
    bufferLeft = 0;
    bytesReceived = 0;

    //
    // get per-thread info block to determine blocking/non-blocking socket calls
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    //DWORD asyncFlags;
    //
    //asyncFlags = 0;
    //asyncFlags = lpThreadInfo->IsAsyncWorkerThread ? SF_NON_BLOCKING : 0;

    //
    // this flag is set to FALSE once the digits are definitely NOT seen, and
    // needs to be TRUE at the start of each new line
    //

    fLastLineDigitsSeen = TRUE;
    nNumericReply = 0;
    idxPosInLine = 0;
    fAtEOL = FALSE;

    BOOL eofData;

    while (TRUE) {

        //
        // get the next chunk of response data from the server. Since we are
        // expecting a response (i.e. small amount of data (< 1K)) we shouldn't
        // have to do this too many times
        //

        error = Socket->Receive(
                              (LPVOID *)&pchBuffer,
                              &bufferLength,
                              &bufferLeft,
                              &bytesReceived,
                              sizeof('\0'),     // dwExtraSpace
                              SF_EXPAND         // SocketReceive will allocate/grow the buffer
                              | SF_COMPRESS     //  and compress any unused space at EOF
                              | SF_INDICATE,     //  and make indications to app
                              &eofData
                              );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // if we have hit the end of the data then zero-terminate the buffer
        //

        if (eofData) {

            INET_ASSERT(bufferLeft > 0);

            pchBuffer[bytesReceived] = '\0';
        }

        //
        // if requested, find out if we got the end-of-line (on the last line)
        //

        if (fEndOfLineCheck) {

            //
            // look for the final line of the response
            //

            for (DWORD bytesChecked = 0;
                bytesChecked < bytesReceived;
                ++bytesChecked, ++idxPosInLine) {

                //
                // ISSUE: fAtEOL && fLastLineDigitsSeen ==> protocol error,
                // since there is data after the response line
                //

                //
                // if the previous character was an EOL, then the next one must
                // be the start of the next line. Reinitialize the response-
                // decoding state
                //

                if (fAtEOL) {
                    fLastLineDigitsSeen = TRUE;
                    nNumericReply = 0;
                    idxPosInLine = 0;
                    fAtEOL = FALSE;
                }

                //
                // if (so far) this line has been of the form we expect for a
                // response last-line, check the chars we just received to see
                // if it maintains that form
                //
                // since this form is completely determined by the first four
                // characters of the line, limit the checking to those chars
                //

                if (fLastLineDigitsSeen && (idxPosInLine < 4)) {
                    if (idxPosInLine < 3) {

                        //
                        // if one of the first three chars is not a digit, mark
                        // this line as a non-response
                        //

                        if (!isdigit(pchBuffer[bytesChecked])) {
                            fLastLineDigitsSeen = FALSE;
                        } else {

                            //
                            // store the numeric reply in an int
                            //

                            nNumericReply *= 10;
                            nNumericReply += (int)(pchBuffer[bytesChecked] - '0');
                        }
                    } else if (idxPosInLine == 3) {

                        //
                        // the first three chars must be digits.  If the fourth is a
                        // space, then this line has the right form
                        //

                        if (pchBuffer[bytesChecked] != ' ') {
                            fLastLineDigitsSeen = FALSE;
                        }
                    }
                }

                //
                // if at the end of a line, reset vars for the next line
                //

                if (pchBuffer[bytesChecked] == '\n') {

                    DEBUG_PRINT(PROTOCOL,
                                INFO,
                                ("At EOL, and fDigits = %B\n",
                                fLastLineDigitsSeen
                                ));

                    fAtEOL = TRUE;
                }
            }
        }

        //
        // if this data is expected to end in a proper response line, and that
        // line has been received, stop receiving more data
        //

        if (fEndOfLineCheck && fLastLineDigitsSeen && fAtEOL) {

            //
            // we found the end of the response, although we may not have
            // received an EOF(transmission) indication, because the server
            // didn't close the connection. Zero terminate the buffer
            //

            INET_ASSERT(bufferLeft > 0);

            pchBuffer[bytesReceived] = '\0';

            //
            // tear the numeric reply up by digit, placing it into our nicer
            // reply structure
            //

            INET_ASSERT(nNumericReply >= 0);
            INET_ASSERT(nNumericReply < 1000);

            prcResponse->Major = (nNumericReply / 100) % 10;
            prcResponse->Minor = (nNumericReply / 10) % 10;
            prcResponse->Detail = (nNumericReply) % 10;
            error = ERROR_SUCCESS;
            goto done;
        }

        //
        // if we were receiving data that was supposed to end in an EOL then this
        // is an error condition. Otherwise, it is the expected way to signal the
        // end of transmission
        //

        if (eofData) {
            if (fEndOfLineCheck) {
                error = ERROR_FTP_DROPPED;
                goto quit;
            } else {

                //
                // caller just receiving data. We're done
                //

                error = ERROR_SUCCESS;
                goto done;
            }
        }
    }

    INET_ASSERT(FALSE);

quit:

    INET_ASSERT(error != ERROR_SUCCESS);

    if (pchBuffer != NULL) {
        pchBuffer = (LPSTR)FREE_MEMORY((HLOCAL)pchBuffer);

        INET_ASSERT(pchBuffer == NULL);

    }

done:

    *lpBuffer = pchBuffer;
    *lpdwBufferLength = bytesReceived;

    INET_ASSERT((pchBuffer != NULL) ? (*(LPDWORD)pchBuffer != 0xc5c5c5c5) : TRUE);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
AbortTransfer(
    IN LPFTP_SESSION_INFO lpSessionInfo
    )

/*++

Routine Description:

    Aborts an ongoing transfer. Typically used to terminate a read file
    operation early. We don't expect a response from the server in this case

Arguments:

    lpSessionInfo   - pointer to FTP_SESSION_INFO containing context for sesion
                      to abort

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_EXTENDED_ERROR

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "AbortTransfer",
                "%#x",
                lpSessionInfo
                ));

    //
    // get per-thread info block to determine blocking/non-blocking socket calls
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //DWORD asyncFlags;
    //
    //asyncFlags = 0;
    //asyncFlags = lpThreadInfo->IsAsyncWorkerThread ? SF_NON_BLOCKING : 0;

    //
    // send an ABORT preceded by the following NVT/Telnet sequences:
    //
    //  FF F4 = Interrupt Process (IP)
    //  FF F2 = Data Mark (DM)
    //

#define ABORT_COMMAND NVT_INTERRUPT_PROCESS_STRING  \
                      NVT_DATA_MARK_STRING          \
                      "ABOR"                        \
                      "\r\n"

    //
    // BUGBUG - the IP/DM sequence should be sent as URGENT data
    //

    error = lpSessionInfo->socketControl->Send((LPBYTE)ABORT_COMMAND,
                                               sizeof(ABORT_COMMAND) - 1,
                                               SF_INDICATE
                                               );

quit:

    DEBUG_LEAVE(error);

    return error;
}

//
// private functions
//


PRIVATE
DWORD
__cdecl
SendCommand(
    IN ICSocket * Socket,
    IN LPCSTR lpszFormat,
    IN ...
    )

/*++

Routine Description:

    Sends a command string to the FTP server. Wrapper for I_SendCommand()

Arguments:

    Socket      - socket to send data on

    lpszFormat  - printf-style format string

    ...         - arguments for lpszFormat

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "SendCommand",
                "%#x, %q",
                Socket,
                lpszFormat
                ));

    va_list arglist;
    DWORD error;

    va_start(arglist, lpszFormat);

    error = I_SendCommand(Socket,
                          lpszFormat,
                          arglist
                          );

    va_end(arglist);

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
I_SendCommand(
    IN ICSocket * Socket,
    IN LPCSTR lpszFormat,
    va_list arglist
    )

/*++

Routine Description:

    Sends a command string to a server

Arguments:

    Socket      - socket to send command on

    lpszFormat  - printf-style format string for command

    arglist     - variable list of arguments for format string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error
                    error returned from Windows Sockets

                  ERROR_INTERNET_INTERNAL_ERROR
                    The string was too large to fit into our stack buffer

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "I_SendCommand",
                "%#x, %q, %#x",
                Socket,
                lpszFormat,
                arglist
                ));

#define I_SEND_COMMAND_BUFFER_LENGTH    2048    // Arbitrary (but large)

    //
    // get per-thread info block to determine blocking/non-blocking socket calls
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // we need a buffer large enough to be able to handle the longest path plus
    // the command overhead. Typically, we will only be getting short strings
    // (USER, PORT, etc.)
    //

    //
    // BUGBUG
    // use _vsnprintf() to create the buffer - this allows us to specify the
    // maximum offset in the buffer, so avoiding a stack crash but this
    // brings in the cruntimes which we don't want to do.
    //

    CHAR buf[I_SEND_COMMAND_BUFFER_LENGTH];

    {
        int numChars = wvnsprintf(buf, I_SEND_COMMAND_BUFFER_LENGTH, lpszFormat, arglist);

        INET_ASSERT(numChars <= I_SEND_COMMAND_BUFFER_LENGTH - 2);
        INET_ASSERT(numChars > 0);

        if (numChars <= I_SEND_COMMAND_BUFFER_LENGTH - 2) {

            //
            // append trailing "\r\n"
            //

            buf[numChars++] = '\r';
            buf[numChars++] = '\n';

            error = Socket->Send((LPVOID)buf, numChars, SF_INDICATE);
        } else {

            DEBUG_PRINT(PROTOCOL,
                        ERROR,
                        ("%d chars blows internal buffer limit (%d chars)\n",
                        numChars,
                        I_SEND_COMMAND_BUFFER_LENGTH
                        ));

            error = ERROR_INVALID_PARAMETER;
        }
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
I_AttemptDataNegotiation(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN va_list arglist
    )

/*++

Routine Description:

    Attempts to generate a data connection with server. If passive mode is
    selected & passive mode fails, then we back-down to non-passive

Arguments:

    lpSessionInfo       - pointer to FTP_SESSION_INFO describing server to
                          connect to

    dwFlags             - controlling how to transfer data between the client
                          and server data connection, and how to set up data
                          connection

    prcResponse         - pointer to response code variable

    lpszCommandFormat   - command to send

    arglist             - any arguments for command

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - same as I_NegotiateDataConnection

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "I_AttemptDataNegotiation",
                "%#x, %#x, %#x, %q, %#x",
                lpSessionInfo,
                dwFlags,
                prcResponse,
                lpszCommandFormat,
                arglist
                ));

    //
    // first try it without overriding passive mode
    //

    DWORD error = I_NegotiateDataConnection(lpSessionInfo,
                                            dwFlags,
                                            prcResponse,
                                            lpszCommandFormat,
                                            arglist,
                                            FALSE
                                            );

    //
    // if the negotiation failed because passive mode wasn't supported or not
    // allowed then try non-passive (if ok to do so)
    //

    if (error == ERROR_FTP_NO_PASSIVE_MODE) {

        INET_ASSERT(IsPassiveModeSession(lpSessionInfo));

        error = I_NegotiateDataConnection(lpSessionInfo,
                                          dwFlags,
                                          prcResponse,
                                          lpszCommandFormat,
                                          arglist,
                                          TRUE  // no passive mode this time
                                          );
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
I_NegotiateDataConnection(
    IN LPFTP_SESSION_INFO lpSessionInfo,
    IN DWORD dwFlags,
    IN OUT FTP_RESPONSE_CODE *prcResponse,
    IN LPCSTR lpszCommandFormat,
    IN va_list arglist,
    IN BOOL bOverridePassive
    )

/*++

Routine Description:

    Sends a command to the FTP server and opens a data channel

Arguments:

    lpSessionInfo       - pointer to FTP_SESSION_INFO describing server to
                          connect to

    dwFlags             - controlling how to transfer data between the client
                          and server data connection, and how to set up data
                          connection

    prcResponse         - pointer to response code variable

    lpszCommandFormat   - command to send

    arglist             - any arguments for command

    bOverridePassive    - TRUE if OK to override passive mode (set at connect
                          level)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_FTP,
                Dword,
                "I_NegotiateDataConnection",
                "%#x, %#x, %#x, %q, %#x, %B",
                lpSessionInfo,
                dwFlags,
                prcResponse,
                lpszCommandFormat,
                arglist,
                bOverridePassive
                ));

    PUCHAR puchAddr;
    PCHAR pch;

    ICSocket * socketControl;
    ICSocket * socketData ;
    ICSocket * socketListener ;
    DWORD error;
    int serr;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    BOOL isAsync;
    BOOL bPassiveMode = FALSE;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    isAsync = lpThreadInfo->IsAsyncWorkerThread;

//Command(lpSessionInfo,
//        FALSE,
//        FTP_TRANSFER_TYPE_UNKNOWN,
//        prcResponse,
//        "MODE B"
////        "MODE S"
//        );

    //
    // tell the server the type of transfer we want - ASCII ('A') or Binary ('I')
    //

    error = Command(lpSessionInfo,
                    FALSE,
                    FTP_TRANSFER_TYPE_UNKNOWN,
                    prcResponse,
                    "TYPE %s",
                    ((dwFlags & FTP_TRANSFER_TYPE_MASK)
                        == FTP_TRANSFER_TYPE_ASCII) ? "A" : "I"
                    );

    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    if (prcResponse->Major != FTP_RESPONSE_COMPLETE) {
        error = ERROR_INTERNET_EXTENDED_ERROR;
        goto quit;
    }

    socketControl = lpSessionInfo->socketControl;
    socketData = lpSessionInfo->socketData;
    socketListener = lpSessionInfo->socketListener;

    INET_ASSERT(!socketData->IsValid());
    INET_ASSERT(!socketListener->IsValid());

    //
    // we make passive connection if passive mode was specified in
    // InternetConnect() AND we are not overriding it
    //

    bPassiveMode = IsPassiveModeSession(lpSessionInfo) && !bOverridePassive;

    //
    // make the required type of data connection
    //

    if (!bPassiveMode) {

        SOCKADDR_IN ourDataAddr;
        SOCKADDR_IN ourCtrlAddr;
        int cbAddrLen;

        //
        // standard (non-passive (ACTIVE?)) transfers. We create a socket and
        // tell the server which socket to connect to. The server then initiates
        // the connection with this client
        //

        error = socketListener->CreateSocket(/*isAsync ? SF_NON_BLOCKING :*/ 0);
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }

#if INET_DEBUG
        socketListener->SetSourcePort();
#endif

        error = socketListener->Listen();
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // now tell the server the remote address to use when it initiates the
        // data connection. The PORT command consists of our 4 number sequence
        // IP address followed by the socket number converted to 2 bytes
        //

        error = socketControl->GetSockName((PSOCKADDR)&ourCtrlAddr);
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }
        error = socketListener->GetSockName((PSOCKADDR)&ourDataAddr);
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }
        puchAddr = (PUCHAR)&ourCtrlAddr.sin_addr;

        //
        // BUGBUG - using (global) winsock ntohs() (i.e. not SOCKS)
        //

        u_short port = _I_ntohs(ourDataAddr.sin_port);

        error = SendCommand(socketControl,
                            "PORT %d,%d,%d,%d,%d,%d",
                            puchAddr[0],
                            puchAddr[1],
                            puchAddr[2],
                            puchAddr[3],
                            HIBYTE(port),
                            LOBYTE(port)
                            );
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // read the response from the server. We are expecting something along
        // the lines: "200 PORT Command Successful"
        //

        error = GetReply(lpSessionInfo, prcResponse);
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }
        if (prcResponse->Major != FTP_RESPONSE_COMPLETE) {
            error = ERROR_INTERNET_EXTENDED_ERROR;
            goto error_exit;
        }
    } else {

        //
        // PASSIVE mode. Due to problems with firewalls not allowing incoming
        // connection requests, we have to ask the server to create a new socket
        // which we then connect to. This is the inverse of the non-PASV connect
        // case above
        //

        error = socketData->CreateSocket(/*isAsync ? SF_NON_BLOCKING:*/ 0);
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // send the PASV request to the server
        //

        error = SendCommand(socketControl, "PASV");
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }

        //
        // and read the response. We are expecting it to return the full address
        // information for the port in the form "227 (h1,h2,h3,h4,p1,p2) meaning
        // that the server is entering PASSIVE mode (227), the IP address and
        // socket to connect to being h1.h2.h3.h4, p1p2
        //

        error = GetReply(lpSessionInfo, prcResponse);
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }
        if (prcResponse->Major != FTP_RESPONSE_COMPLETE) {

            //
            // if we get 502 (command not implemented) or 425 (can't open data
            // connection) then return ERROR_FTP_NO_PASSIVE_MODE, else
            // ERROR_INTERNET_EXTENDED_ERROR
            //
            // QFE489: Same rule also now applied to 530 (not logged in).
            //

            if ((prcResponse->Status == FTP_RESPONSE_CANT_OPEN_DATA)
            ||  (prcResponse->Status == FTP_RESPONSE_CMD_NOT_IMPL)
            ||  (prcResponse->Status == FTP_RESPONSE_NOT_LOGGED_IN)) {
                error = ERROR_FTP_NO_PASSIVE_MODE;
            } else {
                error = ERROR_INTERNET_EXTENDED_ERROR;
            }
            goto error_exit;
        }

        //
        // parse the endpoint out of the server response (stored in the per-
        // thread last response info buffer).
        //
        // If we fail to lock the response text, then that's an internal error.
        // If we fail to parse the required information out of the response text
        // then we return an extended error indication, since presumably the
        // server sent response text, but not what we were expecting
        //

        pch = (PCHAR)InternetLockErrorText();
        if (pch != NULL) {
            pch = strstr(pch, "227 ");
            if (pch != NULL) {
                pch += sizeof("227 ") - 1;
                while (!isdigit(*pch) && *pch) {
                    ++pch;
                }
                if (isdigit(*pch)) {

                    int i;
                    DWORD octets[6];

                    //
                    // parse the individual address parts out of the response
                    //

                    for (i = 0; (i < ARRAY_ELEMENTS(octets)) && isdigit(*pch); ++i) {
                        if (!ExtractInt(&pch, 0, (LPINT)&octets[i])) {
                            break;
                        }
                        if (octets[i] > 255) {
                            break;
                        }
                        if (i < ARRAY_ELEMENTS(octets) - 1) {
                            while (!isdigit(*pch) && *pch) {
                                ++pch;
                            }
                        }
                    }

                    //
                    // if we successfully parsed the server's address info then
                    // try to connect with the address it sent us
                    //

                    if ((i == ARRAY_ELEMENTS(octets)) && (octets[i - 1] <= 255)) {

                        SOCKADDR_IN remoteSockaddr;

                        remoteSockaddr.sin_family = AF_INET;

                        //
                        // N.B. Using (global) winsock ntohs() (i.e. not SOCKS)
                        //

                        remoteSockaddr.sin_port = _I_htons(
                                                    (USHORT)((octets[4] << 8)
                                                    | (USHORT)octets[5]));
                        puchAddr = (PUCHAR)&remoteSockaddr.sin_addr;
                        puchAddr[0] = (UCHAR)octets[0];
                        puchAddr[1] = (UCHAR)octets[1];
                        puchAddr[2] = (UCHAR)octets[2];
                        puchAddr[3] = (UCHAR)octets[3];
                        error = socketData->DirectConnect(
                                                (PSOCKADDR)&remoteSockaddr
                                                );
                    } else {

                        DEBUG_PRINT(PROTOCOL,
                                    ERROR,
                                    ("failed to parse 6 address elements\n"
                                    ));

                        error = ERROR_INTERNET_EXTENDED_ERROR;
                    }
                } else {

                    DEBUG_PRINT(PROTOCOL,
                                ERROR,
                                ("failed to locate start of address info in response text\n"
                                ));

                    error = ERROR_INTERNET_EXTENDED_ERROR;
                }
            } else {

                DEBUG_PRINT(PROTOCOL,
                            ERROR,
                            ("failed to locate \"227\" in response text\n"
                            ));

                error = ERROR_INTERNET_EXTENDED_ERROR;
            }

            //
            // unlock the response text
            //

            //InternetUnlockErrorText();
        } else {

            DEBUG_PRINT(PROTOCOL,
                        ERROR,
                        ("failed to lock last response text\n"
                        ));

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }

        //
        // bail out if we met with any errors
        //

        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    //
    // issue SIZE command to get file size
    //
    if(StrCmpNI(lpszCommandFormat, "RETR", 4) == 0) {   // If we're downloading a file
        error = I_SendCommand(socketControl,
                              "SIZE %s",
                              arglist
                              );
        if (error != ERROR_SUCCESS) {
            goto error_exit;
        }

        error = GetReply(lpSessionInfo, prcResponse);
        if (error != ERROR_SUCCESS) {
            if (socketData->IsValid()) {
                ResetSocket(socketData);
            }
            goto error_exit;
        }

        if (prcResponse->Major == FTP_RESPONSE_COMPLETE) {
            pch = (PCHAR)InternetLockErrorText();
            if (pch != NULL) {
                pch = strstr(pch, "213 ");
                if (pch != NULL) {

                    pch += sizeof("213 ") - 1;
                    if ( pch )
                    {
                        if (*pch && isdigit(*pch)) {
                            if (ExtractInt(&pch, 0, (int *) &(lpSessionInfo->dwFileSizeLow))) {
                                lpSessionInfo->Flags |= FFTP_KNOWN_FILE_SIZE;
                            }
                        }
                    }

                }
            }
        }
    }

    //
    // we have the data connection, send the command
    //

    error = I_SendCommand(socketControl,
                          lpszCommandFormat,
                          arglist
                          );
    if (error != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // get the preliminary reply from the FTP server
    //

    error = GetReply(lpSessionInfo, prcResponse);
    if (error != ERROR_SUCCESS) {
        if (socketData->IsValid()) {
            ResetSocket(socketData);
        }
        goto error_exit;
    }

    if ((prcResponse->Major != FTP_RESPONSE_PRELIMINARY)
    && (prcResponse->Major != FTP_RESPONSE_COMPLETE)) {
        if (socketData->IsValid()) {
            ResetSocket(socketData);
        }
        error = ERROR_INTERNET_EXTENDED_ERROR;
        goto error_exit;
    }

    //
    // Parse out file size if its around
    // Assumes the substring containing the file size looks like; "(99999 bytes)"
    //
    if(!(lpSessionInfo->Flags & FFTP_KNOWN_FILE_SIZE)) {  // Don't already have the size
        pch = (PCHAR)InternetLockErrorText();
        if (pch != NULL) {
            pch = strstr(pch, "150 ");
            if (pch != NULL) {

                pch += sizeof("150 ") - 1;
                PCHAR pchEnd = strstr(pch, " bytes)");
                if (pchEnd) {
                    DWORD dwFileSize = 0;
                    DWORD dwMul = 1;
                    PCHAR pchBeg = pch;
                    pch = pchEnd - 1;
                    while (pch && (pch > pchBeg) && isdigit(*pch)) {
                        dwFileSize += ((*pch - '0') * dwMul);
                        dwMul *= 10;
                        pch--;
                    }
                    
                    if (pch && (*pch == '(') && (dwFileSize > 0)) {
                        lpSessionInfo->dwFileSizeLow = dwFileSize;
                        lpSessionInfo->Flags |= FFTP_KNOWN_FILE_SIZE;
                    }
                }
            }
        }
    }

    //
    // unlock the response text
    //

    //InternetUnlockErrorText();

    //
    // if we are expecting the server to create the connection (ACTIVE mode)
    // then perform an accept() on our listening socket in order to establish
    // the connection
    //

    if (!bPassiveMode) {

        //
        // if we just accept(), we may hang forever if the server doesn't call
        // back (servers are all alike), so we use select() to wait for the
        // socket to be acceptable before calling accept() proper
        //

        error = socketListener->SelectAccept(
                                    *socketData,

                                    //
                                    // BUGBUG - call GetTimeoutValue()
                                    //

                                    GlobalFtpAcceptTimeout
                                    );

        if ( error != ERROR_SUCCESS ) {
            goto error_exit;
        }

        //
        // we no longer require the socket we created for listening for the
        // incoming server connection request
        //

        INET_ASSERT(socketListener != socketControl);
        INET_ASSERT(socketListener != socketData);
        INET_ASSERT(socketListener->GetSocket() != socketControl->GetSocket());
        INET_ASSERT(socketListener->GetSocket() != socketData->GetSocket());

#if INET_DEBUG
        socketData->SetSourcePort();
#endif

        socketListener->Close();
        if (!socketData->IsValid()) {
            goto error_exit;
        }
    }

    //
    // set send and receive timeouts for data socket. If we get an error, ignore
    // it. Note this probably means that the socket is (somehow) invalid, and
    // that we should really return an error. But for now (as with the other
    // protocols), I will presume that the error is non-fatal (but note it)
    //

    socketData->SetTimeout(
                SEND_TIMEOUT,
                (int)GetTimeoutValue(INTERNET_OPTION_DATA_SEND_TIMEOUT)
                );
    socketData->SetTimeout(
                RECEIVE_TIMEOUT,
                (int)GetTimeoutValue(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT)
                );

    INET_ASSERT(error == ERROR_SUCCESS);

    INET_ASSERT(lpSessionInfo->socketData == socketData);

quit:

    DEBUG_LEAVE(error);

    return error;


error_exit:

    if (socketData->IsValid()) {
        socketData->Close();
    }
    if (socketListener->IsValid()) {
        socketListener->Close();
    }
    goto quit;
}
