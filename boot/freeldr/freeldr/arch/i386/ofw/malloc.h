// See license at end of file

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * A  "smarter" malloc				William L. Sebok
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define	MAGIC_FREE	0x548a934c
#define	MAGIC_BUSY	0xc139569a

#define NBUCKETS	24
#define NALIGN		sizeof(long)

struct qelem {
	struct qelem *q_forw;
	struct qelem *q_back;
};

struct overhead {
	struct qelem	ov_adj;		/* adjacency chain pointers */ 
	struct qelem	ov_buk;		/* bucket chain pointers */
	long		ov_magic;
	unsigned long	ov_length;
};

#ifdef MALLOC
char endfree = 0;
struct qelem adjhead = { &adjhead, &adjhead };
struct qelem buckets[NBUCKETS] = {
	&buckets[0],  &buckets[0],
	&buckets[1],  &buckets[1],
	&buckets[2],  &buckets[2],
	&buckets[3],  &buckets[3],
	&buckets[4],  &buckets[4],
	&buckets[5],  &buckets[5],
	&buckets[6],  &buckets[6],
	&buckets[7],  &buckets[7],
	&buckets[8],  &buckets[8],
	&buckets[9],  &buckets[9],
	&buckets[10], &buckets[10],
	&buckets[11], &buckets[11],
	&buckets[12], &buckets[12],
	&buckets[13], &buckets[13],
	&buckets[14], &buckets[14],
	&buckets[15], &buckets[15],
	&buckets[16], &buckets[16],
	&buckets[17], &buckets[17],
	&buckets[18], &buckets[18],
	&buckets[19], &buckets[19],
	&buckets[20], &buckets[20],
	&buckets[21], &buckets[21],
	&buckets[22], &buckets[22],
	&buckets[23], &buckets[23],
};
#else
extern char endfree;
extern struct qelem adjhead, buckets[NBUCKETS];
#endif

/*
 * The following macros depend on the order of the elements in struct overhead
 */
#define TOADJ(p)	((struct qelem *)(p))
#define FROMADJ(p)	((struct overhead *)(p))
#define FROMBUK(p)	((struct overhead *)( (char *)p - sizeof(struct qelem)))
#define TOBUK(p)	((struct qelem *)( (char *)p + sizeof(struct qelem)))

#ifndef CURBRK
#define CURBRK	sbrk(0)
#endif CURBRK

extern void insque(), remque();
extern char *malloc(), *realloc();

// LICENSE_BEGIN
// Copyright (c) 2006 FirmWorks
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// LICENSE_END
