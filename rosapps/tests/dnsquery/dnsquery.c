#include <windows.h>
#include <stdio.h>
#include <winerror.h>
#include <windns.h>
#include <assert.h>

int main( int argc, char **argv ) {
  PDNS_RECORD QueryReply, AddrResponse;
  DWORD Addr;

  assert (DnsQuery ("www.reactos.com", DNS_TYPE_A, DNS_QUERY_STANDARD,
		    NULL, &QueryReply, NULL) == ERROR_SUCCESS);
  AddrResponse = QueryReply;
  while( AddrResponse ) {
    if( AddrResponse->wType == DNS_TYPE_A ) {
      Addr = ntohl( AddrResponse->Data.A.IpAddress );
      printf( "www.reactos.com == %d.%d.%d.%d\n",
	      (int)(Addr >> 24) & 0xff,
	      (int)(Addr >> 16) & 0xff,
	      (int)(Addr >> 8) & 0xff,
	      (int)Addr & 0xff );
    }
    AddrResponse = AddrResponse->pNext;
  }
  DnsRecordListFree( QueryReply, DnsFreeRecordList );

  return 0;
}
