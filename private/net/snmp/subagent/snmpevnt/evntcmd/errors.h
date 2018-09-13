#ifndef _ERRORS_H
#define _ERRORS_H

#define WARN_SILENT     0
#define WARN_ERROR      1
#define WARN_ALERT      3
#define WARN_CHECKPOINT 5
#define WARN_ATTENTION  8
#define WARN_TRACK      10

DWORD _E(DWORD dwErrCode,
         DWORD dwMsgId,
         ...);

DWORD _W(DWORD dwWarnLevel,
         DWORD dwMsgId,
         ...);

#endif

