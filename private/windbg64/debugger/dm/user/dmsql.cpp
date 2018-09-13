
// SQL-specific support routines for the DM

#include "precomp.h"
extern "C" {
#include "dmsql.h"
}

#if SQLDBG

#include "sqlinfo.h"
extern "C" {
#include "i386troj.h"
};
#include "sqlprxky.h"

/* Here is how SQL debugging works in the DM:

  If the ENT package isnt found, all this stuff is disabled.

  Just before starting the debuggee, the src/ent package sends down a SDCI packet.

  When a DLL is loaded, we get a look-in to see if it is an 'interesting' one.
  If it is, we pass on the earlier SDCI packet to 'interesting' DLLs together with a pid.
  At this time we also create a special 'invisible' thread in the debugger that we
  use for SQL notifications.
*/


// super-hack until we have a pointer in the real process structure

static SqlDBInfo *pHackSqlDB;


// global to determine whether the next launched exe should be SQL debugged

static BOOL g_bWantSqlDebug;


SqlDBInfo *GetSqlDB( HPRCX pprc )
{
        if (pHackSqlDB==NULL)
                pHackSqlDB = new SqlDBInfo( pprc );

        return pHackSqlDB;
}

// called just before loading an exe, we can use the flags to dermine whether
// SQL debugging is desired or not (its a global)

void DMSqlPreLoad( DWORD flags )
{
        g_bWantSqlDebug = (flags & ulfSqlDebug) ? TRUE : FALSE;
}


// called after we hit the very first breakpoint. This is a good time to
// trojan code for statically-linked DLLs

void DMSqlStartup( HPRCX pprc )
{
        SqlDBInfo *pSqlDB = GetSqlDB( pprc );

        if (!pSqlDB->DebugEnabled())
                return;

        SqlDLL *pDll = pSqlDB->GetFirstDll();
        while (pDll)
        {
                pDll->RegisterIfNeeded();
                pDll = pSqlDB->GetNextDll( pDll );
        }

        // if we haven't made the thread, lets do it now, as leaving it
        // until dyna DLL-load time screws us on Win95 (sometimes)
        if ( pSqlDB->m_hSqlThread==NULL )
        {
                pSqlDB->MakeThread();
        }
}

// called when DLLs are being loaded so we can see if it is an 'interesting' one

void
DMSqlLoadDll(
    HPRCX hprc,
    LOAD_DLL_DEBUG_INFO64 *ldd,
    int iDll
    )
{
        SqlDBInfo *pSqlDb = GetSqlDB( hprc );

        if (!pSqlDb->DebugEnabled())
                return;

        // does the DLL have an interesting name?
        // we want SQLSRV32.DLL or DBLib
        // the export is called 'SQLDebug' in both DLLs
        TCHAR szBase[_MAX_FNAME];
        TCHAR szExt [_MAX_EXT];

        TCHAR *pFullName = hprc->rgDllList[iDll].szDllName;

        if (pFullName==NULL)
        {
                char iName[_MAX_PATH];

                // On Win95 pFullName is always NULL, so get the name the other way
                DWORDLONG pName = ldd->lpImageName;
                if (pName &&
                        DbgReadMemory( hprc, pName, &pName, sizeof(DWORD), NULL) &&
                        pName &&
                        DbgReadMemory( hprc, pName, iName, sizeof(iName), NULL)
                        )
                {
                        // got the name from the clients address space
                        pFullName = iName;
                }
                else {
                        return;                                                 // cannot get name, so bail
                }
        }

        _tsplitpath( pFullName, NULL, NULL, szBase, szExt );
        _tcscat( szBase, szExt );

        SqlDLL *pDll = pSqlDb->GetFirstDll();

        // if proxy, use first DLL (which had better be ODBC)
        if (_tcsicmp( szBase, szSqlProxyBase ".exe")==0)
        {
                assert(pDll);
                pDll->DoRegister( "SQLSRV32.DLL", TRUE );
                pDll = NULL;
        }

        while (pDll)
        {
                if (_tcsicmp( szBase, pDll->m_szBaseName )==0)
                {
                        pDll->DoRegister( pFullName, FGetExport( &hprc->rgDllList[iDll], (HFILE)ldd->hFile, pDll->m_szExportName, NULL) );
                        break;
                }
                pDll = pSqlDb->GetNextDll(pDll);
        }

}

