#include <stdlib.h>
#include <mainwin.h>

#define IEREMOTE_CMDLINE        1
#define IEREMOTECLASS           TEXT("IEFrame")

BOOL ConnectRemoteIE(LPTSTR pszCmdLine, HINSTANCE hInstance);
BOOL IsCommandSwitch(LPTSTR lpszCmdLine, LPTSTR pszSwitch);
BOOL RemoteIENewWindow(LPTSTR pszCmdLine);

#if defined(UNIX)

#include <sys/time.h>
#include <sys/resource.h>

#define INCREASE_FILEHANDLE_LIMIT  \
    struct rlimit rl; \
    if ( 0 == getrlimit(RLIMIT_NOFILE, &rl)) { \
       rl.rlim_cur = rl.rlim_max; \
       setrlimit(RLIMIT_NOFILE, &rl); \
    } \

#endif
