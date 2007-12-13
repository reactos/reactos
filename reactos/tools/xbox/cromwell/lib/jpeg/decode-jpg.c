/* 
 *    linux/drivers/video/bootsplash/decode-jpg.c - a tiny jpeg decoder.
 *      
 *      (w) August 2001 by Michael Schroeder, <mls@suse.de>
 *                  
 */

#include "boot.h"

#include "decode-jpg.h"

#define ISHIFT 11

#define IFIX(a) ((int)((a) * (1 << ISHIFT) + .5))
#define IMULT(a, b) (((a) * (b)) >> ISHIFT)
#define ITOINT(a) ((a) >> ISHIFT)

#ifndef __P
# define __P(x) x
#endif

/* special markers */
#define M_BADHUFF	-1
#define M_EOF		0x80

struct in {
	unsigned char *p;
	unsigned int bits;
	int left;
	int marker;

	int (*func) __P((void *));
	void *data;
};

/*********************************/
struct dec_hufftbl;
struct enc_hufftbl;

union hufftblp {
	struct dec_hufftbl *dhuff;
	struct enc_hufftbl *ehuff;
};

struct scan {
	int dc;			/* old dc value */

	union hufftblp hudc;
	union hufftblp huac;
	int next;		/* when to switch to next scan */

	int cid;		/* component id */
	int hv;			/* horiz/vert, copied from comp */
	int tq;			/* quant tbl, copied from comp */
};

/*********************************/

#define DECBITS 10		/* seems to be the optimum */

struct dec_hufftbl {
	int maxcode[17];
	int valptr[16];
	unsigned char vals[256];
	unsigned int llvals[1 << DECBITS];
};

static void decode_mcus __P((struct in *, int *, int, struct scan *, int *));
static int dec_readmarker __P((struct in *));
static void dec_makehuff __P((struct dec_hufftbl *, int *, unsigned char *));

static void setinput __P((struct in *, unsigned char *));
/*********************************/

#undef PREC
#define PREC int

static void idctqtab __P((unsigned char *, PREC *));
static void idct __P((int *, int *, PREC *, PREC, int));
static void scaleidctqtab __P((PREC *, PREC));

/*********************************/

static void initcol __P((PREC[][64]));

static void col221111 __P((int *, unsigned char *, int));
static void col221111_16 __P((int *, unsigned char *, int));

/*********************************/

#define M_SOI	0xd8
#define M_APP0	0xe0
#define M_DQT	0xdb
#define M_SOF0	0xc0
#define M_DHT   0xc4
#define M_DRI	0xdd
#define M_SOS	0xda
#define M_RST0	0xd0
#define M_EOI	0xd9
#define M_COM	0xfe

static unsigned char *datap;

static int getbyte(void)
{
	return *datap++;
}

static int getword(void)
{
	int c1, c2;
	c1 = *datap++;
	c2 = *datap++;
	return c1 << 8 | c2;
}

struct comp {
	int cid;
	int hv;
	int tq;
};

#define MAXCOMP 4
struct jpginfo {
	int nc;			/* number of components */
	int ns;			/* number of scans */
	int dri;		/* restart interval */
	int nm;			/* mcus til next marker */
	int rm;			/* next restart marker */
};

static struct jpginfo info;
static struct comp comps[MAXCOMP];

static struct scan dscans[MAXCOMP];

static unsigned char quant[4][64];

static struct dec_hufftbl dhuff[4];

#define dec_huffdc (dhuff + 0)
#define dec_huffac (dhuff + 2)

static struct in in;

