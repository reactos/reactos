//
// Prototypes for debug dump functions
//

#ifndef _DUMP_H_
#define _DUMP_H_

#ifdef DEBUG

EXTERN_C LPCTSTR Dbg_GetReadyState(LONG state);

#ifndef DOWNLEVEL_PLATFORM
EXTERN_C LPCTSTR Dbg_GetGuid(REFGUID rguid, LPTSTR pszBuf, int cch);
#endif //DOWNLEVEL_PLATFORM

EXTERN_C LPCTSTR Dbg_GetBool(BOOL bVal);

#ifdef _DATASRC_H_
EXTERN_C LPCTSTR Dbg_GetLS(LOAD_STATE state);
#endif

#ifdef _MTXARRAY_H_
EXTERN_C LPCTSTR Dbg_GetAppCmd(APPCMD appcmd);
#endif

#ifdef __simpdata_h__
EXTERN_C LPCTSTR Dbg_GetOSPRW(OSPRW state);
#endif

#else

#define Dbg_GetReadyState(state)            TEXT("")

#ifndef DOWNLEVEL_PLATFORM
#define Dbg_GetGuid(rguid, pszBuf, cch)     TEXT("")
#endif //DOWNLEVEL_PLATFORM

#define Dbg_GetBool(bVal)                   TEXT("")
#define Dbg_GetOSPRW(status)                TEXT("")
#define Dbg_GetLS(status)                   TEXT("")
#define Dbg_GetAppCmd(appcmd)               TEXT("")

#endif // DEBUG


#endif // _DUMP_H_
