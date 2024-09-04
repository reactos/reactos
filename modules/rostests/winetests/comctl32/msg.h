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

#ifdef __REACTOS__
#pragma once
#endif

#include <assert.h>
#include <windows.h>
#include "wine/heap.h"
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
    id = 0x400,
    custdraw = 0x800
} msg_flags_t;

struct message
{
    UINT message;       /* the WM_* code */
    msg_flags_t flags;  /* message props */
    WPARAM wParam;      /* expected value of wParam */
    LPARAM lParam;      /* expected value of lParam */
    UINT id;            /* extra message data: id of the window,
                           notify code etc. */
    DWORD stage;        /* custom draw stage */
};

struct msg_sequence
{
    int count;
    int size;
    struct message *sequence;
};

static void add_message(struct msg_sequence **seq, int sequence_index,
    const struct message *msg)
{
    struct msg_sequence *msg_seq = seq[sequence_index];

    if (!msg_seq->sequence)
    {
        msg_seq->size = 10;
        msg_seq->sequence = heap_alloc(msg_seq->size * sizeof (*msg_seq->sequence));
    }

    if (msg_seq->count == msg_seq->size)
    {
        msg_seq->size *= 2;
        msg_seq->sequence = heap_realloc(msg_seq->sequence, msg_seq->size * sizeof (*msg_seq->sequence));
    }

    assert(msg_seq->sequence);

    msg_seq->sequence[msg_seq->count] = *msg;
    msg_seq->count++;
}

static inline void flush_sequence(struct msg_sequence **seg, int sequence_index)
{
    struct msg_sequence *msg_seq = seg[sequence_index];
    heap_free(msg_seq->sequence);
    msg_seq->sequence = NULL;
    msg_seq->count = msg_seq->size = 0;
}

static inline void flush_sequences(struct msg_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        flush_sequence(seq, i);
}

static void dump_sequence( struct msg_sequence **seq, int sequence_index,
                           const struct message *expected, const char *context,
                           const char *file, int line )
{
    struct msg_sequence *msg_seq = seq[sequence_index];
    const struct message *actual, *sequence;
    unsigned int count = 0;

    sequence = msg_seq->sequence;
    actual = sequence;

    trace_(file, line)("Failed sequence %s:\n", context );
    while (expected->message && actual->message)
    {
        trace_(file, line)( "  %u: expected: %04x - actual: %04x wp %08lx lp %08lx\n",
                            count, expected->message, actual->message, actual->wParam, actual->lParam );

	if (expected->message == actual->message)
	{
	    if ((expected->flags & defwinproc) != (actual->flags & defwinproc) &&
                (expected->flags & optional))
            {
                /* don't match messages if their defwinproc status differs */
                expected++;
            }
            else
            {
                expected++;
                actual++;
            }
	}
        else
        {
            expected++;
            actual++;
        }
        count++;
    }

    /* optional trailing messages */
    while (expected->message && expected->flags & optional)
    {
        trace_(file, line)( "  %u: expected: msg %04x - actual: nothing\n", count, expected->message );
	expected++;
        count++;
    }

    if (expected->message)
    {
        trace_(file, line)( "  %u: expected: msg %04x - actual: nothing\n", count, expected->message );
        return;
    }

    while (actual->message)
    {
        trace_(file, line)( "  %u: expected: nothing - actual: %04x wp %08lx lp %08lx\n",
                            count, actual->message, actual->wParam, actual->lParam );
        actual++;
        count++;
    }
}

