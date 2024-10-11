//
// ioinit.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines initialization and termination routines for the lowio library, along
// with global data shared by most of the lowio library.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_stdio.h>



// This is a special static lowio file object referenced only by the safe access
// functionality in the internal headers.  It is used in certain stdio-level
// functions to more gracefully handle a FILE with -1 as its lowio file id.
extern "C" { __crt_lowio_handle_data __badioinfo =
{
    { },                         // lock
    static_cast<intptr_t>(-1),   // osfhnd
    0,                           // startpos
    FTEXT,                       // osfile
    __crt_lowio_text_mode::ansi, // textmode
    { LF, LF, LF },              // _pipe_lookahead
}; }



// This is the number of lowio file objects that have been allocated.  This
// includes both in-use and unused elements, since not all allocated files
// are necessarily in use at any given time.
//
// This number is in the range of [IOINFO_ARRAY_ELTS, _NHANDLE_]
extern "C" { int _nhandle = 0; }



// This is the global array of file object arrays:
extern "C" { __crt_lowio_handle_data* __pioinfo[IOINFO_ARRAYS] = { 0 }; }



static DWORD __cdecl get_std_handle_id(int const fh) throw()
{
    // Convert the CRT file handle to the OS file handle for the three 
    // standard streams:
    switch (fh)
    {
    case 0: return STD_INPUT_HANDLE;
    case 1: return STD_OUTPUT_HANDLE;
    case 2: return STD_ERROR_HANDLE;
    }

    return STD_ERROR_HANDLE; // Unreachable, but the compiler can't know.
}



static void __cdecl initialize_inherited_file_handles_nolock() throw()
{
    STARTUPINFOW startup_info;
    GetStartupInfoW(&startup_info);

    // First check and see if we inherited any file handles.  If we didn't, then
    // we don't have anything to initialize:
    if (startup_info.cbReserved2 == 0 || startup_info.lpReserved2 == nullptr)
        return;

    // Get the number of inherited handles:
    int const handle_count = *reinterpret_cast<UNALIGNED int*>(startup_info.lpReserved2);

    // Compute the start of the passed file info and OS HANDLEs:
    unsigned char* const first_file =
        reinterpret_cast<unsigned char*>(startup_info.lpReserved2) + sizeof(int);

    UNALIGNED intptr_t* const first_handle =
        reinterpret_cast<UNALIGNED intptr_t*>(first_file + handle_count);

    // Do not attempt to inherit more than the maximum number of supported handles:
    int handles_to_inherit = handle_count < _NHANDLE_ 
        ? handle_count
        : _NHANDLE_;

    // Attempt to allocate the required number of handles.  If we fail for any
    // reason, we'll inherit as many handles as we can:
    __acrt_lowio_ensure_fh_exists(handles_to_inherit);
    if (handles_to_inherit > _nhandle)
        handles_to_inherit = _nhandle;

    // Validate and copy the provided file information:
    unsigned char*      it_file   = first_file;
    UNALIGNED intptr_t* it_handle = first_handle;

    for (int fh = 0; fh != handles_to_inherit; ++fh, ++it_file, ++it_handle)
    {
        HANDLE const real_handle = reinterpret_cast<HANDLE>(*it_handle);

        // If the provided information does not appear to describe an open,
        // valid file or device, skip it:
        if (real_handle == INVALID_HANDLE_VALUE)
            continue;

        if (*it_handle == _NO_CONSOLE_FILENO)
            continue;

        if ((*it_file & FOPEN) == 0)
            continue;

        // GetFileType cannot be called for pipe handles since it may "hang" if
        // there is a blocked read pending on the pipe in the parent.
        if ((*it_file & FPIPE) == 0 && GetFileType(real_handle) == FILE_TYPE_UNKNOWN)
            continue;

        // Okay, the file looks valid:
        __crt_lowio_handle_data* const pio = _pioinfo(fh);
        pio->osfhnd = *it_handle;
        pio->osfile = *it_file;
    }
}



