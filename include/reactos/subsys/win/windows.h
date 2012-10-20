#if !defined(__INCLUDE_WIN_WINDOWS_H)
#define __INCLUDE_WIN_WINDOWS_H

#include <csr/protocol.h>

/* w32 console server */
CSR_SERVER_DLL_INIT(ConServerDllInitialization);

/* w32 user server */
CSR_SERVER_DLL_INIT(UserServerDllInitialization);

#endif /* ndef __INCLUDE_WIN_WINDOWS_H */


