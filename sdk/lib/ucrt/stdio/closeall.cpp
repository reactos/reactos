//
// closeall.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _fcloseall(), which closes all opened files.
//
#include <corecrt_internal_stdio.h>



// Closes all streams currently open except for stdin, stdout, and stderr.  All
// tmpfile()-created streams are closed as well.  Returns EOF on failure; returns
// the number of closed streams on success.
extern "C" int __cdecl _fcloseall()
{
    int count = 0;

    __acrt_lock(__acrt_stdio_index_lock);
    __try
    {
        for (int i = _IOB_ENTRIES; i != _nstream; ++i)
        {
            if (__piob[i] == nullptr)
                continue;

            // If the stream is in use, close it:
            if (__crt_stdio_stream(__piob[i]).is_in_use() && fclose(&__piob[i]->_public_file) != EOF)
            {
                ++count;
            }

            DeleteCriticalSection(&__piob[i]->_lock);
            _free_crt(__piob[i]);
            __piob[i] = nullptr;
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_stdio_index_lock);
    }

    return count;
}
