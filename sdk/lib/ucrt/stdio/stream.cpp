//
// getstream.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _getstream(), which finds and locks a stream that is available for use.
//
#include <corecrt_internal_stdio.h>



static __crt_stdio_stream __cdecl find_or_allocate_unused_stream_nolock() throw()
{
    __crt_stdio_stream_data** const first_stream = __piob + _IOB_ENTRIES;
    __crt_stdio_stream_data** const last_stream  = first_stream + _nstream - _IOB_ENTRIES;

    for (__crt_stdio_stream_data** it = first_stream; it != last_stream; ++it)
    {
        // First, check to see whether the stream is valid and free for use:
        {
            __crt_stdio_stream stream(*it);
            if (stream.valid())
            {
                if (stream.is_in_use())
                    continue;

                stream.lock();
                if (!stream.try_allocate())
                {
                    stream.unlock();
                    continue;
                }

                return stream;
            }
        }

        // Otherwise, there is no stream at this index yet, so we allocate one
        // and return it:
        {
            *it = _calloc_crt_t(__crt_stdio_stream_data, 1).detach();
            if (*it == nullptr)
                break;

            // Initialize the stream.  Everything requires zero-initialization
            // (which we get from calloc), except the file handle and the lock:
            (*it)->_file = -1;
            __acrt_InitializeCriticalSectionEx(&(*it)->_lock, _CORECRT_SPINCOUNT, 0);

            __crt_stdio_stream stream(*it);

            // Note:  This attempt will always succeed, because we hold the only
            // pointer to the stream object (since we just allocated it):
            stream.try_allocate();
            stream.lock();

            return stream;
        }
    }

    return __crt_stdio_stream();
}



// Finds a stream not in use and makes it available to the caller.  It is
// intended for internal use inside the library only.  It returns a pointer to
// a free stream, or nullptr if all are in use.  A stream becomes allocated
// only if the caller decides to use it by setting a mode (r, w, or r/w).  The
// stream is returned locked; the caller is responsible for unlocking the stream.
__crt_stdio_stream __cdecl __acrt_stdio_allocate_stream() throw()
{
    __crt_stdio_stream stream;

    __acrt_lock(__acrt_stdio_index_lock);
    __try
    {
        stream = find_or_allocate_unused_stream_nolock();
        if (!stream.valid())
            __leave;

        stream->_cnt = 0;
        stream->_tmpfname = nullptr;
        stream->_ptr = nullptr;
        stream->_base = nullptr;
        stream->_file = -1;
    }
    __finally
    {
        __acrt_unlock(__acrt_stdio_index_lock);
    }

    return stream;
}

void __cdecl __acrt_stdio_free_stream(__crt_stdio_stream stream) throw()
{
    stream->_ptr      = nullptr;
    stream->_base     = nullptr;
    stream->_cnt      =  0;
    stream->_file     = -1;
    stream->_charbuf  =  0;
    stream->_bufsiz   =  0;
    stream->_tmpfname = nullptr;
    stream.deallocate();
}
