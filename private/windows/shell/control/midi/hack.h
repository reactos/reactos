#if defined(WIN32)

#define _based(x)

#define huge

#define _loadds

#define _fmemset memset
#define _fmemcpy memcpy
#define DosDelete(fn) DeleteFile(fn)

#endif //WIN32

