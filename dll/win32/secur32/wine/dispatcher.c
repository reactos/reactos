/*
 * Copyright 2005, 2006 Kai Blin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * A dispatcher to run ntlm_auth for wine's sspi module.
 */

#include "precomp.h"

#include <stdio.h>
#include <process.h>
#include <io.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __REACTOS__
#define close _close
#define read  _read
#define write _write
#endif

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

#define INITIAL_BUFFER_SIZE 200

char* flatten_cmdline(const char *prog, char* const argv[])
{
    int i;
    SIZE_T argstr_size = 0;
    char *argstr, *p;

    /* Compute space needed for the new string, and allocate it */
    argstr_size += strlen(prog) + 3; // 3 == 2 quotes between 'prog', and 1 space
    for(i = 0; argv[i] != NULL; ++i)
    {
        argstr_size += strlen(argv[i]) + 1; // 1 for space
    }
    argstr = HeapAlloc(GetProcessHeap(), 0, (argstr_size + 1) * sizeof(CHAR));
    if (argstr == NULL)
    {
        ERR("ERROR: Not enough memory\n");
        return NULL;
    }

    /* Copy the contents and NULL-terminate the string */
    p = argstr;
    strcpy(p, "\"");    // Open quote
    strcat(p, prog);
    strcat(p, "\" ");   // Close quote + space
    p += strlen(p);
    for(i = 0; argv[i] != NULL; ++i)
    {
        strcpy(p, argv[i]);
        p += strlen(argv[i]);
        *p++ = ' ';
    }
    *p = '\0';

    return argstr;
}

SECURITY_STATUS fork_helper(PNegoHelper *new_helper, const char *prog,
        char* const argv[])
{
    int pipe_in[2];
    int pipe_out[2];
#ifdef __REACTOS__
    HANDLE hPipe;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    char* cmdline;
#endif
    int i;
    PNegoHelper helper;

    TRACE("%s ", debugstr_a(prog));
    for(i = 0; argv[i] != NULL; ++i)
    {
        TRACE("%s ", debugstr_a(argv[i]));
    }
    TRACE("\n");

#ifndef __REACTOS__

#ifdef HAVE_PIPE2
    if (pipe2( pipe_in, O_CLOEXEC ) < 0 )
#endif
    {
        if( pipe(pipe_in) < 0 ) return SEC_E_INTERNAL_ERROR;
        fcntl( pipe_in[0], F_SETFD, FD_CLOEXEC );
        fcntl( pipe_in[1], F_SETFD, FD_CLOEXEC );
    }
#ifdef HAVE_PIPE2
    if (pipe2( pipe_out, O_CLOEXEC ) < 0 )
#endif
    {
        if( pipe(pipe_out) < 0 )
        {
            close(pipe_in[0]);
            close(pipe_in[1]);
            return SEC_E_INTERNAL_ERROR;
        }
        fcntl( pipe_out[0], F_SETFD, FD_CLOEXEC );
        fcntl( pipe_out[1], F_SETFD, FD_CLOEXEC );
    }

#else

    if (_pipe(pipe_in, 0, _O_BINARY /* _O_TEXT */ | _O_NOINHERIT) < 0)
    {
        return SEC_E_INTERNAL_ERROR;
    }

    if (_pipe(pipe_out, 0, _O_BINARY /* _O_TEXT */ | _O_NOINHERIT) < 0)
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        return SEC_E_INTERNAL_ERROR;
    }

#endif

    if (!(helper = HeapAlloc(GetProcessHeap(),0, sizeof(NegoHelper))))
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        return SEC_E_INSUFFICIENT_MEMORY;
    }

#ifndef __REACTOS__
    helper->helper_pid = fork();