static int readtables(int till)
{
	int m, l, i, j, lq, pq, tq;
	int tc, th, tt;

	for (;;) {
		if (getbyte() != 0xff)
			return -1;
		if ((m = getbyte()) == till)
			break;

		switch (m) {
		case 0xc2:
			return 0;

		case M_DQT:
			lq = getword();
			while (lq > 2) {
				pq = getbyte();
				tq = pq & 15;
				if (tq > 3)
					return -1;
				pq >>= 4;
				if (pq != 0)
					return -1;
				for (i = 0; i < 64; i++)
					quant[tq][i] = getbyte();
				lq -= 64 + 1;
			}
			break;

		case M_DHT:
			l = getword();
			while (l > 2) {
				int hufflen[16], k;
				unsigned char huffvals[256];

				tc = getbyte();
				th = tc & 15;
				tc >>= 4;
				tt = tc * 2 + th;
				if (tc > 1 || th > 1)
					return -1;
				for (i = 0; i < 16; i++)
					hufflen[i] = getbyte();
				l -= 1 + 16;
				k = 0;
				for (i = 0; i < 16; i++) {
					for (j = 0; j < hufflen[i]; j++)
						huffvals[k++] = getbyte();
					l -= hufflen[i];
				}
				dec_makehuff(dhuff + tt, hufflen,
					     huffvals);
			}
			break;

		case M_DRI:
			l = getword();
			info.dri = getword();
			break;

		default:
			l = getword();
			while (l-- > 2)
				getbyte();
			break;
		}
	}
	return 0;
}

static void dec_initscans(void)
{
	int i;

	info.nm = info.dri + 1;
	info.rm = M_RST0;
	for (i = 0; i < info.ns; i++)
		dscans[i].dc = 0;
}

static int dec_checkmarker(void)
{
	int i;

	if (dec_readmarker(&in) != info.rm)
		return -1;
	info.nm = info.dri;
	info.rm = (info.rm + 1) & ~0x08;
	for (i = 0; i < info.ns; i++)
		dscans[i].dc = 0;
	return 0;
}

int jpeg_get_size(unsigned char *buf, int *width, int *height, int *depth)
{
  	datap = buf;
	getbyte(); 
	getbyte(); 
	readtables(M_SOF0);
	getword();
	getbyte();
        *height = getword();
	*width = getword();
	*depth = getbyte() << 3;
	return 0;
}

int jpeg_decode(buf, pic, width, height, depth, decdata)
unsigned char *buf, *pic;
int width, height, depth;
struct jpeg_decdata *decdata;
{
	int i, j, m, tac, tdc;
	int mcusx, mcusy, mx, my;
	int max[6];

	if (!decdata)
		return -1;
	datap = buf;
	if (getbyte() != 0xff)
		return ERR_NO_SOI;
	if (getbyte() != M_SOI)
		return ERR_NO_SOI;
	if (readtables(M_SOF0))
		return ERR_BAD_TABLES;
	getword();
	i = getbyte();
	if (i != 8)
		return ERR_NOT_8BIT;
	if (((getword() + 15) & ~15) != height)
		return ERR_HEIGHT_MISMATCH;
	if (((getword() + 15) & ~15) != width)
		return ERR_WIDTH_MISMATCH;
	if ((height & 15) || (width & 15))
		return ERR_BAD_WIDTH_OR_HEIGHT;
	info.nc = getbyte();
	if (info.nc > MAXCOMP)
		return ERR_TOO_MANY_COMPPS;
	for (i = 0; i < info.nc; i++) {
		int h, v;
		comps[i].cid = getbyte();
		comps[i].hv = getbyte();
		v = comps[i].hv & 15;
		h = comps[i].hv >> 4;
		comps[i].tq = getbyte();
		if (h > 3 || v > 3)
			return ERR_ILLEGAL_HV;
		if (comps[i].tq > 3)
			return ERR_QUANT_TABLE_SELECTOR;
	}
	if (readtables(M_SOS))
		return ERR_BAD_TABLES;
	getword();
	info.ns = getbyte();
	if (info.ns != 3)
		return ERR_NOT_YCBCR_221111;
	for (i = 0; i < 3; i++) {
		dscans[i].cid = getbyte();
		tdc = getbyte();
		tac = tdc & 15;
		tdc >>= 4;
		if (tdc > 1 || tac > 1)
			return ERR_QUANT_TABLE_SELECTOR;
		for (j = 0; j < info.nc; j++)
			if (comps[j].cid == dscans[i].cid)
				break;
		if (j == info.nc)
			return ERR_UNKNOWN_CID_IN_SCAN;
		dscans[i].hv = comps[j].hv;
		dscans[i].tq = comps[j].tq;
		dscans[i].hudc.dhuff = dec_huffdc + tdc;
		dscans[i].huac.dhuff = dec_huffac + tac;
	}
	
	i = getbyte();
	j = getbyte();
	m = getbyte();
	