void DMSqlTerminate( HPRCX pprc )
{
        SqlDBInfo *pSqlDB = GetSqlDB( pprc );

        if (pSqlDB->DebugEnabled())
        {
                // we are not registered with anything any more
                pSqlDB->ProcessTerminate();
        }

        if (pHackSqlDB)
        {
                delete pHackSqlDB;
                pHackSqlDB = NULL;
        }
}

// called when we get a ssvcSQLDebug command to enable/disable

XOSD DMSqlSystemService( HPRCX pprc, LPBYTE command )
{
        SqlDBInfo *pSqlDB = GetSqlDB( pprc );

        if (pSqlDB==NULL)
                return xosdNone;

        // don't check debugging enabled here - that is the whole point

        return pSqlDB->DoCommand( command );
}


SqlDBInfo::SqlDBInfo( HPRCX hprcx )
{
        m_pDllList = NULL;

        m_pProcess = hprcx;
        m_hSqlThread = NULL;
        m_idSqlThread = 0;
        m_bSqlThreadRunning = FALSE;
        memset( &m_SDCI, 0, sizeof(m_SDCI) );

        m_bWantSqlDebug = g_bWantSqlDebug;                      // take current global setting

        if (m_bWantSqlDebug)
        {
                // build list of DLLs that we want to know about
                // ODBC must come first as the Proxy code relies on the fact
                m_pDllList = new SqlDLL( "SQLSRV32.DLL", MAGIC_EXPORT, this, 0x00020041 );              // ODBC v2.65
                assert( m_pDllList );
                m_pDllList->m_pNextDll =
                                new SqlDLL( "NTWDBLIB.DLL", MAGIC_EXPORT, this, 0x00060032 );           // DBLib v6.50
                assert( m_pDllList->m_pNextDll );
                // at some point, add 3rd party DLLs here by enumerating the registry
        }
}

SqlDBInfo::~SqlDBInfo()
{
        FreeDLLs();
}

// shell calls this with a structure pointer to use

void SqlDBInfo::SetSDCI( SDCI *pSDCI )
{
        m_SDCI = *pSDCI;

        // we make our own copy of the void data so the caller can free it
        if (m_SDCI.pvData)
        {
                m_SDCI.pvData = MHAlloc( m_SDCI.cbData );
                memcpy( m_SDCI.pvData, pSDCI->pvData, m_SDCI.cbData );
        }
}


// make the thread if required. It is the callers responsibility to ensure
// the owner process is in a suitable state to cope with thread creation
// (which means no Win95 until process has finished loading)

BOOL SqlDBInfo::MakeThread()
{
        assert( m_hSqlThread==NULL );                   // make sure we haven't done this already

        m_hSqlThread = NULL;

        // we need the address of DebugBreak. This address will be the same for
        // our client app as it is for us, as it is in the kernel. This is also
        // true for SuspendThread and is processor independent!

        LPTHREAD_START_ROUTINE pStart = (LPTHREAD_START_ROUTINE)DebugBreak;

#ifndef TARGET_i386
        // NT-only code
        HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, m_pProcess->pid );

        if (hProc)
        {
                m_hSqlThread = CreateRemoteThread( hProc, NULL, 512, pStart,
                                0, CREATE_SUSPENDED, &m_idSqlThread );
                CloseHandle( hProc );
        }
#else
        // on x86 we always trojan, even though we could use the NT code above

        // Win95 doesn't do CreateRemoteThread so have to trojan it in, which only
        // works if the process has finished loading

        HTHDX hthd = FindStoppedThread(m_pProcess);
        assert( hthd );
        if (!hthd) {
            return FALSE;
        }

        m_idSqlThread = FTrojanCreateSQLThread( hthd, (DWORD)pStart, &m_hSqlThread );

        if (m_hSqlThread)
        {
                // the thread is lacking in GET_CONTEXT access rights, so increase them
                VERIFY(DuplicateHandle( m_pProcess->rwHand, m_hSqlThread, GetCurrentProcess(), &m_hSqlThread,
                                THREAD_ALL_ACCESS, FALSE, DUPLICATE_CLOSE_SOURCE ));
        }
#endif

        if (m_hSqlThread)
        {
                m_bSqlThreadRunning = FALSE;

                // remember this lovely fresh context
                m_ctxSql.ContextFlags = CONTEXT_FULL;
                VERIFY(GetThreadContext( m_hSqlThread, &m_ctxSql ));
                m_bContextFresh = TRUE;

                // tell our NM the ID of this special thread
                DMSendDebugPacket(dbcSQLThread, m_pProcess->hpid, NULL, sizeof(m_idSqlThread), &m_idSqlThread);
        }

