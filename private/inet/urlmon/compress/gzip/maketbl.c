//
// maketbl.c
//
// Creates Huffman decoding tables
//
#include <windows.h>
#include <crtdbg.h>
#include "common.h"
#include "maketbl.h"


//
// Reverse the bits, len > 0
//
static unsigned int bitReverse(unsigned int code, int len)
{
	unsigned int new_code = 0;

    _ASSERT(len > 0);

	do
	{
		new_code |= (code & 1);
		new_code <<= 1;
		code >>= 1;
	} while (--len > 0);

	return new_code >> 1;
}


BOOL makeTable(
	int				num_elements, 
	int				table_bits, 
	const byte *	code_length, 
	short *			table, 
	short *			left, 
	short *			right
)
{
	int				bl_count[17];
	unsigned int	next_code[17];
	unsigned int	code[MAX_LITERAL_TREE_ELEMENTS];
	int				temp_code;
	int				avail;
	int				i, bits, ch;
	int				table_size, table_mask;

	table_size = 1 << table_bits;
	table_mask = table_size - 1;

	for (i = 0; i <= 16; i++)
		bl_count[i] = 0;

	for (i = 0; i < num_elements; i++)
		bl_count[ code_length[i] ]++;

	//
	// If there are any codes larger than table_bits in length, then
	// we will have to clear the table for our left/right spillover
    // code to work correctly.
	//
	// If there aren't any codes that large, then all table entries
	// will be written over without being read, so we don't need to
	// initialise them
	//
	for (i = table_bits; i <= 16; i++)
	{
		if (bl_count[i] > 0)
		{
			int j;

			// found a code larger than table_bits
			for (j = 0; j < table_size; j++)
				table[j] = 0;

			break;
		}
	}

	temp_code	= 0;
	bl_count[0] = 0;

	for (bits = 1; bits <= 16; bits++)
	{
		temp_code = (temp_code + bl_count[bits-1]) << 1;
		next_code[bits] = temp_code;
	}

	for (i = 0; i < num_elements; i++)
	{
		int len = code_length[i];

		if (len > 0)
		{
			code[i] = bitReverse(next_code[len], len);
			next_code[len]++;
		}
	}

	avail = num_elements;

	for (ch = 0; ch < num_elements; ch++)
	{
		int	start_at, len;

		// length of this code
		len = code_length[ch];

		// start value (bit reversed)
		start_at = code[ch];

		if (len > 0)
		{
			if (len <= table_bits)
			{
				int locs = 1 << (table_bits - len);
				int increment = 1 << len;
				int j;

				// 
				// Make sure that in the loop below, start_at is always
				// less than table_size.
				//
				// On last iteration we store at array index:
				//    initial_start_at + (locs-1)*increment
				//  = initial_start_at + locs*increment - increment
				//  = initial_start_at + (1 << table_bits) - increment
				//  = initial_start_at + table_size - increment
				//
				// Therefore we must ensure:
				//     initial_start_at + table_size - increment < table_size
				// or: initial_start_at < increment
				//
				if (start_at >= increment)
					return FALSE; // invalid table!

				for (j = 0; j < locs; j++)
				{
					table[start_at] = (short) ch;
					start_at += increment;
				}
			}
			else
			{
				int		overflow_bits;
				int		code_bit_mask;
				short *	p;

				overflow_bits = len - table_bits;
				code_bit_mask = 1 << table_bits;

				p = &table[start_at & table_mask];

				do
				{
					short value;

					value = *p;

					if (value == 0)
					{
						left[avail]		= 0;
						right[avail]	= 0;

						*p = -avail;

						value = -avail;
						avail++;
					}

					if ((start_at & code_bit_mask) == 0)
						p = &left[-value];
					else
						p = &right[-value];

					code_bit_mask <<= 1;
					overflow_bits--;
				} while (overflow_bits != 0);

				*p = (short) ch;
			}
		}
	}

	return TRUE;
}