	if (i != 0 || j != 63 || m != 0)
		return ERR_NOT_SEQUENTIAL_DCT;
	
	if (dscans[0].cid != 1 || dscans[1].cid != 2 || dscans[2].cid != 3)
		return ERR_NOT_YCBCR_221111;

	if (dscans[0].hv != 0x22 || dscans[1].hv != 0x11 || dscans[2].hv != 0x11)
		return ERR_NOT_YCBCR_221111;

	mcusx = width >> 4;
	mcusy = height >> 4;


	idctqtab(quant[dscans[0].tq], decdata->dquant[0]);
	idctqtab(quant[dscans[1].tq], decdata->dquant[1]);
	idctqtab(quant[dscans[2].tq], decdata->dquant[2]);
	initcol(decdata->dquant);
	setinput(&in, datap);

#if 0
	/* landing zone */
	img[len] = 0;
	img[len + 1] = 0xff;
	img[len + 2] = M_EOF;
#endif

	dec_initscans();

	dscans[0].next = 6 - 4;
	dscans[1].next = 6 - 4 - 1;
	dscans[2].next = 6 - 4 - 1 - 1;	/* 411 encoding */
	for (my = 0; my < mcusy; my++) {
		for (mx = 0; mx < mcusx; mx++) {
			if (info.dri && !--info.nm)
				if (dec_checkmarker())
					return ERR_WRONG_MARKER;
			
			decode_mcus(&in, decdata->dcts, 6, dscans, max);
			idct(decdata->dcts, decdata->out, decdata->dquant[0], IFIX(128.5), max[0]);
			idct(decdata->dcts + 64, decdata->out + 64, decdata->dquant[0], IFIX(128.5), max[1]);
			idct(decdata->dcts + 128, decdata->out + 128, decdata->dquant[0], IFIX(128.5), max[2]);
			idct(decdata->dcts + 192, decdata->out + 192, decdata->dquant[0], IFIX(128.5), max[3]);
			idct(decdata->dcts + 256, decdata->out + 256, decdata->dquant[1], IFIX(0.5), max[4]);
			idct(decdata->dcts + 320, decdata->out + 320, decdata->dquant[2], IFIX(0.5), max[5]);

			switch (depth) {
			case 24:
				col221111(decdata->out, pic + (my * 16 * mcusx + mx) * 16 * 3, mcusx * 16 * 3);
				break;
			case 16:
				col221111_16(decdata->out, pic + (my * 16 * mcusx + mx) * (16 * 2), mcusx * (16 * 2));
				break;
			default:
				return ERR_DEPTH_MISMATCH;
				break;
			}
		}
	}
	
	m = dec_readmarker(&in);
	if (m != M_EOI)
		return ERR_NO_EOI;

	return 0;
}

/****************************************************************/
/**************       huffman decoder             ***************/
/****************************************************************/

static int fillbits __P((struct in *, int, unsigned int));
static int dec_rec2
__P((struct in *, struct dec_hufftbl *, int *, int, int));

static void setinput(in, p)
struct in *in;
unsigned char *p;
{
	in->p = p;
	in->left = 0;
	in->bits = 0;
	in->marker = 0;
}

static int fillbits(in, le, bi)
struct in *in;
int le;
unsigned int bi;
{
	int b, m;

	if (in->marker) {
		if (le <= 16)
			in->bits = bi << 16, le += 16;
		return le;
	}
	while (le <= 24) {
		b = *in->p++;
		if (b == 0xff && (m = *in->p++) != 0) {
			if (m == M_EOF) {
				if (in->func && (m = in->func(in->data)) == 0)
					continue;
			}
			in->marker = m;
			if (le <= 16)
				bi = bi << 16, le += 16;
			break;
		}
		bi = bi << 8 | b;
		le += 8;
	}
	in->bits = bi;		/* tmp... 2 return values needed */
	return le;
}

static int dec_readmarker(in)
struct in *in;
{
	int m;

	in->left = fillbits(in, in->left, in->bits);
	if ((m = in->marker) == 0)
		return 0;
	in->left = 0;
	in->marker = 0;
	return m;
}

#define LEBI_DCL	int le, bi
#define LEBI_GET(in)	(le = in->left, bi = in->bits)
#define LEBI_PUT(in)	(in->left = le, in->bits = bi)

