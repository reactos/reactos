/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/wine/stubs.c
 * PURPOSE:         Wine server stubs
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include "object.h"
#include "request.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

DECL_HANDLER(new_process)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_new_process_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(new_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_startup_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(init_process_done)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(init_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(terminate_process)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(terminate_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_process_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_process_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_thread_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_thread_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_dll_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(suspend_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(resume_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(load_dll)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(unload_dll)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(queue_apc)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_apc_result)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(dup_handle)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_process)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(select)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(event_op)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_mutex)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(release_mutex)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_mutex)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_semaphore)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(release_semaphore)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_semaphore)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_file)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_file_object)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(alloc_file_handle)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_handle_fd)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(flush_file)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(lock_file)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(unlock_file)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_socket)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(accept_socket)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_socket_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_socket_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(enable_socket_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_socket_deferred)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(alloc_console)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(free_console)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_console_renderer_events)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_console)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_console_wait_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_console_mode)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_console_mode)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_console_input_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_console_input_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(append_console_input_history)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_console_input_history)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_console_output)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_console_output_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_console_output_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(write_console_input)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(read_console_input)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(write_console_output)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(fill_console_output)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(read_console_output)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(move_console_output)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(send_console_signal)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(read_directory_changes)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(read_change)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_mapping)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_mapping)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_mapping_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_mapping_committed_range)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(add_mapping_committed_range)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_snapshot)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(next_process)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(next_thread)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(wait_debug_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(queue_exception_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_exception_status)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(output_debug_string)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(continue_debug_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(debug_process)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(debug_break)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_debugger_kill_on_exit)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(read_process_memory)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(write_process_memory)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_key)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_key)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(delete_key)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(flush_key)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(enum_key)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_key_value)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_key_value)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(enum_key_value)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(delete_key_value)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(load_registry)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(unload_registry)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(save_registry)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_registry_notification)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_timer)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_timer)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_timer)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(cancel_timer)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_timer_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_thread_context)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_thread_context)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_selector_entry)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_atom_information)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_process_idle_event)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_serial_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_serial_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(register_async)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(cancel_async)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(ioctl)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_ioctl_result)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_named_pipe)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_named_pipe_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_clipboard_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_token)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(adjust_token_privileges)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_token_privileges)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(check_token_privileges)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(duplicate_token)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(access_check)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_token_user)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_token_groups)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_token_default_dacl)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_token_default_dacl)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_security_object)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_mailslot)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_mailslot_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_symlink)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_symlink)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(query_symlink)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_token_impersonation_level)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(allocate_locally_unique_id)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_device_manager)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_device)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(delete_device)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_next_device_request)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(make_process_system)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(get_token_statistics)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(create_completion)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(open_completion)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(add_completion)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(remove_completion)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(query_completion)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(set_completion_info)
{
    UNIMPLEMENTED;
}

DECL_HANDLER(add_fd_completion)
{
    UNIMPLEMENTED;
}

/* EOF */
