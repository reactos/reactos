/*
   This file was derived from the libwww code, version 2.15, from CERN.
   A number of modifications have been made by Spyglass.

   eric@spyglass.com

   This file was removed from LibWWW and placed into the
   Security Protocol Module.

   jeff@spyglass.com
 */

/* MODULE                           HTUU.c
   **           UUENCODE AND UUDECODE
   **
   ** ACKNOWLEDGEMENT:
   **   This code is taken from rpem distribution, and was originally
   **   written by Mark Riordan.
   **
   ** AUTHORS:
   **   MR  Mark Riordan    riordanmr@clvax1.cl.msu.edu
   **   AL  Ari Luotonen    luotonen@dxcern.cern.ch
   **
   ** HISTORY:
   **   Added as part of the WWW library and edited to conform
   **   with the WWW project coding standards by:   AL  5 Aug 1993
   **   Originally written by:              MR 12 Aug 1990
   **   Original header text:
   ** -------------------------------------------------------------
   **  File containing routines to convert a buffer
   **  of bytes to/from RFC 1113 printable encoding format.
   **
   **  This technique is similar to the familiar Unix uuencode
   **  format in that it maps 6 binary bits to one ASCII
   **  character (or more aptly, 3 binary bytes to 4 ASCII
   **  characters).  However, RFC 1113 does not use the same
   **  mapping to printable characters as uuencode.
   **
   **  Mark Riordan   12 August 1990 and 17 Feb 1991.
   **  This code is hereby placed in the public domain.
   ** -------------------------------------------------------------
   **
   ** BUGS:
   **
   **
 */


const static char six2pr[64] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static unsigned char pr2six[256];


/*--- function HTUU_encode -----------------------------------------------
 *
 *   Encode a single line of binary data to a standard format that
 *   uses only printing ASCII characters (but takes up 33% more bytes).
 *
 *    Entry    bufin    points to a buffer of bytes.  If nbytes is not
 *                      a multiple of three, then the byte just beyond
 *                      the last byte in the buffer must be 0.
 *             nbytes   is the number of bytes in that buffer.
 *                      This cannot be more than 48.
 *             bufcoded points to an output buffer.  Be sure that this
 *                      can hold at least 1 + (4*nbytes)/3 characters.
 *             outbufmax maximum size of the buffer bufcoded.
 *
 *    Exit     bufcoded contains the coded line.  The first 4*nbytes/3 bytes
 *                      contain printing ASCII characters representing
 *                      those binary bytes. This may include one or
 *                      two '=' characters used as padding at the end.
 *                      The last byte is a zero byte.
 *             Returns the number of ASCII characters in "bufcoded".
 */
