/*
 * optfmtch.c
 *
 * Match finder for the optimal parser
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


#define VERIFY_SEARCH_CODE(routine_name) \
{ \
	int debug_search; \
	for (debug_search = 0; debug_search < clen; debug_search++) \
	{ \
		if (window[ptr+debug_search] != window[BufPos+debug_search]) \
		{ \
			_RPT2( \
				_CRT_WARN, \
				routine_name \
				" char mismatch @%3d (clen=%d)\n", \
				debug_search, clen); \
			\
			_RPT3( \
				_CRT_WARN, \
				" ptr=%8d, bufpos=%8d, end_pos=%8d\n\n", \
				ptr, BufPos, end_pos); \
			_ASSERT(0); \
		} \
	} \
}


#define VERIFY_MULTI_TREE_SEARCH_CODE(routine_name) \
	_ASSERT(window[BufPos] == window[ptr]); \
	_ASSERT(window[BufPos+1] == window[ptr+1]);


/*
 * Finds the closest matches of all possible lengths, MIN_MATCH <= x <= MAX_MATCH,
 * at position BufPos.
 *
 * The positions of each match location are stored in context->matchpos_table[]
 *
 * Returns the longest such match length found, or zero if no matches found.
 */
int optimal_find_match(t_encoder_context *context, long BufPos)
{
	ULONG		ptr;
	ULONG       a, b;
	t_search_node *small_ptr, *big_ptr;
	t_search_node *left = context->optimal_encoder->search_left;
	t_search_node *right = context->optimal_encoder->search_right;
	t_match_pos *matchpos_table = context->optimal_encoder->matchpos_table;
	BYTE *window = context->optimal_encoder->window;
	ULONG       end_pos;
	int         val; /* must be signed */
	int         clen;
	int         same;
	int         match_length;
	int         small_len, big_len;
	USHORT      tree_to_use;

	/*
	 * Retrieve root node of tree to search, and insert current node at
	 * the root.
	 */
	tree_to_use = *((USHORT UNALIGNED *) &window[BufPos]);
	
	ptr        = context->optimal_encoder->search_tree_root[tree_to_use];
	context->optimal_encoder->search_tree_root[tree_to_use] = (t_search_node) BufPos;

	/*
	 * end_pos is the furthest location back we will search for matches 
	 *
	 * Remember that our window size is reduced by 3 bytes because of
	 * our repeated offset codes.
	 *
	 * Since BufPos starts at WINDOW_SIZE when compression begins,
	 * end_pos will never become negative.  
	 */
	end_pos = BufPos - (WINDOW_SIZE-4);

	/*
	 * Root node is either NULL, or points to a really distant position.
	 */
	if (ptr <= end_pos)
	{
		left[BufPos] = right[BufPos] = 0;
		return 0;
	}

	/*
	 * confirmed length (no need to check the first clen chars in a search)
	 *
	 * note: clen is always equal to min(small_len, big_len)
	 */
	clen            = 2;

	/*
	 * current best match length
	 */
	match_length    = 2;

	/*
	 * longest match which is < our string
	 */
	small_len       = 2;

	/*
	 * longest match which is > our string
	 */
	big_len         = 2;

#ifdef _DEBUG
	VERIFY_MULTI_TREE_SEARCH_CODE("binary_search_findmatch()");
#endif

	/*
	 * pointers to nodes to check
	 */
	small_ptr             = &left[BufPos];
	big_ptr               = &right[BufPos];

	do
	{
		/* compare bytes at current node */
		same = clen;

#ifdef _DEBUG
		VERIFY_SEARCH_CODE("optimal_findmatch()")
#endif

		/* don't need to check first clen characters */
		a    = ptr + clen;
		b    = BufPos + clen;

		while ((val = ((int) window[a++]) - ((int) window[b++])) == 0)
		{
			/* don't exceed MAX_MATCH */
			if (++same >= MAX_MATCH)
				goto long_match;
		}

		if (val < 0)
		{
			if (same > big_len)
			{
				if (same > match_length)
				{
long_match:
					do
					{
						matchpos_table[++match_length] = BufPos-ptr-1;
					} while (match_length < same);

					if (same >= BREAK_LENGTH)
					{
						*small_ptr = left[ptr];
						*big_ptr   = right[ptr];
						goto end_bsearch;
					}
				}

				big_len = same;
				clen = min(small_len, big_len);
			}

			*big_ptr = (t_search_node) ptr;
			big_ptr  = &left[ptr];
			ptr      = *big_ptr;
		}
		else
		{
			if (same > small_len)
			{
				if (same > match_length)
				{
					do
					{
						matchpos_table[++match_length] = BufPos-ptr-1;
					} while (match_length < same);

					if (same >= BREAK_LENGTH)
					{
						*small_ptr = left[ptr];
						*big_ptr   = right[ptr];
						goto end_bsearch;
					}
				}

				small_len = same;
				clen = min(small_len, big_len);
			}
		
			*small_ptr = (t_search_node) ptr;
			small_ptr  = &right[ptr];
			ptr        = *small_ptr;
		}
	} while (ptr > end_pos); /* while we don't go too far backwards */

	*small_ptr = 0;
	*big_ptr   = 0;


end_bsearch:

	/*
	 * If we have multiple search trees, we are already guaranteed
	 * a minimum match length of 2 when we reach here.
	 *
	 * If we only have one tree, then we're not guaranteed anything.
	 */
    if (match_length < MIN_MATCH)
        return 0;
    else
	    return (long) match_length;
}