#else

    cmdline = flatten_cmdline(prog, argv);
    if (!cmdline)
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        HeapFree( GetProcessHeap(), 0, helper );
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    si.dwFlags |= STARTF_USESTDHANDLES;

    /* The reading side of the pipe is STDIN for this process */
    hPipe = (HANDLE)_get_osfhandle(pipe_out[0]);
    SetHandleInformation(hPipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    si.hStdInput = hPipe;

    /* The writing side of the pipe is STDOUT for this process */
    hPipe = (HANDLE)_get_osfhandle(pipe_in[1]);
    SetHandleInformation(hPipe, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    si.hStdOutput = hPipe;
    si.hStdError  = hPipe;

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        /* We fail just afterwards */
        helper->helper_pid = (HANDLE)-1;
    }
    else
    {
        helper->helper_pid = pi.hProcess;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    HeapFree( GetProcessHeap(), 0, cmdline );

#endif

#ifndef __REACTOS__
    if(helper->helper_pid == -1)
#else
    if(helper->helper_pid == (HANDLE)-1)
#endif
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        HeapFree( GetProcessHeap(), 0, helper );
        return SEC_E_INTERNAL_ERROR;
    }

#ifndef __REACTOS__
    if(helper->helper_pid == 0)
    {
        /* We're in the child now */
        dup2(pipe_out[0], 0);
        close(pipe_out[0]);
        close(pipe_out[1]);

        dup2(pipe_in[1], 1);
        close(pipe_in[0]);
        close(pipe_in[1]);

        execvp(prog, argv);

        /* Whoops, we shouldn't get here. Big badaboom.*/
        write(STDOUT_FILENO, "BH\n", 3);
        _exit(1);
    }
    else
#endif
    {
        *new_helper = helper;
        helper->major = helper->minor = helper->micro = -1;
        helper->com_buf = NULL;
        helper->com_buf_size = 0;
        helper->com_buf_offset = 0;
        helper->session_key = NULL;
        helper->neg_flags = 0;
        helper->crypt.ntlm.a4i = NULL;
        helper->crypt.ntlm2.send_a4i = NULL;
        helper->crypt.ntlm2.recv_a4i = NULL;
        helper->crypt.ntlm2.send_sign_key = NULL;
        helper->crypt.ntlm2.send_seal_key = NULL;
        helper->crypt.ntlm2.recv_sign_key = NULL;
        helper->crypt.ntlm2.recv_seal_key = NULL;
        helper->pipe_in = pipe_in[0];   // Keep in(read)
        close(pipe_in[1]);              // Close in(write)
        helper->pipe_out = pipe_out[1]; // Keep out(write)
        close(pipe_out[0]);             // Close out(read)
    }

    return SEC_E_OK;
}

static SECURITY_STATUS read_line(PNegoHelper helper, int *offset_len)
{
    char *newline;
    int read_size;
    
    if(helper->com_buf == NULL)
    {
        TRACE("Creating a new buffer for the helper\n");
        if((helper->com_buf = HeapAlloc(GetProcessHeap(), 0, INITIAL_BUFFER_SIZE)) == NULL)
            return SEC_E_INSUFFICIENT_MEMORY;
        
        /* Created a new buffer, size is INITIAL_BUFFER_SIZE, offset is 0 */
        helper->com_buf_size = INITIAL_BUFFER_SIZE;
        helper->com_buf_offset = 0;
    }

    do
    {
        TRACE("offset = %d, size = %d\n", helper->com_buf_offset, helper->com_buf_size);
        if(helper->com_buf_offset + INITIAL_BUFFER_SIZE > helper->com_buf_size)
        {
            /* increment buffer size in INITIAL_BUFFER_SIZE steps */
            char *buf = HeapReAlloc(GetProcessHeap(), 0, helper->com_buf,
                                    helper->com_buf_size + INITIAL_BUFFER_SIZE);
            TRACE("Resizing buffer!\n");
            if (!buf) return SEC_E_INSUFFICIENT_MEMORY;
            helper->com_buf_size += INITIAL_BUFFER_SIZE;
            helper->com_buf = buf;
        }
        if((read_size = read(helper->pipe_in, helper->com_buf + helper->com_buf_offset,
                    helper->com_buf_size - helper->com_buf_offset)) <= 0)
        {
            return SEC_E_INTERNAL_ERROR;
        }
        
        TRACE("read_size = %d, read: %s\n", read_size, 
                debugstr_a(helper->com_buf + helper->com_buf_offset));
        helper->com_buf_offset += read_size;
        newline = memchr(helper->com_buf, '\n', helper->com_buf_offset);
    }while(newline == NULL);

    /* Now, if there's a newline character, and we read more than that newline,
     * we have to store the offset so we can preserve the additional data.*/
    if( newline != helper->com_buf + helper->com_buf_offset)
    {
        TRACE("offset_len is calculated from %p - %p\n", 
                (helper->com_buf + helper->com_buf_offset), newline+1);
        /* the length of the offset is the number of chars after the newline */
        *offset_len = (helper->com_buf + helper->com_buf_offset) - (newline + 1);
    }
    else
    {
        *offset_len = 0;
    }
    
    *newline = '\0';

    return SEC_E_OK;
}

