/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/stdio/_flsbuf.c
 * PURPOSE:         Implementation of _flsbuf / _flswbuf
 * PROGRAMMER:      Timo Kreuzer
 */

#include <stdio.h>
#include <io.h>
#include <tchar.h>

void __cdecl alloc_buffer(FILE *stream);

int __cdecl
_flsbuf(int ch, FILE *stream)
{
    int count, written;

    /* Check if the stream supports flushing */
    if ((stream->_flag & _IOSTRG) || !(stream->_flag & (_IORW|_IOWRT)))
    {
        stream->_flag |= _IOERR;
        return EOF;
    }

    /* Is this was a read buffer */
    if (stream->_flag & _IOREAD)
    {
        /* Must be at the end of the file */
        if (!(stream->_flag & _IOEOF))
        {
            stream->_flag |= _IOERR;
            stream->_cnt = 0;
            return EOF;
        }

        /* Reset buffer */
        stream->_ptr = stream->_base;
    }

    /* Fixup flags */
    stream->_flag &= ~(_IOREAD|_IOEOF);
    stream->_flag |= _IOWRT;

    /* If we have no buffer, try to allocate one */
    if (!stream->_base && stream != stdout && stream != stderr)
    {
        alloc_buffer(stream);
    }

    /* Check if we have a buffer now */
    if (stream->_base)
    {
        /* We have one, check if there is something to write */
        count = stream->_ptr - stream->_base;
        if (count > 0)
            written = _write(stream->_file, stream->_base, count);
        else
            written = 0;

        /* Reset buffer and put the char into it */
        stream->_ptr = stream->_base + sizeof(TCHAR);
        stream->_cnt = stream->_bufsiz - sizeof(TCHAR);
        *(TCHAR*)stream->_base = ch;
    }
    else
    {
        /* There is no buffer, write the char directly */
        count = sizeof(TCHAR);
        written = _write(stream->_file, &ch, sizeof(TCHAR));
    }

    /* Check for failure */
    if (written != count)
    {
        stream->_flag |= _IOERR;
        return EOF;
    }

    return (TCHAR)ch;
}
