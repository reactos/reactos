#ifndef _LMCONS_H
#define _LMCONS_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#define MESSAGE_FILENAME TEXT("NETMSG")
#define OS2MSG_FILENAME TEXT("BASE")
#define HELP_MSG_FILENAME TEXT("NETH")
#define NET_API_FUNCTION __stdcall
#define NET_API_STATUS DWORD
#define API_RET_TYPE NET_API_STATUS
#define MIN_LANMAN_MESSAGE_ID NERR_BASE
#define MAX_LANMAN_MESSAGE_ID 5799
#ifndef CNLEN /* also in nddeapi.h */
#define CNLEN 15
#define UNCLEN (CNLEN + 2)
#endif
#define DNLEN 15
#define LM20_CNLEN 15
#define LM20_DNLEN 15
#define LM20_SNLEN 15
#define LM20_STXTLEN 63
#define LM20_UNCLEN (LM20_CNLEN+2)
#define LM20_NNLEN 12
#define LM20_RMLEN (LM20_UNCLEN+1+LM20_NNLEN)
#define NNLEN 80
#define RMLEN (UNCLEN+1+NNLEN)
#define SNLEN 80
#define STXTLEN 256
#define PATHLEN 256
#define LM20_PATHLEN 256
#define DEVLEN 80
#define LM20_DEVLEN 8
#define EVLEN 16
#define UNLEN 256
#define LM20_UNLEN 20
#define GNLEN UNLEN
#define LM20_GNLEN LM20_UNLEN
#define PWLEN 256
#define LM20_PWLEN 14
#define SHPWLEN 8
#define CLTYPE_LEN 12
#define MAXCOMMENTSZ 256
#define LM20_MAXCOMMENTSZ 48
#define QNLEN NNLEN
#define LM20_QNLEN LM20_NNLEN
#define ALERTSZ 128
#define MAXDEVENTRIES (sizeof(int)*8)
#define NETBIOS_NAME_LEN 16
#define MAX_PREFERRED_LENGTH ((DWORD)-1)
#define CRYPT_KEY_LEN 7
#define CRYPT_TXT_LEN 8
#define ENCRYPTED_PWLEN 16
#define SESSION_PWLEN 24
#define SESSION_CRYPT_KLEN 21
#define PARMNUM_ALL             0
#define PARM_ERROR_UNKNOWN ((DWORD)-1)
#define PARM_ERROR_NONE 0
#define PARMNUM_BASE_INFOLEVEL 1000
#define PLATFORM_ID_DOS 300
#define PLATFORM_ID_OS2 400
#define PLATFORM_ID_NT 500
#define PLATFORM_ID_OSF 600
#define PLATFORM_ID_VMS 700
/* new typedef in W2K */
#if defined( _WIN32_WINNT ) || defined( WINNT ) || defined( FORCE_UNICODE )
#define LMSTR   LPWSTR
#define LMCSTR  LPCWSTR
#else
#define LMSTR   LPSTR
#define LMCSTR  LPCSTR
#endif

#endif