#define GETBITS(in, n) (					\
  (le < (n) ? le = fillbits(in, le, bi), bi = in->bits : 0),	\
  (le -= (n)),							\
  bi >> le & ((1 << (n)) - 1)					\
)

#define UNGETBITS(in, n) (	\
  le += (n)			\
)


static int dec_rec2(in, hu, runp, c, i)
struct in *in;
struct dec_hufftbl *hu;
int *runp;
int c, i;
{
	LEBI_DCL;

	LEBI_GET(in);
	if (i) {
		UNGETBITS(in, i & 127);
		*runp = i >> 8 & 15;
		i >>= 16;
	} else {
		for (i = DECBITS; (c = ((c << 1) | GETBITS(in, 1))) >= (hu->maxcode[i]); i++);
		if (i >= 16) {
			in->marker = M_BADHUFF;
			return 0;
		}
		i = hu->vals[hu->valptr[i] + c - hu->maxcode[i - 1] * 2];
		*runp = i >> 4;
		i &= 15;
	}
	if (i == 0) {		/* sigh, 0xf0 is 11 bit */
		LEBI_PUT(in);
		return 0;
	}
	/* receive part */
	c = GETBITS(in, i);
	if (c < (1 << (i - 1)))
		c += (-1 << i) + 1;
	LEBI_PUT(in);
	return c;
}

#define DEC_REC(in, hu, r, i)	 (	\
  r = GETBITS(in, DECBITS),		\
  i = hu->llvals[r],			\
  i & 128 ?				\
    (					\
      UNGETBITS(in, i & 127),		\
      r = i >> 8 & 15,			\
      i >> 16				\
    )					\
  :					\
    (					\
      LEBI_PUT(in),			\
      i = dec_rec2(in, hu, &r, r, i),	\
      LEBI_GET(in),			\
      i					\
    )					\
)

static void decode_mcus(in, dct, n, sc, maxp)
struct in *in;
int *dct;
int n;
struct scan *sc;
int *maxp;
{
	struct dec_hufftbl *hu;
	int i, r, t;
	LEBI_DCL;

	memset(dct, 0, n * 64 * sizeof(*dct));
	LEBI_GET(in);
	while (n-- > 0) {
		hu = sc->hudc.dhuff;
		*dct++ = (sc->dc += DEC_REC(in, hu, r, t));

		hu = sc->huac.dhuff;
		i = 63;
		while (i > 0) {
			t = DEC_REC(in, hu, r, t);
			if (t == 0 && r == 0) {
				dct += i;
				break;
			}
			dct += r;
			*dct++ = t;
			i -= r + 1;
		}
		*maxp++ = 64 - i;
		if (n == sc->next)
			sc++;
	}
	LEBI_PUT(in);
}

static void dec_makehuff(hu, hufflen, huffvals)
struct dec_hufftbl *hu;
int *hufflen;
unsigned char *huffvals;
{
	int code, k, i, j, d, x, c, v;
	for (i = 0; i < (1 << DECBITS); i++)
		hu->llvals[i] = 0;

/*
 * llvals layout:
 *
 * value v already known, run r, backup u bits:
 *  vvvvvvvvvvvvvvvv 0000 rrrr 1 uuuuuuu
 * value unknown, size b bits, run r, backup u bits:
 *  000000000000bbbb 0000 rrrr 0 uuuuuuu
 * value and size unknown:
 *  0000000000000000 0000 0000 0 0000000
 */
	code = 0;
	k = 0;
	for (i = 0; i < 16; i++, code <<= 1) {	/* sizes */
		hu->valptr[i] = k;
		for (j = 0; j < hufflen[i]; j++) {
			hu->vals[k] = *huffvals++;
			if (i < DECBITS) {
				c = code << (DECBITS - 1 - i);
				v = hu->vals[k] & 0x0f;	/* size */
				for (d = 1 << (DECBITS - 1 - i); --d >= 0;) {
					if (v + i < DECBITS) {	/* both fit in table */
						x = d >> (DECBITS - 1 - v -
							  i);
						if (v && x < (1 << (v - 1)))
							x += (-1 << v) + 1;
						x = x << 16 | (hu-> vals[k] & 0xf0) << 4 |
							(DECBITS - (i + 1 + v)) | 128;
					} else
						x = v << 16 | (hu-> vals[k] & 0xf0) << 4 |
						        (DECBITS - (i + 1));
					hu->llvals[c | d] = x;
				}
			}
			code++;
			k++;
		}
		hu->maxcode[i] = code;
	}
	hu->maxcode[16] = 0x20000;	/* always terminate decode */
}

