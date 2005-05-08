/* Perform simple test of headers to avoid typos and such */
#define __USE_W32_SOCKETS
#include <w32api.h>
#include <windows.h>

#ifdef __OBJC__
#define BOOL WINBOOL
#endif
#include <windowsx.h>
#include <commctrl.h>
#include <largeint.h>
#include <mmsystem.h>
#include <mciavi.h>
#include <mcx.h>
#include <sql.h>
#include <sqlext.h>
#include <imm.h>
#include <lm.h>
#include <zmouse.h>
#include <scrnsave.h>
#include <cpl.h>
#include <cplext.h>
#include <wincrypt.h>
#include <pbt.h>
#include <wininet.h>
#include <regstr.h>
#include <custcntl.h>
#include <mapi.h>

#include <dbt.h>
#include <httpext.h>
#include <imagehlp.h>
#include <initguid.h>
#include <ipexport.h>
#include <iphlpapi.h>
#include <ipifcons.h>
#include <iprtrmib.h>
#include <iptypes.h>
#include <isguids.h>
#include <lmbrowsr.h>
#include <mswsock.h>
#include <nddeapi.h>
#include <ntdef.h>
#include <ntsecapi.h>
#include <odbcinst.h>
#include <powrprof.h>
#include <psapi.h>
#include <ras.h>
#include <rasdlg.h>
#include <raserror.h>
#include <rassapi.h>
#include <richedit.h>
#include <rpcdce2.h>
#include <subauth.h>
#include <tlhelp32.h>
#include <userenv.h>
#include <winioctl.h>
#include <winresrc.h>
#include <winsock.h>
#ifdef _WINSOCK2_H
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <wsahelp.h>
#endif
#include <wsipx.h>
#include <wsnetbs.h>
#include <svcguid.h>
#include <setupapi.h>
#include <aclapi.h>
#include <security.h>
#include <secext.h>
#include <schnlsp.h>
#include <ntldap.h>
#include <winber.h>
#include <winldap.h>
#include <shlwapi.h>
#include <snmp.h>
#include <winsnmp.h>
#include <mgmtapi.h>
#include <vfw.h>
#include <uxtheme.h>
#include <tmschema.h>
#include <dhcpcsdk.h>
#include <errorrep.h>
#include <windns.h>

#ifndef __OBJC__  /* problems with BOOL */
#include <ole2.h>
#include <comcat.h>
#include <shlobj.h>
#include <intshcut.h>
#include <ocidl.h>
#include <ole2ver.h>
#include <oleacc.h>
#include <winable.h>
#include <olectl.h>
#include <oledlg.h>
#include <docobj.h>
#include <idispids.h>
#include <rapi.h>
#include <richole.h>
#include <rpcproxy.h>
#include <exdisp.h>
#include <mshtml.h>
#include <servprov.h>

#else
#undef BOOL
#endif

#include <stdio.h>

int main()
{
  return 0;
}
