//
// SMTP - Simple Mail Transfer Protocol Code
//
// Implements sends mail using SMTP RFC 821.
// Julian Jiggins, 12 January 1997
//

#include "private.h"
#include <winsock.h>

#define TF_THISMODULE  TF_MAILAGENT

#define IS_DIGIT(ch)    InRange(ch, TEXT('0'), TEXT('9'))

//
// Function prototypes for this module
//
SOCKET Connect(char *host, char *port);

#define READ_BUF_LEN 512

//
// Read all you can from a socket
//
void Read(SOCKET sock, char * readBuffer, int bufLen)
{
    int numRead;
    int totalRead = 0;

    do
    {
        numRead = recv(sock, readBuffer+totalRead, bufLen, 0);
        totalRead += numRead;
    }
    while (0);
    // while (numRead > 0);

    //
    // NULL terminate read string
    //
    readBuffer[totalRead] = 0;
}

//
// Send a string specifying an SMTP command and read in response, returning
// TRUE if it is the one expected.
// Note the SMTP protocol is designed such that only the 1 character of the 
// response need be checked, but we check for the exact response (mostly cause
// I just read that bit in the RFC)
// 
BOOL SendAndExpect(SOCKET sock, char * sendBuffer, char * szExpect)
{
    char readBuffer[READ_BUF_LEN];
    int len;
    int numSent;

    //
    // Send string to socket
    //
    numSent = send(sock, sendBuffer, lstrlenA(sendBuffer), 0);
    if (numSent == SOCKET_ERROR) 
    {
        DBG_WARN("Error on send");
        return FALSE;
    }

    //
    // Now read in response
    //
    Read(sock, readBuffer, READ_BUF_LEN);

    DBG2("Sent: %s", sendBuffer);
    DBG2("Read: %s", readBuffer);

    //
    // Expect beginning of response to contain szExpect string
    //
    len = lstrlenA(szExpect);
    if (CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, 
        readBuffer, len, szExpect, len) == 2)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#define SMTP_221 "221"
#define SMTP_250 "250"
#define SMTP_354 "354"
#define SMTP_EOM "\r\n.\r\n"

//
// Carry out SMTP negotiation
//
BOOL SMTPSendEmail(SOCKET sock, char * szToAddress, char * szFromAddress, char *szMessage)
{
    char sendBuffer[256];
    char readBuffer[READ_BUF_LEN];
    BOOL b = TRUE;
    int r, len;

    //
    // Read the opening response
    //
    Read(sock, readBuffer, sizeof(readBuffer));
    DBG(readBuffer);

    //
    // say Hello and specify my domain
    //
    b = SendAndExpect(sock, "HELO ActiveDesktop\r\n", SMTP_250);
    if (!b) goto done;

    //
    // First special sender in MAIL command
    //
    wnsprintfA(sendBuffer, ARRAYSIZE(sendBuffer), "MAIL FROM:<%s>\r\n", 
              szFromAddress);//BUGBUG-FIXED-OVERFLOW
    b = SendAndExpect(sock, sendBuffer, SMTP_250);
    if (!b) goto done;

    //
    // Now specify recipient(s)
    //
    wnsprintfA(sendBuffer, ARRAYSIZE(sendBuffer), "RCPT TO:<%s>\r\n", 
              szToAddress);//BUGBUG-FIXED-OVERFLOW
    b = SendAndExpect(sock, sendBuffer, SMTP_250);
    if (!b) goto done;

    //
    // Now send DATA command
    //
    b = SendAndExpect(sock, "DATA\r\n", SMTP_354);
    if (!b) goto done;

    //
    // Now send mail message
    //
    len = lstrlenA(szMessage);
    r = send(sock, szMessage, len, 0);
    ASSERT(r != SOCKET_ERROR);
    ASSERT(r == len);

    //
    // Specify end of message with single period.
    //
    b = SendAndExpect(sock, SMTP_EOM, SMTP_250);
    if (!b) goto done;

    //
    // Say goodbye
    //
    b = SendAndExpect(sock, "QUIT\r\n", SMTP_221);
 
done:
    return b;
}

