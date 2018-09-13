// placeholder

#ifdef __cplusplus
extern "C" {
#endif

void * WINAPI MemAlloc(long cb);
void * WINAPI MemReAlloc(void * pb, long cb);
BOOL WINAPI MemFree(void * pb);

#define memcmpf(d,s,l)  memcmp((d),(s),(l))
#define memcpyf(d,s,l)  memcpy((d),(s),(l))
#define memmovef(d,s,l) MoveMemory((d),(s),(l))
#define memsetf(s,c,l)  memset((s),(c),(l))
#define strcatf(d,s)    lstrcat((d),(s))
#define strcmpf(s1,s2)  lstrcmp(s1,s2)
#define strcpyf(d,s)    lstrcpy((d),(s))
#define stricmpf(s1,s2) lstrcmpi(s1,s2)
#define strlenf(s)      lstrlen((s))

LPSTR WINAPI strncpyf(LPSTR, LPCSTR, UINT);
LPSTR WINAPI strrchrf(LPCSTR, UINT);
UINT  WINAPI strspnf(LPCSTR, LPCSTR);
LPSTR WINAPI strchrf(LPCSTR, UINT);
int   WINAPI strnicmpf(LPCSTR, LPCSTR, UINT);
UINT  WINAPI strcspnf(LPCSTR, LPCSTR);
LPSTR WINAPI strtokf(LPSTR, LPSTR);
LPSTR WINAPI strstrf(LPCSTR, LPCSTR);
LPSTR WINAPI stristrf(LPCSTR, LPCSTR);
int   WINAPI strncmpf(LPCSTR, LPCSTR, UINT);

LPSTR WINAPI struprf(LPSTR s);

extern BOOL fDBCSEnabled;

#ifdef __cplusplus
#define IS_LEAD_BYTE(c)     (fDBCSEnabled ? ::IsDBCSLeadByte(c) : 0)
#else
#define IS_LEAD_BYTE(c)     (fDBCSEnabled ? IsDBCSLeadByte(c) : 0)
#endif

void WINAPI InitStringLibrary(void);

#ifdef __cplusplus
}
#endif

