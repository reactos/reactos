
#ifndef  H_WIN32K_DEBUG
#define  H_WIN32K_DEBUG

#ifdef CHECKED_BUILD
#define  FIXME(S) DbgPrint ("win32k: FIXME at: File:%s line:%d reason:%s", __FILE__, __LINE__, S)
#else
#define  FIXME(S)
#endif

#endif

