#ifndef _PHBKDEBUG
#define _PHBKDEBUG

//extern "C" {
	void Dprintf(LPCSTR pcsz, ...);
//}

#ifdef DEBUG
//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus
	BOOL FAssertProc(LPCSTR szFile,  DWORD dwLine, LPCSTR szMsg, DWORD dwFlags);
	void DebugSz(LPCSTR psz);
//#ifdef __cplusplus
//}
//#endif // __cplusplus
	#define AssertSzFlg(f, sz, dwFlg)		( (f) ? 0 : FAssertProc(__FILE__, __LINE__, sz, dwFlg) ? DebugBreak() : 1 )
	#define AssertSz(f, sz)				AssertSzFlg(f, sz, 0)
	#define Assert(f)					AssertSz((f), "!(" #f ")")
#else
	#define DebugSz(x)
	#define AssertSzFlg(f, sz, dwFlg) (f)
	#define AssertSz(f, sz) (f)
	#define Assert(f) (f)
#endif
#endif //_PHBKDEBUG
