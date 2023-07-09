#ifdef _WIN64
typedef __int64 DWORD_PTR, *PDWORD_PTR;
#else
typedef long DWORD_PTR, *PDWORD_PTR;
#endif
