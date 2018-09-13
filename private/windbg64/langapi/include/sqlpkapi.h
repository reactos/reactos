/////////////////////////////////////////////////////////////////////////////
//	SQLPKAPI.H
//		SQL specific package interface declarations.

// IMPORTANT: this file is used by debugger components so:
// 1. Needs to be C-parsable ie no C++ luxuries (easier said than done I know)
// 2. Must not require MFC types or headers
// 3. Make sure you use the OLE macros in a C-compatible way (ditto)
// 4. If this changes, the debugger bits will need a rebuild

#ifndef __SQLPKAPI_H__
#define __SQLPKAPI_H__

interface ISqlExec;
interface ISrcDebug;

#ifdef __cplusplus
typedef ISqlExec* LPSQLEXEC;
typedef ISrcDebug* LPSRCDEBUG;
#endif

/////////////////////////////////////////////////////////////////////////////
// ISqlExec

#undef  INTERFACE
#define INTERFACE ISqlExec

enum LoadNotifyType {lntBeforeLoad, lntAfterLoad, lntTerminatingAndRestoringBPs, lntDebugSessionEnding, lntSQLThreadCreated,
			lntProgLoad, lntProgTerm, lntBadDriver};

typedef WORD SPCOOKIE;
typedef SPCOOKIE *PSPCOOKIE;

DECLARE_INTERFACE_(ISqlExec, IUnknown)
{
	// IUnknown methods

	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// ISqlExec methods

	STDMETHOD_(void, GoSql)(THIS) PURE;
	STDMETHOD_(void, StepIntoSql)(THIS) PURE;
	STDMETHOD_(void, StepOverSql)(THIS) PURE;
	STDMETHOD_(void, AddTmpBpSql)(THIS) PURE;
	STDMETHOD_(void, ToggleBPSql)(THIS) PURE;

	STDMETHOD_(BOOL, DebuggeeLoadNotify)(THIS_ enum LoadNotifyType lnt, LPVOID) PURE;
	STDMETHOD_(BOOL, GetSqlSPInfo)(THIS_ SPCOOKIE Cookie, char *ServerName, char *DBN, char *Name, WORD *cLine, WORD) PURE;
	STDMETHOD_(BOOL, GetSqlLocation)(THIS_ WORD *pline, PSPCOOKIE pCookie, WORD) PURE;

	STDMETHOD_(BOOL, FindSqlSymbol)(THIS_ PVOID pSymInfo, WORD nlvl ) PURE;
	STDMETHOD_(BOOL, SetSqlSymbol)(THIS_ PVOID pSymInfo, WORD nlvl ) PURE;
	STDMETHOD_(UINT, CountSqlLocals)(THIS_ WORD nlvl, BOOL ) PURE;
	STDMETHOD_(BOOL, GetSqlLocalName)(THIS_ char*, WORD, UINT, BOOL ) PURE;

	STDMETHOD_(void, StepToReturnSql)(THIS) PURE;
	STDMETHOD_(void, HackRefCount)(THIS_ int ) PURE;
	STDMETHOD_(void, KillSql)(THIS_ unsigned long, BOOL fAsync ) PURE;

	STDMETHOD_(void*,FindSqlDocument)(THIS_ char*, BOOL) PURE;
	STDMETHOD_(BOOL, GetSqlStack)(THIS_ WORD*, int, PSPCOOKIE, WORD*) PURE;
	STDMETHOD_(BOOL, SetSqlBreakpoint)(THIS_ BOOL, SPCOOKIE, WORD ) PURE;

	STDMETHOD_(BOOL, FindCookie)(THIS_ char* szSrvr, char* szDBN, char* szProc, SPCOOKIE *pCookie) PURE;

	STDMETHOD_(BOOL, ParseSqlPath)(THIS_ char* szPath, char *szServ, char *szDB, char *szSP, int) PURE;
	STDMETHOD_(BOOL, BuildSqlPath)(THIS_ char* szPath, char *szServ, char *szDB, char *szSP, int) PURE;
	STDMETHOD_(BOOL, GetCodeLines)(THIS_ SPCOOKIE, int*pSize, unsigned short** ppList) PURE;

	STDMETHOD_(BOOL, InProcSql)(THIS_ char* szPath) PURE;
    STDMETHOD(LoadDocument)(LPCTSTR szName) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// ISrcDebug

#undef  INTERFACE
#define INTERFACE ISrcDebug

DECLARE_INTERFACE_(ISrcDebug, IUnknown)
{
	// IUnknown methods

	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	// ISrcDebug methods

	STDMETHOD_(BOOL, DebugSystemService)(THIS_ int i1, int i2, int i3, void *pv1, int i4, void *pv2) PURE;
	STDMETHOD_(void, UpdateBPViews)(THIS) PURE;
};


#endif	// __SQLPKAPI_H__