int HTUU_encode(unsigned char *bufin, unsigned int nbytes, char *bufcoded,
                long outbufmax)
{
/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) six2pr[c]

	register char *outptr = bufcoded;
	unsigned int i;

	for (i = 0; i < nbytes; i += 3)
	{
        if ( (outptr - bufcoded + 4) > outbufmax )
            return (-1);

		*(outptr++) = ENC(*bufin >> 2);		/* c1 */
		*(outptr++) = ENC(((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017));		/*c2 */
		*(outptr++) = ENC(((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03));	/*c3 */
		*(outptr++) = ENC(bufin[2] & 077);	/* c4 */

		bufin += 3;
	}

	/* If nbytes was not a multiple of 3, then we have encoded too
	 * many characters.  Adjust appropriately.
	 */
	if (i == nbytes + 1)
	{
		/* There were only 2 bytes in that last group */
		outptr[-1] = '=';
	}
	else if (i == nbytes + 2)
	{
		/* There was only 1 byte in that last group */
		outptr[-1] = '=';
		outptr[-2] = '=';
	}

    if ( (outptr - bufcoded) + 1 < outbufmax )
    	*outptr = '\0';

	return ((int)(outptr - bufcoded));
}


/*--- function HTUU_decode ------------------------------------------------
 *
 *  Decode an ASCII-encoded buffer back to its original binary form.
 *
 *    Entry    bufcoded    points to a uuencoded string.  It is 
 *                         terminated by any character not in
 *                         the printable character table six2pr, but
 *                         leading whitespace is stripped.
 *             bufplain    points to the output buffer; must be big
 *                         enough to hold the decoded string (generally
 *                         shorter than the encoded string) plus
 *                         as many as two extra bytes used during
 *                         the decoding process.
 *             outbufsize  is the maximum number of bytes that
 *                         can fit in bufplain.
 *
 *    Exit     Returns the number of binary bytes decoded.
 *             bufplain    contains these bytes.
 */
int HTUU_decode(char *bufcoded, unsigned char *bufplain, int outbufsize)
{
/* single character decode */
#define DEC(c) pr2six[(int)c]
#define MAXVAL 63

	static int first = 1;

	int nbytesdecoded, j;
	register char *bufin = bufcoded;
	register unsigned char *bufout = bufplain;
	register int nprbytes;

	/* If this is the first call, initialize the mapping table.
	 * This code should work even on non-ASCII machines.
	 */
	if (first)
	{
		first = 0;
		for (j = 0; j < 256; j++)
			pr2six[j] = MAXVAL + 1;

		for (j = 0; j < 64; j++)
			pr2six[(int) six2pr[j]] = (unsigned char) j;
#if 0
		pr2six['A'] = 0;
		pr2six['B'] = 1;
		pr2six['C'] = 2;
		pr2six['D'] = 3;
		pr2six['E'] = 4;
		pr2six['F'] = 5;
		pr2six['G'] = 6;
		pr2six['H'] = 7;
		pr2six['I'] = 8;
		pr2six['J'] = 9;
		pr2six['K'] = 10;
		pr2six['L'] = 11;
		pr2six['M'] = 12;
		pr2six['N'] = 13;
		pr2six['O'] = 14;
		pr2six['P'] = 15;
		pr2six['Q'] = 16;
		pr2six['R'] = 17;
		pr2six['S'] = 18;
		pr2six['T'] = 19;
		pr2six['U'] = 20;
		pr2six['V'] = 21;
		pr2six['W'] = 22;
		pr2six['X'] = 23;
		pr2six['Y'] = 24;
		pr2six['Z'] = 25;
		pr2six['a'] = 26;
		pr2six['b'] = 27;
		pr2six['c'] = 28;
		pr2six['d'] = 29;
		pr2six['e'] = 30;
		pr2six['f'] = 31;
		pr2six['g'] = 32;
		pr2six['h'] = 33;
		pr2six['i'] = 34;
		pr2six['j'] = 35;
		pr2six['k'] = 36;
		pr2six['l'] = 37;
		pr2six['m'] = 38;
		pr2six['n'] = 39;
		pr2six['o'] = 40;
		pr2six['p'] = 41;
		pr2six['q'] = 42;
		pr2six['r'] = 43;
		pr2six['s'] = 44;
		pr2six['t'] = 45;
		pr2six['u'] = 46;
		pr2six['v'] = 47;
		pr2six['w'] = 48;
		pr2six['x'] = 49;
		pr2six['y'] = 50;
		pr2six['z'] = 51;
		pr2six['0'] = 52;
		pr2six['1'] = 53;
		pr2six['2'] = 54;
		pr2six['3'] = 55;
		pr2six['4'] = 56;
		pr2six['5'] = 57;
		pr2six['6'] = 58;
		pr2six['7'] = 59;
		pr2six['8'] = 60;
		pr2six['9'] = 61;
		pr2six['+'] = 62;
		pr2six['/'] = 63;
#endif
	}

	/* Strip leading whitespace. */

	while (*bufcoded == ' ' || *bufcoded == '\t')
		bufcoded++;

	/* Figure out how many characters are in the input buffer.
	 * If this would decode into more bytes than would fit into
	 * the output buffer, adjust the number of input bytes downwards.
	 */
	bufin = bufcoded;
	while (pr2six[(int) *(bufin++)] <= MAXVAL) ;
	nprbytes = (int)(bufin - bufcoded) - 1;
	nbytesdecoded = ((nprbytes + 3) / 4) * 3;
	if (nbytesdecoded > outbufsize)
	{
		nprbytes = (outbufsize * 4) / 3;
	}

	bufin = bufcoded;

	while (nprbytes > 0)
	{
		*(bufout++) = (unsigned char) (DEC(*bufin) << 2 | DEC(bufin[1]) >> 4);
		*(bufout++) = (unsigned char) (DEC(bufin[1]) << 4 | DEC(bufin[2]) >> 2);
		*(bufout++) = (unsigned char) (DEC(bufin[2]) << 6 | DEC(bufin[3]));
		bufin += 4;
		nprbytes -= 4;
	}

	if (nprbytes & 03)
	{
		if (pr2six[(int) bufin[-2]] > MAXVAL)
		{
			nbytesdecoded -= 2;
		}
		else
		{
			nbytesdecoded -= 1;
		}
	}

	return (nbytesdecoded);
}
