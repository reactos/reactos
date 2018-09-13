//
// The TCP/IP performance counter DLL loads INETMIB1.DLL
// directly and impersonates the SNMP master agent.  In
// order to accomplish this the DLL needs some of the
// utilities included in SNMPAPI.DLL but the Perf dudes
// do not want to load another DLL into their process.
//
// This static library hopefully will be only temporary
// until we can get the performance DLL rewritten to use
// the new MIB2UTIL.DLL.
//

#include "..\dll\any.c"
#include "..\dll\dbg.c"
#include "..\dll\mem.c"
#include "..\dll\oid.c"
#include "..\dll\octets.c"
#include "..\dll\vb.c"
#include "..\dll\vbl.c"
