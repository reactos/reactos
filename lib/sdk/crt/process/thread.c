/*
 * msvcrt.dll thread functions
 *
 * Copyright 2000 Jon Griffiths
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
 */

#include <precomp.h>
#include <malloc.h>
#include <process.h>

typedef void (*_beginthread_start_routine_t)(void *);
typedef unsigned int (__stdcall *_beginthreadex_start_routine_t)(void *);

/********************************************************************/

typedef struct {
  HANDLE thread;
  _beginthread_start_routine_t start_address;
  void *arglist;
} _beginthread_trampoline_t;

/*********************************************************************
 *		_beginthread_trampoline
 */
static DWORD CALLBACK _beginthread_trampoline(LPVOID arg)
{
    _beginthread_trampoline_t local_trampoline;
    thread_data_t *data = msvcrt_get_thread_data();

    memcpy(&local_trampoline,arg,sizeof(local_trampoline));
    data->handle = local_trampoline.thread;
    free(arg);

    local_trampoline.start_address(local_trampoline.arglist);
    return 0;
}

/*********************************************************************
 *		_beginthread (MSVCRT.@)
 */
uintptr_t _beginthread(
  _beginthread_start_routine_t start_address, /* [in] Start address of routine that begins execution of new thread */
  unsigned int stack_size, /* [in] Stack size for new thread or 0 */
  void *arglist)           /* [in] Argument list to be passed to new thread or NULL */
{
  _beginthread_trampoline_t* trampoline;
  HANDLE thread;

  TRACE("(%p, %d, %p)\n", start_address, stack_size, arglist);

  trampoline = malloc(sizeof(*trampoline));
  if(!trampoline) {
      *_errno() = EAGAIN;
      return -1;
  }

  thread = CreateThread(NULL, stack_size, _beginthread_trampoline,
          trampoline, CREATE_SUSPENDED, NULL);
  if(!thread) {
      free(trampoline);
      *_errno() = EAGAIN;
      return -1;
  }

  trampoline->thread = thread;
  trampoline->start_address = start_address;
  trampoline->arglist = arglist;

  if(ResumeThread(thread) == -1) {
      free(trampoline);
      *_errno() = EAGAIN;
      return -1;
  }

  return (uintptr_t)thread;
}

/*********************************************************************
 *		_endthread (MSVCRT.@)
 */
void CDECL _endthread(void)
{
  TRACE("(void)\n");

  /* FIXME */
  ExitThread(0);
}
