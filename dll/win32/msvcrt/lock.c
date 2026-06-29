/*
 * Copyright (c) 2002, TransGaming Technologies Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdbool.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "mtdll.h"
#include "cxx.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct
{
  BOOL             bInit;
  CRITICAL_SECTION crit;
} LOCKTABLEENTRY;

static LOCKTABLEENTRY lock_table[ _TOTAL_LOCKS ];

static inline void msvcrt_mlock_set_entry_initialized( int locknum, BOOL initialized )
{
  lock_table[ locknum ].bInit = initialized;
}

static inline void msvcrt_initialize_mlock( int locknum )
{
  InitializeCriticalSectionEx( &(lock_table[ locknum ].crit), 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
  lock_table[ locknum ].crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": LOCKTABLEENTRY.crit");
  msvcrt_mlock_set_entry_initialized( locknum, TRUE );
}

static inline void msvcrt_uninitialize_mlock( int locknum )
{
  lock_table[ locknum ].crit.DebugInfo->Spare[0] = 0;
  DeleteCriticalSection( &(lock_table[ locknum ].crit) );
  msvcrt_mlock_set_entry_initialized( locknum, FALSE );
}

/**********************************************************************
 *     msvcrt_init_mt_locks (internal)
 *
 * Initialize the table lock. All other locks will be initialized
 * upon first use.
 *
 */
void msvcrt_init_mt_locks(void)
{
  int i;

  TRACE( "initializing mtlocks\n" );

  /* Initialize the table */
  for( i=0; i < _TOTAL_LOCKS; i++ )
  {
    msvcrt_mlock_set_entry_initialized( i, FALSE );
  }

  /* Initialize our lock table lock */
  msvcrt_initialize_mlock( _LOCKTAB_LOCK );
}

/**********************************************************************
 *              _lock (MSVCRT.@)
 */
void CDECL _lock( int locknum )
{
  TRACE( "(%d)\n", locknum );

  /* If the lock doesn't exist yet, create it */
  if( lock_table[ locknum ].bInit == FALSE )
  {
    /* Lock while we're changing the lock table */
    _lock( _LOCKTAB_LOCK );

    /* Check again if we've got a bit of a race on lock creation */
    if( lock_table[ locknum ].bInit == FALSE )
    {
      TRACE( ": creating lock #%d\n", locknum );
      msvcrt_initialize_mlock( locknum );
    }

    /* Unlock ourselves */
    _unlock( _LOCKTAB_LOCK );
  }

  EnterCriticalSection( &(lock_table[ locknum ].crit) );
}

/**********************************************************************
 *              _unlock (MSVCRT.@)
 *
 * NOTE: There is no error detection to make sure the lock exists and is acquired.
 */
void CDECL _unlock( int locknum )
{
  TRACE( "(%d)\n", locknum );

  LeaveCriticalSection( &(lock_table[ locknum ].crit) );
}

#if _MSVCR_VER == 110
static LONG shared_ptr_lock;

void __cdecl _Lock_shared_ptr_spin_lock(void)
{
    LONG l = 0;

    while(InterlockedCompareExchange(&shared_ptr_lock, 1, 0) != 0) {
        if(l++ == 1000) {
            Sleep(0);
            l = 0;
        }
    }
}

void __cdecl _Unlock_shared_ptr_spin_lock(void)
{
    shared_ptr_lock = 0;
}
#endif

/**********************************************************************
 *     msvcrt_free_locks (internal)
 *
 * Uninitialize all mt locks. Assume that neither _lock or _unlock will
 * be called once we're calling this routine (ie _LOCKTAB_LOCK can be deleted)
 *
 */
void msvcrt_free_locks(void)
{
  int i;

  TRACE( ": uninitializing all mtlocks\n" );

  /* Uninitialize the table */
  for( i=0; i < _TOTAL_LOCKS; i++ )
  {
    if( lock_table[ i ].bInit )
    {
      msvcrt_uninitialize_mlock( i );
    }
  }
}
