//
// popen.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// The _popen() and _pclose() functions, which open a pipe to a child process.
//
#include <corecrt_internal_stdio.h>
#include <process.h>



#define STDIN     0
#define STDOUT    1



namespace {

    template <typename Character>
    struct fdopen_mode
    {
        Character mode[3];
    };

    // This is the entry type for the stream pointer / process handle pairs that
    // are stored for each outstanding popen.
    struct process_handle_pair
    {
        FILE*    stream;
        intptr_t process_handle;
    };

    struct stream_traits
    {
        typedef FILE* type;

        static bool close(_In_ type h) throw()
        {
            fclose(h);
            return true;
        }

        static type get_invalid_value() throw()
        {
            return nullptr;
        }
    };

    struct process_handle_pair_traits
    {
        typedef process_handle_pair* type;

        static bool close(_In_ type h) throw()
        {
            h->process_handle = 0;
            h->stream         = nullptr;
            return true;
        }

        static type get_invalid_value() throw()
        {
            return nullptr;
        }
    };

    typedef __crt_unique_handle_t<stream_traits>              unique_stream;
    typedef __crt_unique_handle_t<process_handle_pair_traits> unique_process_handle_pair;
}



// The global table of stream pointer / process handle pairs.  Access to this
// global tbale is only done via the idtab function.  The table is expanded as
// necessary (by idtab), and free table entries are reused.  (An entry is free
// if its stream is null.)  The table is never contracted.
static unsigned             __idtabsiz;
static process_handle_pair* __idpairs;



// Finds the entry for the given stream in the global table.  If the stream is
// found, a pointer to it is returned; if the stream is not found, null is
// returned.
//
// If the stream is null, a new entry is allocated and a pointer to it is
// returned.  If no entries are available and expansion of the table fails,
// null is returned.
//
// This function assumes the caller has acquired the lock on the table already.
static process_handle_pair* __cdecl idtab(FILE* const stream) throw()
{
    // Search the table, and return the matching entry if one is found:
    process_handle_pair* const first = __idpairs;
    process_handle_pair* const last  = first + __idtabsiz;
    for (process_handle_pair* it = first; it != last; ++it)
    {
        if (it->stream == stream)
            return it;
    }

    // We did not find an entry in the table.  If the stream is null, then we
    // try creating or expanding the table.  Otherwise, we return null.  Note
    // that when the table is created or expanded, exactly one new entry is
    // produced.  This must not be changed unless code is added to mark the
    // extra entries as being free (e.g., by setting their stream fields to null.
    if (stream != nullptr)
        return nullptr;

    if (__idtabsiz + 1 < __idtabsiz)
        return nullptr;

    if (__idtabsiz + 1 >= SIZE_MAX / sizeof(process_handle_pair))
        return nullptr;

    process_handle_pair* const newptr = _recalloc_crt_t(process_handle_pair, __idpairs, __idtabsiz + 1).detach();
    if (newptr == nullptr)
        return nullptr;

    __idpairs = newptr;
    process_handle_pair* const pairptr = newptr + __idtabsiz;
    ++__idtabsiz;

    return pairptr;
}



template <typename Character>
static fdopen_mode<Character> __cdecl convert_popen_type_to_fdopen_mode(
    Character const* const type
    ) throw()
{
    fdopen_mode<Character> result = fdopen_mode<Character>();

    Character const* type_it = type;

    while (*type_it == ' ')
        ++type_it;

    _VALIDATE_RETURN(*type_it == 'w' || *type_it == 'r', EINVAL, result);
    result.mode[0] = *type_it++;

    while (*type_it == ' ')
        ++type_it;

    _VALIDATE_RETURN(*type_it == '\0' || *type_it == 't' || *type_it == 'b', EINVAL, result);
    result.mode[1] = *type_it;

    return result;
}



template <typename Character>
static Character const* __cdecl get_comspec() throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    static Character const comspec_name[] = { 'C', 'O', 'M', 'S', 'P', 'E', 'C', '\0' };

    Character* comspec_value = nullptr;
    if (_ERRCHECK_EINVAL(stdio_traits::tdupenv_s_crt(&comspec_value, nullptr, comspec_name)) != 0)
        return nullptr;

    return comspec_value;
}



template <typename Character>
static Character const* __cdecl get_path() throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    static Character const path_name[] = { 'P', 'A', 'T', 'H', '\0' };

    Character* path_value = nullptr;
    if (_ERRCHECK_EINVAL(stdio_traits::tdupenv_s_crt(&path_value, nullptr, path_name)) != 0)
        return nullptr;

    return path_value;
}



