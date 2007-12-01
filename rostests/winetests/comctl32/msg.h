/* Message Sequence Testing Code
 *
 * Copyright (C) 2007 James Hawkins
 * Copyright (C) 2007 Lei Zhang
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

#include <assert.h>
#include <windows.h>
#include "wine/test.h"

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

typedef enum
{
    sent = 0x1,
    posted = 0x2,
    parent = 0x4,
    wparam = 0x8,
    lparam = 0x10,
    defwinproc = 0x20,
    beginpaint = 0x40,
    optional = 0x80,
    hook = 0x100,
    winevent_hook =0x200,
    id = 0x400
} msg_flags_t;

struct message
{
    UINT message;       /* the WM_* code */
    msg_flags_t flags;  /* message props */
    WPARAM wParam;      /* expected value of wParam */
    LPARAM lParam;      /* expected value of lParam */
    UINT id;            /* id of the window */
};

struct msg_sequence
{
    int count;
    int size;
    struct message *sequence;
};

void add_message(struct msg_sequence **seq, int sequence_index,
    const struct message *msg);
void flush_sequence(struct msg_sequence **seg, int sequence_index);
void flush_sequences(struct msg_sequence **seq, int n);

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)


void ok_sequence_(struct msg_sequence **seq, int sequence_index,
    const struct message *expected, const char *context, int todo,
    const char *file, int line);

void init_msg_sequences(struct msg_sequence **seq, int n);