#ifdef _DEBUG
        if (m_hSqlThread==NULL)
                OutputDebugString("ERROR: SQLNM MakeThread failed\n");
#endif

        return m_hSqlThread ? TRUE : FALSE;
}



// wake up the SQL thread, which should cause an int 3

void SqlDBInfo::WakeThread()
{
        if (m_bSqlThreadRunning)
                return;

        m_bSqlThreadRunning = TRUE;

        // the thread is suspended, so set its context so it hits the int 3
        if (!m_bContextFresh)
                m_ctxSql.Eip = (DWORD)DebugBreak;

        VERIFY( SetThreadContext( m_hSqlThread, &m_ctxSql ) );

        // now resume it so it hits it
        ResumeThread( m_hSqlThread );
}

void SqlDBInfo::SleepThread()
{
        HANDLE args[2];
        DWORD result;

        if (!m_bSqlThreadRunning)
                return;

        m_bSqlThreadRunning = FALSE;

        // now get it to suspend itself by pointing to the kernel routine
        // calling SuspendThread from here is bad because it makes the Resume
        // code real horrible under NT as NT cannot resume suspended threads
        VERIFY( GetThreadContext( m_hSqlThread, &m_ctxSql ) );

#if defined(TARGET_i386)

        m_ctxSql.Eip = (DWORD)SuspendThread;

        if (m_bContextFresh)
        {
                // first time so make room on stack
                m_ctxSql.Esp = m_ctxSql.Esp - sizeof(args);
                m_bContextFresh = FALSE;
        }

        args[0] = GetCurrentThread();                                   // pseudo-handle
        args[1] = args[0];

        VERIFY(WriteProcessMemory( m_pProcess->rwHand, (LPVOID)m_ctxSql.Esp, args, sizeof(args), &result ));
        assert(result==sizeof(args));

#else
#pragma error( "Need stack manipulation for SQL support" )
#endif

        VERIFY(SetThreadContext( m_hSqlThread, &m_ctxSql ));

        // hose any cache of the thread's context else Go will cause thread to exit immediately
        HTHDX hthd = HTHDXFromPIDTID( m_pProcess->pid, m_idSqlThread );
        assert(hthd);
        hthd->fContextDirty = FALSE;                                    // don't use cache
        ClearBPFlag( hthd );                                                    // its not a BP now (else it does horrible step stuff)
}

void SqlDBInfo::FreeDLLs()
{
        SqlDLL *pDll = m_pDllList;
        while (pDll)
        {
                SqlDLL *pNext = pDll->m_pNextDll;
                pDll->UnRegister();
                delete pDll;
                pDll = pNext;
        }
        m_pDllList = NULL;
}

// caleed when we get the process terminate message. We need to tell SQL/DBLib that we
// are not interested in this pid any more and free the DLL list

void SqlDBInfo::ProcessTerminate()
{
        FreeDLLs();

        m_bWantSqlDebug = FALSE;
        // tidy up the extra thread
        if (m_hSqlThread)
        {
                CloseHandle( m_hSqlThread );
                m_hSqlThread = NULL;
        }
        m_idSqlThread = 0;
        m_bSqlThreadRunning = FALSE;
}



XOSD SqlDBInfo::DoCommand( LPBYTE command )
{
        XOSD xosd = xosdNone;
        SDCI *pSDCI = (SDCI*)command;
        switch (pSDCI->cbLength)
        {
                case sizeof(SDCI):
                        m_bWantSqlDebug = TRUE;
                        // the void data immediately follows the SDCI struct so fix it up
                        if (pSDCI->pvData)
                                pSDCI->pvData = (pSDCI+1);
                        SetSDCI( pSDCI );
                        break;

                case SDCI_CODE_BREAK:
                        if (m_hSqlThread)
                                WakeThread();
                        break;

                case SDCI_CODE_RESUME:
                        if (m_hSqlThread)
                                SleepThread();
                        break;

                default:
#ifdef _DEBUG
                        OutputDebugString("ERROR: DoCommand passed invalid struct\n");
#endif
                        break;
        }
        return xosd;
}

#endif                  // SQLDBG