template <typename Character>
static Character const* __cdecl get_executable_path(
    Character const* const executable
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    // If we can access the given path, just use it:
    if (stdio_traits::taccess_s(executable, 0) == 0)
        return executable;

    // Otherwise, we need to search the PATH:
    __crt_unique_heap_ptr<Character> buffer(_calloc_crt_t(Character, MAX_PATH));
    if (buffer.get() == nullptr)
        return nullptr;

    __crt_unique_heap_ptr<Character const> path(get_path<Character>());

    Character const* current = path.get();
    while ((current = stdio_traits::tgetpath(current, buffer.get(), MAX_PATH - 1)) != 0)
    {
        if (__crt_stdio_path_requires_backslash(buffer.get()))
        {
            static Character const backslash[] = { '\\', '\0' };
            _ERRCHECK(stdio_traits::tcscat_s(buffer.get(), MAX_PATH, backslash));
        }

        if (stdio_traits::tcslen(buffer.get()) + stdio_traits::tcslen(executable) >= MAX_PATH)
            return nullptr;

        _ERRCHECK(stdio_traits::tcscat_s(buffer.get(), MAX_PATH, executable));

        if (stdio_traits::taccess_s(buffer.get(), 0) == 0)
            return buffer.detach();
    }

    return nullptr;
}



template <typename Character>
static FILE* __cdecl common_popen_nolock(
    Character const* const command,
    Character const* const fdopen_mode,
    int              const std_fh,
    int (&pipe_handles)[2]
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    HANDLE const process_handle = GetCurrentProcess();

    // We only return the second pipe handle to the caller; for the first pipe,
    // we just need to use the HANDLE:
    __crt_unique_handle new_pipe_handle;
    if (!DuplicateHandle(
            process_handle,
            reinterpret_cast<HANDLE>(_osfhnd(pipe_handles[0])),
            process_handle,
            new_pipe_handle.get_address_of(),
            0,
            TRUE,
            DUPLICATE_SAME_ACCESS))
    {
        return nullptr;
    }

    _close(pipe_handles[0]);
    pipe_handles[0] = -1;

    // Associate a stream with the pipe handle to be returned to the caller:
    unique_stream pipe_stream(stdio_traits::tfdopen(pipe_handles[1], fdopen_mode));
    if (!pipe_stream)
        return nullptr;

    // Obtain a proces handle pair in which to store the process handle:
    unique_process_handle_pair id_pair(idtab(nullptr));
    if (!id_pair)
        return nullptr;

    // Determine which command processor to use:  command.com or cmd.exe:
    static Character const default_cmd_exe[] = { 'c', 'm', 'd', '.', 'e', 'x', 'e', '\0' };

    __crt_unique_heap_ptr<Character const> const comspec_variable(get_comspec<Character>());
    Character const* const cmd_exe = comspec_variable.get() != nullptr
        ? comspec_variable.get()
        : default_cmd_exe;

    STARTUPINFOW startup_info = { 0 };
    startup_info.cb = sizeof(startup_info);

    // The following arguments are used by the OS for duplicating the handles:
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput  = std_fh == STDIN  ? new_pipe_handle.get() : reinterpret_cast<HANDLE>(_osfhnd(0));
    startup_info.hStdOutput = std_fh == STDOUT ? new_pipe_handle.get() : reinterpret_cast<HANDLE>(_osfhnd(1));
    startup_info.hStdError  = reinterpret_cast<HANDLE>(_osfhnd(2));

    static Character const slash_c[] = { ' ', '/', 'c', ' ', '\0' };

    size_t const command_line_count =
        stdio_traits::tcslen(cmd_exe) +
        stdio_traits::tcslen(slash_c) +
        stdio_traits::tcslen(command) +
        1;

    __crt_unique_heap_ptr<Character> const command_line(_calloc_crt_t(Character, command_line_count));
    if (command_line.get() == nullptr)
        return nullptr;

    _ERRCHECK(stdio_traits::tcscpy_s(command_line.get(), command_line_count, cmd_exe));
    _ERRCHECK(stdio_traits::tcscat_s(command_line.get(), command_line_count, slash_c));
    _ERRCHECK(stdio_traits::tcscat_s(command_line.get(), command_line_count, command));

    // Find the path at which the executable is accessible:
    Character const* const selected_cmd_exe(get_executable_path(cmd_exe));
    if (selected_cmd_exe == nullptr)
        return nullptr;

    // If get_executable_path() returned a path other than the one we gave it,
    // we must be sure to free the string when we return:
    __crt_unique_heap_ptr<Character const> const owned_final_exe_path(selected_cmd_exe != cmd_exe
        ? selected_cmd_exe
        : nullptr);

    PROCESS_INFORMATION process_info = PROCESS_INFORMATION();
    BOOL const child_status = stdio_traits::create_process(
        selected_cmd_exe,
        command_line.get(),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &startup_info,
        &process_info);

    if (!child_status)
        return nullptr;

    FILE* const result_stream = pipe_stream.detach();

    CloseHandle(process_info.hThread);
    id_pair.get()->process_handle = reinterpret_cast<intptr_t>(process_info.hProcess);
    id_pair.get()->stream         = result_stream;
    id_pair.detach();
    return result_stream;
}