/****************************************************************/
/**************             idct                  ***************/
/****************************************************************/

#define ONE ((PREC)IFIX(1.))
#define S2  ((PREC)IFIX(0.382683432))
#define C2  ((PREC)IFIX(0.923879532))
#define C4  ((PREC)IFIX(0.707106781))

#define S22 ((PREC)IFIX(2 * 0.382683432))
#define C22 ((PREC)IFIX(2 * 0.923879532))
#define IC4 ((PREC)IFIX(1 / 0.707106781))

#define C3IC1 ((PREC)IFIX(0.847759065))	/* c3/c1 */
#define C5IC1 ((PREC)IFIX(0.566454497))	/* c5/c1 */
#define C7IC1 ((PREC)IFIX(0.198912367))	/* c7/c1 */

#define XPP(a,b) (t = a + b, b = a - b, a = t)
#define XMP(a,b) (t = a - b, b = a + b, a = t)
#define XPM(a,b) (t = a + b, b = b - a, a = t)

#define ROT(a,b,s,c) (	t = IMULT(a + b, s),	\
			a = IMULT(a, c - s) + t,	\
			b = IMULT(b, c + s) - t)

#define IDCT		\
(			\
  XPP(t0, t1),		\
  XMP(t2, t3),		\
  t2 = IMULT(t2, IC4) - t3,	\
  XPP(t0, t3),		\
  XPP(t1, t2),		\
  XMP(t4, t7),		\
  XPP(t5, t6),		\
  XMP(t5, t7),		\
  t5 = IMULT(t5, IC4),	\
  ROT(t4, t6, S22, C22),\
  t6 -= t7,		\
  t5 -= t6,		\
  t4 -= t5,		\
  XPP(t0, t7),		\
  XPP(t1, t6),		\
  XPP(t2, t5),		\
  XPP(t3, t4)		\
)

static unsigned char zig2[64] = {
	0, 2, 3, 9, 10, 20, 21, 35,
	14, 16, 25, 31, 39, 46, 50, 57,
	5, 7, 12, 18, 23, 33, 37, 48,
	27, 29, 41, 44, 52, 55, 59, 62,
	15, 26, 30, 40, 45, 51, 56, 58,
	1, 4, 8, 11, 19, 22, 34, 36,
	28, 42, 43, 53, 54, 60, 61, 63,
	6, 13, 17, 24, 32, 38, 47, 49
};

void idct(in, out, quant, off, max)
int *in;
int *out;
PREC *quant;
PREC off;
int max;
{
	PREC t0, t1, t2, t3, t4, t5, t6, t7, t;
	PREC tmp[64], *tmpp;
	int i, j;
	unsigned char *zig2p;

	t0 = off;
	if (max == 1) {
		t0 += in[0] * quant[0];
		for (i = 0; i < 64; i++)
			out[i] = ITOINT(t0);
		return;
	}
	zig2p = zig2;
	tmpp = tmp;
	for (i = 0; i < 8; i++) {
		j = *zig2p++;
		t0 += in[j] * quant[j];
		j = *zig2p++;
		t5 = in[j] * quant[j];
		j = *zig2p++;
		t2 = in[j] * quant[j];
		j = *zig2p++;
		t7 = in[j] * quant[j];
		j = *zig2p++;
		t1 = in[j] * quant[j];
		j = *zig2p++;
		t4 = in[j] * quant[j];
		j = *zig2p++;
		t3 = in[j] * quant[j];
		j = *zig2p++;
		t6 = in[j] * quant[j];
		IDCT;
		tmpp[0 * 8] = t0;
		tmpp[1 * 8] = t1;
		tmpp[2 * 8] = t2;
		tmpp[3 * 8] = t3;
		tmpp[4 * 8] = t4;
		tmpp[5 * 8] = t5;
		tmpp[6 * 8] = t6;
		tmpp[7 * 8] = t7;
		tmpp++;
		t0 = 0;
	}
	for (i = 0; i < 8; i++) {
		t0 = tmp[8 * i + 0];
		t1 = tmp[8 * i + 1];
		t2 = tmp[8 * i + 2];
		t3 = tmp[8 * i + 3];
		t4 = tmp[8 * i + 4];
		t5 = tmp[8 * i + 5];
		t6 = tmp[8 * i + 6];
		t7 = tmp[8 * i + 7];
		IDCT;
		out[8 * i + 0] = ITOINT(t0);
		out[8 * i + 1] = ITOINT(t1);
		out[8 * i + 2] = ITOINT(t2);
		out[8 * i + 3] = ITOINT(t3);
		out[8 * i + 4] = ITOINT(t4);
		out[8 * i + 5] = ITOINT(t5);
		out[8 * i + 6] = ITOINT(t6);
		out[8 * i + 7] = ITOINT(t7);
	}
}

