#ifndef _EAPP_HXX_
#define _EAPP_HXX_

#include <urlint.h>
#include <stdio.h>
#include <sem.hxx>
#include <wininet.h>
#include <crtsubst.h>

#if DBG == 1

extern LPCWSTR v_gDbgFacilitieNames[];

void EProtUrlSpy(int iOption, const char *pscFormat, ...);
#ifndef unix
    DECLARE_DEBUG(EProt)
#endif /* unix */
#   define EProtDebugOut(x)     EProtUrlSpy x
#   define EProtAssert(x)       Win4Assert(x)
#   define EProtVerify(x)       EProtAssert(x)
#   undef  DEB_PLUGPROT
#   define DEB_PLUGPROT         DEB_USER1
#   undef  DEB_BASE
#   define DEB_BASE             DEB_USER2
#   define DEB_RES              DEB_USER3
#   define DEB_NAMESPACE        DEB_USER4
#   define DEB_NOTFSINK         DEB_USER5

/**
#   undef UrlMkDebugOut
#   undef UrlMkAssert
#   undef UrlMkVerify
#   undef DEB_ASYNCAPIS
#   undef DEB_URLMON
#   undef DEB_ISTREAM
#   undef DEB_DLL
#   undef DEB_FORMAT
#   undef DEB_CODEDL

#   undef TransDebugOut
#   undef TransAssert
#   undef TransVerify
#   undef DEB_BINDING
#   undef DEB_TRANS
#   undef DEB_TRANSPACKET
#   undef DEB_DATA
#   undef DEB_PROT
#   undef DEB_PROTHTTP
#   undef DEB_TRANSMGR
// needed for the CThreadPacket
//#   undef DEB_SESSION
**/



#elif DBGASSERT == 1

#   define EProtAssert(x)  (void) ((x) || (DebugBreak(),0))

#   define EProtDebugOut(x)
#   define EProtVerify(x)         x


#else

#   define EProtDebugOut(x)
#   define EProtAssert(x)
#   define EProtVerify(x)         x

#endif

#include "selfreg.hxx"
#include "cmimeft.hxx"
#include "comp.hxx"
#include "ccodeft.hxx"

#include "protbase.hxx"
#include "cdlbsc.hxx"
#include "cdlprot.hxx"
#include "clshndlr.hxx"

#ifdef EAPP_TEST
#include "mimehndl.hxx"
#include "resprot.hxx"
#include "nspohsrv.hxx"
#include "multicst.hxx"
#endif //EAPP_TEST

#endif //_EAPP_HXX_

 

