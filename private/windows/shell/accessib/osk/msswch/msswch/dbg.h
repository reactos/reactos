#ifndef __DBG_H__
#define __DBG_H__  

#ifdef __cplusplus
extern "C" 
{
#endif  // __cplusplus

int Dbg(const PTCHAR f, ...);
int xDbg(const PTCHAR f, ...);

#define xDBG xDbg
//#define xDBG(a) 		// alternate form

#ifdef _DEBUG
   #define DBGMSG Dbg   // rename DBG to DBGMSG
#else
   #define DBGMSG xDbg  // rename DBG to DBGMSG
	// #define DBG(a) 	// alternate form
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // __DBG_H__
