#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iphlpapi.h>

int main( int argc, char **argv ) {
  ULONG OutBufLen = 0;
  PFIXED_INFO pFixedInfo;
  PIP_ADDR_STRING Addr;

  GetNetworkParams(NULL, &OutBufLen);
  pFixedInfo = malloc(OutBufLen);
  if (!pFixedInfo) {
    printf( "Failed to alloc %d bytes.\n", (int)OutBufLen );
    return 1;
  }

  printf( "%d Bytes allocated\n", (int)OutBufLen );

  GetNetworkParams(pFixedInfo,&OutBufLen);

  for( Addr = &pFixedInfo->DnsServerList;
       Addr;
       Addr = Addr->Next ) {
    printf( "%c%s\n",
	    Addr == pFixedInfo->CurrentDnsServer ? '*' : ' ',
	    Addr->IpAddress.String );
  }

  free( pFixedInfo );
  return 0;
}
