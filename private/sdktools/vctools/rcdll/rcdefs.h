/*
** RCDEFS.H
**
** author:  JeffBog
**
** Change Log
**	FloydR	3/7/94	Renamed from rcdll.h
*/

#ifndef _RCDEFS_H
#define _RCDEFS_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define RC_OK			1
#define RC_FAILED		0
#define RC_CANCELED		-1

#define WM_RC_ERROR 	(0x0400+0x0019)
#define WM_RC_STATUS    (0x0400+0x0020)


// How often do I update status information (lineo & RC_xxx == 0)
#define RC_PREPROCESS_UPDATE	63
#define RC_COMPILE_UPDATE		31
extern void UpdateStatus(unsigned nCode, unsigned long dwStatus);

#define MAX_SYMBOL 247 /* from ApStudio */

typedef int (CALLBACK *RC_MESSAGE_CALLBACK)(ULONG, ULONG, PCHAR);

typedef struct tagRESINFO_PARSE
{
	LONG size;
	PWCHAR type;
	USHORT typeord;
	PWCHAR name;
	USHORT nameord;
	WORD flags;
	WORD language;
	DWORD version;
	DWORD characteristics;
} RESINFO_PARSE, *PRESINFO_PARSE;

typedef struct tagCONTEXTINFO_PARSE
{
	HANDLE hHeap;
	HWND hWndCaller;
	RC_MESSAGE_CALLBACK lpfnMsg;
	unsigned line;
	PCHAR format;
} CONTEXTINFO_PARSE, *PCONTEXTINFO_PARSE;

typedef int (CALLBACK *RC_PARSE_CALLBACK)(PRESINFO_PARSE lpRes, void** lplpData,
	PCONTEXTINFO_PARSE lpContext);

typedef int (CALLBACK *RCPROC)(HWND hWndCaller, int fStatus,
		RC_MESSAGE_CALLBACK lpfnMsg, RC_PARSE_CALLBACK lpfnParse,
		int argc,	PCHAR* argv);

int CALLBACK RC(HWND hWndCaller, int fStatus,
		RC_MESSAGE_CALLBACK lpfnMsg, RC_PARSE_CALLBACK lpfnParse,
		int argc, PCHAR* argv);

int RCPP(int argc, PCHAR *argv, PCHAR env);

#define RC_ORDINAL (MAKEINTRESOURCE(2))

#ifdef __cplusplus
}
#endif

#endif // _RCDEFS_H