static unsigned char zig[64] = {
	0, 1, 5, 6, 14, 15, 27, 28,
	2, 4, 7, 13, 16, 26, 29, 42,
	3, 8, 12, 17, 25, 30, 41, 43,
	9, 11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63
};

static PREC aaidct[8] = {
	IFIX(0.3535533906), IFIX(0.4903926402),
	IFIX(0.4619397663), IFIX(0.4157348062),
	IFIX(0.3535533906), IFIX(0.2777851165),
	IFIX(0.1913417162), IFIX(0.0975451610)
};


static void idctqtab(qin, qout)
unsigned char *qin;
PREC *qout;
{
	int i, j;

	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
			qout[zig[i * 8 + j]] = qin[zig[i * 8 + j]] * 
			  			IMULT(aaidct[i], aaidct[j]);
}

static void scaleidctqtab(q, sc)
PREC *q;
PREC sc;
{
	int i;

	for (i = 0; i < 64; i++)
		q[i] = IMULT(q[i], sc);
}

/****************************************************************/
/**************          color decoder            ***************/
/****************************************************************/

#define ROUND

/*
 * YCbCr Color transformation:
 *
 * y:0..255   Cb:-128..127   Cr:-128..127
 *
 *      R = Y                + 1.40200 * Cr
 *      G = Y - 0.34414 * Cb - 0.71414 * Cr
 *      B = Y + 1.77200 * Cb
 *
 * =>
 *      Cr *= 1.40200;
 *      Cb *= 1.77200;
 *      Cg = 0.19421 * Cb + .50937 * Cr;
 *      R = Y + Cr;
 *      G = Y - Cg;
 *      B = Y + Cb;
 *
 * =>
 *      Cg = (50 * Cb + 130 * Cr + 128) >> 8;
 */

static void initcol(q)
PREC q[][64];
{
	scaleidctqtab(q[1], IFIX(1.77200));
	scaleidctqtab(q[2], IFIX(1.40200));
}

/* This is optimized for the stupid sun SUNWspro compiler. */
#define STORECLAMP(a,x)				\
(						\
  (a) = (x),					\
  (unsigned int)(x) >= 256 ? 			\
    ((a) = (x) < 0 ? 0 : 255)			\
  :						\
    0						\
)

#define CLAMP(x) ((unsigned int)(x) >= 256 ? ((x) < 0 ? 0 : 255) : (x))

#ifdef ROUND

#define CBCRCG(yin, xin)			\
(						\
  cb = outc[0 +yin*8+xin],			\
  cr = outc[64+yin*8+xin],			\
  cg = (50 * cb + 130 * cr + 128) >> 8		\
)

#else

#define CBCRCG(yin, xin)			\
(						\
  cb = outc[0 +yin*8+xin],			\
  cr = outc[64+yin*8+xin],			\
  cg = (3 * cb + 8 * cr) >> 4			\
)

#endif

#define PIC(yin, xin, p, xout)			\
(						\
  y = outy[(yin) * 8 + xin],			\
  STORECLAMP(p[(xout) * 3 + 0], y + cr),	\
  STORECLAMP(p[(xout) * 3 + 1], y - cg),	\
  STORECLAMP(p[(xout) * 3 + 2], y + cb)		\
)