/*
 * Inserts the string at the current BufPos into the tree.
 *
 * Does not record all the best match lengths or otherwise attempt
 * to search for matches
 *
 * Similar to the above function.
 */
void optimal_insert(t_encoder_context *context, long BufPos, long end_pos)
{
	long        ptr;
	ULONG       a,b;
	t_search_node *small_ptr, *big_ptr;
	t_search_node *left = context->optimal_encoder->search_left;
	t_search_node *right = context->optimal_encoder->search_right;
	BYTE *window = context->optimal_encoder->window;
	int         val;
	int         small_len, big_len;
	int         same;
	int         clen;
	USHORT      tree_to_use;

	tree_to_use = *((USHORT UNALIGNED *) &window[BufPos]);
	ptr        = context->optimal_encoder->search_tree_root[tree_to_use];
	context->optimal_encoder->search_tree_root[tree_to_use] = (t_search_node) BufPos;

	if (ptr <= end_pos)
	{
		left[BufPos] = right[BufPos] = 0;
		return;
	}

	clen            = 2;
	small_len       = 2;
	big_len         = 2;

#ifdef _DEBUG
	VERIFY_MULTI_TREE_SEARCH_CODE("quick_insert_bsearch_findmatch()");
#endif

	small_ptr       = &left[BufPos];
	big_ptr         = &right[BufPos];

	do
	{
		same = clen;

		a    = ptr+clen;
		b    = BufPos+clen;

#ifdef _DEBUG
		VERIFY_SEARCH_CODE("quick_insert_bsearch_findmatch()")
#endif

		while ((val = ((int) window[a++]) - ((int) window[b++])) == 0)
		{
			/*
			 * Here we break on BREAK_LENGTH, not MAX_MATCH
			 */
			if (++same >= BREAK_LENGTH) 
				break;
		}

		if (val < 0)
		{
			if (same > big_len)
			{
				if (same >= BREAK_LENGTH)
				{
					*small_ptr = left[ptr];
					*big_ptr = right[ptr];
					return;
				}

				big_len = same;
				clen = min(small_len, big_len);
			}
			
			*big_ptr = (t_search_node) ptr;
			big_ptr  = &left[ptr];
			ptr      = *big_ptr;
		}
		else
		{
			if (same > small_len)
			{
				if (same >= BREAK_LENGTH)
				{
					*small_ptr = left[ptr];
					*big_ptr = right[ptr];
					return;
				}

				small_len = same;
				clen = min(small_len, big_len);
			}

			*small_ptr = (t_search_node) ptr;
			small_ptr  = &right[ptr];
			ptr        = *small_ptr;
		}
   } while (ptr > end_pos);

	*small_ptr = 0;
	*big_ptr   = 0;
}


