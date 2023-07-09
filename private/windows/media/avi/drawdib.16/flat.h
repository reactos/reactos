#ifndef WIN32
extern void  FAR PASCAL FlatInit(void);
extern void  FAR PASCAL FlatTerm(void);
extern DWORD FAR PASCAL MapFlat(LPVOID);
#endif
