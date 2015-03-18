/*
 * NTDLL error handling
 *
 * Copyright 2000 Alexandre Julliard
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2010 André Hentschel
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

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define RTL_SEM_FAILCRITICALERRORS      (SEM_FAILCRITICALERRORS << 4)
#define RTL_SEM_NOGPFAULTERRORBOX       (SEM_NOGPFAULTERRORBOX << 4)
#define RTL_SEM_NOALIGNMENTFAULTEXCEPT  (SEM_NOALIGNMENTFAULTEXCEPT << 4)

struct error_table
{
    DWORD       start;
    DWORD       end;
    const DWORD *table;
};

static const struct error_table error_table[20];

/**************************************************************************
 *           RtlNtStatusToDosErrorNoTeb (NTDLL.@)
 *
 * Convert an NTSTATUS code to a Win32 error code.
 *
 * PARAMS
 *  status [I] Nt error code to map.
 *
 * RETURNS
 *  The mapped Win32 error code, or ERROR_MR_MID_NOT_FOUND if there is no
 *  mapping defined.
 */
ULONG WINAPI RtlNtStatusToDosErrorNoTeb( NTSTATUS status )
{
    const struct error_table *table = error_table;

    if (!status || (status & 0x20000000)) return status;

    /* 0xd... is equivalent to 0xc... */
    if ((status & 0xf0000000) == 0xd0000000) status &= ~0x10000000;

    while (table->start)
    {
        if ((ULONG)status < table->start) break;
        if ((ULONG)status < table->end)
        {
            DWORD ret = table->table[status - table->start];
            /* unknown entries are 0 */
            if (!ret) goto no_mapping;
            return ret;
        }
        table++;
    }

    /* now some special cases */
    if (HIWORD(status) == 0xc001) return LOWORD(status);
    if (HIWORD(status) == 0x8007) return LOWORD(status);

no_mapping:
    DPRINT1( "no mapping for %08x\n", status );
    return ERROR_MR_MID_NOT_FOUND;
}

/**************************************************************************
 *           RtlNtStatusToDosError (NTDLL.@)
 *
 * Convert an NTSTATUS code to a Win32 error code.
 *
 * PARAMS
 *  status [I] Nt error code to map.
 *
 * RETURNS
 *  The mapped Win32 error code, or ERROR_MR_MID_NOT_FOUND if there is no
 *  mapping defined.
 */
ULONG WINAPI RtlNtStatusToDosError( NTSTATUS status )
{
    PTEB Teb = NtCurrentTeb ();

    if (NULL != Teb)
    {
       Teb->LastStatusValue = status;
    }
    return RtlNtStatusToDosErrorNoTeb( status );
}

/**********************************************************************
 *      RtlGetLastNtStatus (NTDLL.@)
 *
 * Get the current per-thread status.
 */
NTSTATUS WINAPI RtlGetLastNtStatus(void)
{
    return NtCurrentTeb()->LastStatusValue;
}

/**********************************************************************
 *      RtlGetLastWin32Error (NTDLL.@)
 *
 * Get the current per-thread error value set by a system function or the user.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current error value for the thread, as set by SetLastWin32Error() or SetLastError().
 */
DWORD WINAPI RtlGetLastWin32Error(void)
{
    return NtCurrentTeb()->LastErrorValue;
}

