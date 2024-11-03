/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/wkssvc/info.c
 * PURPOSE:          Workstation service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wkssvc);

/* GLOBALS *******************************************************************/

WKSTA_INFO_502 WkstaInfo502;


/* FUNCTIONS *****************************************************************/

VOID
InitWorkstationInfo(VOID)
{
    HKEY hInfoKey = NULL;
    DWORD dwType, dwSize, dwValue;
    DWORD dwError;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters",
                            0,
                            KEY_READ,
                            &hInfoKey);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW() failed!\n");
        return;
    }

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"CharWait",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue <= 65535))
        WkstaInfo502.wki502_char_wait = dwValue;
    else
        WkstaInfo502.wki502_char_wait = 0;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"CollectionTime",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue <= 65535000))
        WkstaInfo502.wki502_collection_time = dwValue;
    else
        WkstaInfo502.wki502_collection_time = 250;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"MaximumCollectionCount",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue <= 65535))
        WkstaInfo502.wki502_maximum_collection_count = dwValue;
    else
        WkstaInfo502.wki502_maximum_collection_count = 16;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"KeepConn",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue >= 1) && (dwValue <= 65535))
        WkstaInfo502.wki502_keep_conn = dwValue;
    else
        WkstaInfo502.wki502_keep_conn = 600;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"MaxCmds",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue >= 50) && (dwValue <= 65535))
        WkstaInfo502.wki502_max_cmds = dwValue;
    else
        WkstaInfo502.wki502_max_cmds = 50;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"SessTimeout",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue >= 60) && (dwValue <= 65535))
        WkstaInfo502.wki502_sess_timeout = dwValue;
    else
        WkstaInfo502.wki502_sess_timeout = 60;


    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"SizCharBuf",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue >= 64) && (dwValue <= 4096))
        WkstaInfo502.wki502_siz_char_buf = dwValue;
    else
        WkstaInfo502.wki502_siz_char_buf = 512;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"MaxThreads",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if ((dwError == ERROR_SUCCESS) && (dwValue >= 1) && (dwValue <= 256))
        WkstaInfo502.wki502_max_threads = dwValue;
    else
        WkstaInfo502.wki502_max_threads = 17;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"LockQuota",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_lock_quota = dwValue;
    else
        WkstaInfo502.wki502_lock_quota = 6144;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"LockQuota",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_lock_quota = dwValue;
    else
        WkstaInfo502.wki502_lock_quota = 6144;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"LockIncrement",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_lock_increment = dwValue;
    else
        WkstaInfo502.wki502_lock_increment = 10;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"LockMaximum",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_lock_maximum = dwValue;
    else
        WkstaInfo502.wki502_lock_maximum = 500;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"PipeIncrement",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_pipe_increment = dwValue;
    else
        WkstaInfo502.wki502_pipe_increment = 10;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"PipeMaximum",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_pipe_maximum = dwValue;
    else
        WkstaInfo502.wki502_pipe_maximum = 500;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"CacheFileTimeout",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_cache_file_timeout = dwValue;
    else
        WkstaInfo502.wki502_cache_file_timeout = 40;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"DormantFileLimit",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS && dwValue >= 1)
        WkstaInfo502.wki502_dormant_file_limit = dwValue;
    else
        WkstaInfo502.wki502_dormant_file_limit = 1;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"ReadAheadThroughput",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_read_ahead_throughput = dwValue;
    else
        WkstaInfo502.wki502_read_ahead_throughput = 0;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"MailslotBuffers",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_num_mailslot_buffers = dwValue;
    else
        WkstaInfo502.wki502_num_mailslot_buffers = 3;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"ServerAnnounceBuffers",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_num_srv_announce_buffers = dwValue;
    else
        WkstaInfo502.wki502_num_srv_announce_buffers = 20;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"NumIllegalDatagramEvents",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_max_illegal_datagram_events = dwValue;
    else
        WkstaInfo502.wki502_max_illegal_datagram_events = 5;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"IllegalDatagramResetTime",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_illegal_datagram_event_reset_frequency = dwValue;
    else
        WkstaInfo502.wki502_illegal_datagram_event_reset_frequency = 3600;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"LogElectionPackets",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_log_election_packets = dwValue;
    else
        WkstaInfo502.wki502_log_election_packets = 0;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"UseOpportunisticLocking",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_opportunistic_locking = dwValue;
    else
        WkstaInfo502.wki502_use_opportunistic_locking = 1;

    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hInfoKey,
                               L"UseUnlockBehind",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_unlock_behind = dwValue;
    else
        WkstaInfo502.wki502_use_unlock_behind = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UseCloseBehind",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_close_behind = dwValue;
    else
        WkstaInfo502.wki502_use_close_behind = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"BufNamedPipes",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_buf_named_pipes = dwValue;
    else
        WkstaInfo502.wki502_buf_named_pipes = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UseLockReadUnlock",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_lock_read_unlock = dwValue;
    else
        WkstaInfo502.wki502_use_lock_read_unlock = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UtilizeNtCaching",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_utilize_nt_caching = dwValue;
    else
        WkstaInfo502.wki502_utilize_nt_caching = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UseRawRead",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_raw_read = dwValue;
    else
        WkstaInfo502.wki502_use_raw_read = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UseRawWrite",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_raw_write = dwValue;
    else
        WkstaInfo502.wki502_use_raw_write = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UseWriteRawData",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_write_raw_data = dwValue;
    else
        WkstaInfo502.wki502_use_write_raw_data = 0;

    dwError = RegQueryValueExW(hInfoKey,
                               L"UseEncryption",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_encryption = dwValue;
    else
        WkstaInfo502.wki502_use_encryption = 1;

    dwError = RegQueryValueExW(hInfoKey,
                               L"BufFilesDenyWrite",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_buf_files_deny_write = dwValue;
    else
        WkstaInfo502.wki502_buf_files_deny_write = 0;

    dwError = RegQueryValueExW(hInfoKey,
                               L"BufReadOnlyFiles",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_buf_read_only_files = dwValue;
    else
        WkstaInfo502.wki502_buf_read_only_files = 0;

    dwError = RegQueryValueExW(hInfoKey,
                               L"ForceCoreCreateMode",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_force_core_create_mode = dwValue;
    else
        WkstaInfo502.wki502_force_core_create_mode = 0;

    dwError = RegQueryValueExW(hInfoKey,
                               L"Use512ByteMaxTransfer",
                               0,
                               &dwType,
                               (PBYTE)&dwValue,
                               &dwSize);
    if (dwError == ERROR_SUCCESS)
        WkstaInfo502.wki502_use_512_byte_max_transfer = dwValue;
    else
        WkstaInfo502.wki502_use_512_byte_max_transfer = 0;

    RegCloseKey(hInfoKey);
}


VOID
SaveWorkstationInfo(
    _In_ DWORD Level)
{
    HKEY hInfoKey = NULL;
    DWORD dwError;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters",
                            0,
                            KEY_WRITE,
                            &hInfoKey);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW() failed!\n");
        return;
    }

    switch (Level)
    {
        case 502:
            RegSetValueExW(hInfoKey,
                           L"CharWait",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_char_wait,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"CollectionTime",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_collection_time,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"MaximumCollectionCount",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_maximum_collection_count,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"KeepConn",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_keep_conn,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"MaxCmds",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_max_cmds,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"SessTimeout",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_sess_timeout,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"SizCharBuf",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_siz_char_buf,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"MaxThreads",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_max_threads,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"LockQuota",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_lock_quota,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"LockIncrement",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_lock_increment,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"LockMaximum",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_lock_maximum,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"PipeIncrement",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_pipe_increment,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"PipeMaximum",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_pipe_maximum,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"CacheFileTimeout",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_cache_file_timeout,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"DormantFileLimit",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_dormant_file_limit,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"ReadAheadThroughput",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_read_ahead_throughput,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"MailslotBuffers",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_num_mailslot_buffers,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"ServerAnnounceBuffers",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_num_srv_announce_buffers,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"NumIllegalDatagramEvents",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_max_illegal_datagram_events,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"IllegalDatagramResetTime",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_illegal_datagram_event_reset_frequency,
                           sizeof(DWORD));

            RegSetValueExW(hInfoKey,
                           L"LogElectionPackets",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_log_election_packets,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseOpportunisticLocking",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_opportunistic_locking,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseUnlockBehind",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_unlock_behind,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseCloseBehind",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_close_behind,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"BufNamedPipes",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_buf_named_pipes,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseLockReadUnlock",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_lock_read_unlock,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UtilizeNtCaching",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_utilize_nt_caching,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseRawRead",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_raw_read,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseRawWrite",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_raw_write,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseWriteRawData",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_write_raw_data,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"UseEncryption",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_encryption,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"BufFilesDenyWrite",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_buf_files_deny_write,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"BufReadOnlyFiles",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_buf_read_only_files,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"ForceCoreCreateMode",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_force_core_create_mode,
                           sizeof(BOOL));

            RegSetValueExW(hInfoKey,
                           L"Use512ByteMaxTransfer",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_use_512_byte_max_transfer,
                           sizeof(BOOL));
            break;

        case 1013:
            RegSetValueExW(hInfoKey,
                           L"KeepConn",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_keep_conn,
                           sizeof(DWORD));
            break;

        case 1018:
            RegSetValueExW(hInfoKey,
                           L"SessTimeout",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_sess_timeout,
                           sizeof(DWORD));
            break;

        case 1046:
            RegSetValueExW(hInfoKey,
                           L"DormantFileLimit",
                           0,
                           REG_DWORD,
                           (PBYTE)&WkstaInfo502.wki502_dormant_file_limit,
                           sizeof(DWORD));
            break;
    }

    RegCloseKey(hInfoKey);
}

/* EOF */
