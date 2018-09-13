//
// deftree.c
//
// Tree creation for the compressor
//
#include "deflate.h"
#include <string.h>
#include <crtdbg.h>


//
// MAX_LITERAL_TREE elements is the largest number of elements that will ever be passed
// in to this routine
//
typedef struct 
{
    // Made left_right a single long array for performance reasons.  We always access them
    // one after the other, so there is no disadvantage.
    // left[] in lower 16 bits, right[] in upper 16 bits
//	short           left[2*MAX_LITERAL_TREE_ELEMENTS];
//	short           right[2*MAX_LITERAL_TREE_ELEMENTS];
    unsigned long   left_right[2*MAX_LITERAL_TREE_ELEMENTS];

	int             heap[MAX_LITERAL_TREE_ELEMENTS+1];

	int				num_elements;

	// Maximum allowable code length (7 for pre-tree, 15 for other trees)
	int				max_code_length;

	unsigned short *freq; // passed in as parameter
	unsigned short *code; // passed in as parameter
	
	short *			sortptr;
	int				depth;
	int				heapsize;
	int             len_cnt[17];
} t_tree_context;



static void countLen(t_tree_context *context, int i)  /* call with i = root */
{
	if (i < context->num_elements)
	{
		// check for max code length allowed
		context->len_cnt[(context->depth < context->max_code_length) ? context->depth : context->max_code_length]++;
	}
	else
	{
        unsigned long lr_value = context->left_right[i];

		context->depth++;
		countLen(context, lr_value & 65535); // formerly left[i]
		countLen(context, lr_value >> 16); // formerly right[i]
		context->depth--;
	}
}


static void makeLen(t_tree_context *context, int root, BYTE *len)
{
	int		k;
	int		cum;
	int		i;

	for (i = 0; i <= 16; i++)
		context->len_cnt[i] = 0;

	countLen(context, root);

	cum = 0;

	for (i = context->max_code_length; i > 0; i--)
		cum += (context->len_cnt[i] << (context->max_code_length - i));

	while (cum != (1 << context->max_code_length))
	{
		context->len_cnt[context->max_code_length]--;

		for (i = context->max_code_length-1; i > 0; i--)
		{
			if (context->len_cnt[i] != 0)
			{
				context->len_cnt[i]--;
				context->len_cnt[i+1] += 2;
				break;
			}
		}

		cum--;
	}

	for (i = 16; i > 0; i--)
	{
		k = context->len_cnt[i];

		while (--k >= 0)
			len[ *context->sortptr++ ] = (byte) i;
	}
}


/* priority queue; send i-th entry down heap */
static void downHeap(t_tree_context *context, int i)
{
	int j, k;

	k = context->heap[i];

	while ((j = (i<<1)) <= context->heapsize)
	{
		if (j < context->heapsize && 
			context->freq[context->heap[j]] > context->freq[context->heap[j + 1]])
	 		j++;

		if (context->freq[k] <= context->freq[context->heap[j]])
			break;

		context->heap[i] = context->heap[j];
		i = j;
	}

	context->heap[i] = k;
}


//
// Reverse the bits, len > 0
//
static unsigned int bitReverse(unsigned int code, int len)
{
	unsigned int new_code = 0;

	do
	{
		new_code |= (code & 1);
		new_code <<= 1;
		code >>= 1;

	} while (--len > 0);

	return new_code >> 1;
}


void makeCode(int num_elements, const int *len_cnt, const BYTE *len, USHORT *code)
{
	int start[18];
	int i;
	
	start[1] = 0;

	for (i = 1; i <= 16; i++)
		start[i + 1] = (start[i] + len_cnt[i]) << 1;

	for (i = 0; i < num_elements; i++)
	{
		unsigned int unreversed_code;
		
		unreversed_code = start[len[i]]++;
		code[i] = (USHORT) bitReverse(unreversed_code, len[i]);
	}
}


void makeTree(
	int					num_elements,
	int					max_code_length,
	unsigned short *	freq,
	unsigned short *	code,
	byte *				len
)
{
	t_tree_context	tree;
	int				k;
	int				avail;
	int				i;

    _ASSERT(num_elements <= MAX_LITERAL_TREE_ELEMENTS);

	// init tree context
	tree.depth	= 0;
	tree.freq	= freq;
	tree.code	= code;
	tree.num_elements = num_elements;
	tree.max_code_length = max_code_length;

	avail				= num_elements;
	tree.heapsize		= 0;
	tree.heap[1]		= 0;

	for (i = 0; i < tree.num_elements; i++)
	{
		len[i] = 0;

		if (tree.freq[i] != 0)
			tree.heap[++tree.heapsize] = i;
	}

	//
	// Less than 2 elements in the tree?
	//
	if (tree.heapsize < 2)
	{
		if (tree.heapsize == 0)
		{
			//
			// No elements in the tree?
			//
			// Then insert two fake elements and retry.
			//
			tree.freq[0] = 1;
			tree.freq[1] = 1;
		}	
		else
		{
			//
			// One element in the tree, so add a fake code
			//
			// If our only element is element #0 (heap[1] == 0), then
			// make element #1 have a frequency of 1.
			//
			// Else make element #0 have a frequency of 1.
			//
			if (tree.heap[1] == 0)
				tree.freq[1] = 1;
			else
				tree.freq[0] = 1;
		}

		//
		// Retry with these new frequencies
		//
		makeTree(num_elements, max_code_length, freq, code, len);
		return;
	}

	for (i = tree.heapsize >> 1; i >= 1; i--)
		downHeap(&tree, i);  /* make priority queue */

	tree.sortptr = tree.code;

	do
	{
		int i, j;

		/* while queue has at least two entries */
		i = tree.heap[1];  /* take out least-freq entry */

		if (i < tree.num_elements)
			*tree.sortptr++ = (short) i; 

		tree.heap[1] = tree.heap[tree.heapsize--];
		downHeap(&tree, 1);

		j = tree.heap[1];  /* next least-freq entry */

		if (j < tree.num_elements)
			*tree.sortptr++ = (short) j; 

		k = avail++;  /* generate new node */

		tree.freq[k] = tree.freq[i] + tree.freq[j];
		tree.heap[1] = k;
		downHeap(&tree, 1);  /* put into queue */

//		tree.left[k] = (short) i;
//		tree.right[k] = (short) j;
		tree.left_right[k] = (j << 16) | i;

	} while (tree.heapsize > 1);

	tree.sortptr = tree.code;

	makeLen(&tree, k, len);
	makeCode(num_elements, tree.len_cnt, len, code);
}