/*
 * Remove a node from the search tree; this is ONLY done for the last
 * BREAK_LENGTH symbols (see optenc.c).  This is because we will have
 * inserted strings that contain undefined data (e.g. we're at the 4th
 * last byte from the file and binary_search_findmatch() a string into
 * the tree - everything from the 4th symbol onwards is invalid, and
 * would cause problems if it remained in the tree, so we have to
 * remove it).
 */
void optimal_remove_node(t_encoder_context *context, long BufPos, ULONG end_pos)
{
	ULONG   ptr;
	ULONG   left_node_pos;
	ULONG   right_node_pos;
	USHORT  tree_to_use;
	t_search_node *link;
	t_search_node *left = context->optimal_encoder->search_left;
	t_search_node *right = context->optimal_encoder->search_right;
	BYTE *window = context->optimal_encoder->window;

	/*
	 * The root node of tree_to_use should equal BufPos, since that is
	 * the most recent insertion into that tree - but if we never
	 * inserted this string (because it was a near match or a long
	 * string of zeroes), then we can't remove it.
	 */
	tree_to_use = *((USHORT UNALIGNED *) &window[BufPos]);


	/*
	 * If we never inserted this string, do not attempt to remove it
	 */

	if (context->optimal_encoder->search_tree_root[tree_to_use] != BufPos)
		return;

	link = &context->optimal_encoder->search_tree_root[tree_to_use];

	/*
	 * If the last occurence was too far away
	 */
	if (*link <= end_pos)
	{
		*link = 0;
		left[BufPos] = right[BufPos] = 0;
		return;
	}

	/*
	 * Most recent location of these chars
	 */
	ptr             = BufPos;

	/*
	 * Most recent location of a string which is "less than" it
	 */
	left_node_pos   = left[ptr];

	if (left_node_pos <= end_pos)
		left_node_pos = left[ptr] = 0;

	/*
	 * Most recent location of a string which is "greater than" it
	 */
	right_node_pos  = right[ptr];

	if (right_node_pos <= end_pos)
		right_node_pos = right[ptr] = 0;

	while (1)
	{
		/*
		 * If left node position is greater than right node position
		 * then follow the left node, since that is the more recent
		 * insertion into the tree.  Otherwise follow the right node.
		 */
		if (left_node_pos > right_node_pos)
		{
			/*
			 * If it's too far away, then store that it never happened
			 */
			if (left_node_pos <= end_pos)
				left_node_pos = 0;

			ptr = *link = (t_search_node) left_node_pos;

			if (!ptr)
				break;

			left_node_pos   = right[ptr];
			link            = &right[ptr];
		}
		else
		{
			/*
			 * If it's too far away, then store that it never happened
			 */
			if (right_node_pos <= end_pos)
				right_node_pos = 0;

			ptr = *link = (t_search_node) right_node_pos;

			if (!ptr) 
				break;

			right_node_pos  = left[ptr];
			link            = &left[ptr];
		}
	}
}


void removeNodes(t_encoder_context *context)
{
	long i;

	// remove the most recent insertions into the hash table, since we had invalid data 
	// sitting at the end of the window
	for (i = 0; i <= BREAK_LENGTH; i++)
	{
		if (context->bufpos-i-1 < WINDOW_SIZE)
			break;

		optimal_remove_node(context, context->bufpos-i-1, context->bufpos-WINDOW_SIZE+BREAK_LENGTH);
	}
}


//
// Reinsert the tree nodes we removed previously
//
void reinsertRemovedNodes(t_encoder_context *context)
{
	long j;

	for (j = BREAK_LENGTH; j > 0; j--)
	{
		if (context->bufpos - j > WINDOW_SIZE)
		{
			optimal_insert(
				context,
	            context->bufpos - j,
		        context->bufpos - j - WINDOW_SIZE + 4
			);
		}
	}
}


