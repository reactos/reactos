#include <windows.h>

#ifdef RC_INVOKED
#define ID(id)              id
#else
#define ID(id)              MAKEINTRESOURCE(id)
#endif

#define GWLP_HWNDEDIT       0
#define IDUSERBENCH         ID(1)
#define IDNOTE              ID(2)

#ifndef RC_INVOKED

#ifndef WIN32
#define APIENTRY FAR PASCAL
typedef int INT;
typedef char CHAR;
#else
#define DLGPROC WNDPROC
#endif

#define A   0
#define W   1

#endif
