#include <roscfg.h>
#include "../../include/precomp.h"
#include "regtests.h"

#define MTU 1500

struct packet {
    int size;
    char data[MTU];
};

static void RunTest() {
    const struct packet Packets[] = {
	{ 0 }
    };
    int i;
    IP_INTERFACE IF;
    IP_PACKET IPPacket;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Init interface */

    /* Init packet */

    for( i = 0; NT_SUCCESS(Status) && i < Packets[i].size; i++ ) {
	IPPacket.Header = (PUCHAR)Packets[i].data;
	IPPacket.TotalSize = Packets[i].size;
	IPReceive( &IF, &IPPacket );
    }
    _AssertEqualValue(STATUS_SUCCESS, Status);
}

_Dispatcher(IpreceiveTest, "IPReceive");
