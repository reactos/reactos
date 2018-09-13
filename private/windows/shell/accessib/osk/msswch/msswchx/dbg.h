#ifndef __DBG_H__
#define __DBG_H__  

#ifdef __cplusplus
extern "C" 
{
#endif  // __cplusplus

int Dbg(const char *f, ...);
int xDbg(const char *f, ...);

#define xDBG xDbg
//#define xDBG(a) 		// alternate form

#ifdef _DEBUG
   #define DBGMSG Dbg   // rename DBG to DBGMSG (DBG is used elsewhere)
#else
   #define DBGMSG xDbg  // rename DBG to DBGMSG
	// #define DBG(a) 	// alternate form
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // __DBG_H__
