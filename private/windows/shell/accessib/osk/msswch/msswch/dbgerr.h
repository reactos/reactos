// DBGERR.H 

#ifndef __DBGERR_H__
#define __DBGERR_H__  

#ifdef __cplusplus
extern "C" 
{
#endif  // __cplusplus

void dbgerr( LPTSTR szInfo, BOOL bError );

#define xDBGERR(a,b)

#ifdef _DEBUG
	#define DBGERR(a,b) dbgerr(a,b)
#else
	#define DBGERR(a,b)
#endif



#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // __DBGERR_H__
