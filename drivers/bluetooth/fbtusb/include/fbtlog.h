#ifndef _FBT_DEBUG
#define _FBT_DEBUG

#define fbtLog_None			0
#define fbtLog_Failure		1
#define fbtLog_Warning		2
#define fbtLog_Notice		3
#define fbtLog_Enter		4
#define fbtLog_Exit			4
#define fbtLog_Verbose		5
#define fbtLog_Exception	0

#ifdef __cplusplus
extern "C" {
#endif

void fbtLog(unsigned int nLevel, const char *szText, ...);
BOOL fbtLogSetFile(char *szDebugFile);
void fbtLogSetLevel(unsigned int nLevel);
unsigned int fbtLogGetLevel(void);

#ifdef __cplusplus
}
#endif

#endif