/***********************************************************************
 *      RtlSetLastWin32Error (NTDLL.@)
 *      RtlRestoreLastWin32Error (NTDLL.@)
 *
 * Set the per-thread error value.
 *
 * PARAMS
 *  err [I] The new error value to set
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlSetLastWin32Error( DWORD err )
{
    NtCurrentTeb()->LastErrorValue = err;
}

/***********************************************************************
 *      RtlSetLastWin32ErrorAndNtStatusFromNtStatus (NTDLL.@)
 *
 * Set the per-thread status and error values.
 *
 * PARAMS
 *  err [I] The new status value to set
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlSetLastWin32ErrorAndNtStatusFromNtStatus( NTSTATUS status )
{
    PTEB Teb = NtCurrentTeb ();

    Teb->LastErrorValue = RtlNtStatusToDosError( status );
    Teb->LastStatusValue = status;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlMapSecurityErrorToNtStatus(
    IN ULONG SecurityError
    )
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetThreadErrorMode(IN ULONG NewMode,
                      OUT PULONG OldMode OPTIONAL)
{
    PTEB Teb = NtCurrentTeb();

    /* Ignore invalid error modes */
    if (NewMode & ~(RTL_SEM_FAILCRITICALERRORS |
                    RTL_SEM_NOGPFAULTERRORBOX |
                    RTL_SEM_NOALIGNMENTFAULTEXCEPT))
    {
        DPRINT1("Invalid error mode\n");
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Return old mode */
    if (OldMode) *OldMode = Teb->HardErrorMode;
    
    /* Set new one and return success */
    Teb->HardErrorMode = NewMode;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlGetThreadErrorMode(VOID)
{
    /* Return it from the TEB */
    return NtCurrentTeb()->HardErrorMode;
}

/* conversion tables */

static const DWORD table_00000102[32] =
{
   ERROR_TIMEOUT,                          /* 00000102 (STATUS_TIMEOUT) */
   ERROR_IO_PENDING,                       /* 00000103 (STATUS_PENDING) */
   0,                                      /* 00000104 (STATUS_REPARSE) */
   ERROR_MORE_DATA,                        /* 00000105 (STATUS_MORE_ENTRIES) */
   ERROR_NOT_ALL_ASSIGNED,                 /* 00000106 (STATUS_NOT_ALL_ASSIGNED) */
   ERROR_SOME_NOT_MAPPED,                  /* 00000107 (STATUS_SOME_NOT_MAPPED) */
   0,                                      /* 00000108 (STATUS_OPLOCK_BREAK_IN_PROGRESS) */
   0,                                      /* 00000109 (STATUS_VOLUME_MOUNTED) */
   0,                                      /* 0000010a (STATUS_RXACT_COMMITTED) */
   0,                                      /* 0000010b (STATUS_NOTIFY_CLEANUP) */
   ERROR_NOTIFY_ENUM_DIR,                  /* 0000010c (STATUS_NOTIFY_ENUM_DIR) */
   ERROR_NO_QUOTAS_FOR_ACCOUNT,            /* 0000010d (STATUS_NO_QUOTAS_FOR_ACCOUNT) */
   0,                                      /* 0000010e (STATUS_PRIMARY_TRANSPORT_CONNECT_FAILED) */
   0,                                      /* 0000010f */
   0,                                      /* 00000110 (STATUS_PAGE_FAULT_TRANSITION) */
   0,                                      /* 00000111 (STATUS_PAGE_FAULT_DEMAND_ZERO) */
   0,                                      /* 00000112 (STATUS_PAGE_FAULT_COPY_ON_WRITE) */
   0,                                      /* 00000113 (STATUS_PAGE_FAULT_GUARD_PAGE) */
   0,                                      /* 00000114 (STATUS_PAGE_FAULT_PAGING_FILE) */
   0,                                      /* 00000115 (STATUS_CACHE_PAGE_LOCKED) */
   0,                                      /* 00000116 (STATUS_CRASH_DUMP) */
   0,                                      /* 00000117 (STATUS_BUFFER_ALL_ZEROS) */
   0,                                      /* 00000118 (STATUS_REPARSE_OBJECT) */
   0,                                      /* 00000119 (STATUS_RESOURCE_REQUIREMENTS_CHANGED) */
   0,                                      /* 0000011a */
   0,                                      /* 0000011b */
   0,                                      /* 0000011c */
   0,                                      /* 0000011d */
   0,                                      /* 0000011e */
   0,                                      /* 0000011f */
   0,                                      /* 00000120 (STATUS_TRANSLATION_COMPLETE) */
   ERROR_DS_MEMBERSHIP_EVALUATED_LOCALLY   /* 00000121 (STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY) */
};

static const DWORD table_40000002[36] =
{
   ERROR_INVALID_PARAMETER,                /* 40000002 (STATUS_WORKING_SET_LIMIT_RANGE) */
   ERROR_IMAGE_NOT_AT_BASE,                /* 40000003 (STATUS_IMAGE_NOT_AT_BASE) */
   0,                                      /* 40000004 (STATUS_RXACT_STATE_CREATED) */
   0,                                      /* 40000005 (STATUS_SEGMENT_NOTIFICATION) */
   ERROR_LOCAL_USER_SESSION_KEY,           /* 40000006 (STATUS_LOCAL_USER_SESSION_KEY) */
   0,                                      /* 40000007 (STATUS_BAD_CURRENT_DIRECTORY) */
   ERROR_MORE_WRITES,                      /* 40000008 (STATUS_SERIAL_MORE_WRITES) */
   ERROR_REGISTRY_RECOVERED,               /* 40000009 (STATUS_REGISTRY_RECOVERED) */
   0,                                      /* 4000000a (STATUS_FT_READ_RECOVERY_FROM_BACKUP) */
   0,                                      /* 4000000b (STATUS_FT_WRITE_RECOVERY) */
   ERROR_COUNTER_TIMEOUT,                  /* 4000000c (STATUS_SERIAL_COUNTER_TIMEOUT) */
   ERROR_NULL_LM_PASSWORD,                 /* 4000000d (STATUS_NULL_LM_PASSWORD) */
   ERROR_IMAGE_MACHINE_TYPE_MISMATCH,      /* 4000000e (STATUS_IMAGE_MACHINE_TYPE_MISMATCH) */
   ERROR_RECEIVE_PARTIAL,                  /* 4000000f (STATUS_RECEIVE_PARTIAL) */
   ERROR_RECEIVE_EXPEDITED,                /* 40000010 (STATUS_RECEIVE_EXPEDITED) */
   ERROR_RECEIVE_PARTIAL_EXPEDITED,        /* 40000011 (STATUS_RECEIVE_PARTIAL_EXPEDITED) */
   ERROR_EVENT_DONE,                       /* 40000012 (STATUS_EVENT_DONE) */
   ERROR_EVENT_PENDING,                    /* 40000013 (STATUS_EVENT_PENDING) */
   ERROR_CHECKING_FILE_SYSTEM,             /* 40000014 (STATUS_CHECKING_FILE_SYSTEM) */
   ERROR_FATAL_APP_EXIT,                   /* 40000015 (STATUS_FATAL_APP_EXIT) */
   ERROR_PREDEFINED_HANDLE,                /* 40000016 (STATUS_PREDEFINED_HANDLE) */
   ERROR_WAS_UNLOCKED,                     /* 40000017 (STATUS_WAS_UNLOCKED) */
   ERROR_SERVICE_NOTIFICATION,             /* 40000018 (STATUS_SERVICE_NOTIFICATION) */
   ERROR_WAS_LOCKED,                       /* 40000019 (STATUS_WAS_LOCKED) */
   ERROR_LOG_HARD_ERROR,                   /* 4000001a (STATUS_LOG_HARD_ERROR) */
   ERROR_ALREADY_WIN32,                    /* 4000001b (STATUS_ALREADY_WIN32) */
   0,                                      /* 4000001c (STATUS_WX86_UNSIMULATE) */
   0,                                      /* 4000001d (STATUS_WX86_CONTINUE) */
   0,                                      /* 4000001e (STATUS_WX86_SINGLE_STEP) */
   0,                                      /* 4000001f (STATUS_WX86_BREAKPOINT) */
   0,                                      /* 40000020 (STATUS_WX86_EXCEPTION_CONTINUE) */
   0,                                      /* 40000021 (STATUS_WX86_EXCEPTION_LASTCHANCE) */
   0,                                      /* 40000022 (STATUS_WX86_EXCEPTION_CHAIN) */
   ERROR_IMAGE_MACHINE_TYPE_MISMATCH_EXE,  /* 40000023 (STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE) */
   ERROR_NO_YIELD_PERFORMED,               /* 40000024 (STATUS_NO_YIELD_PERFORMED) */
   ERROR_TIMER_RESUME_IGNORED              /* 40000025 (STATUS_TIMER_RESUME_IGNORED) */
};

static const DWORD table_40000370[1] =
{
   ERROR_DS_SHUTTING_DOWN                  /* 40000370 (STATUS_DS_SHUTTING_DOWN) */
};

static const DWORD table_40020056[1] =
{
   RPC_S_UUID_LOCAL_ONLY                   /* 40020056 (RPC_NT_UUID_LOCAL_ONLY) */
};

static const DWORD table_400200af[1] =
{
   RPC_S_SEND_INCOMPLETE                   /* 400200af (RPC_NT_SEND_INCOMPLETE) */
};

static const DWORD table_80000001[39] =
{
   STATUS_GUARD_PAGE_VIOLATION,            /* 80000001 (STATUS_GUARD_PAGE_VIOLATION) */
   ERROR_NOACCESS,                         /* 80000002 (STATUS_DATATYPE_MISALIGNMENT) */
   STATUS_BREAKPOINT,                      /* 80000003 (STATUS_BREAKPOINT) */
   STATUS_SINGLE_STEP,                     /* 80000004 (STATUS_SINGLE_STEP) */
   ERROR_MORE_DATA,                        /* 80000005 (STATUS_BUFFER_OVERFLOW) */
   ERROR_NO_MORE_FILES,                    /* 80000006 (STATUS_NO_MORE_FILES) */
   0,                                      /* 80000007 (STATUS_WAKE_SYSTEM_DEBUGGER) */
   0,                                      /* 80000008 */
   0,                                      /* 80000009 */
   ERROR_HANDLES_CLOSED,                   /* 8000000a (STATUS_HANDLES_CLOSED) */
   ERROR_NO_INHERITANCE,                   /* 8000000b (STATUS_NO_INHERITANCE) */
   0,                                      /* 8000000c (STATUS_GUID_SUBSTITUTION_MADE) */
   ERROR_PARTIAL_COPY,                     /* 8000000d (STATUS_PARTIAL_COPY) */
   ERROR_OUT_OF_PAPER,                     /* 8000000e (STATUS_DEVICE_PAPER_EMPTY) */
   ERROR_NOT_READY,                        /* 8000000f (STATUS_DEVICE_POWERED_OFF) */
   ERROR_NOT_READY,                        /* 80000010 (STATUS_DEVICE_OFF_LINE) */
   ERROR_BUSY,                             /* 80000011 (STATUS_DEVICE_BUSY) */
   ERROR_NO_MORE_ITEMS,                    /* 80000012 (STATUS_NO_MORE_EAS) */
   ERROR_INVALID_EA_NAME,                  /* 80000013 (STATUS_INVALID_EA_NAME) */
   ERROR_EA_LIST_INCONSISTENT,             /* 80000014 (STATUS_EA_LIST_INCONSISTENT) */
   ERROR_EA_LIST_INCONSISTENT,             /* 80000015 (STATUS_INVALID_EA_FLAG) */
   ERROR_MEDIA_CHANGED,                    /* 80000016 (STATUS_VERIFY_REQUIRED) */
   0,                                      /* 80000017 (STATUS_EXTRANEOUS_INFORMATION) */
   0,                                      /* 80000018 (STATUS_RXACT_COMMIT_NECESSARY) */
   0,                                      /* 80000019 */
   ERROR_NO_MORE_ITEMS,                    /* 8000001a (STATUS_NO_MORE_ENTRIES) */
   ERROR_FILEMARK_DETECTED,                /* 8000001b (STATUS_FILEMARK_DETECTED) */
   ERROR_MEDIA_CHANGED,                    /* 8000001c (STATUS_MEDIA_CHANGED) */
   ERROR_BUS_RESET,                        /* 8000001d (STATUS_BUS_RESET) */
   ERROR_END_OF_MEDIA,                     /* 8000001e (STATUS_END_OF_MEDIA) */
   ERROR_BEGINNING_OF_MEDIA,               /* 8000001f (STATUS_BEGINNING_OF_MEDIA) */
   0,                                      /* 80000020 (STATUS_MEDIA_CHECK) */
   ERROR_SETMARK_DETECTED,                 /* 80000021 (STATUS_SETMARK_DETECTED) */
   ERROR_NO_DATA_DETECTED,                 /* 80000022 (STATUS_NO_DATA_DETECTED) */
   0,                                      /* 80000023 (STATUS_REDIRECTOR_HAS_OPEN_HANDLES) */
   0,                                      /* 80000024 (STATUS_SERVER_HAS_OPEN_HANDLES) */
   ERROR_ACTIVE_CONNECTIONS,               /* 80000025 (STATUS_ALREADY_DISCONNECTED) */
   0,                                      /* 80000026 (STATUS_LONGJUMP) */
   ERROR_CLEANER_CARTRIDGE_INSTALLED       /* 80000027 (STATUS_CLEANER_CARTRIDGE_INSTALLED) */
};

static const DWORD table_80000288[2] =
{
   ERROR_DEVICE_REQUIRES_CLEANING,         /* 80000288 (STATUS_DEVICE_REQUIRES_CLEANING) */
   ERROR_DEVICE_DOOR_OPEN                  /* 80000289 (STATUS_DEVICE_DOOR_OPEN) */
};

static const DWORD table_80090300[72] =
{
   ERROR_NO_SYSTEM_RESOURCES,              /* 80090300 (SEC_E_INSUFFICIENT_MEMORY) */
   ERROR_INVALID_HANDLE,                   /* 80090301 (SEC_E_INVALID_HANDLE) */
   ERROR_INVALID_FUNCTION,                 /* 80090302 (SEC_E_UNSUPPORTED_FUNCTION) */
   ERROR_BAD_NETPATH,                      /* 80090303 (SEC_E_TARGET_UNKNOWN) */
   ERROR_INTERNAL_ERROR,                   /* 80090304 (SEC_E_INTERNAL_ERROR) */
   ERROR_NO_SUCH_PACKAGE,                  /* 80090305 (SEC_E_SECPKG_NOT_FOUND) */
   ERROR_NOT_OWNER,                        /* 80090306 (SEC_E_NOT_OWNER) */
   ERROR_NO_SUCH_PACKAGE,                  /* 80090307 (SEC_E_CANNOT_INSTALL) */
   ERROR_INVALID_PARAMETER,                /* 80090308 (SEC_E_INVALID_TOKEN) */
   ERROR_INVALID_PARAMETER,                /* 80090309 (SEC_E_CANNOT_PACK) */
   ERROR_NOT_SUPPORTED,                    /* 8009030a (SEC_E_QOP_NOT_SUPPORTED) */
   ERROR_CANNOT_IMPERSONATE,               /* 8009030b (SEC_E_NO_IMPERSONATION) */
   ERROR_LOGON_FAILURE,                    /* 8009030c (SEC_E_LOGON_DENIED) */
   ERROR_INVALID_PARAMETER,                /* 8009030d (SEC_E_UNKNOWN_CREDENTIALS) */
   ERROR_NO_SUCH_LOGON_SESSION,            /* 8009030e (SEC_E_NO_CREDENTIALS) */
   ERROR_ACCESS_DENIED,                    /* 8009030f (SEC_E_MESSAGE_ALTERED) */
   ERROR_ACCESS_DENIED,                    /* 80090310 (SEC_E_OUT_OF_SEQUENCE) */
   ERROR_NO_LOGON_SERVERS,                 /* 80090311 (SEC_E_NO_AUTHENTICATING_AUTHORITY) */
   0,                                      /* 80090312 */
   0,                                      /* 80090313 */
   0,                                      /* 80090314 */
   0,                                      /* 80090315 */
   ERROR_NO_SUCH_PACKAGE,                  /* 80090316 (SEC_E_BAD_PKGID) */
   ERROR_CONTEXT_EXPIRED,                  /* 80090317 (SEC_E_CONTEXT_EXPIRED) */
   ERROR_INVALID_USER_BUFFER,              /* 80090318 (SEC_E_INCOMPLETE_MESSAGE) */
   0,                                      /* 80090319 */
   0,                                      /* 8009031a */
   0,                                      /* 8009031b */
   0,                                      /* 8009031c */
   0,                                      /* 8009031d */
   0,                                      /* 8009031e */
   0,                                      /* 8009031f */
   ERROR_INVALID_PARAMETER,                /* 80090320 (SEC_E_INCOMPLETE_CREDENTIALS) */
   ERROR_INSUFFICIENT_BUFFER,              /* 80090321 (SEC_E_BUFFER_TOO_SMALL) */
   ERROR_WRONG_TARGET_NAME,                /* 80090322 (SEC_E_WRONG_PRINCIPAL) */
   0,                                      /* 80090323 */
   0,                                      /* 80090324 (SEC_E_TIME_SKEW) */
   ERROR_TRUST_FAILURE,                    /* 80090325 (SEC_E_UNTRUSTED_ROOT) */
   ERROR_INVALID_PARAMETER,                /* 80090326 (SEC_E_ILLEGAL_MESSAGE) */
   ERROR_INVALID_PARAMETER,                /* 80090327 (SEC_E_CERT_UNKNOWN) */
   ERROR_PASSWORD_EXPIRED,                 /* 80090328 (SEC_E_CERT_EXPIRED) */
   ERROR_ENCRYPTION_FAILED,                /* 80090329 (SEC_E_ENCRYPT_FAILURE) */
   0,                                      /* 8009032a */
   0,                                      /* 8009032b */
   0,                                      /* 8009032c */
   0,                                      /* 8009032d */
   0,                                      /* 8009032e */
   0,                                      /* 8009032f */
   ERROR_DECRYPTION_FAILED,                /* 80090330 (SEC_E_DECRYPT_FAILURE) */
   ERROR_INVALID_FUNCTION,                 /* 80090331 (SEC_E_ALGORITHM_MISMATCH) */
   0,                                      /* 80090332 (SEC_E_SECURITY_QOS_FAILED) */
   0,                                      /* 80090333 (SEC_E_UNFINISHED_CONTEXT_DELETED) */
   0,                                      /* 80090334 (SEC_E_NO_TGT_REPLY) */
   0,                                      /* 80090335 (SEC_E_NO_IP_ADDRESSES) */
   0,                                      /* 80090336 (SEC_E_WRONG_CREDENTIAL_HANDLE) */
   0,                                      /* 80090337 (SEC_E_CRYPTO_SYSTEM_INVALID) */
   0,                                      /* 80090338 (SEC_E_MAX_REFERRALS_EXCEEDED) */
   0,                                      /* 80090339 (SEC_E_MUST_BE_KDC) */
   0,                                      /* 8009033a (SEC_E_STRONG_CRYPTO_NOT_SUPPORTED) */
   0,                                      /* 8009033b (SEC_E_TOO_MANY_PRINCIPALS) */
   0,                                      /* 8009033c (SEC_E_NO_PA_DATA) */
   0,                                      /* 8009033d (SEC_E_PKINIT_NAME_MISMATCH) */
   0,                                      /* 8009033e (SEC_E_SMARTCARD_LOGON_REQUIRED) */
   0,                                      /* 8009033f (SEC_E_SHUTDOWN_IN_PROGRESS) */
   0,                                      /* 80090340 (SEC_E_KDC_INVALID_REQUEST) */
   0,                                      /* 80090341 (SEC_E_KDC_UNABLE_TO_REFER) */
   0,                                      /* 80090342 (SEC_E_KDC_UNKNOWN_ETYPE) */
   0,                                      /* 80090343 (SEC_E_UNSUPPORTED_PREAUTH) */
   0,                                      /* 80090344 */
   0,                                      /* 80090345 (SEC_E_DELEGATION_REQUIRED) */
   0,                                      /* 80090346 (SEC_E_BAD_BINDINGS) */
   ERROR_CANNOT_IMPERSONATE                /* 80090347 (SEC_E_MULTIPLE_ACCOUNTS) */
};

static const DWORD table_80092010[4] =
{
    ERROR_MUTUAL_AUTH_FAILED,              /* 80092010 (CRYPT_E_REVOKED) */
    0,                                     /* 80092011 (CRYPT_E_NO_REVOCATION_DLL) */
    ERROR_MUTUAL_AUTH_FAILED,              /* 80092012 (CRYPT_E_NO_REVOCATION_CHECK) */
    ERROR_MUTUAL_AUTH_FAILED               /* 80092013 (CRYPT_E_REVOCATION_OFFLINE) */
};

static const DWORD table_80096004[1] =
{
   ERROR_MUTUAL_AUTH_FAILED                /* 80096004 (TRUST_E_CERT_SIGNATURE) */
};

static const DWORD table_80130001[5] =
{
    ERROR_CLUSTER_NODE_ALREADY_UP,         /* 80130001 (STATUS_CLUSTER_NODE_ALREADY_UP) */
    ERROR_CLUSTER_NODE_ALREADY_DOWN,       /* 80130002 (STATUS_CLUSTER_NODE_ALREADY_DOWN) */
    ERROR_CLUSTER_NETWORK_ALREADY_ONLINE,  /* 80130003 (STATUS_CLUSTER_NETWORK_ALREADY_ONLINE) */
    ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE, /* 80130004 (STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE) */
    ERROR_CLUSTER_NODE_ALREADY_MEMBER      /* 80130005 (STATUS_CLUSTER_NODE_ALREADY_MEMBER) */
};

static const DWORD table_c0000001[411] =
{
   ERROR_GEN_FAILURE,                      /* c0000001 (STATUS_UNSUCCESSFUL) */
   ERROR_INVALID_FUNCTION,                 /* c0000002 (STATUS_NOT_IMPLEMENTED) */
   ERROR_INVALID_PARAMETER,                /* c0000003 (STATUS_INVALID_INFO_CLASS) */
   ERROR_BAD_LENGTH,                       /* c0000004 (STATUS_INFO_LENGTH_MISMATCH) */
   ERROR_NOACCESS,                         /* c0000005 (STATUS_ACCESS_VIOLATION) */
   ERROR_SWAPERROR,                        /* c0000006 (STATUS_IN_PAGE_ERROR) */
   ERROR_PAGEFILE_QUOTA,                   /* c0000007 (STATUS_PAGEFILE_QUOTA) */
   ERROR_INVALID_HANDLE,                   /* c0000008 (STATUS_INVALID_HANDLE) */
   ERROR_STACK_OVERFLOW,                   /* c0000009 (STATUS_BAD_INITIAL_STACK) */
   ERROR_BAD_EXE_FORMAT,                   /* c000000a (STATUS_BAD_INITIAL_PC) */
   ERROR_INVALID_PARAMETER,                /* c000000b (STATUS_INVALID_CID) */
   0,                                      /* c000000c (STATUS_TIMER_NOT_CANCELED) */
   ERROR_INVALID_PARAMETER,                /* c000000d (STATUS_INVALID_PARAMETER) */
   ERROR_FILE_NOT_FOUND,                   /* c000000e (STATUS_NO_SUCH_DEVICE) */
   ERROR_FILE_NOT_FOUND,                   /* c000000f (STATUS_NO_SUCH_FILE) */
   ERROR_INVALID_FUNCTION,                 /* c0000010 (STATUS_INVALID_DEVICE_REQUEST) */
   ERROR_HANDLE_EOF,                       /* c0000011 (STATUS_END_OF_FILE) */
   ERROR_WRONG_DISK,                       /* c0000012 (STATUS_WRONG_VOLUME) */
   ERROR_NOT_READY,                        /* c0000013 (STATUS_NO_MEDIA_IN_DEVICE) */
   ERROR_UNRECOGNIZED_MEDIA,               /* c0000014 (STATUS_UNRECOGNIZED_MEDIA) */
   ERROR_SECTOR_NOT_FOUND,                 /* c0000015 (STATUS_NONEXISTENT_SECTOR) */
   ERROR_MORE_DATA,                        /* c0000016 (STATUS_MORE_PROCESSING_REQUIRED) */
   ERROR_NOT_ENOUGH_MEMORY,                /* c0000017 (STATUS_NO_MEMORY) */
   ERROR_INVALID_ADDRESS,                  /* c0000018 (STATUS_CONFLICTING_ADDRESSES) */
   ERROR_INVALID_ADDRESS,                  /* c0000019 (STATUS_NOT_MAPPED_VIEW) */
   ERROR_INVALID_PARAMETER,                /* c000001a (STATUS_UNABLE_TO_FREE_VM) */
   ERROR_INVALID_PARAMETER,                /* c000001b (STATUS_UNABLE_TO_DELETE_SECTION) */
   ERROR_INVALID_FUNCTION,                 /* c000001c (STATUS_INVALID_SYSTEM_SERVICE) */
   ERROR_INVALID_FUNCTION,                 /* c000001d (STATUS_ILLEGAL_INSTRUCTION) */
   ERROR_ACCESS_DENIED,                    /* c000001e (STATUS_INVALID_LOCK_SEQUENCE) */
   ERROR_ACCESS_DENIED,                    /* c000001f (STATUS_INVALID_VIEW_SIZE) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000020 (STATUS_INVALID_FILE_FOR_SECTION) */
   ERROR_ACCESS_DENIED,                    /* c0000021 (STATUS_ALREADY_COMMITTED) */
   ERROR_ACCESS_DENIED,                    /* c0000022 (STATUS_ACCESS_DENIED) */
   ERROR_INSUFFICIENT_BUFFER,              /* c0000023 (STATUS_BUFFER_TOO_SMALL) */
   ERROR_INVALID_HANDLE,                   /* c0000024 (STATUS_OBJECT_TYPE_MISMATCH) */
   STATUS_NONCONTINUABLE_EXCEPTION,        /* c0000025 (STATUS_NONCONTINUABLE_EXCEPTION) */
   STATUS_INVALID_DISPOSITION,             /* c0000026 (STATUS_INVALID_DISPOSITION) */
   0,                                      /* c0000027 (STATUS_UNWIND) */
   0,                                      /* c0000028 (STATUS_BAD_STACK) */
   0,                                      /* c0000029 (STATUS_INVALID_UNWIND_TARGET) */
   ERROR_NOT_LOCKED,                       /* c000002a (STATUS_NOT_LOCKED) */
   STATUS_PARITY_ERROR,                    /* c000002b (STATUS_PARITY_ERROR) */
   ERROR_INVALID_ADDRESS,                  /* c000002c (STATUS_UNABLE_TO_DECOMMIT_VM) */
   ERROR_INVALID_ADDRESS,                  /* c000002d (STATUS_NOT_COMMITTED) */
   0,                                      /* c000002e (STATUS_INVALID_PORT_ATTRIBUTES) */
   0,                                      /* c000002f (STATUS_PORT_MESSAGE_TOO_LONG) */
   ERROR_INVALID_PARAMETER,                /* c0000030 (STATUS_INVALID_PARAMETER_MIX) */
   0,                                      /* c0000031 (STATUS_INVALID_QUOTA_LOWER) */
   ERROR_DISK_CORRUPT,                     /* c0000032 (STATUS_DISK_CORRUPT_ERROR) */
   ERROR_INVALID_NAME,                     /* c0000033 (STATUS_OBJECT_NAME_INVALID) */
   ERROR_FILE_NOT_FOUND,                   /* c0000034 (STATUS_OBJECT_NAME_NOT_FOUND) */
   ERROR_ALREADY_EXISTS,                   /* c0000035 (STATUS_OBJECT_NAME_COLLISION) */
   0,                                      /* c0000036 */
   ERROR_INVALID_HANDLE,                   /* c0000037 (STATUS_PORT_DISCONNECTED) */
   0,                                      /* c0000038 (STATUS_DEVICE_ALREADY_ATTACHED) */
   ERROR_BAD_PATHNAME,                     /* c0000039 (STATUS_OBJECT_PATH_INVALID) */
   ERROR_PATH_NOT_FOUND,                   /* c000003a (STATUS_OBJECT_PATH_NOT_FOUND) */
   ERROR_BAD_PATHNAME,                     /* c000003b (STATUS_OBJECT_PATH_SYNTAX_BAD) */
   ERROR_IO_DEVICE,                        /* c000003c (STATUS_DATA_OVERRUN) */
   ERROR_IO_DEVICE,                        /* c000003d (STATUS_DATA_LATE_ERROR) */
   ERROR_CRC,                              /* c000003e (STATUS_DATA_ERROR) */
   ERROR_CRC,                              /* c000003f (STATUS_CRC_ERROR) */
   ERROR_NOT_ENOUGH_MEMORY,                /* c0000040 (STATUS_SECTION_TOO_BIG) */
   ERROR_ACCESS_DENIED,                    /* c0000041 (STATUS_PORT_CONNECTION_REFUSED) */
   ERROR_INVALID_HANDLE,                   /* c0000042 (STATUS_INVALID_PORT_HANDLE) */
   ERROR_SHARING_VIOLATION,                /* c0000043 (STATUS_SHARING_VIOLATION) */
   ERROR_NOT_ENOUGH_QUOTA,                 /* c0000044 (STATUS_QUOTA_EXCEEDED) */
   ERROR_INVALID_PARAMETER,                /* c0000045 (STATUS_INVALID_PAGE_PROTECTION) */
   ERROR_NOT_OWNER,                        /* c0000046 (STATUS_MUTANT_NOT_OWNED) */
   ERROR_TOO_MANY_POSTS,                   /* c0000047 (STATUS_SEMAPHORE_LIMIT_EXCEEDED) */
   ERROR_INVALID_PARAMETER,                /* c0000048 (STATUS_PORT_ALREADY_SET) */
   ERROR_INVALID_PARAMETER,                /* c0000049 (STATUS_SECTION_NOT_IMAGE) */
   ERROR_SIGNAL_REFUSED,                   /* c000004a (STATUS_SUSPEND_COUNT_EXCEEDED) */
   ERROR_ACCESS_DENIED,                    /* c000004b (STATUS_THREAD_IS_TERMINATING) */
   ERROR_INVALID_PARAMETER,                /* c000004c (STATUS_BAD_WORKING_SET_LIMIT) */
   ERROR_INVALID_PARAMETER,                /* c000004d (STATUS_INCOMPATIBLE_FILE_MAP) */
   ERROR_INVALID_PARAMETER,                /* c000004e (STATUS_SECTION_PROTECTION) */
   ERROR_EAS_NOT_SUPPORTED,                /* c000004f (STATUS_EAS_NOT_SUPPORTED) */
   ERROR_EA_LIST_INCONSISTENT,             /* c0000050 (STATUS_EA_TOO_LARGE) */
   ERROR_FILE_CORRUPT,                     /* c0000051 (STATUS_NONEXISTENT_EA_ENTRY) */
   ERROR_FILE_CORRUPT,                     /* c0000052 (STATUS_NO_EAS_ON_FILE) */
   ERROR_FILE_CORRUPT,                     /* c0000053 (STATUS_EA_CORRUPT_ERROR) */
   ERROR_LOCK_VIOLATION,                   /* c0000054 (STATUS_FILE_LOCK_CONFLICT) */
   ERROR_LOCK_VIOLATION,                   /* c0000055 (STATUS_LOCK_NOT_GRANTED) */
   ERROR_ACCESS_DENIED,                    /* c0000056 (STATUS_DELETE_PENDING) */
   ERROR_NOT_SUPPORTED,                    /* c0000057 (STATUS_CTL_FILE_NOT_SUPPORTED) */
   ERROR_UNKNOWN_REVISION,                 /* c0000058 (STATUS_UNKNOWN_REVISION) */
   ERROR_REVISION_MISMATCH,                /* c0000059 (STATUS_REVISION_MISMATCH) */
   ERROR_INVALID_OWNER,                    /* c000005a (STATUS_INVALID_OWNER) */
   ERROR_INVALID_PRIMARY_GROUP,            /* c000005b (STATUS_INVALID_PRIMARY_GROUP) */
   ERROR_NO_IMPERSONATION_TOKEN,           /* c000005c (STATUS_NO_IMPERSONATION_TOKEN) */
   ERROR_CANT_DISABLE_MANDATORY,           /* c000005d (STATUS_CANT_DISABLE_MANDATORY) */
   ERROR_NO_LOGON_SERVERS,                 /* c000005e (STATUS_NO_LOGON_SERVERS) */
   ERROR_NO_SUCH_LOGON_SESSION,            /* c000005f (STATUS_NO_SUCH_LOGON_SESSION) */
   ERROR_NO_SUCH_PRIVILEGE,                /* c0000060 (STATUS_NO_SUCH_PRIVILEGE) */
   ERROR_PRIVILEGE_NOT_HELD,               /* c0000061 (STATUS_PRIVILEGE_NOT_HELD) */
   ERROR_INVALID_ACCOUNT_NAME,             /* c0000062 (STATUS_INVALID_ACCOUNT_NAME) */
   ERROR_USER_EXISTS,                      /* c0000063 (STATUS_USER_EXISTS) */
   ERROR_NO_SUCH_USER,                     /* c0000064 (STATUS_NO_SUCH_USER) */
   ERROR_GROUP_EXISTS,                     /* c0000065 (STATUS_GROUP_EXISTS) */
   ERROR_NO_SUCH_GROUP,                    /* c0000066 (STATUS_NO_SUCH_GROUP) */
   ERROR_MEMBER_IN_GROUP,                  /* c0000067 (STATUS_MEMBER_IN_GROUP) */
   ERROR_MEMBER_NOT_IN_GROUP,              /* c0000068 (STATUS_MEMBER_NOT_IN_GROUP) */
   ERROR_LAST_ADMIN,                       /* c0000069 (STATUS_LAST_ADMIN) */
   ERROR_INVALID_PASSWORD,                 /* c000006a (STATUS_WRONG_PASSWORD) */
   ERROR_ILL_FORMED_PASSWORD,              /* c000006b (STATUS_ILL_FORMED_PASSWORD) */
   ERROR_PASSWORD_RESTRICTION,             /* c000006c (STATUS_PASSWORD_RESTRICTION) */
   ERROR_LOGON_FAILURE,                    /* c000006d (STATUS_LOGON_FAILURE) */
   ERROR_ACCOUNT_RESTRICTION,              /* c000006e (STATUS_ACCOUNT_RESTRICTION) */
   ERROR_INVALID_LOGON_HOURS,              /* c000006f (STATUS_INVALID_LOGON_HOURS) */
   ERROR_INVALID_WORKSTATION,              /* c0000070 (STATUS_INVALID_WORKSTATION) */
   ERROR_PASSWORD_EXPIRED,                 /* c0000071 (STATUS_PASSWORD_EXPIRED) */
   ERROR_ACCOUNT_DISABLED,                 /* c0000072 (STATUS_ACCOUNT_DISABLED) */
   ERROR_NONE_MAPPED,                      /* c0000073 (STATUS_NONE_MAPPED) */
   ERROR_TOO_MANY_LUIDS_REQUESTED,         /* c0000074 (STATUS_TOO_MANY_LUIDS_REQUESTED) */
   ERROR_LUIDS_EXHAUSTED,                  /* c0000075 (STATUS_LUIDS_EXHAUSTED) */
   ERROR_INVALID_SUB_AUTHORITY,            /* c0000076 (STATUS_INVALID_SUB_AUTHORITY) */
   ERROR_INVALID_ACL,                      /* c0000077 (STATUS_INVALID_ACL) */
   ERROR_INVALID_SID,                      /* c0000078 (STATUS_INVALID_SID) */
   ERROR_INVALID_SECURITY_DESCR,           /* c0000079 (STATUS_INVALID_SECURITY_DESCR) */
   ERROR_PROC_NOT_FOUND,                   /* c000007a (STATUS_PROCEDURE_NOT_FOUND) */
   ERROR_BAD_EXE_FORMAT,                   /* c000007b (STATUS_INVALID_IMAGE_FORMAT) */
   ERROR_NO_TOKEN,                         /* c000007c (STATUS_NO_TOKEN) */
   ERROR_BAD_INHERITANCE_ACL,              /* c000007d (STATUS_BAD_INHERITANCE_ACL) */
   ERROR_NOT_LOCKED,                       /* c000007e (STATUS_RANGE_NOT_LOCKED) */
   ERROR_DISK_FULL,                        /* c000007f (STATUS_DISK_FULL) */
   ERROR_SERVER_DISABLED,                  /* c0000080 (STATUS_SERVER_DISABLED) */
   ERROR_SERVER_NOT_DISABLED,              /* c0000081 (STATUS_SERVER_NOT_DISABLED) */
   ERROR_TOO_MANY_NAMES,                   /* c0000082 (STATUS_TOO_MANY_GUIDS_REQUESTED) */
   ERROR_NO_MORE_ITEMS,                    /* c0000083 (STATUS_GUIDS_EXHAUSTED) */
   ERROR_INVALID_ID_AUTHORITY,             /* c0000084 (STATUS_INVALID_ID_AUTHORITY) */
   ERROR_NO_MORE_ITEMS,                    /* c0000085 (STATUS_AGENTS_EXHAUSTED) */
   ERROR_LABEL_TOO_LONG,                   /* c0000086 (STATUS_INVALID_VOLUME_LABEL) */
   ERROR_OUTOFMEMORY,                      /* c0000087 (STATUS_SECTION_NOT_EXTENDED) */
   ERROR_INVALID_ADDRESS,                  /* c0000088 (STATUS_NOT_MAPPED_DATA) */
   ERROR_RESOURCE_DATA_NOT_FOUND,          /* c0000089 (STATUS_RESOURCE_DATA_NOT_FOUND) */
   ERROR_RESOURCE_TYPE_NOT_FOUND,          /* c000008a (STATUS_RESOURCE_TYPE_NOT_FOUND) */
   ERROR_RESOURCE_NAME_NOT_FOUND,          /* c000008b (STATUS_RESOURCE_NAME_NOT_FOUND) */
   STATUS_ARRAY_BOUNDS_EXCEEDED,           /* c000008c (STATUS_ARRAY_BOUNDS_EXCEEDED) */
   STATUS_FLOAT_DENORMAL_OPERAND,          /* c000008d (STATUS_FLOAT_DENORMAL_OPERAND) */
   STATUS_FLOAT_DIVIDE_BY_ZERO,            /* c000008e (STATUS_FLOAT_DIVIDE_BY_ZERO) */
   STATUS_FLOAT_INEXACT_RESULT,            /* c000008f (STATUS_FLOAT_INEXACT_RESULT) */
   STATUS_FLOAT_INVALID_OPERATION,         /* c0000090 (STATUS_FLOAT_INVALID_OPERATION) */
   STATUS_FLOAT_OVERFLOW,                  /* c0000091 (STATUS_FLOAT_OVERFLOW) */
   STATUS_FLOAT_STACK_CHECK,               /* c0000092 (STATUS_FLOAT_STACK_CHECK) */
   STATUS_FLOAT_UNDERFLOW,                 /* c0000093 (STATUS_FLOAT_UNDERFLOW) */
   STATUS_INTEGER_DIVIDE_BY_ZERO,          /* c0000094 (STATUS_INTEGER_DIVIDE_BY_ZERO) */
   ERROR_ARITHMETIC_OVERFLOW,              /* c0000095 (STATUS_INTEGER_OVERFLOW) */
   STATUS_PRIVILEGED_INSTRUCTION,          /* c0000096 (STATUS_PRIVILEGED_INSTRUCTION) */
   ERROR_NOT_ENOUGH_MEMORY,                /* c0000097 (STATUS_TOO_MANY_PAGING_FILES) */
   ERROR_FILE_INVALID,                     /* c0000098 (STATUS_FILE_INVALID) */
   ERROR_ALLOTTED_SPACE_EXCEEDED,          /* c0000099 (STATUS_ALLOTTED_SPACE_EXCEEDED) */
   ERROR_NO_SYSTEM_RESOURCES,              /* c000009a (STATUS_INSUFFICIENT_RESOURCES) */
   ERROR_PATH_NOT_FOUND,                   /* c000009b (STATUS_DFS_EXIT_PATH_FOUND) */
   ERROR_CRC,                              /* c000009c (STATUS_DEVICE_DATA_ERROR) */
   ERROR_DEVICE_NOT_CONNECTED,             /* c000009d (STATUS_DEVICE_NOT_CONNECTED) */
   ERROR_NOT_READY,                        /* c000009e (STATUS_DEVICE_POWER_FAILURE) */
   ERROR_INVALID_ADDRESS,                  /* c000009f (STATUS_FREE_VM_NOT_AT_BASE) */
   ERROR_INVALID_ADDRESS,                  /* c00000a0 (STATUS_MEMORY_NOT_ALLOCATED) */
   ERROR_WORKING_SET_QUOTA,                /* c00000a1 (STATUS_WORKING_SET_QUOTA) */
   ERROR_WRITE_PROTECT,                    /* c00000a2 (STATUS_MEDIA_WRITE_PROTECTED) */
   ERROR_NOT_READY,                        /* c00000a3 (STATUS_DEVICE_NOT_READY) */
   ERROR_INVALID_GROUP_ATTRIBUTES,         /* c00000a4 (STATUS_INVALID_GROUP_ATTRIBUTES) */
   ERROR_BAD_IMPERSONATION_LEVEL,          /* c00000a5 (STATUS_BAD_IMPERSONATION_LEVEL) */
   ERROR_CANT_OPEN_ANONYMOUS,              /* c00000a6 (STATUS_CANT_OPEN_ANONYMOUS) */
   ERROR_BAD_VALIDATION_CLASS,             /* c00000a7 (STATUS_BAD_VALIDATION_CLASS) */
   ERROR_BAD_TOKEN_TYPE,                   /* c00000a8 (STATUS_BAD_TOKEN_TYPE) */
   ERROR_INVALID_PARAMETER,                /* c00000a9 (STATUS_BAD_MASTER_BOOT_RECORD) */
   0,                                      /* c00000aa (STATUS_INSTRUCTION_MISALIGNMENT) */
   ERROR_PIPE_BUSY,                        /* c00000ab (STATUS_INSTANCE_NOT_AVAILABLE) */
   ERROR_PIPE_BUSY,                        /* c00000ac (STATUS_PIPE_NOT_AVAILABLE) */
   ERROR_BAD_PIPE,                         /* c00000ad (STATUS_INVALID_PIPE_STATE) */
   ERROR_PIPE_BUSY,                        /* c00000ae (STATUS_PIPE_BUSY) */
   ERROR_INVALID_FUNCTION,                 /* c00000af (STATUS_ILLEGAL_FUNCTION) */
   ERROR_PIPE_NOT_CONNECTED,               /* c00000b0 (STATUS_PIPE_DISCONNECTED) */
   ERROR_NO_DATA,                          /* c00000b1 (STATUS_PIPE_CLOSING) */
   ERROR_PIPE_CONNECTED,                   /* c00000b2 (STATUS_PIPE_CONNECTED) */
   ERROR_PIPE_LISTENING,                   /* c00000b3 (STATUS_PIPE_LISTENING) */
   ERROR_BAD_PIPE,                         /* c00000b4 (STATUS_INVALID_READ_MODE) */
   ERROR_SEM_TIMEOUT,                      /* c00000b5 (STATUS_IO_TIMEOUT) */
   ERROR_HANDLE_EOF,                       /* c00000b6 (STATUS_FILE_FORCED_CLOSED) */
   0,                                      /* c00000b7 (STATUS_PROFILING_NOT_STARTED) */
   0,                                      /* c00000b8 (STATUS_PROFILING_NOT_STOPPED) */
   0,                                      /* c00000b9 (STATUS_COULD_NOT_INTERPRET) */
   ERROR_ACCESS_DENIED,                    /* c00000ba (STATUS_FILE_IS_A_DIRECTORY) */
   ERROR_NOT_SUPPORTED,                    /* c00000bb (STATUS_NOT_SUPPORTED) */
   ERROR_REM_NOT_LIST,                     /* c00000bc (STATUS_REMOTE_NOT_LISTENING) */
   ERROR_DUP_NAME,                         /* c00000bd (STATUS_DUPLICATE_NAME) */
   ERROR_BAD_NETPATH,                      /* c00000be (STATUS_BAD_NETWORK_PATH) */
   ERROR_NETWORK_BUSY,                     /* c00000bf (STATUS_NETWORK_BUSY) */
   ERROR_DEV_NOT_EXIST,                    /* c00000c0 (STATUS_DEVICE_DOES_NOT_EXIST) */
   ERROR_TOO_MANY_CMDS,                    /* c00000c1 (STATUS_TOO_MANY_COMMANDS) */
   ERROR_ADAP_HDW_ERR,                     /* c00000c2 (STATUS_ADAPTER_HARDWARE_ERROR) */
   ERROR_BAD_NET_RESP,                     /* c00000c3 (STATUS_INVALID_NETWORK_RESPONSE) */
   ERROR_UNEXP_NET_ERR,                    /* c00000c4 (STATUS_UNEXPECTED_NETWORK_ERROR) */
   ERROR_BAD_REM_ADAP,                     /* c00000c5 (STATUS_BAD_REMOTE_ADAPTER) */
   ERROR_PRINTQ_FULL,                      /* c00000c6 (STATUS_PRINT_QUEUE_FULL) */
   ERROR_NO_SPOOL_SPACE,                   /* c00000c7 (STATUS_NO_SPOOL_SPACE) */
   ERROR_PRINT_CANCELLED,                  /* c00000c8 (STATUS_PRINT_CANCELLED) */
   ERROR_NETNAME_DELETED,                  /* c00000c9 (STATUS_NETWORK_NAME_DELETED) */
   ERROR_NETWORK_ACCESS_DENIED,            /* c00000ca (STATUS_NETWORK_ACCESS_DENIED) */
   ERROR_BAD_DEV_TYPE,                     /* c00000cb (STATUS_BAD_DEVICE_TYPE) */
   ERROR_BAD_NET_NAME,                     /* c00000cc (STATUS_BAD_NETWORK_NAME) */
   ERROR_TOO_MANY_NAMES,                   /* c00000cd (STATUS_TOO_MANY_NAMES) */
   ERROR_TOO_MANY_SESS,                    /* c00000ce (STATUS_TOO_MANY_SESSIONS) */
   ERROR_SHARING_PAUSED,                   /* c00000cf (STATUS_SHARING_PAUSED) */
   ERROR_REQ_NOT_ACCEP,                    /* c00000d0 (STATUS_REQUEST_NOT_ACCEPTED) */
   ERROR_REDIR_PAUSED,                     /* c00000d1 (STATUS_REDIRECTOR_PAUSED) */
   ERROR_NET_WRITE_FAULT,                  /* c00000d2 (STATUS_NET_WRITE_FAULT) */
   0,                                      /* c00000d3 (STATUS_PROFILING_AT_LIMIT) */
   ERROR_NOT_SAME_DEVICE,                  /* c00000d4 (STATUS_NOT_SAME_DEVICE) */
   ERROR_ACCESS_DENIED,                    /* c00000d5 (STATUS_FILE_RENAMED) */
   ERROR_VC_DISCONNECTED,                  /* c00000d6 (STATUS_VIRTUAL_CIRCUIT_CLOSED) */
   ERROR_NO_SECURITY_ON_OBJECT,            /* c00000d7 (STATUS_NO_SECURITY_ON_OBJECT) */
   0,                                      /* c00000d8 (STATUS_CANT_WAIT) */
   ERROR_NO_DATA,                          /* c00000d9 (STATUS_PIPE_EMPTY) */
   ERROR_CANT_ACCESS_DOMAIN_INFO,          /* c00000da (STATUS_CANT_ACCESS_DOMAIN_INFO) */
   0,                                      /* c00000db (STATUS_CANT_TERMINATE_SELF) */
   ERROR_INVALID_SERVER_STATE,             /* c00000dc (STATUS_INVALID_SERVER_STATE) */
   ERROR_INVALID_DOMAIN_STATE,             /* c00000dd (STATUS_INVALID_DOMAIN_STATE) */
   ERROR_INVALID_DOMAIN_ROLE,              /* c00000de (STATUS_INVALID_DOMAIN_ROLE) */
   ERROR_NO_SUCH_DOMAIN,                   /* c00000df (STATUS_NO_SUCH_DOMAIN) */
   ERROR_DOMAIN_EXISTS,                    /* c00000e0 (STATUS_DOMAIN_EXISTS) */
   ERROR_DOMAIN_LIMIT_EXCEEDED,            /* c00000e1 (STATUS_DOMAIN_LIMIT_EXCEEDED) */
   ERROR_OPLOCK_NOT_GRANTED,               /* c00000e2 (STATUS_OPLOCK_NOT_GRANTED) */
   ERROR_INVALID_OPLOCK_PROTOCOL,          /* c00000e3 (STATUS_INVALID_OPLOCK_PROTOCOL) */
   ERROR_INTERNAL_DB_CORRUPTION,           /* c00000e4 (STATUS_INTERNAL_DB_CORRUPTION) */
   ERROR_INTERNAL_ERROR,                   /* c00000e5 (STATUS_INTERNAL_ERROR) */
   ERROR_GENERIC_NOT_MAPPED,               /* c00000e6 (STATUS_GENERIC_NOT_MAPPED) */
   ERROR_BAD_DESCRIPTOR_FORMAT,            /* c00000e7 (STATUS_BAD_DESCRIPTOR_FORMAT) */
   ERROR_INVALID_USER_BUFFER,              /* c00000e8 (STATUS_INVALID_USER_BUFFER) */
   0,                                      /* c00000e9 (STATUS_UNEXPECTED_IO_ERROR) */
   0,                                      /* c00000ea (STATUS_UNEXPECTED_MM_CREATE_ERR) */
   0,                                      /* c00000eb (STATUS_UNEXPECTED_MM_MAP_ERROR) */
   0,                                      /* c00000ec (STATUS_UNEXPECTED_MM_EXTEND_ERR) */
   ERROR_NOT_LOGON_PROCESS,                /* c00000ed (STATUS_NOT_LOGON_PROCESS) */
   ERROR_LOGON_SESSION_EXISTS,             /* c00000ee (STATUS_LOGON_SESSION_EXISTS) */
   ERROR_INVALID_PARAMETER,                /* c00000ef (STATUS_INVALID_PARAMETER_1) */
   ERROR_INVALID_PARAMETER,                /* c00000f0 (STATUS_INVALID_PARAMETER_2) */
   ERROR_INVALID_PARAMETER,                /* c00000f1 (STATUS_INVALID_PARAMETER_3) */
   ERROR_INVALID_PARAMETER,                /* c00000f2 (STATUS_INVALID_PARAMETER_4) */
   ERROR_INVALID_PARAMETER,                /* c00000f3 (STATUS_INVALID_PARAMETER_5) */
   ERROR_INVALID_PARAMETER,                /* c00000f4 (STATUS_INVALID_PARAMETER_6) */
   ERROR_INVALID_PARAMETER,                /* c00000f5 (STATUS_INVALID_PARAMETER_7) */
   ERROR_INVALID_PARAMETER,                /* c00000f6 (STATUS_INVALID_PARAMETER_8) */
   ERROR_INVALID_PARAMETER,                /* c00000f7 (STATUS_INVALID_PARAMETER_9) */
   ERROR_INVALID_PARAMETER,                /* c00000f8 (STATUS_INVALID_PARAMETER_10) */
   ERROR_INVALID_PARAMETER,                /* c00000f9 (STATUS_INVALID_PARAMETER_11) */
   ERROR_INVALID_PARAMETER,                /* c00000fa (STATUS_INVALID_PARAMETER_12) */
   ERROR_PATH_NOT_FOUND,                   /* c00000fb (STATUS_REDIRECTOR_NOT_STARTED) */
   ERROR_SERVICE_ALREADY_RUNNING,          /* c00000fc (STATUS_REDIRECTOR_STARTED) */
   ERROR_STACK_OVERFLOW,                   /* c00000fd (STATUS_STACK_OVERFLOW) */
   ERROR_NO_SUCH_PACKAGE,                  /* c00000fe (STATUS_NO_SUCH_PACKAGE) */
   0,                                      /* c00000ff (STATUS_BAD_FUNCTION_TABLE) */
   ERROR_ENVVAR_NOT_FOUND,                 /* c0000100 (STATUS_VARIABLE_NOT_FOUND) */
   ERROR_DIR_NOT_EMPTY,                    /* c0000101 (STATUS_DIRECTORY_NOT_EMPTY) */
   ERROR_FILE_CORRUPT,                     /* c0000102 (STATUS_FILE_CORRUPT_ERROR) */
   ERROR_DIRECTORY,                        /* c0000103 (STATUS_NOT_A_DIRECTORY) */
   ERROR_BAD_LOGON_SESSION_STATE,          /* c0000104 (STATUS_BAD_LOGON_SESSION_STATE) */
   ERROR_LOGON_SESSION_COLLISION,          /* c0000105 (STATUS_LOGON_SESSION_COLLISION) */
   ERROR_FILENAME_EXCED_RANGE,             /* c0000106 (STATUS_NAME_TOO_LONG) */
   ERROR_OPEN_FILES,                       /* c0000107 (STATUS_FILES_OPEN) */
   ERROR_DEVICE_IN_USE,                    /* c0000108 (STATUS_CONNECTION_IN_USE) */
   ERROR_MR_MID_NOT_FOUND,                 /* c0000109 (STATUS_MESSAGE_NOT_FOUND) */
   ERROR_ACCESS_DENIED,                    /* c000010a (STATUS_PROCESS_IS_TERMINATING) */
   ERROR_INVALID_LOGON_TYPE,               /* c000010b (STATUS_INVALID_LOGON_TYPE) */
   0,                                      /* c000010c (STATUS_NO_GUID_TRANSLATION) */
   ERROR_CANNOT_IMPERSONATE,               /* c000010d (STATUS_CANNOT_IMPERSONATE) */
   ERROR_SERVICE_ALREADY_RUNNING,          /* c000010e (STATUS_IMAGE_ALREADY_LOADED) */
   0,                                      /* c000010f (STATUS_ABIOS_NOT_PRESENT) */
   0,                                      /* c0000110 (STATUS_ABIOS_LID_NOT_EXIST) */
   0,                                      /* c0000111 (STATUS_ABIOS_LID_ALREADY_OWNED) */
   0,                                      /* c0000112 (STATUS_ABIOS_NOT_LID_OWNER) */
   0,                                      /* c0000113 (STATUS_ABIOS_INVALID_COMMAND) */
   0,                                      /* c0000114 (STATUS_ABIOS_INVALID_LID) */
   0,                                      /* c0000115 (STATUS_ABIOS_SELECTOR_NOT_AVAILABLE) */
   0,                                      /* c0000116 (STATUS_ABIOS_INVALID_SELECTOR) */
   ERROR_INVALID_THREAD_ID,                /* c0000117 (STATUS_NO_LDT) */
   ERROR_INVALID_LDT_SIZE,                 /* c0000118 (STATUS_INVALID_LDT_SIZE) */
   ERROR_INVALID_LDT_OFFSET,               /* c0000119 (STATUS_INVALID_LDT_OFFSET) */
   ERROR_INVALID_LDT_DESCRIPTOR,           /* c000011a (STATUS_INVALID_LDT_DESCRIPTOR) */
   ERROR_BAD_EXE_FORMAT,                   /* c000011b (STATUS_INVALID_IMAGE_NE_FORMAT) */
   ERROR_RXACT_INVALID_STATE,              /* c000011c (STATUS_RXACT_INVALID_STATE) */
   ERROR_RXACT_COMMIT_FAILURE,             /* c000011d (STATUS_RXACT_COMMIT_FAILURE) */
   ERROR_FILE_INVALID,                     /* c000011e (STATUS_MAPPED_FILE_SIZE_ZERO) */
   ERROR_TOO_MANY_OPEN_FILES,              /* c000011f (STATUS_TOO_MANY_OPENED_FILES) */
   ERROR_OPERATION_ABORTED,                /* c0000120 (STATUS_CANCELLED) */
   ERROR_ACCESS_DENIED,                    /* c0000121 (STATUS_CANNOT_DELETE) */
   ERROR_INVALID_COMPUTERNAME,             /* c0000122 (STATUS_INVALID_COMPUTER_NAME) */
   ERROR_ACCESS_DENIED,                    /* c0000123 (STATUS_FILE_DELETED) */
   ERROR_SPECIAL_ACCOUNT,                  /* c0000124 (STATUS_SPECIAL_ACCOUNT) */
   ERROR_SPECIAL_GROUP,                    /* c0000125 (STATUS_SPECIAL_GROUP) */
   ERROR_SPECIAL_USER,                     /* c0000126 (STATUS_SPECIAL_USER) */
   ERROR_MEMBERS_PRIMARY_GROUP,            /* c0000127 (STATUS_MEMBERS_PRIMARY_GROUP) */
   ERROR_INVALID_HANDLE,                   /* c0000128 (STATUS_FILE_CLOSED) */
   0,                                      /* c0000129 (STATUS_TOO_MANY_THREADS) */
   0,                                      /* c000012a (STATUS_THREAD_NOT_IN_PROCESS) */
   ERROR_TOKEN_ALREADY_IN_USE,             /* c000012b (STATUS_TOKEN_ALREADY_IN_USE) */
   0,                                      /* c000012c (STATUS_PAGEFILE_QUOTA_EXCEEDED) */
   ERROR_COMMITMENT_LIMIT,                 /* c000012d (STATUS_COMMITMENT_LIMIT) */
   ERROR_BAD_EXE_FORMAT,                   /* c000012e (STATUS_INVALID_IMAGE_LE_FORMAT) */
   ERROR_BAD_EXE_FORMAT,                   /* c000012f (STATUS_INVALID_IMAGE_NOT_MZ) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000130 (STATUS_INVALID_IMAGE_PROTECT) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000131 (STATUS_INVALID_IMAGE_WIN_16) */
   0,                                      /* c0000132 (STATUS_LOGON_SERVER_CONFLICT) */
   ERROR_TIME_SKEW,                        /* c0000133 (STATUS_TIME_DIFFERENCE_AT_DC) */
   0,                                      /* c0000134 (STATUS_SYNCHRONIZATION_REQUIRED) */
   ERROR_MOD_NOT_FOUND,                    /* c0000135 (STATUS_DLL_NOT_FOUND) */
   ERROR_NET_OPEN_FAILED,                  /* c0000136 (STATUS_OPEN_FAILED) */
   ERROR_IO_PRIVILEGE_FAILED,              /* c0000137 (STATUS_IO_PRIVILEGE_FAILED) */
   ERROR_INVALID_ORDINAL,                  /* c0000138 (STATUS_ORDINAL_NOT_FOUND) */
   ERROR_PROC_NOT_FOUND,                   /* c0000139 (STATUS_ENTRYPOINT_NOT_FOUND) */
   ERROR_CONTROL_C_EXIT,                   /* c000013a (STATUS_CONTROL_C_EXIT) */
   ERROR_NETNAME_DELETED,                  /* c000013b (STATUS_LOCAL_DISCONNECT) */
   ERROR_NETNAME_DELETED,                  /* c000013c (STATUS_REMOTE_DISCONNECT) */
   ERROR_REM_NOT_LIST,                     /* c000013d (STATUS_REMOTE_RESOURCES) */
   ERROR_UNEXP_NET_ERR,                    /* c000013e (STATUS_LINK_FAILED) */
   ERROR_UNEXP_NET_ERR,                    /* c000013f (STATUS_LINK_TIMEOUT) */
   ERROR_UNEXP_NET_ERR,                    /* c0000140 (STATUS_INVALID_CONNECTION) */
   ERROR_UNEXP_NET_ERR,                    /* c0000141 (STATUS_INVALID_ADDRESS) */
   ERROR_DLL_INIT_FAILED,                  /* c0000142 (STATUS_DLL_INIT_FAILED) */
   ERROR_MISSING_SYSTEMFILE,               /* c0000143 (STATUS_MISSING_SYSTEMFILE) */
   ERROR_UNHANDLED_EXCEPTION,              /* c0000144 (STATUS_UNHANDLED_EXCEPTION) */
   ERROR_APP_INIT_FAILURE,                 /* c0000145 (STATUS_APP_INIT_FAILURE) */
   ERROR_PAGEFILE_CREATE_FAILED,           /* c0000146 (STATUS_PAGEFILE_CREATE_FAILED) */
   ERROR_NO_PAGEFILE,                      /* c0000147 (STATUS_NO_PAGEFILE) */
   ERROR_INVALID_LEVEL,                    /* c0000148 (STATUS_INVALID_LEVEL) */
   ERROR_INVALID_PASSWORD,                 /* c0000149 (STATUS_WRONG_PASSWORD_CORE) */
   ERROR_ILLEGAL_FLOAT_CONTEXT,            /* c000014a (STATUS_ILLEGAL_FLOAT_CONTEXT) */
   ERROR_BROKEN_PIPE,                      /* c000014b (STATUS_PIPE_BROKEN) */
   ERROR_BADDB,                            /* c000014c (STATUS_REGISTRY_CORRUPT) */
   ERROR_REGISTRY_IO_FAILED,               /* c000014d (STATUS_REGISTRY_IO_FAILED) */
   ERROR_NO_EVENT_PAIR,                    /* c000014e (STATUS_NO_EVENT_PAIR) */
   ERROR_UNRECOGNIZED_VOLUME,              /* c000014f (STATUS_UNRECOGNIZED_VOLUME) */
   ERROR_SERIAL_NO_DEVICE,                 /* c0000150 (STATUS_SERIAL_NO_DEVICE_INITED) */
   ERROR_NO_SUCH_ALIAS,                    /* c0000151 (STATUS_NO_SUCH_ALIAS) */
   ERROR_MEMBER_NOT_IN_ALIAS,              /* c0000152 (STATUS_MEMBER_NOT_IN_ALIAS) */
   ERROR_MEMBER_IN_ALIAS,                  /* c0000153 (STATUS_MEMBER_IN_ALIAS) */
   ERROR_ALIAS_EXISTS,                     /* c0000154 (STATUS_ALIAS_EXISTS) */
   ERROR_LOGON_NOT_GRANTED,                /* c0000155 (STATUS_LOGON_NOT_GRANTED) */
   ERROR_TOO_MANY_SECRETS,                 /* c0000156 (STATUS_TOO_MANY_SECRETS) */
   ERROR_SECRET_TOO_LONG,                  /* c0000157 (STATUS_SECRET_TOO_LONG) */
   ERROR_INTERNAL_DB_ERROR,                /* c0000158 (STATUS_INTERNAL_DB_ERROR) */
   ERROR_FULLSCREEN_MODE,                  /* c0000159 (STATUS_FULLSCREEN_MODE) */
   ERROR_TOO_MANY_CONTEXT_IDS,             /* c000015a (STATUS_TOO_MANY_CONTEXT_IDS) */
   ERROR_LOGON_TYPE_NOT_GRANTED,           /* c000015b (STATUS_LOGON_TYPE_NOT_GRANTED) */
   ERROR_NOT_REGISTRY_FILE,                /* c000015c (STATUS_NOT_REGISTRY_FILE) */
   ERROR_NT_CROSS_ENCRYPTION_REQUIRED,     /* c000015d (STATUS_NT_CROSS_ENCRYPTION_REQUIRED) */
   ERROR_DOMAIN_CTRLR_CONFIG_ERROR,        /* c000015e (STATUS_DOMAIN_CTRLR_CONFIG_ERROR) */
   ERROR_IO_DEVICE,                        /* c000015f (STATUS_FT_MISSING_MEMBER) */
   0,                                      /* c0000160 (STATUS_ILL_FORMED_SERVICE_ENTRY) */
   0,                                      /* c0000161 (STATUS_ILLEGAL_CHARACTER) */
   ERROR_NO_UNICODE_TRANSLATION,           /* c0000162 (STATUS_UNMAPPABLE_CHARACTER) */
   0,                                      /* c0000163 (STATUS_UNDEFINED_CHARACTER) */
   0,                                      /* c0000164 (STATUS_FLOPPY_VOLUME) */
   ERROR_FLOPPY_ID_MARK_NOT_FOUND,         /* c0000165 (STATUS_FLOPPY_ID_MARK_NOT_FOUND) */
   ERROR_FLOPPY_WRONG_CYLINDER,            /* c0000166 (STATUS_FLOPPY_WRONG_CYLINDER) */
   ERROR_FLOPPY_UNKNOWN_ERROR,             /* c0000167 (STATUS_FLOPPY_UNKNOWN_ERROR) */
   ERROR_FLOPPY_BAD_REGISTERS,             /* c0000168 (STATUS_FLOPPY_BAD_REGISTERS) */
   ERROR_DISK_RECALIBRATE_FAILED,          /* c0000169 (STATUS_DISK_RECALIBRATE_FAILED) */
   ERROR_DISK_OPERATION_FAILED,            /* c000016a (STATUS_DISK_OPERATION_FAILED) */
   ERROR_DISK_RESET_FAILED,                /* c000016b (STATUS_DISK_RESET_FAILED) */
   ERROR_IRQ_BUSY,                         /* c000016c (STATUS_SHARED_IRQ_BUSY) */
   ERROR_IO_DEVICE,                        /* c000016d (STATUS_FT_ORPHANING) */
   0,                                      /* c000016e (STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT) */
   0,                                      /* c000016f */
   0,                                      /* c0000170 */
   0,                                      /* c0000171 */
   ERROR_PARTITION_FAILURE,                /* c0000172 (STATUS_PARTITION_FAILURE) */
   ERROR_INVALID_BLOCK_LENGTH,             /* c0000173 (STATUS_INVALID_BLOCK_LENGTH) */
   ERROR_DEVICE_NOT_PARTITIONED,           /* c0000174 (STATUS_DEVICE_NOT_PARTITIONED) */
   ERROR_UNABLE_TO_LOCK_MEDIA,             /* c0000175 (STATUS_UNABLE_TO_LOCK_MEDIA) */
   ERROR_UNABLE_TO_UNLOAD_MEDIA,           /* c0000176 (STATUS_UNABLE_TO_UNLOAD_MEDIA) */
   ERROR_EOM_OVERFLOW,                     /* c0000177 (STATUS_EOM_OVERFLOW) */
   ERROR_NO_MEDIA_IN_DRIVE,                /* c0000178 (STATUS_NO_MEDIA) */
   0,                                      /* c0000179 */
   ERROR_NO_SUCH_MEMBER,                   /* c000017a (STATUS_NO_SUCH_MEMBER) */
   ERROR_INVALID_MEMBER,                   /* c000017b (STATUS_INVALID_MEMBER) */
   ERROR_KEY_DELETED,                      /* c000017c (STATUS_KEY_DELETED) */
   ERROR_NO_LOG_SPACE,                     /* c000017d (STATUS_NO_LOG_SPACE) */
   ERROR_TOO_MANY_SIDS,                    /* c000017e (STATUS_TOO_MANY_SIDS) */
   ERROR_LM_CROSS_ENCRYPTION_REQUIRED,     /* c000017f (STATUS_LM_CROSS_ENCRYPTION_REQUIRED) */
   ERROR_KEY_HAS_CHILDREN,                 /* c0000180 (STATUS_KEY_HAS_CHILDREN) */
   ERROR_CHILD_MUST_BE_VOLATILE,           /* c0000181 (STATUS_CHILD_MUST_BE_VOLATILE) */
   ERROR_INVALID_PARAMETER,                /* c0000182 (STATUS_DEVICE_CONFIGURATION_ERROR) */
   ERROR_IO_DEVICE,                        /* c0000183 (STATUS_DRIVER_INTERNAL_ERROR) */
   ERROR_BAD_COMMAND,                      /* c0000184 (STATUS_INVALID_DEVICE_STATE) */
   ERROR_IO_DEVICE,                        /* c0000185 (STATUS_IO_DEVICE_ERROR) */
   ERROR_IO_DEVICE,                        /* c0000186 (STATUS_DEVICE_PROTOCOL_ERROR) */
   0,                                      /* c0000187 (STATUS_BACKUP_CONTROLLER) */
   ERROR_LOG_FILE_FULL,                    /* c0000188 (STATUS_LOG_FILE_FULL) */
   ERROR_WRITE_PROTECT,                    /* c0000189 (STATUS_TOO_LATE) */
   ERROR_NO_TRUST_LSA_SECRET,              /* c000018a (STATUS_NO_TRUST_LSA_SECRET) */
   ERROR_NO_TRUST_SAM_ACCOUNT,             /* c000018b (STATUS_NO_TRUST_SAM_ACCOUNT) */
   ERROR_TRUSTED_DOMAIN_FAILURE,           /* c000018c (STATUS_TRUSTED_DOMAIN_FAILURE) */
   ERROR_TRUSTED_RELATIONSHIP_FAILURE,     /* c000018d (STATUS_TRUSTED_RELATIONSHIP_FAILURE) */
   ERROR_EVENTLOG_FILE_CORRUPT,            /* c000018e (STATUS_EVENTLOG_FILE_CORRUPT) */
   ERROR_EVENTLOG_CANT_START,              /* c000018f (STATUS_EVENTLOG_CANT_START) */
   ERROR_TRUST_FAILURE,                    /* c0000190 (STATUS_TRUST_FAILURE) */
   0,                                      /* c0000191 (STATUS_MUTANT_LIMIT_EXCEEDED) */
   ERROR_NETLOGON_NOT_STARTED,             /* c0000192 (STATUS_NETLOGON_NOT_STARTED) */
   ERROR_ACCOUNT_EXPIRED,                  /* c0000193 (STATUS_ACCOUNT_EXPIRED) */
   ERROR_POSSIBLE_DEADLOCK,                /* c0000194 (STATUS_POSSIBLE_DEADLOCK) */
   ERROR_SESSION_CREDENTIAL_CONFLICT,      /* c0000195 (STATUS_NETWORK_CREDENTIAL_CONFLICT) */
   ERROR_REMOTE_SESSION_LIMIT_EXCEEDED,    /* c0000196 (STATUS_REMOTE_SESSION_LIMIT) */
   ERROR_EVENTLOG_FILE_CHANGED,            /* c0000197 (STATUS_EVENTLOG_FILE_CHANGED) */
   ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT,/* c0000198 (STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT) */
   ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT,/* c0000199 (STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT) */
   ERROR_NOLOGON_SERVER_TRUST_ACCOUNT,     /* c000019a (STATUS_NOLOGON_SERVER_TRUST_ACCOUNT) */
   ERROR_DOMAIN_TRUST_INCONSISTENT         /* c000019b (STATUS_DOMAIN_TRUST_INCONSISTENT) */
};

static const DWORD table_c0000202[396] =
{
   ERROR_NO_USER_SESSION_KEY,              /* c0000202 (STATUS_NO_USER_SESSION_KEY) */
   ERROR_UNEXP_NET_ERR,                    /* c0000203 (STATUS_USER_SESSION_DELETED) */
   ERROR_RESOURCE_LANG_NOT_FOUND,          /* c0000204 (STATUS_RESOURCE_LANG_NOT_FOUND) */
   ERROR_NOT_ENOUGH_SERVER_MEMORY,         /* c0000205 (STATUS_INSUFF_SERVER_RESOURCES) */
   ERROR_INVALID_USER_BUFFER,              /* c0000206 (STATUS_INVALID_BUFFER_SIZE) */
   ERROR_INVALID_NETNAME,                  /* c0000207 (STATUS_INVALID_ADDRESS_COMPONENT) */
   ERROR_INVALID_NETNAME,                  /* c0000208 (STATUS_INVALID_ADDRESS_WILDCARD) */
   ERROR_TOO_MANY_NAMES,                   /* c0000209 (STATUS_TOO_MANY_ADDRESSES) */
   ERROR_DUP_NAME,                         /* c000020a (STATUS_ADDRESS_ALREADY_EXISTS) */
   ERROR_NETNAME_DELETED,                  /* c000020b (STATUS_ADDRESS_CLOSED) */
   ERROR_NETNAME_DELETED,                  /* c000020c (STATUS_CONNECTION_DISCONNECTED) */
   ERROR_NETNAME_DELETED,                  /* c000020d (STATUS_CONNECTION_RESET) */
   ERROR_TOO_MANY_NAMES,                   /* c000020e (STATUS_TOO_MANY_NODES) */
   ERROR_UNEXP_NET_ERR,                    /* c000020f (STATUS_TRANSACTION_ABORTED) */
   ERROR_UNEXP_NET_ERR,                    /* c0000210 (STATUS_TRANSACTION_TIMED_OUT) */
   ERROR_UNEXP_NET_ERR,                    /* c0000211 (STATUS_TRANSACTION_NO_RELEASE) */
   ERROR_UNEXP_NET_ERR,                    /* c0000212 (STATUS_TRANSACTION_NO_MATCH) */
   ERROR_UNEXP_NET_ERR,                    /* c0000213 (STATUS_TRANSACTION_RESPONDED) */
   ERROR_UNEXP_NET_ERR,                    /* c0000214 (STATUS_TRANSACTION_INVALID_ID) */
   ERROR_UNEXP_NET_ERR,                    /* c0000215 (STATUS_TRANSACTION_INVALID_TYPE) */
   ERROR_NOT_SUPPORTED,                    /* c0000216 (STATUS_NOT_SERVER_SESSION) */
   ERROR_NOT_SUPPORTED,                    /* c0000217 (STATUS_NOT_CLIENT_SESSION) */
   0,                                      /* c0000218 (STATUS_CANNOT_LOAD_REGISTRY_FILE) */
   0,                                      /* c0000219 (STATUS_DEBUG_ATTACH_FAILED) */
   0,                                      /* c000021a (STATUS_SYSTEM_PROCESS_TERMINATED) */
   0,                                      /* c000021b (STATUS_DATA_NOT_ACCEPTED) */
   ERROR_NO_BROWSER_SERVERS_FOUND,         /* c000021c (STATUS_NO_BROWSER_SERVERS_FOUND) */
   0,                                      /* c000021d (STATUS_VDM_HARD_ERROR) */
   0,                                      /* c000021e (STATUS_DRIVER_CANCEL_TIMEOUT) */
   0,                                      /* c000021f (STATUS_REPLY_MESSAGE_MISMATCH) */
   ERROR_MAPPED_ALIGNMENT,                 /* c0000220 (STATUS_MAPPED_ALIGNMENT) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000221 (STATUS_IMAGE_CHECKSUM_MISMATCH) */
   0,                                      /* c0000222 (STATUS_LOST_WRITEBEHIND_DATA) */
   0,                                      /* c0000223 (STATUS_CLIENT_SERVER_PARAMETERS_INVALID) */
   ERROR_PASSWORD_MUST_CHANGE,             /* c0000224 (STATUS_PASSWORD_MUST_CHANGE) */
   ERROR_NOT_FOUND,                        /* c0000225 (STATUS_NOT_FOUND) */
   0,                                      /* c0000226 (STATUS_NOT_TINY_STREAM) */
   0,                                      /* c0000227 (STATUS_RECOVERY_FAILURE) */
   0,                                      /* c0000228 (STATUS_STACK_OVERFLOW_READ) */
   ERROR_INVALID_PARAMETER,                /* c0000229 (STATUS_FAIL_CHECK) */
   ERROR_OBJECT_ALREADY_EXISTS,            /* c000022a (STATUS_DUPLICATE_OBJECTID) */
   ERROR_OBJECT_ALREADY_EXISTS,            /* c000022b (STATUS_OBJECTID_EXISTS) */
   0,                                      /* c000022c (STATUS_CONVERT_TO_LARGE) */
   ERROR_RETRY,                            /* c000022d (STATUS_RETRY) */
   0,                                      /* c000022e (STATUS_FOUND_OUT_OF_SCOPE) */
   0,                                      /* c000022f (STATUS_ALLOCATE_BUCKET) */
   ERROR_SET_NOT_FOUND,                    /* c0000230 (STATUS_PROPSET_NOT_FOUND) */
   0,                                      /* c0000231 (STATUS_MARSHALL_OVERFLOW) */
   0,                                      /* c0000232 (STATUS_INVALID_VARIANT) */
   ERROR_DOMAIN_CONTROLLER_NOT_FOUND,      /* c0000233 (STATUS_DOMAIN_CONTROLLER_NOT_FOUND) */
   ERROR_ACCOUNT_LOCKED_OUT,               /* c0000234 (STATUS_ACCOUNT_LOCKED_OUT) */
   ERROR_INVALID_HANDLE,                   /* c0000235 (STATUS_HANDLE_NOT_CLOSABLE) */
   ERROR_CONNECTION_REFUSED,               /* c0000236 (STATUS_CONNECTION_REFUSED) */
   ERROR_GRACEFUL_DISCONNECT,              /* c0000237 (STATUS_GRACEFUL_DISCONNECT) */
   ERROR_ADDRESS_ALREADY_ASSOCIATED,       /* c0000238 (STATUS_ADDRESS_ALREADY_ASSOCIATED) */
   ERROR_ADDRESS_NOT_ASSOCIATED,           /* c0000239 (STATUS_ADDRESS_NOT_ASSOCIATED) */
   ERROR_CONNECTION_INVALID,               /* c000023a (STATUS_CONNECTION_INVALID) */
   ERROR_CONNECTION_ACTIVE,                /* c000023b (STATUS_CONNECTION_ACTIVE) */
   ERROR_NETWORK_UNREACHABLE,              /* c000023c (STATUS_NETWORK_UNREACHABLE) */
   ERROR_HOST_UNREACHABLE,                 /* c000023d (STATUS_HOST_UNREACHABLE) */
   ERROR_PROTOCOL_UNREACHABLE,             /* c000023e (STATUS_PROTOCOL_UNREACHABLE) */
   ERROR_PORT_UNREACHABLE,                 /* c000023f (STATUS_PORT_UNREACHABLE) */
   ERROR_REQUEST_ABORTED,                  /* c0000240 (STATUS_REQUEST_ABORTED) */
   ERROR_CONNECTION_ABORTED,               /* c0000241 (STATUS_CONNECTION_ABORTED) */
   0,                                      /* c0000242 (STATUS_BAD_COMPRESSION_BUFFER) */
   ERROR_USER_MAPPED_FILE,                 /* c0000243 (STATUS_USER_MAPPED_FILE) */
   0,                                      /* c0000244 (STATUS_AUDIT_FAILED) */
   0,                                      /* c0000245 (STATUS_TIMER_RESOLUTION_NOT_SET) */
   ERROR_CONNECTION_COUNT_LIMIT,           /* c0000246 (STATUS_CONNECTION_COUNT_LIMIT) */
   ERROR_LOGIN_TIME_RESTRICTION,           /* c0000247 (STATUS_LOGIN_TIME_RESTRICTION) */
   ERROR_LOGIN_WKSTA_RESTRICTION,          /* c0000248 (STATUS_LOGIN_WKSTA_RESTRICTION) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000249 (STATUS_IMAGE_MP_UP_MISMATCH) */
   0,                                      /* c000024a */
   0,                                      /* c000024b */
   0,                                      /* c000024c */
   0,                                      /* c000024d */
   0,                                      /* c000024e */
   0,                                      /* c000024f */
   0,                                      /* c0000250 (STATUS_INSUFFICIENT_LOGON_INFO) */
   0,                                      /* c0000251 (STATUS_BAD_DLL_ENTRYPOINT) */
   0,                                      /* c0000252 (STATUS_BAD_SERVICE_ENTRYPOINT) */
   ERROR_CONNECTION_ABORTED,               /* c0000253 (STATUS_LPC_REPLY_LOST) */
   0,                                      /* c0000254 (STATUS_IP_ADDRESS_CONFLICT1) */
   0,                                      /* c0000255 (STATUS_IP_ADDRESS_CONFLICT2) */
   0,                                      /* c0000256 (STATUS_REGISTRY_QUOTA_LIMIT) */
   ERROR_HOST_UNREACHABLE,                 /* c0000257 (STATUS_PATH_NOT_COVERED) */
   0,                                      /* c0000258 (STATUS_NO_CALLBACK_ACTIVE) */
   ERROR_LICENSE_QUOTA_EXCEEDED,           /* c0000259 (STATUS_LICENSE_QUOTA_EXCEEDED) */
   0,                                      /* c000025a (STATUS_PWD_TOO_SHORT) */
   0,                                      /* c000025b (STATUS_PWD_TOO_RECENT) */
   0,                                      /* c000025c (STATUS_PWD_HISTORY_CONFLICT) */
   0,                                      /* c000025d */
   ERROR_SERVICE_DISABLED,                 /* c000025e (STATUS_PLUGPLAY_NO_DEVICE) */
   0,                                      /* c000025f (STATUS_UNSUPPORTED_COMPRESSION) */
   0,                                      /* c0000260 (STATUS_INVALID_HW_PROFILE) */
   0,                                      /* c0000261 (STATUS_INVALID_PLUGPLAY_DEVICE_PATH) */
   ERROR_INVALID_ORDINAL,                  /* c0000262 (STATUS_DRIVER_ORDINAL_NOT_FOUND) */
   ERROR_PROC_NOT_FOUND,                   /* c0000263 (STATUS_DRIVER_ENTRYPOINT_NOT_FOUND) */
   ERROR_NOT_OWNER,                        /* c0000264 (STATUS_RESOURCE_NOT_OWNED) */
   ERROR_TOO_MANY_LINKS,                   /* c0000265 (STATUS_TOO_MANY_LINKS) */
   0,                                      /* c0000266 (STATUS_QUOTA_LIST_INCONSISTENT) */
   ERROR_FILE_OFFLINE,                     /* c0000267 (STATUS_FILE_IS_OFFLINE) */
   0,                                      /* c0000268 (STATUS_EVALUATION_EXPIRATION) */
   0,                                      /* c0000269 (STATUS_ILLEGAL_DLL_RELOCATION) */
   ERROR_CTX_LICENSE_NOT_AVAILABLE,        /* c000026a (STATUS_LICENSE_VIOLATION) */
   0,                                      /* c000026b (STATUS_DLL_INIT_FAILED_LOGOFF) */
   ERROR_BAD_DRIVER,                       /* c000026c (STATUS_DRIVER_UNABLE_TO_LOAD) */
   ERROR_CONNECTION_UNAVAIL,               /* c000026d (STATUS_DFS_UNAVAILABLE) */
   ERROR_NOT_READY,                        /* c000026e (STATUS_VOLUME_DISMOUNTED) */
   0,                                      /* c000026f (STATUS_WX86_INTERNAL_ERROR) */
   0,                                      /* c0000270 (STATUS_WX86_FLOAT_STACK_CHECK) */
   0,                                      /* c0000271 (STATUS_VALIDATE_CONTINUE) */
   ERROR_NO_MATCH,                         /* c0000272 (STATUS_NO_MATCH) */
   0,                                      /* c0000273 (STATUS_NO_MORE_MATCHES) */
   0,                                      /* c0000274 */
   ERROR_NOT_A_REPARSE_POINT,              /* c0000275 (STATUS_NOT_A_REPARSE_POINT) */
   ERROR_REPARSE_TAG_INVALID,              /* c0000276 (STATUS_IO_REPARSE_TAG_INVALID) */
   ERROR_REPARSE_TAG_MISMATCH,             /* c0000277 (STATUS_IO_REPARSE_TAG_MISMATCH) */
   ERROR_INVALID_REPARSE_DATA,             /* c0000278 (STATUS_IO_REPARSE_DATA_INVALID) */
   ERROR_CANT_ACCESS_FILE,                 /* c0000279 (STATUS_IO_REPARSE_TAG_NOT_HANDLED) */
   0,                                      /* c000027a */
   0,                                      /* c000027b */
   0,                                      /* c000027c */
   0,                                      /* c000027d */
   0,                                      /* c000027e */
   0,                                      /* c000027f */
   ERROR_CANT_RESOLVE_FILENAME,            /* c0000280 (STATUS_REPARSE_POINT_NOT_RESOLVED) */
   ERROR_BAD_PATHNAME,                     /* c0000281 (STATUS_DIRECTORY_IS_A_REPARSE_POINT) */
   0,                                      /* c0000282 (STATUS_RANGE_LIST_CONFLICT) */
   ERROR_SOURCE_ELEMENT_EMPTY,             /* c0000283 (STATUS_SOURCE_ELEMENT_EMPTY) */
   ERROR_DESTINATION_ELEMENT_FULL,         /* c0000284 (STATUS_DESTINATION_ELEMENT_FULL) */
   ERROR_ILLEGAL_ELEMENT_ADDRESS,          /* c0000285 (STATUS_ILLEGAL_ELEMENT_ADDRESS) */
   ERROR_MAGAZINE_NOT_PRESENT,             /* c0000286 (STATUS_MAGAZINE_NOT_PRESENT) */
   ERROR_DEVICE_REINITIALIZATION_NEEDED,   /* c0000287 (STATUS_REINITIALIZATION_NEEDED) */
   0,                                      /* c0000288 */
   0,                                      /* c0000289 */
   ERROR_ACCESS_DENIED,                    /* c000028a (STATUS_ENCRYPTION_FAILED) */
   ERROR_ACCESS_DENIED,                    /* c000028b (STATUS_DECRYPTION_FAILED) */
   0,                                      /* c000028c (STATUS_RANGE_NOT_FOUND) */
   ERROR_ACCESS_DENIED,                    /* c000028d (STATUS_NO_RECOVERY_POLICY) */
   ERROR_ACCESS_DENIED,                    /* c000028e (STATUS_NO_EFS) */
   ERROR_ACCESS_DENIED,                    /* c000028f (STATUS_WRONG_EFS) */
   ERROR_ACCESS_DENIED,                    /* c0000290 (STATUS_NO_USER_KEYS) */
   ERROR_FILE_NOT_ENCRYPTED,               /* c0000291 (STATUS_FILE_NOT_ENCRYPTED) */
   ERROR_NOT_EXPORT_FORMAT,                /* c0000292 (STATUS_NOT_EXPORT_FORMAT) */
   ERROR_FILE_ENCRYPTED,                   /* c0000293 (STATUS_FILE_ENCRYPTED) */
   0,                                      /* c0000294 */
   ERROR_WMI_GUID_NOT_FOUND,               /* c0000295 (STATUS_WMI_GUID_NOT_FOUND) */
   ERROR_WMI_INSTANCE_NOT_FOUND,           /* c0000296 (STATUS_WMI_INSTANCE_NOT_FOUND) */
   ERROR_WMI_ITEMID_NOT_FOUND,             /* c0000297 (STATUS_WMI_ITEMID_NOT_FOUND) */
   ERROR_WMI_TRY_AGAIN,                    /* c0000298 (STATUS_WMI_TRY_AGAIN) */
   ERROR_SHARED_POLICY,                    /* c0000299 (STATUS_SHARED_POLICY) */
   ERROR_POLICY_OBJECT_NOT_FOUND,          /* c000029a (STATUS_POLICY_OBJECT_NOT_FOUND) */
   ERROR_POLICY_ONLY_IN_DS,                /* c000029b (STATUS_POLICY_ONLY_IN_DS) */
   ERROR_INVALID_FUNCTION,                 /* c000029c (STATUS_VOLUME_NOT_UPGRADED) */
   ERROR_REMOTE_STORAGE_NOT_ACTIVE,        /* c000029d (STATUS_REMOTE_STORAGE_NOT_ACTIVE) */
   ERROR_REMOTE_STORAGE_MEDIA_ERROR,       /* c000029e (STATUS_REMOTE_STORAGE_MEDIA_ERROR) */
   ERROR_NO_TRACKING_SERVICE,              /* c000029f (STATUS_NO_TRACKING_SERVICE) */
   0,                                      /* c00002a0 (STATUS_SERVER_SID_MISMATCH) */
   ERROR_DS_NO_ATTRIBUTE_OR_VALUE,         /* c00002a1 (STATUS_DS_NO_ATTRIBUTE_OR_VALUE) */
   ERROR_DS_INVALID_ATTRIBUTE_SYNTAX,      /* c00002a2 (STATUS_DS_INVALID_ATTRIBUTE_SYNTAX) */
   ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED,      /* c00002a3 (STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED) */
   ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS,     /* c00002a4 (STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS) */
   ERROR_DS_BUSY,                          /* c00002a5 (STATUS_DS_BUSY) */
   ERROR_DS_UNAVAILABLE,                   /* c00002a6 (STATUS_DS_UNAVAILABLE) */
   ERROR_DS_NO_RIDS_ALLOCATED,             /* c00002a7 (STATUS_DS_NO_RIDS_ALLOCATED) */
   ERROR_DS_NO_MORE_RIDS,                  /* c00002a8 (STATUS_DS_NO_MORE_RIDS) */
   ERROR_DS_INCORRECT_ROLE_OWNER,          /* c00002a9 (STATUS_DS_INCORRECT_ROLE_OWNER) */
   ERROR_DS_RIDMGR_INIT_ERROR,             /* c00002aa (STATUS_DS_RIDMGR_INIT_ERROR) */
   ERROR_DS_OBJ_CLASS_VIOLATION,           /* c00002ab (STATUS_DS_OBJ_CLASS_VIOLATION) */
   ERROR_DS_CANT_ON_NON_LEAF,              /* c00002ac (STATUS_DS_CANT_ON_NON_LEAF) */
   ERROR_DS_CANT_ON_RDN,                   /* c00002ad (STATUS_DS_CANT_ON_RDN) */
   ERROR_DS_CANT_MOD_OBJ_CLASS,            /* c00002ae (STATUS_DS_CANT_MOD_OBJ_CLASS) */
   ERROR_DS_CROSS_DOM_MOVE_ERROR,          /* c00002af (STATUS_DS_CROSS_DOM_MOVE_FAILED) */
   ERROR_DS_GC_NOT_AVAILABLE,              /* c00002b0 (STATUS_DS_GC_NOT_AVAILABLE) */
   ERROR_DS_DS_REQUIRED,                   /* c00002b1 (STATUS_DIRECTORY_SERVICE_REQUIRED) */
   ERROR_REPARSE_ATTRIBUTE_CONFLICT,       /* c00002b2 (STATUS_REPARSE_ATTRIBUTE_CONFLICT) */
   0,                                      /* c00002b3 (STATUS_CANT_ENABLE_DENY_ONLY) */
   0,                                      /* c00002b4 (STATUS_FLOAT_MULTIPLE_FAULTS) */
   0,                                      /* c00002b5 (STATUS_FLOAT_MULTIPLE_TRAPS) */
   ERROR_DEVICE_REMOVED,                   /* c00002b6 (STATUS_DEVICE_REMOVED) */
   ERROR_JOURNAL_DELETE_IN_PROGRESS,       /* c00002b7 (STATUS_JOURNAL_DELETE_IN_PROGRESS) */
   ERROR_JOURNAL_NOT_ACTIVE,               /* c00002b8 (STATUS_JOURNAL_NOT_ACTIVE) */
   0,                                      /* c00002b9 (STATUS_NOINTERFACE) */
   0,                                      /* c00002ba */
   0,                                      /* c00002bb */
   0,                                      /* c00002bc */
   0,                                      /* c00002bd */
   0,                                      /* c00002be */
   0,                                      /* c00002bf */
   0,                                      /* c00002c0 */
   ERROR_DS_ADMIN_LIMIT_EXCEEDED,          /* c00002c1 (STATUS_DS_ADMIN_LIMIT_EXCEEDED) */
   0,                                      /* c00002c2 (STATUS_DRIVER_FAILED_SLEEP) */
   ERROR_MUTUAL_AUTH_FAILED,               /* c00002c3 (STATUS_MUTUAL_AUTHENTICATION_FAILED) */
   0,                                      /* c00002c4 (STATUS_CORRUPT_SYSTEM_FILE) */
   ERROR_NOACCESS,                         /* c00002c5 (STATUS_DATATYPE_MISALIGNMENT_ERROR) */
   ERROR_WMI_READ_ONLY,                    /* c00002c6 (STATUS_WMI_READ_ONLY) */
   ERROR_WMI_SET_FAILURE,                  /* c00002c7 (STATUS_WMI_SET_FAILURE) */
   0,                                      /* c00002c8 (STATUS_COMMITMENT_MINIMUM) */
   ERROR_REG_NAT_CONSUMPTION,              /* c00002c9 (STATUS_REG_NAT_CONSUMPTION) */
   ERROR_TRANSPORT_FULL,                   /* c00002ca (STATUS_TRANSPORT_FULL) */
   ERROR_DS_SAM_INIT_FAILURE,              /* c00002cb (STATUS_DS_SAM_INIT_FAILURE) */
   ERROR_ONLY_IF_CONNECTED,                /* c00002cc (STATUS_ONLY_IF_CONNECTED) */
   ERROR_DS_SENSITIVE_GROUP_VIOLATION,     /* c00002cd (STATUS_DS_SENSITIVE_GROUP_VIOLATION) */
   0,                                      /* c00002ce (STATUS_PNP_RESTART_ENUMERATION) */
   ERROR_JOURNAL_ENTRY_DELETED,            /* c00002cf (STATUS_JOURNAL_ENTRY_DELETED) */
   ERROR_DS_CANT_MOD_PRIMARYGROUPID,       /* c00002d0 (STATUS_DS_CANT_MOD_PRIMARYGROUPID) */
   0,                                      /* c00002d1 (STATUS_SYSTEM_IMAGE_BAD_SIGNATURE) */
   0,                                      /* c00002d2 (STATUS_PNP_REBOOT_REQUIRED) */
   0,                                      /* c00002d3 (STATUS_POWER_STATE_INVALID) */
   ERROR_DS_INVALID_GROUP_TYPE,            /* c00002d4 (STATUS_DS_INVALID_GROUP_TYPE) */
   ERROR_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN, /* c00002d5 (STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN) */
   ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN,  /* c00002d6 (STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN) */
   ERROR_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER,      /* c00002d7 (STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER) */
   ERROR_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER,  /* c00002d8 (STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER) */
   ERROR_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER,   /* c00002d9 (STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER) */
   ERROR_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER,/* c00002da (STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER) */
   ERROR_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER,   /* c00002db (STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER) */
   ERROR_DS_HAVE_PRIMARY_MEMBERS,               /* c00002dc (STATUS_DS_HAVE_PRIMARY_MEMBERS) */
   ERROR_NOT_SUPPORTED,                    /* c00002dd (STATUS_WMI_NOT_SUPPORTED) */
   0,                                      /* c00002de (STATUS_INSUFFICIENT_POWER) */
   ERROR_DS_SAM_NEED_BOOTKEY_PASSWORD,     /* c00002df (STATUS_SAM_NEED_BOOTKEY_PASSWORD) */
   ERROR_DS_SAM_NEED_BOOTKEY_FLOPPY,       /* c00002e0 (STATUS_SAM_NEED_BOOTKEY_FLOPPY) */
   ERROR_DS_CANT_START,                    /* c00002e1 (STATUS_DS_CANT_START) */
   ERROR_DS_INIT_FAILURE,                  /* c00002e2 (STATUS_DS_INIT_FAILURE) */
   ERROR_SAM_INIT_FAILURE,                 /* c00002e3 (STATUS_SAM_INIT_FAILURE) */
   ERROR_DS_GC_REQUIRED,                   /* c00002e4 (STATUS_DS_GC_REQUIRED) */
   ERROR_DS_LOCAL_MEMBER_OF_LOCAL_ONLY,    /* c00002e5 (STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY) */
   ERROR_DS_NO_FPO_IN_UNIVERSAL_GROUPS,    /* c00002e6 (STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS) */
   ERROR_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED,     /* c00002e7 (STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED) */
   0,                                      /* c00002e8 (STATUS_MULTIPLE_FAULT_VIOLATION) */
   ERROR_CURRENT_DOMAIN_NOT_ALLOWED,       /* c00002e9 (STATUS_CURRENT_DOMAIN_NOT_ALLOWED) */
   ERROR_CANNOT_MAKE,                      /* c00002ea (STATUS_CANNOT_MAKE) */
   0,                                      /* c00002eb (STATUS_SYSTEM_SHUTDOWN) */
   ERROR_DS_INIT_FAILURE_CONSOLE,          /* c00002ec (STATUS_DS_INIT_FAILURE_CONSOLE) */
   ERROR_DS_SAM_INIT_FAILURE_CONSOLE,      /* c00002ed (STATUS_DS_SAM_INIT_FAILURE_CONSOLE) */
   SEC_E_UNFINISHED_CONTEXT_DELETED,       /* c00002ee (STATUS_UNFINISHED_CONTEXT_DELETED) */
   SEC_E_NO_TGT_REPLY,                     /* c00002ef (STATUS_NO_TGT_REPLY) */
   ERROR_FILE_NOT_FOUND,                   /* c00002f0 (STATUS_OBJECTID_NOT_FOUND) */
   SEC_E_NO_IP_ADDRESSES,                  /* c00002f1 (STATUS_NO_IP_ADDRESSES) */
   SEC_E_WRONG_CREDENTIAL_HANDLE,          /* c00002f2 (STATUS_WRONG_CREDENTIAL_HANDLE) */
   SEC_E_CRYPTO_SYSTEM_INVALID,            /* c00002f3 (STATUS_CRYPTO_SYSTEM_INVALID) */
   SEC_E_MAX_REFERRALS_EXCEEDED,           /* c00002f4 (STATUS_MAX_REFERRALS_EXCEEDED) */
   SEC_E_MUST_BE_KDC,                      /* c00002f5 (STATUS_MUST_BE_KDC) */
   SEC_E_STRONG_CRYPTO_NOT_SUPPORTED,      /* c00002f6 (STATUS_STRONG_CRYPTO_NOT_SUPPORTED) */
   SEC_E_TOO_MANY_PRINCIPALS,              /* c00002f7 (STATUS_TOO_MANY_PRINCIPALS) */
   SEC_E_NO_PA_DATA,                       /* c00002f8 (STATUS_NO_PA_DATA) */
   SEC_E_PKINIT_NAME_MISMATCH,             /* c00002f9 (STATUS_PKINIT_NAME_MISMATCH) */
   SEC_E_SMARTCARD_LOGON_REQUIRED,         /* c00002fa (STATUS_SMARTCARD_LOGON_REQUIRED) */
   SEC_E_KDC_INVALID_REQUEST,              /* c00002fb (STATUS_KDC_INVALID_REQUEST) */
   SEC_E_KDC_UNABLE_TO_REFER,              /* c00002fc (STATUS_KDC_UNABLE_TO_REFER) */
   SEC_E_KDC_UNKNOWN_ETYPE,                /* c00002fd (STATUS_KDC_UNKNOWN_ETYPE) */
   ERROR_SHUTDOWN_IN_PROGRESS,             /* c00002fe (STATUS_SHUTDOWN_IN_PROGRESS) */
   ERROR_SERVER_SHUTDOWN_IN_PROGRESS,      /* c00002ff (STATUS_SERVER_SHUTDOWN_IN_PROGRESS) */
   ERROR_NOT_SUPPORTED_ON_SBS,             /* c0000300 (STATUS_NOT_SUPPORTED_ON_SBS) */
   ERROR_WMI_GUID_DISCONNECTED,            /* c0000301 (STATUS_WMI_GUID_DISCONNECTED) */
   ERROR_WMI_ALREADY_DISABLED,             /* c0000302 (STATUS_WMI_ALREADY_DISABLED) */
   ERROR_WMI_ALREADY_ENABLED,              /* c0000303 (STATUS_WMI_ALREADY_ENABLED) */
   ERROR_DISK_TOO_FRAGMENTED,              /* c0000304 (STATUS_MFT_TOO_FRAGMENTED) */
   STG_E_STATUS_COPY_PROTECTION_FAILURE,   /* c0000305 (STATUS_COPY_PROTECTION_FAILURE) */
   STG_E_CSS_AUTHENTICATION_FAILURE,       /* c0000306 (STATUS_CSS_AUTHENTICATION_FAILURE) */
   STG_E_CSS_KEY_NOT_PRESENT,              /* c0000307 (STATUS_CSS_KEY_NOT_PRESENT) */
   STG_E_CSS_KEY_NOT_ESTABLISHED,          /* c0000308 (STATUS_CSS_KEY_NOT_ESTABLISHED) */
   STG_E_CSS_SCRAMBLED_SECTOR,             /* c0000309 (STATUS_CSS_SCRAMBLED_SECTOR) */
   STG_E_CSS_REGION_MISMATCH,              /* c000030a (STATUS_CSS_REGION_MISMATCH) */
   STG_E_RESETS_EXHAUSTED,                 /* c000030b (STATUS_CSS_RESETS_EXHAUSTED) */
   0,                                      /* c000030c */
   0,                                      /* c000030d */
   0,                                      /* c000030e */
   0,                                      /* c000030f */
   0,                                      /* c0000310 */
   0,                                      /* c0000311 */
   0,                                      /* c0000312 */
   0,                                      /* c0000313 */
   0,                                      /* c0000314 */
   0,                                      /* c0000315 */
   0,                                      /* c0000316 */
   0,                                      /* c0000317 */
   0,                                      /* c0000318 */
   0,                                      /* c0000319 */
   0,                                      /* c000031a */
   0,                                      /* c000031b */
   0,                                      /* c000031c */
   0,                                      /* c000031d */
   0,                                      /* c000031e */
   0,                                      /* c000031f */
   ERROR_PKINIT_FAILURE,                   /* c0000320 (STATUS_PKINIT_FAILURE) */
   ERROR_SMARTCARD_SUBSYSTEM_FAILURE,      /* c0000321 (STATUS_SMARTCARD_SUBSYSTEM_FAILURE) */
   SEC_E_NO_KERB_KEY,                      /* c0000322 (STATUS_NO_KERB_KEY) */
   0,                                      /* c0000323 */
   0,                                      /* c0000324 */
   0,                                      /* c0000325 */
   0,                                      /* c0000326 */
   0,                                      /* c0000327 */
   0,                                      /* c0000328 */
   0,                                      /* c0000329 */
   0,                                      /* c000032a */
   0,                                      /* c000032b */
   0,                                      /* c000032c */
   0,                                      /* c000032d */
   0,                                      /* c000032e */
   0,                                      /* c000032f */
   0,                                      /* c0000330 */
   0,                                      /* c0000331 */
   0,                                      /* c0000332 */
   0,                                      /* c0000333 */
   0,                                      /* c0000334 */
   0,                                      /* c0000335 */
   0,                                      /* c0000336 */
   0,                                      /* c0000337 */
   0,                                      /* c0000338 */
   0,                                      /* c0000339 */
   0,                                      /* c000033a */
   0,                                      /* c000033b */
   0,                                      /* c000033c */
   0,                                      /* c000033d */
   0,                                      /* c000033e */
   0,                                      /* c000033f */
   0,                                      /* c0000340 */
   0,                                      /* c0000341 */
   0,                                      /* c0000342 */
   0,                                      /* c0000343 */
   0,                                      /* c0000344 */
   0,                                      /* c0000345 */
   0,                                      /* c0000346 */
   0,                                      /* c0000347 */
   0,                                      /* c0000348 */
   0,                                      /* c0000349 */
   0,                                      /* c000034a */
   0,                                      /* c000034b */
   0,                                      /* c000034c */
   0,                                      /* c000034d */
   0,                                      /* c000034e */
   0,                                      /* c000034f */
   ERROR_HOST_DOWN,                        /* c0000350 (STATUS_HOST_DOWN) */
   SEC_E_UNSUPPORTED_PREAUTH,              /* c0000351 (STATUS_UNSUPPORTED_PREAUTH) */
   ERROR_EFS_ALG_BLOB_TOO_BIG,             /* c0000352 (STATUS_EFS_ALG_BLOB_TOO_BIG) */
   0,                                      /* c0000353 (STATUS_PORT_NOT_SET) */
   0,                                      /* c0000354 (STATUS_DEBUGGER_INACTIVE) */
   0,                                      /* c0000355 (STATUS_DS_VERSION_CHECK_FAILURE) */
   ERROR_AUDITING_DISABLED,                /* c0000356 (STATUS_AUDITING_DISABLED) */
   ERROR_DS_MACHINE_ACCOUNT_CREATED_PRENT4,/* c0000357 (STATUS_PRENT4_MACHINE_ACCOUNT) */
   ERROR_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER, /* c0000358 (STATUS_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER) */
   ERROR_BAD_EXE_FORMAT,                   /* c0000359 (STATUS_INVALID_IMAGE_WIN_32) */
   ERROR_BAD_EXE_FORMAT,                   /* c000035a (STATUS_INVALID_IMAGE_WIN_64) */
   SEC_E_BAD_BINDINGS,                     /* c000035b (STATUS_BAD_BINDINGS) */
   ERROR_NO_USER_SESSION_KEY,              /* c000035c (STATUS_NETWORK_SESSION_EXPIRED) */
   0,                                      /* c000035d (STATUS_APPHELP_BLOCK) */
   0,                                      /* c000035e (STATUS_ALL_SIDS_FILTERED) */
   0,                                      /* c000035f (STATUS_NOT_SAFE_MODE_DRIVER) */
   0,                                      /* c0000360 */
   ERROR_ACCESS_DISABLED_BY_POLICY,        /* c0000361 (STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT) */
   ERROR_ACCESS_DISABLED_BY_POLICY,        /* c0000362 (STATUS_ACCESS_DISABLED_BY_POLICY_PATH) */
   ERROR_ACCESS_DISABLED_BY_POLICY,        /* c0000363 (STATUS_ACCESS_DISABLED_BY_POLICY_PUBLISHER) */
   ERROR_ACCESS_DISABLED_BY_POLICY,        /* c0000364 (STATUS_ACCESS_DISABLED_BY_POLICY_OTHER) */
   0,                                      /* c0000365 (STATUS_FAILED_DRIVER_ENTRY) */
   0,                                      /* c0000366 (STATUS_DEVICE_ENUMERATION_ERROR) */
   0,                                      /* c0000367 */
   0,                                      /* c0000368 (STATUS_MOUNT_POINT_NOT_RESOLVED) */
   0,                                      /* c0000369 (STATUS_INVALID_DEVICE_OBJECT_PARAMETER) */
   0,                                      /* c000036a (STATUS_MCA_OCCURED) */
   ERROR_DRIVER_BLOCKED,                   /* c000036b (STATUS_DRIVER_BLOCKED_CRITICAL) */
   ERROR_DRIVER_BLOCKED,                   /* c000036c (STATUS_DRIVER_BLOCKED) */
   0,                                      /* c000036d (STATUS_DRIVER_DATABASE_ERROR) */
   0,                                      /* c000036e (STATUS_SYSTEM_HIVE_TOO_LARGE) */
   ERROR_INVALID_IMPORT_OF_NON_DLL,        /* c000036f (STATUS_INVALID_IMPORT_OF_NON_DLL) */
   0,                                      /* c0000370 */
   0,                                      /* c0000371 */
   0,                                      /* c0000372 */
   0,                                      /* c0000373 */
   0,                                      /* c0000374 */
   0,                                      /* c0000375 */
   0,                                      /* c0000376 */
   0,                                      /* c0000377 */
   0,                                      /* c0000378 */
   0,                                      /* c0000379 */
   0,                                      /* c000037a */
   0,                                      /* c000037b */
   0,                                      /* c000037c */
   0,                                      /* c000037d */
   0,                                      /* c000037e */
   0,                                      /* c000037f */
   SCARD_W_WRONG_CHV,                      /* c0000380 (STATUS_SMARTCARD_WRONG_PIN) */
   SCARD_W_CHV_BLOCKED,                    /* c0000381 (STATUS_SMARTCARD_CARD_BLOCKED) */
   SCARD_W_CARD_NOT_AUTHENTICATED,         /* c0000382 (STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED) */
   SCARD_E_NO_SMARTCARD,                   /* c0000383 (STATUS_SMARTCARD_NO_CARD) */
   NTE_NO_KEY,                             /* c0000384 (STATUS_SMARTCARD_NO_KEY_CONTAINER) */
   SCARD_E_NO_SUCH_CERTIFICATE,            /* c0000385 (STATUS_SMARTCARD_NO_CERTIFICATE) */
   NTE_BAD_KEYSET,                         /* c0000386 (STATUS_SMARTCARD_NO_KEYSET) */
   SCARD_E_COMM_DATA_LOST,                 /* c0000387 (STATUS_SMARTCARD_IO_ERROR) */
   ERROR_DOWNGRADE_DETECTED,               /* c0000388 (STATUS_DOWNGRADE_DETECTED) */
   SEC_E_SMARTCARD_CERT_REVOKED,           /* c0000389 (STATUS_SMARTCARD_CERT_REVOKED) */
   SEC_E_ISSUING_CA_UNTRUSTED,             /* c000038a (STATUS_ISSUING_CA_UNTRUSTED) */
   SEC_E_REVOCATION_OFFLINE_C,             /* c000038b (STATUS_REVOCATION_OFFLINE_C) */
   SEC_E_PKINIT_CLIENT_FAILURE,            /* c000038c (STATUS_PKINIT_CLIENT_FAILURE) */
   SEC_E_SMARTCARD_CERT_EXPIRED            /* c000038d (STATUS_SMARTCARD_CERT_EXPIRED) */
};

static const DWORD table_c0020001[99] =
{
   RPC_S_INVALID_STRING_BINDING,           /* c0020001 (RPC_NT_INVALID_STRING_BINDING) */
   RPC_S_WRONG_KIND_OF_BINDING,            /* c0020002 (RPC_NT_WRONG_KIND_OF_BINDING) */
   ERROR_INVALID_HANDLE,                   /* c0020003 (RPC_NT_INVALID_BINDING) */
   RPC_S_PROTSEQ_NOT_SUPPORTED,            /* c0020004 (RPC_NT_PROTSEQ_NOT_SUPPORTED) */
   RPC_S_INVALID_RPC_PROTSEQ,              /* c0020005 (RPC_NT_INVALID_RPC_PROTSEQ) */
   RPC_S_INVALID_STRING_UUID,              /* c0020006 (RPC_NT_INVALID_STRING_UUID) */
   RPC_S_INVALID_ENDPOINT_FORMAT,          /* c0020007 (RPC_NT_INVALID_ENDPOINT_FORMAT) */
   RPC_S_INVALID_NET_ADDR,                 /* c0020008 (RPC_NT_INVALID_NET_ADDR) */
   RPC_S_NO_ENDPOINT_FOUND,                /* c0020009 (RPC_NT_NO_ENDPOINT_FOUND) */
   RPC_S_INVALID_TIMEOUT,                  /* c002000a (RPC_NT_INVALID_TIMEOUT) */
   RPC_S_OBJECT_NOT_FOUND,                 /* c002000b (RPC_NT_OBJECT_NOT_FOUND) */
   RPC_S_ALREADY_REGISTERED,               /* c002000c (RPC_NT_ALREADY_REGISTERED) */
   RPC_S_TYPE_ALREADY_REGISTERED,          /* c002000d (RPC_NT_TYPE_ALREADY_REGISTERED) */
   RPC_S_ALREADY_LISTENING,                /* c002000e (RPC_NT_ALREADY_LISTENING) */
   RPC_S_NO_PROTSEQS_REGISTERED,           /* c002000f (RPC_NT_NO_PROTSEQS_REGISTERED) */
   RPC_S_NOT_LISTENING,                    /* c0020010 (RPC_NT_NOT_LISTENING) */
   RPC_S_UNKNOWN_MGR_TYPE,                 /* c0020011 (RPC_NT_UNKNOWN_MGR_TYPE) */
   RPC_S_UNKNOWN_IF,                       /* c0020012 (RPC_NT_UNKNOWN_IF) */
   RPC_S_NO_BINDINGS,                      /* c0020013 (RPC_NT_NO_BINDINGS) */
   RPC_S_NO_PROTSEQS,                      /* c0020014 (RPC_NT_NO_PROTSEQS) */
   RPC_S_CANT_CREATE_ENDPOINT,             /* c0020015 (RPC_NT_CANT_CREATE_ENDPOINT) */
   RPC_S_OUT_OF_RESOURCES,                 /* c0020016 (RPC_NT_OUT_OF_RESOURCES) */
   RPC_S_SERVER_UNAVAILABLE,               /* c0020017 (RPC_NT_SERVER_UNAVAILABLE) */
   RPC_S_SERVER_TOO_BUSY,                  /* c0020018 (RPC_NT_SERVER_TOO_BUSY) */
   RPC_S_INVALID_NETWORK_OPTIONS,          /* c0020019 (RPC_NT_INVALID_NETWORK_OPTIONS) */
   RPC_S_NO_CALL_ACTIVE,                   /* c002001a (RPC_NT_NO_CALL_ACTIVE) */
   RPC_S_CALL_FAILED,                      /* c002001b (RPC_NT_CALL_FAILED) */
   RPC_S_CALL_FAILED_DNE,                  /* c002001c (RPC_NT_CALL_FAILED_DNE) */
   RPC_S_PROTOCOL_ERROR,                   /* c002001d (RPC_NT_PROTOCOL_ERROR) */
   0,                                      /* c002001e */
   RPC_S_UNSUPPORTED_TRANS_SYN,            /* c002001f (RPC_NT_UNSUPPORTED_TRANS_SYN) */
   0,                                      /* c0020020 */
   RPC_S_UNSUPPORTED_TYPE,                 /* c0020021 (RPC_NT_UNSUPPORTED_TYPE) */
   RPC_S_INVALID_TAG,                      /* c0020022 (RPC_NT_INVALID_TAG) */
   RPC_S_INVALID_BOUND,                    /* c0020023 (RPC_NT_INVALID_BOUND) */
   RPC_S_NO_ENTRY_NAME,                    /* c0020024 (RPC_NT_NO_ENTRY_NAME) */
   RPC_S_INVALID_NAME_SYNTAX,              /* c0020025 (RPC_NT_INVALID_NAME_SYNTAX) */
   RPC_S_UNSUPPORTED_NAME_SYNTAX,          /* c0020026 (RPC_NT_UNSUPPORTED_NAME_SYNTAX) */
   0,                                      /* c0020027 */
   RPC_S_UUID_NO_ADDRESS,                  /* c0020028 (RPC_NT_UUID_NO_ADDRESS) */
   RPC_S_DUPLICATE_ENDPOINT,               /* c0020029 (RPC_NT_DUPLICATE_ENDPOINT) */
   RPC_S_UNKNOWN_AUTHN_TYPE,               /* c002002a (RPC_NT_UNKNOWN_AUTHN_TYPE) */
   RPC_S_MAX_CALLS_TOO_SMALL,              /* c002002b (RPC_NT_MAX_CALLS_TOO_SMALL) */
   RPC_S_STRING_TOO_LONG,                  /* c002002c (RPC_NT_STRING_TOO_LONG) */
   RPC_S_PROTSEQ_NOT_FOUND,                /* c002002d (RPC_NT_PROTSEQ_NOT_FOUND) */
   RPC_S_PROCNUM_OUT_OF_RANGE,             /* c002002e (RPC_NT_PROCNUM_OUT_OF_RANGE) */
   RPC_S_BINDING_HAS_NO_AUTH,              /* c002002f (RPC_NT_BINDING_HAS_NO_AUTH) */
   RPC_S_UNKNOWN_AUTHN_SERVICE,            /* c0020030 (RPC_NT_UNKNOWN_AUTHN_SERVICE) */
   RPC_S_UNKNOWN_AUTHN_LEVEL,              /* c0020031 (RPC_NT_UNKNOWN_AUTHN_LEVEL) */
   RPC_S_INVALID_AUTH_IDENTITY,            /* c0020032 (RPC_NT_INVALID_AUTH_IDENTITY) */
   RPC_S_UNKNOWN_AUTHZ_SERVICE,            /* c0020033 (RPC_NT_UNKNOWN_AUTHZ_SERVICE) */
   EPT_S_INVALID_ENTRY,                    /* c0020034 (EPT_NT_INVALID_ENTRY) */
   EPT_S_CANT_PERFORM_OP,                  /* c0020035 (EPT_NT_CANT_PERFORM_OP) */
   EPT_S_NOT_REGISTERED,                   /* c0020036 (EPT_NT_NOT_REGISTERED) */
   RPC_S_NOTHING_TO_EXPORT,                /* c0020037 (RPC_NT_NOTHING_TO_EXPORT) */
   RPC_S_INCOMPLETE_NAME,                  /* c0020038 (RPC_NT_INCOMPLETE_NAME) */
   RPC_S_INVALID_VERS_OPTION,              /* c0020039 (RPC_NT_INVALID_VERS_OPTION) */
   RPC_S_NO_MORE_MEMBERS,                  /* c002003a (RPC_NT_NO_MORE_MEMBERS) */
   RPC_S_NOT_ALL_OBJS_UNEXPORTED,          /* c002003b (RPC_NT_NOT_ALL_OBJS_UNEXPORTED) */
   RPC_S_INTERFACE_NOT_FOUND,              /* c002003c (RPC_NT_INTERFACE_NOT_FOUND) */
   RPC_S_ENTRY_ALREADY_EXISTS,             /* c002003d (RPC_NT_ENTRY_ALREADY_EXISTS) */
   RPC_S_ENTRY_NOT_FOUND,                  /* c002003e (RPC_NT_ENTRY_NOT_FOUND) */
   RPC_S_NAME_SERVICE_UNAVAILABLE,         /* c002003f (RPC_NT_NAME_SERVICE_UNAVAILABLE) */
   RPC_S_INVALID_NAF_ID,                   /* c0020040 (RPC_NT_INVALID_NAF_ID) */
   RPC_S_CANNOT_SUPPORT,                   /* c0020041 (RPC_NT_CANNOT_SUPPORT) */
   RPC_S_NO_CONTEXT_AVAILABLE,             /* c0020042 (RPC_NT_NO_CONTEXT_AVAILABLE) */
   RPC_S_INTERNAL_ERROR,                   /* c0020043 (RPC_NT_INTERNAL_ERROR) */
   RPC_S_ZERO_DIVIDE,                      /* c0020044 (RPC_NT_ZERO_DIVIDE) */
   RPC_S_ADDRESS_ERROR,                    /* c0020045 (RPC_NT_ADDRESS_ERROR) */
   RPC_S_FP_DIV_ZERO,                      /* c0020046 (RPC_NT_FP_DIV_ZERO) */
   RPC_S_FP_UNDERFLOW,                     /* c0020047 (RPC_NT_FP_UNDERFLOW) */
   RPC_S_FP_OVERFLOW,                      /* c0020048 (RPC_NT_FP_OVERFLOW) */
   RPC_S_CALL_IN_PROGRESS,                 /* c0020049 (RPC_NT_CALL_IN_PROGRESS) */
   RPC_S_NO_MORE_BINDINGS,                 /* c002004a (RPC_NT_NO_MORE_BINDINGS) */
   RPC_S_GROUP_MEMBER_NOT_FOUND,           /* c002004b (RPC_NT_GROUP_MEMBER_NOT_FOUND) */
   EPT_S_CANT_CREATE,                      /* c002004c (EPT_NT_CANT_CREATE) */
   RPC_S_INVALID_OBJECT,                   /* c002004d (RPC_NT_INVALID_OBJECT) */
   0,                                      /* c002004e */
   RPC_S_NO_INTERFACES,                    /* c002004f (RPC_NT_NO_INTERFACES) */
   RPC_S_CALL_CANCELLED,                   /* c0020050 (RPC_NT_CALL_CANCELLED) */
   RPC_S_BINDING_INCOMPLETE,               /* c0020051 (RPC_NT_BINDING_INCOMPLETE) */
   RPC_S_COMM_FAILURE,                     /* c0020052 (RPC_NT_COMM_FAILURE) */
   RPC_S_UNSUPPORTED_AUTHN_LEVEL,          /* c0020053 (RPC_NT_UNSUPPORTED_AUTHN_LEVEL) */
   RPC_S_NO_PRINC_NAME,                    /* c0020054 (RPC_NT_NO_PRINC_NAME) */
   RPC_S_NOT_RPC_ERROR,                    /* c0020055 (RPC_NT_NOT_RPC_ERROR) */
   0,                                      /* c0020056 */
   RPC_S_SEC_PKG_ERROR,                    /* c0020057 (RPC_NT_SEC_PKG_ERROR) */
   RPC_S_NOT_CANCELLED,                    /* c0020058 (RPC_NT_NOT_CANCELLED) */
   0,                                      /* c0020059 */
   0,                                      /* c002005a */
   0,                                      /* c002005b */
   0,                                      /* c002005c */
   0,                                      /* c002005d */
   0,                                      /* c002005e */
   0,                                      /* c002005f */
   0,                                      /* c0020060 */
   0,                                      /* c0020061 */
   RPC_S_INVALID_ASYNC_HANDLE,             /* c0020062 (RPC_NT_INVALID_ASYNC_HANDLE) */
   RPC_S_INVALID_ASYNC_CALL                /* c0020063 (RPC_NT_INVALID_ASYNC_CALL) */
};

static const DWORD table_c0030001[12] =
{
   RPC_X_NO_MORE_ENTRIES,                  /* c0030001 (RPC_NT_NO_MORE_ENTRIES) */
   RPC_X_SS_CHAR_TRANS_OPEN_FAIL,          /* c0030002 (RPC_NT_SS_CHAR_TRANS_OPEN_FAIL) */
   RPC_X_SS_CHAR_TRANS_SHORT_FILE,         /* c0030003 (RPC_NT_SS_CHAR_TRANS_SHORT_FILE) */
   ERROR_INVALID_HANDLE,                   /* c0030004 (RPC_NT_SS_IN_NULL_CONTEXT) */
   ERROR_INVALID_HANDLE,                   /* c0030005 (RPC_NT_SS_CONTEXT_MISMATCH) */
   RPC_X_SS_CONTEXT_DAMAGED,               /* c0030006 (RPC_NT_SS_CONTEXT_DAMAGED) */
   RPC_X_SS_HANDLES_MISMATCH,              /* c0030007 (RPC_NT_SS_HANDLES_MISMATCH) */
   RPC_X_SS_CANNOT_GET_CALL_HANDLE,        /* c0030008 (RPC_NT_SS_CANNOT_GET_CALL_HANDLE) */
   RPC_X_NULL_REF_POINTER,                 /* c0030009 (RPC_NT_NULL_REF_POINTER) */
   RPC_X_ENUM_VALUE_OUT_OF_RANGE,          /* c003000a (RPC_NT_ENUM_VALUE_OUT_OF_RANGE) */
   RPC_X_BYTE_COUNT_TOO_SMALL,             /* c003000b (RPC_NT_BYTE_COUNT_TOO_SMALL) */
   RPC_X_BAD_STUB_DATA                     /* c003000c (RPC_NT_BAD_STUB_DATA) */
};

static const DWORD table_c0030059[9] =
{
   RPC_X_INVALID_ES_ACTION,                /* c0030059 (RPC_NT_INVALID_ES_ACTION) */
   RPC_X_WRONG_ES_VERSION,                 /* c003005a (RPC_NT_WRONG_ES_VERSION) */
   RPC_X_WRONG_STUB_VERSION,               /* c003005b (RPC_NT_WRONG_STUB_VERSION) */
   RPC_X_INVALID_PIPE_OBJECT,              /* c003005c (RPC_NT_INVALID_PIPE_OBJECT) */
   RPC_X_WRONG_PIPE_ORDER,                 /* c003005d (RPC_NT_INVALID_PIPE_OPERATION) */
   RPC_X_WRONG_PIPE_VERSION,               /* c003005e (RPC_NT_WRONG_PIPE_VERSION) */
   RPC_X_PIPE_CLOSED,                      /* c003005f (RPC_NT_PIPE_CLOSED) */
   RPC_X_PIPE_DISCIPLINE_ERROR,            /* c0030060 (RPC_NT_PIPE_DISCIPLINE_ERROR) */
   RPC_X_PIPE_EMPTY                        /* c0030061 (RPC_NT_PIPE_EMPTY) */
};

static const DWORD table_c00a0001[54] =
{
   ERROR_CTX_WINSTATION_NAME_INVALID,      /* c00a0001 (STATUS_CTX_WINSTATION_NAME_INVALID) */
   ERROR_CTX_INVALID_PD,                   /* c00a0002 (STATUS_CTX_INVALID_PD) */
   ERROR_CTX_PD_NOT_FOUND,                 /* c00a0003 (STATUS_CTX_PD_NOT_FOUND) */
   0,                                      /* c00a0004 */
   0,                                      /* c00a0005 */
   ERROR_CTX_CLOSE_PENDING,                /* c00a0006 (STATUS_CTX_CLOSE_PENDING) */
   ERROR_CTX_NO_OUTBUF,                    /* c00a0007 (STATUS_CTX_NO_OUTBUF) */
   ERROR_CTX_MODEM_INF_NOT_FOUND,          /* c00a0008 (STATUS_CTX_MODEM_INF_NOT_FOUND) */
   ERROR_CTX_INVALID_MODEMNAME,            /* c00a0009 (STATUS_CTX_INVALID_MODEMNAME) */
   ERROR_CTX_MODEM_RESPONSE_ERROR,         /* c00a000a (STATUS_CTX_RESPONSE_ERROR) */
   ERROR_CTX_MODEM_RESPONSE_TIMEOUT,       /* c00a000b (STATUS_CTX_MODEM_RESPONSE_TIMEOUT) */
   ERROR_CTX_MODEM_RESPONSE_NO_CARRIER,    /* c00a000c (STATUS_CTX_MODEM_RESPONSE_NO_CARRIER) */
   ERROR_CTX_MODEM_RESPONSE_NO_DIALTONE,   /* c00a000d (STATUS_CTX_MODEM_RESPONSE_NO_DIALTONE) */
   ERROR_CTX_MODEM_RESPONSE_BUSY,          /* c00a000e (STATUS_CTX_MODEM_RESPONSE_BUSY) */
   ERROR_CTX_MODEM_RESPONSE_VOICE,         /* c00a000f (STATUS_CTX_MODEM_RESPONSE_VOICE) */
   ERROR_CTX_TD_ERROR,                     /* c00a0010 (STATUS_CTX_TD_ERROR) */
   0,                                      /* c00a0011 */
   ERROR_CTX_LICENSE_CLIENT_INVALID,       /* c00a0012 (STATUS_CTX_LICENSE_CLIENT_INVALID) */
   ERROR_CTX_LICENSE_NOT_AVAILABLE,        /* c00a0013 (STATUS_CTX_LICENSE_NOT_AVAILABLE) */
   ERROR_CTX_LICENSE_EXPIRED,              /* c00a0014 (STATUS_CTX_LICENSE_EXPIRED) */
   ERROR_CTX_WINSTATION_NOT_FOUND,         /* c00a0015 (STATUS_CTX_WINSTATION_NOT_FOUND) */
   ERROR_CTX_WINSTATION_ALREADY_EXISTS,    /* c00a0016 (STATUS_CTX_WINSTATION_NAME_COLLISION) */
   ERROR_CTX_WINSTATION_BUSY,              /* c00a0017 (STATUS_CTX_WINSTATION_BUSY) */
   ERROR_CTX_BAD_VIDEO_MODE,               /* c00a0018 (STATUS_CTX_BAD_VIDEO_MODE) */
   0,                                      /* c00a0019 */
   0,                                      /* c00a001a */
   0,                                      /* c00a001b */
   0,                                      /* c00a001c */
   0,                                      /* c00a001d */
   0,                                      /* c00a001e */
   0,                                      /* c00a001f */
   0,                                      /* c00a0020 */
   0,                                      /* c00a0021 */
   ERROR_CTX_GRAPHICS_INVALID,             /* c00a0022 (STATUS_CTX_GRAPHICS_INVALID) */
   0,                                      /* c00a0023 */
   ERROR_CTX_NOT_CONSOLE,                  /* c00a0024 (STATUS_CTX_NOT_CONSOLE) */
   0,                                      /* c00a0025 */
   ERROR_CTX_CLIENT_QUERY_TIMEOUT,         /* c00a0026 (STATUS_CTX_CLIENT_QUERY_TIMEOUT) */
   ERROR_CTX_CONSOLE_DISCONNECT,           /* c00a0027 (STATUS_CTX_CONSOLE_DISCONNECT) */
   ERROR_CTX_CONSOLE_CONNECT,              /* c00a0028 (STATUS_CTX_CONSOLE_CONNECT) */
   0,                                      /* c00a0029 */
   ERROR_CTX_SHADOW_DENIED,                /* c00a002a (STATUS_CTX_SHADOW_DENIED) */
   ERROR_CTX_WINSTATION_ACCESS_DENIED,     /* c00a002b (STATUS_CTX_WINSTATION_ACCESS_DENIED) */
   0,                                      /* c00a002c */
   0,                                      /* c00a002d */
   ERROR_CTX_INVALID_WD,                   /* c00a002e (STATUS_CTX_INVALID_WD) */
   ERROR_CTX_WD_NOT_FOUND,                 /* c00a002f (STATUS_CTX_WD_NOT_FOUND) */
   ERROR_CTX_SHADOW_INVALID,               /* c00a0030 (STATUS_CTX_SHADOW_INVALID) */
   ERROR_CTX_SHADOW_DISABLED,              /* c00a0031 (STATUS_CTX_SHADOW_DISABLED) */
   0,                                      /* c00a0032 (STATUS_RDP_PROTOCOL_ERROR) */
   ERROR_CTX_CLIENT_LICENSE_NOT_SET,       /* c00a0033 (STATUS_CTX_CLIENT_LICENSE_NOT_SET) */
   ERROR_CTX_CLIENT_LICENSE_IN_USE,        /* c00a0034 (STATUS_CTX_CLIENT_LICENSE_IN_USE) */
   ERROR_CTX_SHADOW_ENDED_BY_MODE_CHANGE,  /* c00a0035 (STATUS_CTX_SHADOW_ENDED_BY_MODE_CHANGE) */
   ERROR_CTX_SHADOW_NOT_RUNNING            /* c00a0036 (STATUS_CTX_SHADOW_NOT_RUNNING) */
};

static const DWORD table_c0130001[22] =
{
   ERROR_CLUSTER_INVALID_NODE,             /* c0130001 (STATUS_CLUSTER_INVALID_NODE) */
   ERROR_CLUSTER_NODE_EXISTS,              /* c0130002 (STATUS_CLUSTER_NODE_EXISTS) */
   ERROR_CLUSTER_JOIN_IN_PROGRESS,         /* c0130003 (STATUS_CLUSTER_JOIN_IN_PROGRESS) */
   ERROR_CLUSTER_NODE_NOT_FOUND,           /* c0130004 (STATUS_CLUSTER_NODE_NOT_FOUND) */
   ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND,     /* c0130005 (STATUS_CLUSTER_LOCAL_NODE_NOT_FOUND) */
   ERROR_CLUSTER_NETWORK_EXISTS,           /* c0130006 (STATUS_CLUSTER_NETWORK_EXISTS) */
   ERROR_CLUSTER_NETWORK_NOT_FOUND,        /* c0130007 (STATUS_CLUSTER_NETWORK_NOT_FOUND) */
   ERROR_CLUSTER_NETINTERFACE_EXISTS,      /* c0130008 (STATUS_CLUSTER_NETINTERFACE_EXISTS) */
   ERROR_CLUSTER_NETINTERFACE_NOT_FOUND,   /* c0130009 (STATUS_CLUSTER_NETINTERFACE_NOT_FOUND) */
   ERROR_CLUSTER_INVALID_REQUEST,          /* c013000a (STATUS_CLUSTER_INVALID_REQUEST) */
   ERROR_CLUSTER_INVALID_NETWORK_PROVIDER, /* c013000b (STATUS_CLUSTER_INVALID_NETWORK_PROVIDER) */
   ERROR_CLUSTER_NODE_DOWN,                /* c013000c (STATUS_CLUSTER_NODE_DOWN) */
   ERROR_CLUSTER_NODE_UNREACHABLE,         /* c013000d (STATUS_CLUSTER_NODE_UNREACHABLE) */
   ERROR_CLUSTER_NODE_NOT_MEMBER,          /* c013000e (STATUS_CLUSTER_NODE_NOT_MEMBER) */
   ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS,     /* c013000f (STATUS_CLUSTER_JOIN_NOT_IN_PROGRESS) */
   ERROR_CLUSTER_INVALID_NETWORK,          /* c0130010 (STATUS_CLUSTER_INVALID_NETWORK) */
   0,                                      /* c0130011 (STATUS_CLUSTER_NO_NET_ADAPTERS) */
   ERROR_CLUSTER_NODE_UP,                  /* c0130012 (STATUS_CLUSTER_NODE_UP) */
   ERROR_CLUSTER_NODE_PAUSED,              /* c0130013 (STATUS_CLUSTER_NODE_PAUSED) */
   ERROR_CLUSTER_NODE_NOT_PAUSED,          /* c0130014 (STATUS_CLUSTER_NODE_NOT_PAUSED) */
   ERROR_CLUSTER_NO_SECURITY_CONTEXT,      /* c0130015 (STATUS_CLUSTER_NO_SECURITY_CONTEXT) */
   ERROR_CLUSTER_NETWORK_NOT_INTERNAL      /* c0130016 (STATUS_CLUSTER_NETWORK_NOT_INTERNAL) */
};

static const DWORD table_c0150001[39] =
{
   ERROR_SXS_SECTION_NOT_FOUND,            /* c0150001 (STATUS_SXS_SECTION_NOT_FOUND) */
   ERROR_SXS_CANT_GEN_ACTCTX,              /* c0150002 (STATUS_SXS_CANT_GEN_ACTCTX) */
   ERROR_SXS_INVALID_ACTCTXDATA_FORMAT,    /* c0150003 (STATUS_SXS_INVALID_ACTCTXDATA_FORMAT) */
   ERROR_SXS_ASSEMBLY_NOT_FOUND,           /* c0150004 (STATUS_SXS_ASSEMBLY_NOT_FOUND) */
   ERROR_SXS_MANIFEST_FORMAT_ERROR,        /* c0150005 (STATUS_SXS_MANIFEST_FORMAT_ERROR) */
   ERROR_SXS_MANIFEST_PARSE_ERROR,         /* c0150006 (STATUS_SXS_MANIFEST_PARSE_ERROR) */
   ERROR_SXS_ACTIVATION_CONTEXT_DISABLED,  /* c0150007 (STATUS_SXS_ACTIVATION_CONTEXT_DISABLED) */
   ERROR_SXS_KEY_NOT_FOUND,                /* c0150008 (STATUS_SXS_KEY_NOT_FOUND) */
   ERROR_SXS_VERSION_CONFLICT,             /* c0150009 (STATUS_SXS_VERSION_CONFLICT) */
   ERROR_SXS_WRONG_SECTION_TYPE,           /* c015000a (STATUS_SXS_WRONG_SECTION_TYPE) */
   ERROR_SXS_THREAD_QUERIES_DISABLED,      /* c015000b (STATUS_SXS_THREAD_QUERIES_DISABLED) */
   ERROR_SXS_ASSEMBLY_MISSING,             /* c015000c (STATUS_SXS_ASSEMBLY_MISSING) */
   0,                                      /* c015000d */
   ERROR_SXS_PROCESS_DEFAULT_ALREADY_SET,  /* c015000e (STATUS_SXS_PROCESS_DEFAULT_ALREADY_SET) */
   ERROR_SXS_EARLY_DEACTIVATION,           /* c015000f (STATUS_SXS_EARLY_DEACTIVATION) */
   ERROR_SXS_INVALID_DEACTIVATION,         /* c0150010 (STATUS_SXS_INVALID_DEACTIVATION) */
   ERROR_SXS_MULTIPLE_DEACTIVATION,        /* c0150011 (STATUS_SXS_MULTIPLE_DEACTIVATION) */
   ERROR_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY,          /* c0150012 (STATUS_SXS_SYSTEM_DEFAULT_ACTIVATION_CONTEXT_EMPTY) */
   ERROR_SXS_PROCESS_TERMINATION_REQUESTED,                    /* c0150013 (STATUS_SXS_PROCESS_TERMINATION_REQUESTED) */
   ERROR_SXS_CORRUPT_ACTIVATION_STACK,     /* c0150014 (STATUS_SXS_CORRUPT_ACTIVATION_STACK) */
   ERROR_SXS_CORRUPTION,                   /* c0150015 (STATUS_SXS_CORRUPTION) */
   ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE,                 /* c0150016 (STATUS_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE) */
   ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME,                  /* c0150017 (STATUS_SXS_INVALID_IDENTITY_ATTRIBUTE_NAME) */
   ERROR_SXS_IDENTITY_DUPLICATE_ATTRIBUTE, /* c0150018 (STATUS_SXS_IDENTITY_DUPLICATE_ATTRIBUTE) */
   ERROR_SXS_IDENTITY_PARSE_ERROR,         /* c0150019 (STATUS_SXS_IDENTITY_PARSE_ERROR) */
   ERROR_SXS_COMPONENT_STORE_CORRUPT,      /* c015001a (STATUS_SXS_COMPONENT_STORE_CORRUPT) */
   ERROR_SXS_FILE_HASH_MISMATCH,           /* c015001b (STATUS_SXS_FILE_HASH_MISMATCH) */
   ERROR_SXS_MANIFEST_IDENTITY_SAME_BUT_CONTENTS_DIFFERENT,    /* c015001c (STATUS_SXS_MANIFEST_IDENTITY_SAME_BUT_CONTENTS_DIFFERENT) */
   ERROR_SXS_IDENTITIES_DIFFERENT,         /* c015001d (STATUS_SXS_IDENTITIES_DIFFERENT) */
   ERROR_SXS_ASSEMBLY_IS_NOT_A_DEPLOYMENT, /* c015001e (STATUS_SXS_ASSEMBLY_IS_NOT_A_DEPLOYMENT) */
   ERROR_SXS_FILE_NOT_PART_OF_ASSEMBLY,    /* c015001f (STATUS_SXS_FILE_NOT_PART_OF_ASSEMBLY) */
   ERROR_ADVANCED_INSTALLER_FAILED,        /* c0150020 (STATUS_ADVANCED_INSTALLER_FAILED) */
   ERROR_XML_ENCODING_MISMATCH,            /* c0150021 (STATUS_XML_ENCODING_MISMATCH) */
   ERROR_SXS_MANIFEST_TOO_BIG,             /* c0150022 (STATUS_SXS_MANIFEST_TOO_BIG) */
   ERROR_SXS_SETTING_NOT_REGISTERED,       /* c0150023 (STATUS_SXS_SETTING_NOT_REGISTERED) */
   ERROR_SXS_TRANSACTION_CLOSURE_INCOMPLETE,                   /* c0150024 (STATUS_SXS_TRANSACTION_CLOSURE_INCOMPLETE) */
   ERROR_SMI_PRIMITIVE_INSTALLER_FAILED,                       /* c0150025 (STATUS_SMI_PRIMITIVE_INSTALLER_FAILED) */
   ERROR_GENERIC_COMMAND_FAILED,           /* c0150026 (STATUS_GENERIC_COMMAND_FAILED) */
   ERROR_SXS_FILE_HASH_MISSING             /* c0150027 (STATUS_SXS_FILE_HASH_MISSING) */
};

static const struct error_table error_table[] =
{
    { 0x00000102, 0x00000122, table_00000102 },
    { 0x40000002, 0x40000026, table_40000002 },
    { 0x40000370, 0x40000371, table_40000370 },
    { 0x40020056, 0x40020057, table_40020056 },
    { 0x400200af, 0x400200b0, table_400200af },
    { 0x80000001, 0x80000028, table_80000001 },
    { 0x80000288, 0x8000028a, table_80000288 },
    { 0x80090300, 0x80090348, table_80090300 },
    { 0x80092010, 0x80092014, table_80092010 },
    { 0x80096004, 0x80096005, table_80096004 },
    { 0x80130001, 0x80130006, table_80130001 },
    { 0xc0000001, 0xc000019c, table_c0000001 },
    { 0xc0000202, 0xc000038e, table_c0000202 },
    { 0xc0020001, 0xc0020064, table_c0020001 },
    { 0xc0030001, 0xc003000d, table_c0030001 },
    { 0xc0030059, 0xc0030062, table_c0030059 },
    { 0xc00a0001, 0xc00a0037, table_c00a0001 },
    { 0xc0130001, 0xc0130017, table_c0130001 },
    { 0xc0150001, 0xc0150028, table_c0150001 },
    { 0, 0, NULL }  /* last entry */
};
