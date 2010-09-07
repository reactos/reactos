/* Copyright (c) 2003 Juan Lang
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
#ifndef __NBCMDQUEUE_H__
#define __NBCMDQUEUE_H__

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "nb30.h"

/* This file defines a queue of pending NetBIOS commands.  The queue operations
 * are thread safe, with the exception of NBCmdQueueDestroy:  ensure no other
 * threads are manipulating the queue when calling NBCmdQueueDestroy.
 */

struct NBCmdQueue;

/* Allocates a new command queue from heap. */
struct NBCmdQueue *NBCmdQueueCreate(HANDLE heap);

/* Adds ncb to queue.  Assumes queue is not NULL, and ncb is not already in the
 * queue.  If ncb is already in the queue, returns NRC_TOOMANY.
 */
UCHAR NBCmdQueueAdd(struct NBCmdQueue *queue, PNCB ncb);

/* Cancels the given ncb.  Blocks until the command completes.  Implicitly
 * removes ncb from the queue.  Assumes queue and ncb are not NULL, and that
 * ncb has been added to queue previously.
 * Returns NRC_CMDCAN on a successful cancellation, NRC_CMDOCCR if the command
 * completed before it could be cancelled, and various other return values for
 * different failures.
 */
UCHAR NBCmdQueueCancel(struct NBCmdQueue *queue, PNCB ncb);

/* Sets the return code of the given ncb, and implicitly removes the command
 * from the queue.  Assumes queue and ncb are not NULL, and that ncb has been
 * added to queue previously.
 * Returns NRC_GOODRET on success.
 */
UCHAR NBCmdQueueComplete(struct NBCmdQueue *queue, PNCB ncb, UCHAR retcode);

/* Cancels all pending commands in the queue (useful for a RESET or a shutdown).
 * Returns when all commands have been completed.
 */
UCHAR NBCmdQueueCancelAll(struct NBCmdQueue *queue);

/* Frees all memory associated with the queue.  Blocks until all commands
 * pending in the queue have been completed.
 */
void NBCmdQueueDestroy(struct NBCmdQueue *queue);

#endif /* __NBCMDQUEUE_H__ */
