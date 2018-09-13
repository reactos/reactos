// template qsort adapted from  msdev\crt\src\qsort.c
// created 7/96  bobp

// template<class T>
// void QSort (T *base, unsigned nEl, BOOL fSortUp)
// 
// quicksort array of T
//
// Uses class T member functions:
//	operator =
//  operator <=

#ifndef __TQSORT_INCL
#define __TQSORT_INCL

template<class T>
inline void Swap(T &a, T &b)
{
	T x = a; a = b; b = x;
}

template<class T>
static void __cdecl ShortSort (T *lo, T *hi, BOOL fUp)
{
    T *p, *max;

    while (hi > lo) {
        max = lo;
		if (fUp) {
			for (p = lo+1; p <= hi; p++) {
				if ( !(*p <= *max) )
					max = p;
			}
		} else {
			for (p = lo+1; p <= hi; p++) {
				if ( !(*max <= *p) )
					max = p;
			}
		}

		Swap (*max, *hi);

        hi --;
    }
}

#define CUTOFF 8            /* testing shows that this is good value */

template<class T>
void QSort (T *base, unsigned nEl, BOOL fUp)
{
    T *lo, *hi;              /* ends of sub-array currently sorting */
    T *mid;                  /* points to middle of subarray */
    T *loguy, *higuy;        /* traveling pointers for partition step */
    unsigned size;           /* size of the sub-array */
    T *lostk[30], *histk[30];
    int stkptr;              /* stack for saving sub-array to be processed */

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (nEl < 2)
        return;                 /* nothing to do */

    stkptr = 0;                 /* initialize stack */

    lo = base;
    hi = base + (nEl-1);        /* initialize limits */

recurse:

    size = (int)(hi - lo) + 1;   /* number of el's to sort */

    if (size <= CUTOFF) {
         ShortSort(lo, hi, fUp);
    }
    else {
        mid = lo + (size / 2);   /* find middle element */
        Swap(*mid, *lo);         /* swap it to beginning of array */
        loguy = lo;
        higuy = hi + 1;

        for (;;) {
			if (fUp) {
				do  {
					loguy ++;
				} while (loguy <= hi && *loguy <= *lo);

				do  {
					higuy --;
				} while (higuy > lo && *lo <= *higuy);
			} else {
				do  {
					loguy ++;
				} while (loguy <= hi && *lo <= *loguy);

				do  {
					higuy --;
				} while (higuy > lo && *higuy <= *lo);
			}

            if (higuy < loguy)
                break;

            Swap(*loguy, *higuy);
        }

        Swap(*lo, *higuy);     /* put partition element in place */

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + 1 < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - 1;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo + 1 < higuy) {
                hi = higuy - 1;
                goto recurse;           /* do small recursion */
            }
        }
    }

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
}

#endif
