/* tcpServer.c 
 *
 * Simple Winsock TCP server test.
 * Later will be used as base for ReactOS telnetd  
 * 
 * HISTORY: 
 * 6-15-02 - Added Winsock support to UNIX tcp test
 * 6-16-02 - Removed Unix support
 * 6-17-02 - Added extra comments to code
 *
 */

#include <winsock2.h>
#include <stdio.h>

#undef ERROR
#define SUCCESS 0
#define ERROR   1

#define END_LINE 0x0A
#define SERVER_PORT 23
#define MAX_MSG 100

/* function readline */
int read_line();

int main (int argc, char *argv[]) {
  
  WORD       wVersionRequested;
  WSADATA    WsaData;
  INT	     Status;
  int sd, newSd, cliLen;

  struct sockaddr_in cliAddr, servAddr;
  char line[MAX_MSG]; 

    wVersionRequested = MAKEWORD(2, 2);
 
    Status = WSAStartup(wVersionRequested, &WsaData);
    if (Status != 0) {
        printf("Could not initialize winsock dll.\n");	
        return FALSE;
    }

  /* create socket */
  sd = socket(AF_INET, SOCK_STREAM, 0);
   if(sd<0) {
    perror("cannot open socket ");
    WSACleanup();
    return ERROR;
  }
  
  /* bind server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(SERVER_PORT);
  
  if(bind(sd, (struct sockaddr *) &servAddr, sizeof(servAddr))<0) {
    perror("cannot bind port ");
    WSACleanup();
    return ERROR;
  }

  listen(sd,5);
  
  while(1) {

    printf("%s: \n 
          To start test, Please telnet to localhost (127.0.0.1) port 23 \n
          When connected input raw data followed by End of Line\n
          Test is now running on TCP port %u\n",argv[0],SERVER_PORT);

    cliLen = sizeof(cliAddr);
    newSd = accept(sd, (struct sockaddr *) &cliAddr, &cliLen);
    if(newSd<0) {
      perror("cannot accept connection ");
      WSACleanup();
      return ERROR;
    }
    
    /* init line */
    memset(line,0x0,MAX_MSG);
    
    /* receive segments */
    while(read_line(newSd,line)!=ERROR) {
      
      printf("%s: received from %s:TCP%d : %s\n", argv[0], 
	     inet_ntoa(cliAddr.sin_addr),
	     ntohs(cliAddr.sin_port), line);
      /* init line */
      memset(line,0x0,MAX_MSG);
      
    } /* while(read_line) */
    
  } /* while (1) */

}


/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING       */
/* this function is experimental.. I don't know yet if it works  */
/* correctly or not. Use Steven's readline() function to have    */
/* something robust.                                             */
/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING       */

/* rcv_line is my function readline(). Data is read from the socket when */
/* needed, but not byte after bytes. All the received data is read.      */
/* This means only one call to recv(), instead of one call for           */
/* each received byte.                                                   */
/* You can set END_CHAR to whatever means endofline for you. (0x0A is \n)*/
/* read_lin returns the number of bytes returned in line_to_return       */
int read_line(int newSd, char *line_to_return) {
  
  static int rcv_ptr=0;
  static char rcv_msg[MAX_MSG];
  static int n;
  int offset;  

  offset=0;

  while(1) {
    if(rcv_ptr==0) {
      /* read data from socket */
      memset(rcv_msg,0x0,MAX_MSG); /* init buffer */
      n = recv(newSd, rcv_msg, MAX_MSG, 0); /* wait for data */
      if (n<0) {
	perror(" cannot receive data ");
	return ERROR;
      } else if (n==0) {
	printf(" connection closed by client\n");
	close(newSd);
        WSACleanup();
	return ERROR;
      }
    }
  
    /* if new data read on socket */
    /* OR */
    /* if another line is still in buffer */

    /* copy line into 'line_to_return' */
    while(*(rcv_msg+rcv_ptr)!=END_LINE && rcv_ptr<n) {
      memcpy(line_to_return+offset,rcv_msg+rcv_ptr,1);
      offset++;
      rcv_ptr++;
    }
    
    /* end of line + end of buffer => return line */
    if(rcv_ptr==n-1) { 
      /* set last byte to END_LINE */
      *(line_to_return+offset)=END_LINE;
      rcv_ptr=0;
      return ++offset;
    } 
    
    /* end of line but still some data in buffer => return line */
    if(rcv_ptr <n-1) {
      /* set last byte to END_LINE */
      *(line_to_return+offset)=END_LINE;
      rcv_ptr++;
      return ++offset;
    }

    /* end of buffer but line is not ended => */
    /*  wait for more data to arrive on socket */
    if(rcv_ptr == n) {
      rcv_ptr = 0;
    } 
    
  } /* while */
}
  
  