static void initialize_stdio_handles_nolock() throw()
{
    for (int fh = 0; fh != STDIO_HANDLES_COUNT; ++fh)
    {
        __crt_lowio_handle_data* const pio = _pioinfo(fh);

        // If this handle was inherited from the parent process and initialized
        // already, make sure it has the FTEXT flag and continue:
        if (pio->osfhnd != reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE) &&
            pio->osfhnd != _NO_CONSOLE_FILENO)
        {
            pio->osfile |= FTEXT;
            continue;
        }

        // Regardless what happens next, the file will be treated as if it is
        // open in text mode:
        pio->osfile = FOPEN | FTEXT;

        // This handle has not yet been initialized, so  let's see if we can get
        // the handle from the OS:
        intptr_t const os_handle = reinterpret_cast<intptr_t>(GetStdHandle(get_std_handle_id(fh)));

        bool const is_valid_handle = 
            os_handle != reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE) &&
            os_handle != reinterpret_cast<intptr_t>(nullptr);

        DWORD const handle_type = is_valid_handle
            ? GetFileType(reinterpret_cast<HANDLE>(os_handle))
            : FILE_TYPE_UNKNOWN;


        if (handle_type != FILE_TYPE_UNKNOWN)
        {
            // The file type is known, so we obtained a valid handle from the
            // OS.  Finish initializing the lowio file object for this handle,
            // including the flag specifying whether this is a character device
            // or a pipe:
            pio->osfhnd = os_handle;

            if ((handle_type & 0xff) == FILE_TYPE_CHAR)
                pio->osfile |= FDEV;

            else if ((handle_type & 0xff) == FILE_TYPE_PIPE)
                pio->osfile |= FPIPE;
        }
        else
        {
            // We were unable to get the handles from the OS.  For stdin, stdout,
            // and stderr, if there is no valid OS handle, treat the CRT handle
            // as being open in text mode on a device with _NO_CONSOLE_FILENO
            // underlying it.  We use this value instead of INVALID_HANDLE_VALUE
            // to distinguish between a failure in opening a file and a program
            // run without a console:
            pio->osfile |= FDEV;
            pio->osfhnd = _NO_CONSOLE_FILENO;

            // Also update the corresponding stdio stream, unless stdio was
            // already terminated:
            if (__piob)
                __piob[fh]->_file = _NO_CONSOLE_FILENO;
        }
    }
}



// Initializes the lowio library.  This initialization comprises several steps:
//
// [1] An initial array of __crt_lowio_handle_data structures is allocated.
//
// [2] Inherited file handles are initialized.  To do this, sthe startup info
//     is obtained from the OS, via the lpReserved2 member.  The format of the
//     information is as follows:
//
//     [Bytes 0 - 3]       Integer value N, which is the number of handles that
//                         are provided by the parent process.
//
//     [Bytes 4 - N+3]     The N osfile values.
//
//     [Bytes N+4 - 5*N+3] The N OS HANDLE values, as DWORDs
//
// [3] Next, the first three lowio files (corresponding to stdin, stdout, and
//     stderr) are initialized as follows:  If the value in osfhnd is 
//     INVALID_HANDLE_VALUE, then we try to obtain a HANDLE from the OS.  These
//     handles are forced to text mode, as standard input, output, and error
//     always start out in text mode.
//
// Notes:
//
// [1] In general, not all of the pased info from the parent process will
//     describe open handles.  If, for example, only C handle 1 (stdout) and
//     C handle 6 are open in the parent, info for C handles 0 through 6 are
//     passed to the child.  0, 2, 3, 4, and 5 will not describe open handles.
//
// [2] Care is taken not to "overflow" the arrays of lowio file objects.
//
// [3] See the dospawn logic for the encoding of the file handle info to be
//     passed to a child process.
//
// This funtion returns 0 on success; -1 on failure.
extern "C" bool __cdecl __acrt_initialize_lowio()
{
    __acrt_lock(__acrt_lowio_index_lock);
    bool result = false;
    __try
    {
        // First, allocate and initialize the initial array of lowio files:
        if (__acrt_lowio_ensure_fh_exists(0) != 0)
            __leave;

        // Next, process and initialize all inherited file handles:
        initialize_inherited_file_handles_nolock();

        // Finally, initialize the stdio handles, if they were not inherited:
        initialize_stdio_handles_nolock();
        result = true;
    }
    __finally
    {
        __acrt_unlock(__acrt_lowio_index_lock);
    }
    __endtry

    return result;
}



// Shuts down the lowio library, freeing all allocated memory and destroying all
// created synchronization objects.
extern "C" bool __cdecl __acrt_uninitialize_lowio(bool const /* terminating */)
{
    for (size_t i = 0; i < IOINFO_ARRAYS; ++i)
    {
        if (!__pioinfo[i])
            continue;

        __acrt_lowio_destroy_handle_array(__pioinfo[i]);
        __pioinfo[i] = nullptr;
    }

    return true;
}
