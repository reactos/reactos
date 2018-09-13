/*
 * optdebug.c
 *
 * Optimal encoder debugging stubs
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


#ifdef _DEBUG
static void OptimalEncoderVerifyTreeStructure(t_encoder_context *context, byte val1, byte val2, long where)
{
	long left, right;

	if (where == 0)
		return;

	_ASSERT(context->optimal_encoder->window[where] == val1);
	_ASSERT(context->optimal_encoder->window[where+1] == val2);

	left = context->optimal_encoder->search_left[where];
	right = context->optimal_encoder->search_right[where];

	OptimalEncoderVerifyTreeStructure(context, val1, val2, left);
	OptimalEncoderVerifyTreeStructure(context, val1, val2, right);
}


void OptimalEncoderVerifyHashes(t_encoder_context *context, long bufpos)
{
	long i;

	for (i = 0; i < NUM_DIRECT_LOOKUP_TABLE_ELEMENTS; i++)
	{
		long	where = context->optimal_encoder->search_tree_root[i];
		USHORT	tree_to_use;

		if (where == 0)
			continue;

		tree_to_use = *((USHORT UNALIGNED *) &context->optimal_encoder->window[where]);

		_ASSERT(where < bufpos);
		_ASSERT(tree_to_use == i);

		OptimalEncoderVerifyTreeStructure(context, context->optimal_encoder->window[where], context->optimal_encoder->window[where+1], where);
	}
}
#endif