template <typename Character>
static FILE* __cdecl common_popen(
    Character const* const command,
    Character const* const type
    ) throw()
{
    _VALIDATE_RETURN(command != nullptr, EINVAL, nullptr);
    _VALIDATE_RETURN(type    != nullptr, EINVAL, nullptr);

    fdopen_mode<Character> const fdopen_mode = convert_popen_type_to_fdopen_mode(type);
    if (fdopen_mode.mode[0] == '\0')
        return nullptr;

    // Do the _pipe().  Note that neither of the resulting handles is inheritable.
    int pipe_mode = _O_NOINHERIT;
    if (fdopen_mode.mode[1] == 't') { pipe_mode |= _O_TEXT;   }
    if (fdopen_mode.mode[1] == 'b') { pipe_mode |= _O_BINARY; }

    int pipe_handles[2];
    if (_pipe(pipe_handles, 1024, pipe_mode) == -1)
        return nullptr;

    int const std_fh = fdopen_mode.mode[0] == 'w'
        ? STDIN
        : STDOUT;

    int ordered_pipe_handles[] =
    {
        std_fh == STDIN ? pipe_handles[0] : pipe_handles[1],
        std_fh == STDIN ? pipe_handles[1] : pipe_handles[0]
    };

    FILE* return_value = nullptr;

    __acrt_lock(__acrt_popen_lock);
    __try
    {
        errno_t const saved_errno = errno;

        return_value = common_popen_nolock(
            command,
            fdopen_mode.mode,
            std_fh,
            ordered_pipe_handles);

        errno = saved_errno;

        if (return_value != nullptr)
            __leave;

        // If the implementation function returned successfully, everything was
        // cleaned up except the lock.
        int* const first = ordered_pipe_handles;
        int* const last  = first + _countof(ordered_pipe_handles);
        for (int* it = first; it != last; ++it)
        {
            if (*it != -1)
                _close(*it);
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_popen_lock);
    }
    __endtry

    return return_value;
}



// Starts a child process using the given 'command' and opens a pipe to it, as
// requested via the 'type'.  If the 'type' string contains an 'r', the calling
// process can read the child command's standard output via the returned stream.
// If the 'type' string contains a 'w', the calling process can write to the
// child command's standard input via the returned stream.
//
// Returns a usable stream on success; returns null on failure.
extern "C" FILE* __cdecl _popen(
    char const* const command,
    char const* const type
    )
{
    return common_popen(command, type);
}



extern "C" FILE* __cdecl _wpopen(
    wchar_t const* const command,
    wchar_t const* const type
    )
{
    return common_popen(command, type);
}



// Waits on the child command with which the 'stream' is associated, then closes
// the stream and its associated pipe.  The 'stream' must have been returned from
// a previous call to _popen().  This function looks up the process handle in the
// global table, waits on it, then closes the stream.
//
// On success, the exit status of the child command is returned.  The format of
// the return value is the same as for cwait(), except that the low order and
// high order bytes are swapped.  If an error occurs, -1 is returned.
extern "C" int __cdecl _pclose(FILE* const stream)
{
    _VALIDATE_RETURN(stream != nullptr, EINVAL, -1);

    int return_value = -1;

    __acrt_lock(__acrt_popen_lock);
    __try
    {
        process_handle_pair* const id_pair = idtab(stream);
        if (id_pair == nullptr)
        {
            errno = EBADF;
            __leave;
        }

        fclose(stream);

        intptr_t const process_handle = id_pair->process_handle;

        // Mark the id pair as free (we will close the handle in the call to _cwait):
        id_pair->stream         = nullptr;
        id_pair->process_handle = 0;

        // Wait on the child copy of the command processor and its children:
        errno_t const saved_errno = errno;
        errno = 0;

        int status = 0;
        if (_cwait(&status, process_handle, _WAIT_GRANDCHILD) != -1 || errno == EINTR)
        {
            errno = saved_errno;
            return_value = status;
            __leave;
        }

        errno = saved_errno;
    }
    __finally
    {
        __acrt_unlock(__acrt_popen_lock);
    }
    __endtry

    return return_value;
}
