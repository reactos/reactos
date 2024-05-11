//
// rmtmp.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _rmtmp(), which closes and removes all temporary files created by
// tmpfile()
//
#include <corecrt_internal_stdio.h>



_CRT_LINKER_FORCE_INCLUDE(__acrt_tmpfile_terminator);



// These definitions will cause this object to be linked in whenever the
// termination code requires it.
#ifndef CRTDLL
    extern "C" unsigned __acrt_tmpfile_used = 1;
#endif

extern "C" unsigned _tempoff = 1;
extern "C" unsigned _old_pfxlen = 0;



// Closes and removes all temporary files created by tmpfile().  Returns the
// number of streams that were closed.
extern "C" int __cdecl _rmtmp()
{
    int count = 0;

    __acrt_lock(__acrt_stdio_index_lock);
    __try
    {
        for (int i = 0; i != _nstream; ++i)
        {
            __crt_stdio_stream stream(__piob[i]);

            if (!stream.valid())
                continue;

            _lock_file(stream.public_stream());
            __try
            {
                if (!stream.is_in_use())
                    __leave;

                if (stream->_tmpfname == nullptr)
                    __leave;

                // The stream is still in use.  Close it:
                _fclose_nolock(stream.public_stream());
                ++count;
            }
            __finally
            {
                _unlock_file(stream.public_stream());
            }
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_stdio_index_lock);
    }

    return count;
}

extern "C" void __cdecl __acrt_uninitialize_tmpfile()
{
    __acrt_stdio_free_tmpfile_name_buffers_nolock();
    _rmtmp();
}
