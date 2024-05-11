//
// errno.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _errno, _doserrno, and related functions
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal.h>
#include <corecrt_internal_ptd_propagation.h>
#include <errno.h>



// This is the error table that defines the mapping between OS error codes and
// errno values.
namespace
{
    struct errentry
    {
        unsigned long oscode; // OS return value
        int errnocode;        // System V error code
    };
}

static errentry const errtable[]
{
    { ERROR_INVALID_FUNCTION,       EINVAL    },  //    1
    { ERROR_FILE_NOT_FOUND,         ENOENT    },  //    2
    { ERROR_PATH_NOT_FOUND,         ENOENT    },  //    3
    { ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  //    4
    { ERROR_ACCESS_DENIED,          EACCES    },  //    5
    { ERROR_INVALID_HANDLE,         EBADF     },  //    6
    { ERROR_ARENA_TRASHED,          ENOMEM    },  //    7
    { ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  //    8
    { ERROR_INVALID_BLOCK,          ENOMEM    },  //    9
    { ERROR_BAD_ENVIRONMENT,        E2BIG     },  //   10
    { ERROR_BAD_FORMAT,             ENOEXEC   },  //   11
    { ERROR_INVALID_ACCESS,         EINVAL    },  //   12
    { ERROR_INVALID_DATA,           EINVAL    },  //   13
    { ERROR_INVALID_DRIVE,          ENOENT    },  //   15
    { ERROR_CURRENT_DIRECTORY,      EACCES    },  //   16
    { ERROR_NOT_SAME_DEVICE,        EXDEV     },  //   17
    { ERROR_NO_MORE_FILES,          ENOENT    },  //   18
    { ERROR_LOCK_VIOLATION,         EACCES    },  //   33
    { ERROR_BAD_NETPATH,            ENOENT    },  //   53
    { ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  //   65
    { ERROR_BAD_NET_NAME,           ENOENT    },  //   67
    { ERROR_FILE_EXISTS,            EEXIST    },  //   80
    { ERROR_CANNOT_MAKE,            EACCES    },  //   82
    { ERROR_FAIL_I24,               EACCES    },  //   83
    { ERROR_INVALID_PARAMETER,      EINVAL    },  //   87
    { ERROR_NO_PROC_SLOTS,          EAGAIN    },  //   89
    { ERROR_DRIVE_LOCKED,           EACCES    },  //  108
    { ERROR_BROKEN_PIPE,            EPIPE     },  //  109
    { ERROR_DISK_FULL,              ENOSPC    },  //  112
    { ERROR_INVALID_TARGET_HANDLE,  EBADF     },  //  114
    { ERROR_WAIT_NO_CHILDREN,       ECHILD    },  //  128
    { ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  //  129
    { ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  //  130
    { ERROR_NEGATIVE_SEEK,          EINVAL    },  //  131
    { ERROR_SEEK_ON_DEVICE,         EACCES    },  //  132
    { ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  //  145
    { ERROR_NOT_LOCKED,             EACCES    },  //  158
    { ERROR_BAD_PATHNAME,           ENOENT    },  //  161
    { ERROR_MAX_THRDS_REACHED,      EAGAIN    },  //  164
    { ERROR_LOCK_FAILED,            EACCES    },  //  167
    { ERROR_ALREADY_EXISTS,         EEXIST    },  //  183
    { ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  //  206
    { ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  //  215
    { ERROR_NO_UNICODE_TRANSLATION, EILSEQ    },  // 1113
    { ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }   // 1816
};

// Number of elements in the table
#define ERRTABLECOUNT (sizeof(errtable) / sizeof(errtable[0]))

// The following two constants must be the minimum and maximum
// values in the (contiguous) range of Exec Failure errors.
#define MIN_EXEC_ERROR ERROR_INVALID_STARTING_CODESEG
#define MAX_EXEC_ERROR ERROR_INFLOOP_IN_RELOC_CHAIN

// These are the low and high value in the range of errors that are
// access violations
#define MIN_EACCES_RANGE ERROR_WRITE_PROTECT
#define MAX_EACCES_RANGE ERROR_SHARING_BUFFER_EXCEEDED



// These map Windows error codes into errno error codes
extern "C" void __cdecl __acrt_errno_map_os_error(unsigned long const oserrno)
{
    _doserrno = oserrno;
    errno     = __acrt_errno_from_os_error(oserrno);
}

extern "C" void __cdecl __acrt_errno_map_os_error_ptd(unsigned long const oserrno, __crt_cached_ptd_host& ptd)
{
    ptd.get_doserrno().set(oserrno);
    ptd.get_errno().set(__acrt_errno_from_os_error(oserrno));
}

extern "C" int __cdecl __acrt_errno_from_os_error(unsigned long const oserrno)
{
    // Check the table for the OS error code
    for (unsigned i{0}; i < ERRTABLECOUNT; ++i)
    {
        if (oserrno == errtable[i].oscode)
            return errtable[i].errnocode;
    }

    // The error code wasn't in the table.  We check for a range of
    // EACCES errors or exec failure errors (ENOEXEC).  Otherwise
    // EINVAL is returned.
    if (oserrno >= MIN_EACCES_RANGE && oserrno <= MAX_EACCES_RANGE)
    {
        return EACCES;
    }
    else if (oserrno >= MIN_EXEC_ERROR && oserrno <= MAX_EXEC_ERROR)
    {
        return ENOEXEC;
    }
    else
    {
        return EINVAL;
    }
}



// These safely set and get the value of the calling thread's errno
extern "C" errno_t _set_errno(int const value)
{
    __acrt_ptd* const ptd{__acrt_getptd_noexit()};
    if (!ptd)
        return ENOMEM;

    errno = value;
    return 0;
}

extern "C" errno_t _get_errno(int* const result)
{
    _VALIDATE_RETURN_NOERRNO(result != nullptr, EINVAL);

    // Unlike most of our globals, this one is guaranteed to give some answer
    *result = errno;
    return 0;
}



// These safely set and get the value of the calling thread's doserrno
extern "C" errno_t _set_doserrno(unsigned long const value)
{
    __acrt_ptd* const ptd{__acrt_getptd_noexit()};
    if (!ptd)
        return ENOMEM;

    _doserrno = value;
    return 0;
}

extern "C" errno_t _get_doserrno(unsigned long* const result)
{
    _VALIDATE_RETURN_NOERRNO(result != nullptr, EINVAL);

    // Unlike most of our globals, this one is guaranteed to give some answer:
    *result = _doserrno;
    return 0;
}



// These return pointers to the calling thread's errno and doserrno values,
// respectively, and are used to implement errno and _doserrno in the header.
static int           errno_no_memory   {ENOMEM};
static unsigned long doserrno_no_memory{ERROR_NOT_ENOUGH_MEMORY};

extern "C" int* __cdecl _errno()
{
    __acrt_ptd* const ptd{__acrt_getptd_noexit()};
    if (!ptd)
    {
        return &errno_no_memory;
    }

    return &ptd->_terrno;
}

extern "C" unsigned long* __cdecl __doserrno()
{
    __acrt_ptd* const ptd{__acrt_getptd_noexit()};
    if (!ptd)
    {
        return &doserrno_no_memory;
    }

    return &ptd->_tdoserrno;
}