//
// Main entry point - 
//  start winsock dll, 
//  connect to socket,
//  and negotiate transfer
//
SMTPSendMessage(char * szServer, char * szToAddress, char * szFromAddress, char * szMessage)
{
    int err;
    SOCKET sock;
    BOOL b = FALSE;
    WSADATA wsaData;

    //
    // Init the winsock dll specifying which version we want.
    //
    err = WSAStartup((WORD)0x0101, &wsaData);
    if (err)
    {
        DBG_WARN("WinSock startup error");
        return FALSE;
    }
    DBG("WinSock successfully started");

    //
    // Actually form the socket connection to the host on port 25
    //
    sock = Connect(szServer, "25");

    if (sock != 0)
    {
        DBG("Connected");
        b = SMTPSendEmail(sock, szToAddress, szFromAddress, szMessage);
    }

    //
    // Done with winsock dll for now
    //
    WSACleanup();
    return b;
}

#ifdef TEST
int
main(int argc, char * argv[])
{
    char szMessage[1024];
    BOOL b;

    //
    // Build message
    //
    lstrcpy(szMessage, "Subject: Subscription Updated: CNN Interactive\r\n");
    lstrcat(szMessage, "Your subscription to CNN Interactive Subscription has been updated\r\n\r\n");
    lstrcat(szMessage, "To view the subscription offline, just click here: ");
    lstrcat(szMessage, "http://www.cnn.com\r\n");
    lstrcat(szMessage, "\r\nThis message was sent by the IE4.0 Information Delivery Agent\r\n");
    lstrcat(szMessage, "\r\n\r\n\r\nDoes this look okay guys? Julian.");

    b = SMTPSendMessage("saranac", "joepe@microsoft.com", szMessage);
    if (b)
        DBG("Sent mail successfully");
    else
        DBG("Couldn't send email");

    return 0;
}
#endif


SOCKET
Connect(char *host, char *port)
{
    struct sockaddr_in sockaddress;
    DWORD  err;
    SOCKET sock, connectresult;

    //
    // Get the socket address
    //
    if(IS_DIGIT(*host))                            
        sockaddress.sin_addr.s_addr=inet_addr(host);
    else 
    {
        struct hostent *hp;
        if((hp=gethostbyname(host))==NULL) 
        {
            DBG_WARN2("Unknown host %s", host);
            return 0;
        }
        memcpy(&sockaddress.sin_addr, hp->h_addr, sizeof(sockaddress.sin_addr));
    }

    //
    // Get the port address
    //
    if(IS_DIGIT(*port))  
        sockaddress.sin_port=htons((USHORT)StrToIntA(port));      
    else 
    {
        DBG_WARN("The port should be a number");
        return 0;
    }
    sockaddress.sin_family=AF_INET;

    //
    // Create a stream style socket
    //
    if((sock=socket(PF_INET, SOCK_STREAM, 0))<0) 
        DBG_WARN("socket error");


    DBG("Trying to connect");

    connectresult=connect(sock,(struct sockaddr *) &sockaddress, sizeof(sockaddress));

    if (connectresult == SOCKET_ERROR) 
    {
        switch(err = WSAGetLastError()) 
        {
        case WSAECONNREFUSED:
            DBG_WARN("ERROR - CONNECTION REFUSED.");
            break;
        case WSAENETUNREACH:
            DBG_WARN("ERROR - THE NETWORK IS NOT REACHABLE FROM THIS HOST.");
            break;
        case WSAEINVAL:
            DBG_WARN("ERROR - The socket is not already bound to an address.");
            break;
        case WSAETIMEDOUT:
            DBG_WARN("ERROR - Connection timed out.");
            break;
        default:
            DBG_WARN2("Couldn't connect %d", err);
            break;
        } 
        closesocket(sock);
        return 0;
    }
    return sock;
}
