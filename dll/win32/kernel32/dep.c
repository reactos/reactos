/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Data Execution Prevention (DEP) functions from Wine (dlls/kernel32/process.c)
 * COPYRIGHT:   Copyright 2010 Detlef Riekenberg <wine.dev@web.de>
 *              Copyright 2010, 2011 Austin English <austinenglish@gmail.com>
 *              Copyright 2014 Sebastian Lackner <sebastian@fds-team.de>
 *              Copyright 2019 Alexandre Julliard <julliard@winehq.org>
 *              Copyright 2021 Zebediah Figura <z.figura12@gmail.com>
 *              Copyright 2022 Eric Pouech <eric.pouech@gmail.com>
 *              Copyright 2026 Earldridge Jazzed Pineda <earldridgejazzedpineda@gmail.com>
 */

#include <ndk/mmtypes.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <windef.h>
#include <winbase.h>
#include <kernelbase.h>
#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(dep);
 
#define PROCESS_DEP_ENABLE 1
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION 2

/**********************************************************************
 *           GetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI GetProcessDEPPolicy(HANDLE process, LPDWORD flags, PBOOL permanent)
{
    ULONG dep_flags;

    TRACE("(%p %p %p)\n", process, flags, permanent);

#if __REACTOS__
    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessExecuteFlags,
#else
    if (!set_ntstatus( NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
#endif
                                                  &dep_flags, sizeof(dep_flags), NULL )))
        return FALSE;

    if (flags)
	{
	    *flags = 0;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
            *flags |= PROCESS_DEP_ENABLE;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            *flags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
    }
	
	if (permanent) *permanent = (dep_flags & MEM_EXECUTE_OPTION_PERMANENT) != 0;
    return TRUE;
}

/**********************************************************************
 *           GetSystemDEPPolicy     (KERNEL32.@)
 */
DEP_SYSTEM_POLICY_TYPE WINAPI GetSystemDEPPolicy(void)
{
#if __REACTOS__
    return SharedUserData->NXSupportPolicy;
#else
    return user_shared_data->NXSupportPolicy;
#endif
}	

/**********************************************************************
 *           SetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI SetProcessDEPPolicy( DWORD flags )
{
    ULONG dep_flags = 0;

    TRACE("%#lx\n", flags);

    if (flags & PROCESS_DEP_ENABLE)
        dep_flags |= MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_PERMANENT;
    if (flags & PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION)
        dep_flags |= MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION;

    return set_ntstatus( NtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                                  &dep_flags, sizeof(dep_flags) ) );
}
