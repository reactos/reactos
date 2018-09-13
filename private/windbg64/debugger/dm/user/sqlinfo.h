

// Each process has a pointer to a SqlDBInfo struct.
// Each of these contain SqlDLL members for each of the supported DLLs

#include "sdci.h"

// the name of hook function
#define	MAGIC_EXPORT	_T("SQLDebug")


// DMBaseClass is a little class so that memory allocs go through the 'kernel' interface

class DMBaseClass
{
public:
	void *operator new(size_t);
	void operator delete( void *);
};

// classes to make DLLs that we register with easier to handle

class SqlDBInfo;

// one of these per DLL that we know about (ODBC, DBLib, etc)

class SqlDLL : public DMBaseClass
{
private:
	SqlDLL *m_pNextDll;						// linked list

public:

	const TCHAR *m_szBaseName;				// unchanging
	      TCHAR *m_szPathName;				// full path, dynamic
	const TCHAR *m_szExportName;

	SqlDLL( const TCHAR*, const TCHAR *, SqlDBInfo*, DWORD );
	~SqlDLL();
	void DoRegister( const TCHAR *, BOOL );
	BOOL RealRegister();
	void RegisterIfNeeded();
	void UnRegister();

	friend class SqlDBInfo;

private:
	BOOL m_bRegistered : 1;					// TRUE if we have called
	BOOL m_bRegisterFailed : 1;				// TRUE if we have tried and failed
	BOOL m_bNeedToRegister : 1;				// TRUE if couldn't register due to startup

	const DWORD m_MinMajor;
	HINSTANCE m_hModule;
	pfnSQLDebug m_pfnRegister;
	SqlDBInfo *m_pOwner;
	
	BOOL RegisterFailed( BOOL );
	BOOL BadVersion();
};

// one of these per process being debugged

class SqlDBInfo : public DMBaseClass
{
public:
	SqlDBInfo( HPRCX );
	~SqlDBInfo();

	void FreeDLLs();
	void SetSDCI( SDCI *pSDCI );
	BOOL DebugEnabled() { return this ? m_bWantSqlDebug : NULL; }
	BOOL MakeThread();
	void WakeThread();
	void SleepThread();
	void ProcessTerminate();
	XOSD DoCommand( LPBYTE command );

	HPRCX m_pProcess;						// process we are attached to
	HANDLE m_hSqlThread;
	DWORD m_idSqlThread;
	BOOL m_bWantSqlDebug : 1;
	BOOL m_bSqlThreadRunning : 1;
	BOOL m_bContextFresh : 1;
	CONTEXT m_ctxSql;
	SDCI m_SDCI;

	SqlDLL *GetFirstDll() const
	{
		return m_pDllList;
	};

	SqlDLL *GetNextDll( SqlDLL *pCurrent ) const
	{
		assert( pCurrent!=NULL );
		return pCurrent->m_pNextDll;
	};


private:
	SqlDLL *m_pDllList;						// list of DLLs we are interested in
};

#define	SDCI_CODE_BREAK		1
//#define	SDCI_CODE_ENABLE	2
#define	SDCI_CODE_RESUME	3