static SECURITY_STATUS preserve_unused(PNegoHelper helper, int offset_len)
{
    TRACE("offset_len = %d\n", offset_len);

    if(offset_len > 0)
    {
        memmove(helper->com_buf, helper->com_buf + helper->com_buf_offset, 
                offset_len);
        helper->com_buf_offset = offset_len;
    }
    else
    {
        helper->com_buf_offset = 0;
    }

    TRACE("helper->com_buf_offset was set to: %d\n", helper->com_buf_offset);
    return SEC_E_OK;
}

SECURITY_STATUS run_helper(PNegoHelper helper, char *buffer,
        unsigned int max_buflen, int *buflen)
{
    int offset_len;
    SECURITY_STATUS sec_status = SEC_E_OK;

    TRACE("In helper: sending %s\n", debugstr_a(buffer));

    /* buffer + '\n' */
    write(helper->pipe_out, buffer, lstrlenA(buffer));
    write(helper->pipe_out, "\n", 1);

    if((sec_status = read_line(helper, &offset_len)) != SEC_E_OK)
    {
        return sec_status;
    }
    
    TRACE("In helper: received %s\n", debugstr_a(helper->com_buf));
    *buflen = lstrlenA(helper->com_buf);

    if( *buflen > max_buflen)
    {   
        ERR("Buffer size too small(%d given, %d required) dropping data!\n",
                max_buflen, *buflen);
        return SEC_E_BUFFER_TOO_SMALL;
    }

    if( *buflen < 2 )
    {
        return SEC_E_ILLEGAL_MESSAGE;
    }

    /* We only get ERR if the input size is too big. On a GENSEC error,
     * ntlm_auth will return BH */
    if(strncmp(helper->com_buf, "ERR", 3) == 0)
    {
        return SEC_E_INVALID_TOKEN;
    }

    memcpy(buffer, helper->com_buf, *buflen+1);

    sec_status = preserve_unused(helper, offset_len);
    
    return sec_status;
}

void cleanup_helper(PNegoHelper helper)
{

    TRACE("Killing helper %p\n", helper);
    if(helper == NULL)
        return;

    HeapFree(GetProcessHeap(), 0, helper->com_buf);
    HeapFree(GetProcessHeap(), 0, helper->session_key);

    /* closing stdin will terminate ntlm_auth */
    close(helper->pipe_out);
    close(helper->pipe_in);

#ifndef __REACTOS__

#ifdef HAVE_FORK
    if (helper->helper_pid > 0) /* reap child */
    {
        pid_t wret;
        do {
            wret = waitpid(helper->helper_pid, NULL, 0);
        } while (wret < 0 && errno == EINTR);
    }
#endif

#endif

    HeapFree(GetProcessHeap(), 0, helper);
}

void check_version(PNegoHelper helper)
{
    char temp[80];
    char *newline;
    int major = 0, minor = 0, micro = 0, ret;

    TRACE("Checking version of helper\n");
    if(helper != NULL)
    {
        int len = read(helper->pipe_in, temp, sizeof(temp)-1);
        if (len > 8)
        {
            if((newline = memchr(temp, '\n', len)) != NULL)
                *newline = '\0';
            else
                temp[len] = 0;

            TRACE("Exact version is %s\n", debugstr_a(temp));
            ret = sscanf(temp, "Version %d.%d.%d", &major, &minor, &micro);
            if(ret != 3)
            {
                ERR("Failed to get the helper version.\n");
                helper->major = helper->minor = helper->micro = -1;
            }
            else
            {
                TRACE("Version recognized: %d.%d.%d\n", major, minor, micro);
                helper->major = major;
                helper->minor = minor;
                helper->micro = micro;
            }
        }
    }
}
