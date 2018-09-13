#include "priv.h"

//***   dbutil.cpp -- debug helpers
// DESCRIPTION
//  this file has the shared-source 'master' implementation.  it is
// #included in each DLL that uses it.
//  clients do something like:
//      #include "priv.h"   // for types, ASSERT, DM_*, DF_*, etc.
//      #include "../lib/dbutil.cpp"

#ifdef DEBUG // {

#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "shelldbg"
#define SZ_MODULE           "SHELLDBG"
#define DECLARE_DEBUG
#include <debug.h>

#if ( _X86_)

#if 0
// (These are deliberately CHAR)
CHAR const FAR c_szNull[] = "";
CHAR const FAR c_szZero[] = "0";
CHAR const FAR c_szIniKeyBreakFlags[] = "BreakFlags";
CHAR const FAR c_szIniKeyTraceFlags[] = "TraceFlags";
CHAR const FAR c_szIniKeyFuncTraceFlags[] = "FuncTraceFlags";
CHAR const FAR c_szIniKeyDumpFlags[] = "DumpFlags";
CHAR const FAR c_szIniKeyProtoFlags[] = "Prototype";
#endif

// warning: these macros assume we have an ebp-linked chain.  for debug we do.
#define BP_GETOLDBP(pbp)    (*((int *)(pbp) + 0))
#define BP_GETRET(pbp)      (*((int *)(pbp) + 1))

//***   DBGetStackBack -- walk stack frame
// #if 0 w/ sanity checks #endif
// ENTRY/EXIT
//  pfp         INOUT ptr to frame ptr (IN:starting, OUT:ending)
//  pstkback    OUT:fp/ret pairs, IN:optional size/addr pairs for sanity check
//  nstkback    ARRAYSIZE(pstckback)
//  #if 0
//  ncheck      # of IN pstkback size/addr sanity-check pairs
//  #endif
//  n           (return) # of frames successfully walked
// DESCRIPTION
//  fills in OUT pstckback w/ backtrace info for nstckback frames.
//  #if 0
//  if ncheck > 0, makes sure that initial backtrace entries are in the
//  range of the function specified by the IN size/addr pairs in pstkback.
//  #endif
// NOTES
//  BUGBUG not sure if we return the right pfp' value (untested)
int DBGetStackBack(int *pfp, struct DBstkback *pstkback, int nstkback /*,int nchk*/)
{
    int fp = *pfp;
    int ret;
    int i = 0;

    __try {
        for (; i < nstkback; i++, pstkback++) {
            ret = BP_GETRET(fp);
#if 0
            if (i < ncheck && pstkback->ret != 0) {
                ASSERT(pstkback->fp == 0 || pstkback->fp == -1 || pstkback->fp <= 512);
                if (!(pstkback->ret <= ret && ret <= pstkback->ret + pstkback->fp)) {
                    // constraint violated
                    break;
                }
            }
#endif
            fp = BP_GETOLDBP(fp);
            pstkback->ret = ret;
            pstkback->fp = fp;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        TraceMsg(TF_ALWAYS, "atm: GPF");
        // just use last 'ret' we had
    }

    *pfp = fp;
    return i;
}

#endif

//***   SearchDW -- scan for DWORD in buffer
// ENTRY/EXIT
//  pdwBuf  buffer
//  cbBuf   size of buffer in *bytes* (*not* DWORDs)
//  dwVal   DWORD we're looking for
//  dOff    (return) byte offset in buffer; o.w. -1 if not found
//
int SearchDW(DWORD *pdwBuf, int cbBuf, DWORD dwVal)
{
    int dOff;

    for (dOff = 0; dOff < cbBuf; dOff += SIZEOF(DWORD), pdwBuf++) {
        if (*pdwBuf == dwVal)
            return dOff;
    }

    return -1;
}


#endif // }
