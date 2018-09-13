/*
 * fstdebug.c
 *
 * Debugging stubs for fast encoder
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


#ifdef FULL_DEBUG
void FastEncoderVerifyHashes(t_encoder_context *context, long bufpos)
{
	int i;
	const t_search_node *lookup = context->fast_encoder->lookup;
	const t_search_node *prev = context->fast_encoder->prev;
	const BYTE *window = context->fast_encoder->window;

	for (i = 0; i < FAST_ENCODER_HASH_TABLE_SIZE; i++)
	{
		t_search_node where = lookup[i];
		t_search_node next_where;

		while (where != 0 && bufpos - where < FAST_ENCODER_WINDOW_SIZE)
		{
			int hash = FAST_ENCODER_RECALCULATE_HASH(where);

			_ASSERT(hash == i);

			next_where = prev[where & FAST_ENCODER_WINDOW_MASK];

			if (bufpos - next_where >= FAST_ENCODER_WINDOW_SIZE)
				break;

			_ASSERT(next_where < where);

			where = next_where;
		} 
	}
}


void FastEncoderVerifyHashChain(t_encoder_context *context, long bufpos, int chain_number)
{
	const t_search_node *lookup = context->fast_encoder->lookup;
	const t_search_node *prev = context->fast_encoder->prev;
	BYTE *window = context->fast_encoder->window;
	t_search_node where;
	t_search_node next_where;
	int print = 0;

top:
	where = lookup[chain_number];

	if (print)
		printf("Verify chain %d\n", chain_number);

	while (where != 0 && bufpos - where < FAST_ENCODER_WINDOW_SIZE)
	{
		int hash = FAST_ENCODER_RECALCULATE_HASH(where);
        BYTE *window = context->fast_encoder->window;

		if (print)
			printf("   loc %d: char = %3d %3d %3d\n", where, window[where], window[where+1], window[where+2]);

		if (hash != chain_number && print == 0)
		{
			print = 1;
			goto top;
		}

		_ASSERT(hash == chain_number);

		next_where = prev[where & FAST_ENCODER_WINDOW_MASK];

		if (bufpos - next_where >= FAST_ENCODER_WINDOW_SIZE)
			break;

		if (next_where >= where && print == 0)
		{
			print = 1;
			goto top;
		}

		_ASSERT(next_where < where);

		where = next_where;
	}
}
#endif