static void ok_sequence_(struct msg_sequence **seq, int sequence_index,
    const struct message *expected_list, const char *context, BOOL todo,
    const char *file, int line)
{
    static const struct message end_of_sequence = {0, 0, 0, 0};
    struct msg_sequence *msg_seq = seq[sequence_index];
    const struct message *expected = expected_list;
    const struct message *actual, *sequence;
    int failcount = 0, dump = 0;

    add_message(seq, sequence_index, &end_of_sequence);

    sequence = msg_seq->sequence;
    actual = sequence;

    while (expected->message && actual->message)
    {
        if (expected->message == actual->message)
        {
            if (expected->flags & wparam)
            {
                if (expected->wParam != actual->wParam && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        dump++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting wParam 0x%lx got 0x%lx\n",
                            context, expected->message, expected->wParam, actual->wParam);
                    }
                }
                else
                {
                    ok_(file, line) (expected->wParam == actual->wParam,
                        "%s: in msg 0x%04x expecting wParam 0x%lx got 0x%lx\n",
                        context, expected->message, expected->wParam, actual->wParam);
                    if (expected->wParam != actual->wParam) dump++;
                }
            }

            if (expected->flags & lparam)
            {
                if (expected->lParam != actual->lParam && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        dump++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                            context, expected->message, expected->lParam, actual->lParam);
                    }
                }
                else
                {
                    ok_(file, line) (expected->lParam == actual->lParam,
                        "%s: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                        context, expected->message, expected->lParam, actual->lParam);
                    if (expected->lParam != actual->lParam) dump++;
                }
            }

            if (expected->flags & custdraw)
            {
                if (expected->stage != actual->stage && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        dump++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting cd stage 0x%08x got 0x%08x\n",
                            context, expected->message, expected->stage, actual->stage);
                    }
                }
                else
                {
                    ok_(file, line) (expected->stage == actual->stage,
                        "%s: in msg 0x%04x expecting cd stage 0x%08x got 0x%08x\n",
                        context, expected->message, expected->stage, actual->stage);
                    if (expected->stage != actual->stage) dump++;
                }
            }

            if (expected->flags & id)
            {
                if (expected->id != actual->id && expected->flags & optional)
                {
                    expected++;
                    continue;
                }
                if (expected->id != actual->id && todo)
                {
                    todo_wine
                    {
                        failcount++;
                        dump++;
                        ok_(file, line) (FALSE,
                            "%s: in msg 0x%04x expecting id 0x%x got 0x%x\n",
                            context, expected->message, expected->id, actual->id);
                    }
                }
                else
                {
                    ok_(file, line) (expected->id == actual->id,
                        "%s: in msg 0x%04x expecting id 0x%x got 0x%x\n",
                        context, expected->message, expected->id, actual->id);
                    if (expected->id != actual->id) dump++;
                }
            }

            if ((expected->flags & defwinproc) != (actual->flags & defwinproc) && todo)
            {
                todo_wine
                {
                    failcount++;
                    dump++;
                    ok_(file, line) (FALSE,
                        "%s: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                        context, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
                }
            }
            else
            {
                ok_(file, line) ((expected->flags & defwinproc) == (actual->flags & defwinproc),
                    "%s: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                    context, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
                if ((expected->flags & defwinproc) != (actual->flags & defwinproc)) dump++;
            }

            ok_(file, line) ((expected->flags & beginpaint) == (actual->flags & beginpaint),
                "%s: the msg 0x%04x should %shave been sent by BeginPaint\n",
                context, expected->message, (expected->flags & beginpaint) ? "" : "NOT ");
            if ((expected->flags & beginpaint) != (actual->flags & beginpaint)) dump++;

            ok_(file, line) ((expected->flags & (sent|posted)) == (actual->flags & (sent|posted)),
                "%s: the msg 0x%04x should have been %s\n",
                context, expected->message, (expected->flags & posted) ? "posted" : "sent");
            if ((expected->flags & (sent|posted)) != (actual->flags & (sent|posted))) dump++;

            ok_(file, line) ((expected->flags & parent) == (actual->flags & parent),
                "%s: the msg 0x%04x was expected in %s\n",
                context, expected->message, (expected->flags & parent) ? "parent" : "child");
            if ((expected->flags & parent) != (actual->flags & parent)) dump++;

            ok_(file, line) ((expected->flags & hook) == (actual->flags & hook),
                "%s: the msg 0x%04x should have been sent by a hook\n",
                context, expected->message);
            if ((expected->flags & hook) != (actual->flags & hook)) dump++;

            ok_(file, line) ((expected->flags & winevent_hook) == (actual->flags & winevent_hook),
                "%s: the msg 0x%04x should have been sent by a winevent hook\n",
                context, expected->message);
            if ((expected->flags & winevent_hook) != (actual->flags & winevent_hook)) dump++;

            expected++;
            actual++;
        }
        else if (expected->flags & optional)
            expected++;
        else if (todo)
        {
            failcount++;
            dump++;
            todo_wine
            {
                ok_(file, line) (FALSE, "%s: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                    context, expected->message, actual->message);
            }
            goto done;
        }
        else
        {
            ok_(file, line) (FALSE, "%s: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                context, expected->message, actual->message);
            dump++;
            expected++;
            actual++;
        }
    }

    /* skip all optional trailing messages */
    while (expected->message && ((expected->flags & optional)))
        expected++;

    if (todo)
    {
        todo_wine
        {
            if (expected->message || actual->message)
            {
                failcount++;
                dump++;
                ok_(file, line) (FALSE, "%s: the msg sequence is not complete: expected %04x - actual %04x\n",
                    context, expected->message, actual->message);
            }
        }
    }
    else if (expected->message || actual->message)
    {
        dump++;
        ok_(file, line) (FALSE, "%s: the msg sequence is not complete: expected %04x - actual %04x\n",
            context, expected->message, actual->message);
    }

    if(todo && !failcount) /* succeeded yet marked todo */
    {
        if (!strcmp(winetest_platform, "wine")) dump++;
        todo_wine
        {
            ok_(file, line)(TRUE, "%s: marked \"todo_wine\" but succeeds\n", context);
        }
    }

done:
    if (dump) dump_sequence( seq, sequence_index, expected_list, context, file, line );
    flush_sequence(seq, sequence_index);
}

#define ok_sequence(seq, index, exp, contx, todo) \
        ok_sequence_(seq, index, (exp), (contx), (todo), __FILE__, __LINE__)


static void init_msg_sequences(struct msg_sequence **seq, int n)
{
    int i;

    for (i = 0; i < n; i++)
        seq[i] = heap_alloc_zero(sizeof(*seq[i]));
}
