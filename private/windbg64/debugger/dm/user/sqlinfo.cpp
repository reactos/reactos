
#include "precomp.h"
extern "C" {
#include "dmsql.h"
}

#if SQLDBG

#include "sqlinfo.h"
#include "resource.h"

extern "C" HINSTANCE hInstance;


SqlDLL::SqlDLL( const TCHAR *pBaseName, const TCHAR *pExport, SqlDBInfo *pOwner, DWORD ver )
	: m_MinMajor(ver), m_pOwner(pOwner), m_szExportName( pExport ), m_szBaseName( pBaseName )
{
	m_pNextDll = NULL;
	m_szPathName = NULL;
	m_bRegistered = FALSE;
	m_bRegisterFailed = FALSE;
	m_bNeedToRegister = FALSE;
	m_hModule = NULL;
	m_pfnRegister = NULL;
}

SqlDLL::~SqlDLL()
{
	UnRegister();

	if (m_hModule)
	{
		FreeLibrary( m_hModule );
		m_hModule = NULL;
	}

	if (m_szPathName)
		MHFree( m_szPathName );
}

BOOL SqlDLL::RegisterFailed( BOOL bLoud )
{
	m_bRegisterFailed = TRUE;
	if (bLoud)
	{
		// tell the user that the DLL doesn't do debugging
		TCHAR fmt[256];
		LoadString( hInstance, IDS_NOSQLDEBUG, fmt, sizeof(fmt) );

		TCHAR msg[256 + _MAX_PATH ];
		msg[ 0 ] = 0x1;									// special marker
		wsprintf( msg+1, fmt, m_szPathName );
		assert( (_tcslen(msg+1)+2) != sizeof(DWORD) );
        DMSendDebugPacket(dbcSQLThread, m_pOwner->m_pProcess->hpid, NULL, _tcslen(msg+1)+2, msg);
	}
	return FALSE;
}


// this is called when we need to register the current debuggee with ODBC/DBLib
// we only register once per debuggee with each system
// returns TRUE if we registered (or have already registered)

BOOL SqlDLL::RealRegister()
{
	if (m_bRegistered)
		return TRUE;

	if (m_bRegisterFailed)
		return FALSE;

	if (!m_pOwner->m_SDCI.cbData)
		return RegisterFailed(FALSE);

	if (m_szPathName==NULL)
	{
		assert(!"RealRegister called without DLLname");
		return RegisterFailed(FALSE);
	}

	// load the DLL first from the same place as the user's exe did
	if (!m_hModule)
	{
		m_hModule = LoadLibrary( m_szPathName );
		if (!m_hModule)
			return RegisterFailed(FALSE);
	}
	// then find the magic export
	if (!m_pfnRegister)
	{
		m_pfnRegister = (pfnSQLDebug)GetProcAddress( m_hModule, MAGIC_EXPORT );
		if (!m_pfnRegister)
		{
			FreeLibrary( m_hModule );
			m_hModule = NULL;
			return RegisterFailed(TRUE);
		}
	}

	// check the version info to see if it supports debugging
	if (BadVersion())
		return RegisterFailed(TRUE);

	// now we can get the PID
	if (m_pOwner->m_SDCI.pid==0)
		m_pOwner->m_SDCI.pid = (ULONG)m_pOwner->m_pProcess->pid;		// we do the pid if user passes in zero
	// now call the export
	m_pOwner->m_SDCI.fOption = TRUE;
	m_bRegistered = m_pfnRegister( &m_pOwner->m_SDCI );
#ifdef _DEBUG
	if (!m_bRegistered)
		OutputDebugString("SQLDebug returned FALSE\n");
#endif

	if (!m_bRegistered)
		return RegisterFailed(TRUE);

	if ( m_pOwner->m_hSqlThread==NULL )
		m_pOwner->MakeThread();

	if (m_pOwner->m_hSqlThread==NULL)
	{
		UnRegister();					// as thread create failed
		m_bRegistered = FALSE;
		FreeLibrary( m_hModule );
		m_hModule = NULL;
		return RegisterFailed(FALSE);
	}

	return m_bRegistered;
}

// called when the load of the exe has finished. If we had a registration pending,
// now would be a good time to do it

void SqlDLL::RegisterIfNeeded()
{
	if (!m_bNeedToRegister)
		return;

	m_bNeedToRegister = FALSE;

	RealRegister();					// so do it now
}

// called when the DLL load is detected. If possible, then register, else postpone
// unlike 4.x we get a full path to the actual DLL being loaded

void SqlDLL::DoRegister( const TCHAR *pFullPath, BOOL bExport )
{
	// remember the full path to the DLL
	if (m_szPathName)
		MHFree( m_szPathName );
	m_szPathName = (TCHAR*)MHAlloc( _tcslen( pFullPath ) + 1 );
	_tcscpy( m_szPathName, pFullPath );

	if (!bExport)
	{
		// if the DLL we loaded had no export, we can't debug
		// with it, so inform the user
		RegisterFailed(TRUE);
		return;
	}

	if (m_pOwner->m_pProcess->pstate & ps_preStart)
		m_bNeedToRegister = TRUE;			// postpone registration for static loads
	else
		RealRegister();						// do registration immediately for dynaloads
}

void SqlDLL::UnRegister()
{
	if (m_bRegistered)
	{
		m_pOwner->m_SDCI.fOption = FALSE;
		m_pfnRegister( &m_pOwner->m_SDCI );
		m_bRegistered = FALSE;
	}
}



// returns TRUE if DLL is recognised as being a version that does
// not support debugging

BOOL SqlDLL::BadVersion()
{
	BYTE verBuf[1024];
	if (!GetFileVersionInfo( (char*)m_szPathName, 0, sizeof(verBuf), &verBuf ))
		return FALSE;								// no version info = can't tell so try anyway

	VS_FIXEDFILEINFO *pVersion;
	UINT cSize;
	if (!VerQueryValue( verBuf, TEXT("\\"), (LPVOID*)&pVersion, &cSize ))
		return FALSE;

	// check version fields
	// DBLib    is OK from 6.50.00.00 to 6.50.02.00 inc, then 6.50.02.02
	// SQLSVR32 is OK from 2.65.00.00 to 2.65.02.00 inc, then 2.65.02.02
	if (pVersion->dwProductVersionMS < m_MinMajor)
	{
		// major version too old
		return TRUE;
	}
	if (pVersion->dwProductVersionMS == m_MinMajor)
	{
		// major version right, check not the release 1.0 version
		if (pVersion->dwProductVersionLS == 0x20001)
			return TRUE;
	}
	else
	{
		// it is a major new release, so assume it is OK
		;
	}
	return FALSE;
}


void * DMBaseClass::operator new(size_t cSize)
{
	return MHAlloc( cSize );
}

void DMBaseClass::operator delete( void * pData )
{
	MHFree( pData );
}

#endif
