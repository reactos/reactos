/*
 * stddebug.c
 *
 * Debugging stubs for std encoder
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


#ifdef FULL_DEBUG
// verify all hash chains
void StdEncoderVerifyHashes(t_encoder_context *context, long bufpos)
{
	int i;
	const t_search_node *lookup = context->std_encoder->lookup;
	const t_search_node *prev = context->std_encoder->prev;
	const BYTE *window = context->std_encoder->window;

	for (i = 0; i < STD_ENCODER_HASH_TABLE_SIZE; i++)
	{
		t_search_node where = lookup[i];
		t_search_node next_where;

		while (where != 0 && bufpos - where < WINDOW_SIZE)
		{
			int hash = STD_ENCODER_RECALCULATE_HASH(where);

			_ASSERT(hash == i);

			next_where = prev[where & WINDOW_MASK];

			if (bufpos - next_where >= WINDOW_SIZE)
				break;

			_ASSERT(next_where < where);

			where = next_where;
		} 
	}
}


// verify that a particular hash chain is correct
void StdEncoderVerifyHashChain(t_encoder_context *context, long bufpos, int chain_number)
{
	const t_search_node *lookup = context->std_encoder->lookup;
	const t_search_node *prev = context->std_encoder->prev;
	BYTE *window = context->std_encoder->window;
	t_search_node where;
	t_search_node next_where;
	int print = 0;

top:
	where = lookup[chain_number];

//	if (print)
//		printf("Verify chain %d\n", chain_number);

	while (where != 0 && bufpos - where < WINDOW_SIZE)
	{
		int hash = STD_ENCODER_RECALCULATE_HASH(where);
        BYTE *window = context->std_encoder->window;

//		if (print)
//			printf("   loc %d: char = %3d %3d %3d\n", where, window[where], window[where+1], window[where+2]);

		if (hash != chain_number && print == 0)
		{
			print = 1;
			goto top;
		}

		_ASSERT(hash == chain_number);

		next_where = prev[where & WINDOW_MASK];

		if (bufpos - next_where >= WINDOW_SIZE)
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

