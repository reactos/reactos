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
    WkstaInfo502.wki502_char_wait = 0;
    WkstaInfo502.wki502_collection_time = 250;
    WkstaInfo502.wki502_maximum_collection_count = 16;
    WkstaInfo502.wki502_keep_conn = 600;
    WkstaInfo502.wki502_max_cmds = 50;
    WkstaInfo502.wki502_sess_timeout = 60;
    WkstaInfo502.wki502_siz_char_buf = 512;
    WkstaInfo502.wki502_max_threads = 17;
    WkstaInfo502.wki502_lock_quota = 6144;
    WkstaInfo502.wki502_lock_increment = 10;
    WkstaInfo502.wki502_lock_maximum = 500;
    WkstaInfo502.wki502_pipe_increment = 10;
    WkstaInfo502.wki502_pipe_maximum = 500;
    WkstaInfo502.wki502_cache_file_timeout = 40;
    WkstaInfo502.wki502_dormant_file_limit = 0; /* 1 */
    WkstaInfo502.wki502_read_ahead_throughput = 0;
    WkstaInfo502.wki502_num_mailslot_buffers = 3;
    WkstaInfo502.wki502_num_srv_announce_buffers = 20;
    WkstaInfo502.wki502_max_illegal_datagram_events = 5;
    WkstaInfo502.wki502_illegal_datagram_event_reset_frequency = 3600;
    WkstaInfo502.wki502_log_election_packets = 0;
    WkstaInfo502.wki502_use_opportunistic_locking = 1;
    WkstaInfo502.wki502_use_unlock_behind = 1;
    WkstaInfo502.wki502_use_close_behind = 1;
    WkstaInfo502.wki502_buf_named_pipes = 1;
    WkstaInfo502.wki502_use_lock_read_unlock = 1;
    WkstaInfo502.wki502_utilize_nt_caching = 1;
    WkstaInfo502.wki502_use_raw_read = 1;
    WkstaInfo502.wki502_use_raw_write = 1;
    WkstaInfo502.wki502_use_write_raw_data = 0;
    WkstaInfo502.wki502_use_encryption = 1;
    WkstaInfo502.wki502_buf_files_deny_write = 0;
    WkstaInfo502.wki502_buf_read_only_files = 0;
    WkstaInfo502.wki502_force_core_create_mode = 0;
    WkstaInfo502.wki502_use_512_byte_max_transfer = 0;
}

/* EOF */