#ifdef __LITTLE_ENDIAN
#define PIC_16(yin, xin, p, xout, add)		 \
(                                                \
  y = outy[(yin) * 8 + xin],                     \
  y = ((CLAMP(y + cr + add*2+1) & 0xf8) <<  8) | \
      ((CLAMP(y - cg + add    ) & 0xfc) <<  3) | \
      ((CLAMP(y + cb + add*2+1)       ) >>  3),  \
  p[(xout) * 2 + 0] = y & 0xff,                  \
  p[(xout) * 2 + 1] = y >> 8                     \
)
#else
#ifdef CONFIG_PPC
#define PIC_16(yin, xin, p, xout, add)		 \
(                                                \
  y = outy[(yin) * 8 + xin],                     \
  y = ((CLAMP(y + cr + add*2+1) & 0xf8) <<  7) | \
      ((CLAMP(y - cg + add*2+1) & 0xf8) <<  2) | \
      ((CLAMP(y + cb + add*2+1)       ) >>  3),  \
  p[(xout) * 2 + 0] = y >> 8,                    \
  p[(xout) * 2 + 1] = y & 0xff                   \
)
#else
#define PIC_16(yin, xin, p, xout, add)	 	 \
(                                                \
  y = outy[(yin) * 8 + xin],                     \
  y = ((CLAMP(y + cr + add*2+1) & 0xf8) <<  8) | \
      ((CLAMP(y - cg + add    ) & 0xfc) <<  3) | \
      ((CLAMP(y + cb + add*2+1)       ) >>  3),  \
  p[(xout) * 2 + 0] = y >> 8,                    \
  p[(xout) * 2 + 1] = y & 0xff                   \
)
#endif
#endif

#define PIC221111(xin)						\
(								\
  CBCRCG(0, xin),						\
  PIC(xin / 4 * 8 + 0, (xin & 3) * 2 + 0, pic0, xin * 2 + 0),	\
  PIC(xin / 4 * 8 + 0, (xin & 3) * 2 + 1, pic0, xin * 2 + 1),	\
  PIC(xin / 4 * 8 + 1, (xin & 3) * 2 + 0, pic1, xin * 2 + 0),	\
  PIC(xin / 4 * 8 + 1, (xin & 3) * 2 + 1, pic1, xin * 2 + 1)	\
)

#define PIC221111_16(xin)                                               \
(                                                               	\
  CBCRCG(0, xin),                                               	\
  PIC_16(xin / 4 * 8 + 0, (xin & 3) * 2 + 0, pic0, xin * 2 + 0, 3),     \
  PIC_16(xin / 4 * 8 + 0, (xin & 3) * 2 + 1, pic0, xin * 2 + 1, 0),     \
  PIC_16(xin / 4 * 8 + 1, (xin & 3) * 2 + 0, pic1, xin * 2 + 0, 1),     \
  PIC_16(xin / 4 * 8 + 1, (xin & 3) * 2 + 1, pic1, xin * 2 + 1, 2)      \
)

static void col221111(out, pic, width)
int *out;
unsigned char *pic;
int width;
{
	int i, j, k;
	unsigned char *pic0, *pic1;
	int *outy, *outc;
	int cr, cg, cb, y;

	pic0 = pic;
	pic1 = pic + width;
	outy = out;
	outc = out + 64 * 4;
	for (i = 2; i > 0; i--) {
		for (j = 4; j > 0; j--) {
			for (k = 0; k < 8; k++) {
				PIC221111(k);
			}
			outc += 8;
			outy += 16;
			pic0 += 2 * width;
			pic1 += 2 * width;
		}
		outy += 64 * 2 - 16 * 4;
	}
}

static void col221111_16(out, pic, width)
int *out;
unsigned char *pic;
int width;
{
	int i, j, k;
	unsigned char *pic0, *pic1;
	int *outy, *outc;
	int cr, cg, cb, y;

	pic0 = pic;
	pic1 = pic + width;
	outy = out;
	outc = out + 64 * 4;
	for (i = 2; i > 0; i--) {
		for (j = 4; j > 0; j--) {
			for (k = 0; k < 8; k++) {
			    PIC221111_16(k);
			}
			outc += 8;
			outy += 16;
			pic0 += 2 * width;
			pic1 += 2 * width;
		}
		outy += 64 * 2 - 16 * 4;
	}
}
