/*
 * netstat - display IP stack statistics.
 *
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 *
 * Robert Dickenson <robd@reactos.org>, August 15, 2002.
 */
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>

#include <iptypes.h>
#include <ipexport.h>
#include <iphlpapi.h>
#include <snmp.h>

#include "trace.h"
#include "resource.h"


#define MAX_RESLEN 4000

/*
typedef struct {
    UINT idLength;
    UINT* ids;
} AsnObjectIdentifier;

VOID SnmpUtilPrintAsnAny(AsnAny* pAny);  // pointer to value to print
VOID SnmpUtilPrintOid(AsnObjectIdentifier* Oid);  // object identifier to print

 */
void test_snmp(void)
{
    int nBytes = 500;
    BYTE* pCache;

    pCache = (BYTE*)SnmpUtilMemAlloc(nBytes);
    if (pCache != NULL) {
        AsnObjectIdentifier* pOidSrc = NULL;
        AsnObjectIdentifier AsnObId;
        if (SnmpUtilOidCpy(&AsnObId, pOidSrc)) {
            //
            //
            //
            SnmpUtilOidFree(&AsnObId);
        }
        SnmpUtilMemFree(pCache);
    } else {
        _tprintf(_T("ERROR: call to SnmpUtilMemAlloc() failed\n"));
    }
}


void usage(void)
{
    TCHAR buffer[MAX_RESLEN];

    int length = LoadString(GetModuleHandle(NULL), IDS_APP_USAGE, buffer, sizeof(buffer)/sizeof(buffer[0]));
	_fputts(buffer, stderr);
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        usage();
        return 1;
    }
    _tprintf(_T("\nActive Connections\n\n")\
             _T("  Proto  Local Address          Foreign Address        State\n\n"));
    test_snmp();
	return 0;
}
