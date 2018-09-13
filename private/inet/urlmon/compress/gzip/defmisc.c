//
// defmisc.c
//
#include "deflate.h"
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>


//
// Fix the frequency data of the provided literal and distance trees such that no
// element has a zero frequency.  We must never allow the cumulative frequency of
// either tree to be >= 65536, so we divide all of the frequencies by two to make
// sure.
//
void NormaliseFrequencies(USHORT *literal_tree_freq, USHORT *dist_tree_freq)
{
	int i;

	// don't allow any zero frequency items to exist
	// also make sure we don't overflow 65535 cumulative frequency
	for (i = 0; i < MAX_DIST_TREE_ELEMENTS; i++)
	{
		// avoid overflow
		dist_tree_freq[i] >>= 1;

		if (dist_tree_freq[i] == 0)
			dist_tree_freq[i] = 1;
	}

	for (i = 0; i < MAX_LITERAL_TREE_ELEMENTS; i++)
	{
		// avoid overflow
		literal_tree_freq[i] >>= 1;

		if (literal_tree_freq[i] == 0)
			literal_tree_freq[i] = 1;
	}
}
