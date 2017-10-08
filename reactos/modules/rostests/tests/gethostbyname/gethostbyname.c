#include <stdio.h>
#include <winsock2.h>

int main( int argc, char **argv ) {
  WSADATA wdata;

  WSAStartup( 0x0101, &wdata );

  if( argc > 1 ) {
    struct hostent *he = gethostbyname( argv[1] );
    if( !he ) {
      printf( "lookup of host %s failed: %d\n", argv[1], WSAGetLastError() );
      return 1;
    } else {
      printf( "Lookup of host %s returned %s\n",
	      argv[1], inet_ntoa(*((struct in_addr *)he->h_addr_list[0])) );
      return 0;
    }
  } else
    return 1;
}
