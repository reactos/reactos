#ifndef __TWO_INCLUDED__
#define __TWO_INCLUDED__

#ifndef __SHARED_INCLUDED__
#include "shared.h"
#endif

// return largest i such that 2^i <= u
inline int lg(unsigned u) {
	unsigned u0 = u;
	precondition(u > 0);

	int n = 0;
#define t(j) if (u >= (1 << (j))) { u >>= (j); n += (j); }
	t(16); t(8); t(4); t(2); t(1);
#undef t

	postcondition(u == 1);
	postcondition((1U << n) <= u0);
	postcondition(n == 31 || u0 < (1U << (n+1)));
	return n;
}

// return smallest n such that n = 2^i and u <= n
inline unsigned nextPowerOfTwo(unsigned u) {
	precondition(u > 0);

	int lgu = lg(u);
	int lguRoundedUp = lg(u + (1 << lgu) - 1);
	unsigned n = 1 << lguRoundedUp;

	postcondition(n/2 < u && u <= n);
	return n;
	// examples:
	// u lgu lgRU n
	// 1   0   0  1
	// 4   2   2  4
	// 5   2   3  8
	// 7   2   3  8
	// 8   3   3  8
}

inline unsigned nextMultiple(unsigned u, unsigned m) {
	return (u + m - 1) / m * m;
}

inline int nextMultiple(int i, unsigned m) {
	return (i + m - 1) / m * m;
}

// Return number of set bits in the word.
inline unsigned bitcount(unsigned u) {
	// In-place adder tree: perform 16 1-bit adds, 8 2-bit adds, 4 4-bit adds,
	// 2 8=bit adds, and 1 16-bit add.
	u = ((u >> 1)&0x55555555) + (u&0x55555555);
	u = ((u >> 2)&0x33333333) + (u&0x33333333);
	u = ((u >> 4)&0x0F0F0F0F) + (u&0x0F0F0F0F);
	u = ((u >> 8)&0x00FF00FF) + (u&0x00FF00FF);
	u = ((u >>16)&0x0000FFFF) + (u&0x0000FFFF);
	return u;
}

#endif // !__TWO_INCLUDED__
