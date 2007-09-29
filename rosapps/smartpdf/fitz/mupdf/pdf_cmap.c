/*
 * The CMap data structure here is constructed on the fly by
 * adding simple range-to-range mappings. Then the data structure
 * is optimized to contain both range-to-range and range-to-table
 * lookups.
 *
 * Any one-to-many mappings are inserted as one-to-table
 * lookups in the beginning, and are not affected by the optimization
 * stage.
 *
 * There is a special function to add a 256-length range-to-table mapping.
 * The ranges do not have to be added in order.
 *
 * This code can be a lot simpler if we don't care about wasting memory,
 * or can trust the parser to give us optimal mappings.
 */

#include "fitz.h"
#include "mupdf.h"

typedef struct pdf_range_s pdf_range;

enum { MAXCODESPACE = 10 };
enum { SINGLE, RANGE, TABLE, MULTI };

struct pdf_range_s
{
	int low;
	int high;
	int flag;	/* what kind of lookup is this */
	int offset;	/* either range-delta or table-index */
};

static int
cmprange(const void *va, const void *vb)
{
	return ((const pdf_range*)va)->low - ((const pdf_range*)vb)->low;
}

struct pdf_cmap_s
{
	int refs;
	char cmapname[32];

	char usecmapname[32];
	pdf_cmap *usecmap;

	int wmode;

	int ncspace;
	struct {
		int n;
		unsigned char lo[4];
		unsigned char hi[4];
	} cspace[MAXCODESPACE];

	int rlen, rcap;
	pdf_range *ranges;

	int tlen, tcap;
	int *table;
};

/*
 * Allocate, destroy and simple parameters.
 */

fz_error *
pdf_newcmap(pdf_cmap **cmapp)
{
	pdf_cmap *cmap;

	cmap = *cmapp = fz_malloc(sizeof(pdf_cmap));
	if (!cmap)
		return fz_outofmem;

	cmap->refs = 1;
	strcpy(cmap->cmapname, "");

	strcpy(cmap->usecmapname, "");
	cmap->usecmap = nil;

	cmap->wmode = 0;

	cmap->ncspace = 0;

	cmap->rlen = 0;
	cmap->rcap = 0;
	cmap->ranges = nil;

	cmap->tlen = 0;
	cmap->tcap = 0;
	cmap->table = nil;

	return nil;
}

pdf_cmap *
pdf_keepcmap(pdf_cmap *cmap)
{
	cmap->refs ++;
	return cmap;
}

void
pdf_dropcmap(pdf_cmap *cmap)
{
	if (--cmap->refs == 0)
	{
		if (cmap->usecmap)
			pdf_dropcmap(cmap->usecmap);
		fz_free(cmap->ranges);
		fz_free(cmap->table);
		fz_free(cmap);
	}
}

pdf_cmap *
pdf_getusecmap(pdf_cmap *cmap)
{
	return cmap->usecmap;
}

void
pdf_setusecmap(pdf_cmap *cmap, pdf_cmap *usecmap)
{
	int i;

	if (cmap->usecmap)
		pdf_dropcmap(cmap->usecmap);
	cmap->usecmap = pdf_keepcmap(usecmap);

	if (cmap->ncspace == 0)
	{
		cmap->ncspace = usecmap->ncspace;
		for (i = 0; i < usecmap->ncspace; i++)
			cmap->cspace[i] = usecmap->cspace[i];
	}
}

int
pdf_getwmode(pdf_cmap *cmap)
{
	return cmap->wmode;
}

void
pdf_setwmode(pdf_cmap *cmap, int wmode)
{
	cmap->wmode = wmode;
}

void
pdf_debugcmap(pdf_cmap *cmap)
{
	int i, k, n;

	printf("cmap $%p /%s {\n", cmap, cmap->cmapname);

	if (cmap->usecmapname[0])
		printf("  usecmap /%s\n", cmap->usecmapname);
	if (cmap->usecmap)
		printf("  usecmap $%p\n", cmap->usecmap);

	printf("  wmode %d\n", cmap->wmode);

	printf("  codespaces {\n");
	for (i = 0; i < cmap->ncspace; i++)
	{
		printf("    <");
		for (k = 0; k < cmap->cspace[i].n; k++)
			printf("%02x", cmap->cspace[i].lo[k]);
		printf("> <");
		for (k = 0; k < cmap->cspace[i].n; k++)
			printf("%02x", cmap->cspace[i].hi[k]);
		printf(">\n");
	}
	printf("  }\n");

	printf("  ranges (%d,%d) {\n", cmap->rlen, cmap->tlen);
	for (i = 0; i < cmap->rlen; i++)
	{
		pdf_range *r = &cmap->ranges[i];
		printf("    <%04x> <%04x> ", r->low, r->high);
		if (r->flag == TABLE)
		{
			printf("[ ");
			for (k = 0; k < r->high - r->low + 1; k++)
				printf("%d ", cmap->table[r->offset + k]);
			printf("]\n");
		}
		else if (r->flag == MULTI)
		{
			printf("< ");
			n = cmap->table[r->offset];
			for (k = 0; k < n; k++)
				printf("%04x ", cmap->table[r->offset + 1 + k]);
			printf(">\n");
		}
		else
			printf("%d\n", r->offset);
	}
	printf("  }\n}\n");
}

/*
 * Add a codespacerange section.
 * These ranges are used by pdf_decodecmap to decode
 * multi-byte encoded strings.
 */
fz_error *
pdf_addcodespace(pdf_cmap *cmap, unsigned lo, unsigned hi, int n)
{
	int i;

	if (cmap->ncspace + 1 == MAXCODESPACE)
		return fz_throw("rangelimit: too many code space ranges");

	cmap->cspace[cmap->ncspace].n = n;

	for (i = 0; i < n; i++)
	{
		int o = (n - i - 1) * 8;
		cmap->cspace[cmap->ncspace].lo[i] = (lo >> o) & 0xFF;
		cmap->cspace[cmap->ncspace].hi[i] = (hi >> o) & 0xFF;
	}

	cmap->ncspace ++;

	return nil;
}

/*
 * Add an integer to the table.
 */
static fz_error *
addtable(pdf_cmap *cmap, int value)
{
	if (cmap->tlen + 1 > cmap->tcap)
	{
		int newcap = cmap->tcap == 0 ? 256 : cmap->tcap * 2;
		int *newtable = fz_realloc(cmap->table, newcap * sizeof(int));
		if (!newtable)
			return fz_outofmem;
		cmap->tcap = newcap;
		cmap->table = newtable;
	}

	cmap->table[cmap->tlen++] = value;

	return nil;
}

/*
 * Add a range.
 */
static fz_error *
addrange(pdf_cmap *cmap, int low, int high, int flag, int offset)
{
	if (cmap->rlen + 1 > cmap->rcap)
	{
		pdf_range *newranges;
		int newcap = cmap->rcap == 0 ? 256 : cmap->rcap * 2;
		newranges = fz_realloc(cmap->ranges, newcap * sizeof(pdf_range));
		if (!newranges)
			return fz_outofmem;
		cmap->rcap = newcap;
		cmap->ranges = newranges;
	}

	cmap->ranges[cmap->rlen].low = low;
	cmap->ranges[cmap->rlen].high = high;
	cmap->ranges[cmap->rlen].flag = flag;
	cmap->ranges[cmap->rlen].offset = offset;
	cmap->rlen ++;

	return nil;
}

/*
 * Add a range-to-table mapping.
 */
fz_error *
pdf_maprangetotable(pdf_cmap *cmap, int low, int *table, int len)
{
	fz_error *error;
	int offset;
	int high;
	int i;

	high = low + len;
	offset = cmap->tlen;

	for (i = 0; i < len; i++)
	{
		error = addtable(cmap, table[i]);
		if (error)
			return error;
	}

	return addrange(cmap, low, high, TABLE, offset);
}

/*
 * Add a range of contiguous one-to-one mappings (ie 1..5 maps to 21..25)
 */
fz_error *
pdf_maprangetorange(pdf_cmap *cmap, int low, int high, int offset)
{
	return addrange(cmap, low, high, high - low == 0 ? SINGLE : RANGE, offset);
}

/*
 * Add a single one-to-many mapping.
 */
fz_error *
pdf_maponetomany(pdf_cmap *cmap, int low, int *values, int len)
{
	fz_error *error;
	int offset;
	int i;

	if (len == 1)
		return addrange(cmap, low, low, SINGLE, values[0]);

	offset = cmap->tlen;

	error = addtable(cmap, len);
	if (error)
		return error;

	for (i = 0; i < len; i++)
	{
		addtable(cmap, values[i]);
		if (error)
			return error;
	}

	return addrange(cmap, low, low, MULTI, offset);
}

/*
 * Sort the input ranges.
 * Merge contiguous input ranges to range-to-range if the output is contiguos.
 * Merge contiguous input ranges to range-to-table if the output is random.
 */
fz_error *
pdf_sortcmap(pdf_cmap *cmap)
{
	fz_error *error;
	pdf_range *newranges;
	int *newtable;
	pdf_range *a;			/* last written range on output */
	pdf_range *b;			/* current range examined on input */

	qsort(cmap->ranges, cmap->rlen, sizeof(pdf_range), cmprange);

	a = cmap->ranges;
	b = cmap->ranges + 1;

	while (b < cmap->ranges + cmap->rlen)
	{
		/* ignore one-to-many mappings */
		if (b->flag == MULTI)
		{
			*(++a) = *b;
		}

		/* input contiguous */
		else if (a->high + 1 == b->low)
		{
			/* output contiguous */
			if (a->high - a->low + a->offset + 1 == b->offset)
			{
				/* SR -> R and SS -> R and RR -> R and RS -> R */
				if (a->flag == SINGLE || a->flag == RANGE)
				{
					a->flag = RANGE;
					a->high = b->high;
				}

				/* LS -> L */
				else if (a->flag == TABLE && b->flag == SINGLE)
				{
					a->high = b->high;
					error = addtable(cmap, b->offset);
					if (error)
						return error;
				}

				/* LR -> LR */
				else if (a->flag == TABLE && b->flag == RANGE)
				{
					*(++a) = *b;
				}

				/* XX -> XX */
				else
				{
					*(++a) = *b;
				}
			}

			/* output separated */
			else
			{
				/* SS -> L */
				if (a->flag == SINGLE && b->flag == SINGLE)
				{
					a->flag = TABLE;
					a->high = b->high;

					error = addtable(cmap, a->offset);
					if (error)
						return error;

					error = addtable(cmap, b->offset);
					if (error)
						return error;

					a->offset = cmap->tlen - 2;
				}

				/* LS -> L */
				else if (a->flag == TABLE && b->flag == SINGLE)
				{
					a->high = b->high;
					error = addtable(cmap, b->offset);
					if (error)
						return error;
				}

				/* XX -> XX */
				else
				{
					*(++a) = *b;
				}
			}
		}

		/* input separated: XX -> XX */
		else
		{
			*(++a) = *b;
		}

		b ++;
	}

	cmap->rlen = a - cmap->ranges + 1;

	assert(cmap->rlen > 0);

	newranges = fz_realloc(cmap->ranges, cmap->rlen * sizeof(pdf_range));
	if (!newranges)
		return fz_outofmem;
	cmap->rcap = cmap->rlen;
	cmap->ranges = newranges;

	if (cmap->tlen)
	{
		newtable = fz_realloc(cmap->table, cmap->tlen * sizeof(int));
		if (!newtable)
			return fz_outofmem;
		cmap->tcap = cmap->tlen;
		cmap->table = newtable;
	}

	return nil;
}

/*
 * Lookup the mapping of a codepoint.
 */
int
pdf_lookupcmap(pdf_cmap *cmap, int cpt)
{
	int l = 0;
	int r = cmap->rlen - 1;
	int m;

	while (l <= r)
	{
		m = (l + r) >> 1;
		if (cpt < cmap->ranges[m].low)
			r = m - 1;
		else if (cpt > cmap->ranges[m].high)
			l = m + 1;
		else
		{
			int i = cpt - cmap->ranges[m].low + cmap->ranges[m].offset;
			if (cmap->ranges[m].flag == TABLE)
				return cmap->table[i];
			if (cmap->ranges[m].flag == MULTI)
				return -1;
			return i;
		}
	}

	if (cmap->usecmap)
		return pdf_lookupcmap(cmap->usecmap, cpt);

	return -1;
}

/*
 * Use the codespace ranges to extract a codepoint from a
 * multi-byte encoded string.
 */
unsigned char *
pdf_decodecmap(pdf_cmap *cmap, unsigned char *buf, int *cpt)
{
	int i, k;

	for (k = 0; k < cmap->ncspace; k++)
	{
		unsigned char *lo = cmap->cspace[k].lo;
		unsigned char *hi = cmap->cspace[k].hi;
		int n = cmap->cspace[k].n;
		int c = 0;

		for (i = 0; i < n; i++)
		{
			if (lo[i] <= buf[i] && buf[i] <= hi[i])
				c = (c << 8) | buf[i];
			else
				break;
		}

		if (i == n) {
			*cpt = c;
			return buf + n;
		}
	}

	*cpt = 0;
	return buf + 1;
}

/*
 * CMap parser
 */

enum
{
	TUSECMAP = PDF_NTOKENS,
	TBEGINCODESPACERANGE,
	TENDCODESPACERANGE,
	TBEGINBFCHAR,
	TENDBFCHAR,
	TBEGINBFRANGE,
	TENDBFRANGE,
	TBEGINCIDCHAR,
	TENDCIDCHAR,
	TBEGINCIDRANGE,
	TENDCIDRANGE
};

static int tokenfromkeyword(char *key)
{
	if (!strcmp(key, "usecmap")) return TUSECMAP;
	if (!strcmp(key, "begincodespacerange")) return TBEGINCODESPACERANGE;
	if (!strcmp(key, "endcodespacerange")) return TENDCODESPACERANGE;
	if (!strcmp(key, "beginbfchar")) return TBEGINBFCHAR;
	if (!strcmp(key, "endbfchar")) return TENDBFCHAR;
	if (!strcmp(key, "beginbfrange")) return TBEGINBFRANGE;
	if (!strcmp(key, "endbfrange")) return TENDBFRANGE;
	if (!strcmp(key, "begincidchar")) return TBEGINCIDCHAR;
	if (!strcmp(key, "endcidchar")) return TENDCIDCHAR;
	if (!strcmp(key, "begincidrange")) return TBEGINCIDRANGE;
	if (!strcmp(key, "endcidrange")) return TENDCIDRANGE;
	return PDF_TKEYWORD;
}

static int codefromstring(unsigned char *buf, int len)
{
	int a = 0;
	while (len--)
		a = (a << 8) | *buf++;
	return a;
}

static int mylex(fz_stream *file, char *buf, int n, int *sl)
{
	int token = pdf_lex(file, buf, n, sl);
	if (token == PDF_TKEYWORD)
		token = tokenfromkeyword(buf);
	return token;
}

static fz_error *parsecmapname(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;

	token = mylex(file, buf, sizeof buf, &len);
	if (token == PDF_TNAME) {
		strlcpy(cmap->cmapname, buf, sizeof(cmap->cmapname));
		return nil;
	}

	return fz_throw("syntaxerror in CMap after /CMapName");
}

static fz_error *parsewmode(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;

	token = mylex(file, buf, sizeof buf, &len);
	if (token == PDF_TINT) {
		pdf_setwmode(cmap, atoi(buf));
		return nil;
	}

	return fz_throw("syntaxerror in CMap after /WMode");
}

static fz_error *parsecodespacerange(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;
	fz_error *error;
	int lo, hi;

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);

		if (token == TENDCODESPACERANGE)
			return nil;

		else if (token == PDF_TSTRING)
		{
			lo = codefromstring(buf, len);
			token = mylex(file, buf, sizeof buf, &len);
			if (token == PDF_TSTRING)
			{
				hi = codefromstring(buf, len);
				error = pdf_addcodespace(cmap, lo, hi, len);
				if (error)
					return error;
			}
			else break;
		}

		else break;
	}

	return fz_throw("syntaxerror in CMap codespacerange section");
}

static fz_error *parsecidrange(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;
	fz_error *error;
	int lo, hi, dst;

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);

		if (token == TENDCIDRANGE)
			return nil;

		else if (token != PDF_TSTRING)
			goto cleanup;

		lo = codefromstring(buf, len);

		token = mylex(file, buf, sizeof buf, &len);
		if (token != PDF_TSTRING)
			goto cleanup;

		hi = codefromstring(buf, len);

		token = mylex(file, buf, sizeof buf, &len);
		if (token != PDF_TINT)
			goto cleanup;

		dst = atoi(buf);

		error = pdf_maprangetorange(cmap, lo, hi, dst);
		if (error)
			return error;
	}

cleanup:
	return fz_throw("syntaxerror in CMap cidrange section");
}

static fz_error *parsecidchar(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;
	fz_error *error;
	int src, dst;

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);

		if (token == TENDCIDCHAR)
			return nil;

		else if (token != PDF_TSTRING)
			goto cleanup;

		src = codefromstring(buf, len);

		token = mylex(file, buf, sizeof buf, &len);
		if (token != PDF_TINT)
			goto cleanup;

		dst = atoi(buf);

		error = pdf_maprangetorange(cmap, src, src, dst);
		if (error)
			return error;
	}

cleanup:
	return fz_throw("syntaxerror in CMap cidchar section");
}

static fz_error *parsebfrangearray(pdf_cmap *cmap, fz_stream *file, int lo, int hi)
{
	char buf[256];
	int token;
	int len;
	fz_error *error;
	int dst[256];
	int i;

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);
		/* Note: does not handle [ /Name /Name ... ] */

		if (token == PDF_TCARRAY)
			return nil;

		else if (token != PDF_TSTRING)
			return fz_throw("syntaxerror in CMap bfrange array section");

		if (len / 2)
		{
			for (i = 0; i < len / 2; i++)
				dst[i] = codefromstring(buf + i * 2, 2);

			error = pdf_maponetomany(cmap, lo, dst, len / 2);
			if (error)
				return error;
		}

		lo ++;
	}
}

static fz_error *parsebfrange(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;
	fz_error *error;
	int lo, hi, dst;

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);

		if (token == TENDBFRANGE)
			return nil;

		else if (token != PDF_TSTRING)
			goto cleanup;

		lo = codefromstring(buf, len);

		token = mylex(file, buf, sizeof buf, &len);
		if (token != PDF_TSTRING)
			goto cleanup;

		hi = codefromstring(buf, len);

		token = mylex(file, buf, sizeof buf, &len);

		if (token == PDF_TSTRING)
		{
			if (len == 2)
			{
				dst = codefromstring(buf, len);
				error = pdf_maprangetorange(cmap, lo, hi, dst);
				if (error)
					return error;
			}
			else
			{
				int dststr[256];
				int i;

				if (len / 2)
				{
					for (i = 0; i < len / 2; i++)
						dststr[i] = codefromstring(buf + i * 2, 2);

					while (lo <= hi)
					{
						dststr[i-1] ++;
						error = pdf_maponetomany(cmap, lo, dststr, i);
						if (error)
							return error;
						lo ++;
					}
				}
			}
		}

		else if (token == PDF_TOARRAY)
		{
			error = parsebfrangearray(cmap, file, lo, hi);
			if (error)
				return error;
		}

		else
		{
			goto cleanup;
		}
	}

cleanup:
	return fz_throw("syntaxerror in CMap bfrange section");
}

static fz_error *parsebfchar(pdf_cmap *cmap, fz_stream *file)
{
	char buf[256];
	int token;
	int len;
	fz_error *error;
	int dst[256];
	int src;
	int i;

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);

		if (token == TENDBFCHAR)
			return nil;

		else if (token != PDF_TSTRING)
			goto cleanup;

		src = codefromstring(buf, len);

		token = mylex(file, buf, sizeof buf, &len);
		/* Note: does not handle /dstName */
		if (token != PDF_TSTRING)
			goto cleanup;

		if (len / 2)
		{
			for (i = 0; i < len / 2; i++)
				dst[i] = codefromstring(buf + i * 2, 2);

			error = pdf_maponetomany(cmap, src, dst, i);
			if (error)
				return error;
		}
	}

cleanup:
	return fz_throw("syntaxerror in CMap bfchar section");
}

fz_error *
pdf_parsecmap(pdf_cmap **cmapp, fz_stream *file)
{
	fz_error *error;
	pdf_cmap *cmap;
	char key[64];
	char buf[256];
	int token;
	int len;

	error = pdf_newcmap(&cmap);
	if (error)
		return error;

	strcpy(key, ".notdef");

	while (1)
	{
		token = mylex(file, buf, sizeof buf, &len);

		if (token == PDF_TEOF)
			break;

		else if (token == PDF_TERROR)
		{
			error = fz_throw("syntaxerror in CMap");
			goto cleanup;
		}

		else if (token == PDF_TNAME)
		{
			if (!strcmp(buf, "CMapName"))
			{
				error = parsecmapname(cmap, file);
				if (error)
					goto cleanup;
			}
			else if (!strcmp(buf, "WMode"))
			{
				error = parsewmode(cmap, file);
				if (error)
					goto cleanup;
			}
			else
				strlcpy(key, buf, sizeof key);
		}

		else if (token == TUSECMAP)
		{
			strlcpy(cmap->usecmapname, key, sizeof(cmap->usecmapname));
		}

		else if (token == TBEGINCODESPACERANGE)
		{
			error = parsecodespacerange(cmap, file);
			if (error)
				goto cleanup;
		}

		else if (token == TBEGINBFCHAR)
		{
			error = parsebfchar(cmap, file);
			if (error)
				goto cleanup;
		}

		else if (token == TBEGINCIDCHAR)
		{
			error = parsecidchar(cmap, file);
			if (error)
				goto cleanup;
		}

		else if (token == TBEGINBFRANGE)
		{
			error = parsebfrange(cmap, file);
			if (error)
				goto cleanup;
		}

		else if (token == TBEGINCIDRANGE)
		{
			error = parsecidrange(cmap, file);
			if (error)
				goto cleanup;
		}

		/* ignore everything else */
	}

	error = pdf_sortcmap(cmap);
	if (error)
		goto cleanup;

	*cmapp = cmap;
	return nil;

cleanup:
	pdf_dropcmap(cmap);
	return error;
}

/*
 * Load CMap stream in PDF file
 */
fz_error *
pdf_loadembeddedcmap(pdf_cmap **cmapp, pdf_xref *xref, fz_obj *stmref)
{
	fz_obj *stmobj = stmref;
	fz_error *error = nil;
	fz_stream *file;
	pdf_cmap *cmap = nil;
	pdf_cmap *usecmap;
	fz_obj *wmode;
	fz_obj *obj;

	if ((*cmapp = pdf_finditem(xref->store, PDF_KCMAP, stmref)))
	{
		pdf_keepcmap(*cmapp);
		return nil;
	}

	pdf_logfont("load embedded cmap %d %d {\n", fz_tonum(stmref), fz_togen(stmref));

	error = pdf_resolve(&stmobj, xref);
	if (error)
		return error;

	error = pdf_openstream(&file, xref, fz_tonum(stmref), fz_togen(stmref));
	if (error)
		goto cleanup;

	error = pdf_parsecmap(&cmap, file);
	if (error)
		goto cleanup;

	fz_dropstream(file);

	wmode = fz_dictgets(stmobj, "WMode");
	if (fz_isint(wmode))
	{
		pdf_logfont("wmode %d\n", wmode);
		pdf_setwmode(cmap, fz_toint(wmode));
	}

	obj = fz_dictgets(stmobj, "UseCMap");
	if (fz_isname(obj))
	{
		pdf_logfont("usecmap /%s\n", fz_toname(obj));
		error = pdf_loadsystemcmap(&usecmap, fz_toname(obj));
		if (error)
			goto cleanup;
		pdf_setusecmap(cmap, usecmap);
		pdf_dropcmap(usecmap);
	}
	else if (fz_isindirect(obj))
	{
		pdf_logfont("usecmap %d %d R\n", fz_tonum(obj), fz_togen(obj));
		error = pdf_loadembeddedcmap(&usecmap, xref, obj);
		if (error)
			goto cleanup;
		pdf_setusecmap(cmap, usecmap);
		pdf_dropcmap(usecmap);
	}

	pdf_logfont("}\n");

	error = pdf_storeitem(xref->store, PDF_KCMAP, stmref, cmap);
	if (error)
		goto cleanup;

	fz_dropobj(stmobj);

	*cmapp = cmap;
	return nil;

cleanup:
	if (cmap)
		pdf_dropcmap(cmap);
	fz_dropobj(stmobj);
	return error;
}

/*
 * Load predefined CMap from system
 */
fz_error *
pdf_loadsystemcmap(pdf_cmap **cmapp, char *name)
{
	fz_error *error = nil;
	fz_stream *file;
	char *cmapdir;
	char *usecmapname;
	pdf_cmap *usecmap;
	pdf_cmap *cmap;
	char path[1024];

	cmap = nil;
	file = nil;

	pdf_logfont("load system cmap %s {\n", name);

	cmapdir = getenv("CMAPDIR");
	if (!cmapdir)
		return fz_throw("ioerror: CMAPDIR environment not set");

	strlcpy(path, cmapdir, sizeof path);
	strlcat(path, "/", sizeof path);
	strlcat(path, name, sizeof path);

	error = fz_openrfile(&file, path);
	if (error)
		goto cleanup;

	error = pdf_parsecmap(&cmap, file);
	if (error)
		goto cleanup;

	fz_dropstream(file);

	usecmapname = cmap->usecmapname;
	if (usecmapname[0])
	{
		pdf_logfont("usecmap %s\n", usecmapname);
		error = pdf_loadsystemcmap(&usecmap, usecmapname);
		if (error)
			goto cleanup;
		pdf_setusecmap(cmap, usecmap);
		pdf_dropcmap(usecmap);
	}

	pdf_logfont("}\n");

	*cmapp = cmap;
	return nil;

cleanup:
	if (cmap)
		pdf_dropcmap(cmap);
	if (file)
		fz_dropstream(file);
	return error;
}

/*
 * Create an Identity-* CMap (for both 1 and 2-byte encodings)
 */
fz_error *
pdf_newidentitycmap(pdf_cmap **cmapp, int wmode, int bytes)
{
	fz_error *error;
	pdf_cmap *cmap;

	error = pdf_newcmap(&cmap);
	if (error)
		return error;

	sprintf(cmap->cmapname, "Identity-%c", wmode ? 'V' : 'H');

	error = pdf_addcodespace(cmap, 0x0000, 0xffff, bytes);
	if (error) {
		pdf_dropcmap(cmap);
		return error;
	}

	error = pdf_maprangetorange(cmap, 0x0000, 0xffff, 0);
	if (error) {
		pdf_dropcmap(cmap);
		return error;
	}

	error = pdf_sortcmap(cmap);
	if (error) {
		pdf_dropcmap(cmap);
		return error;
	}

	pdf_setwmode(cmap, wmode);

	*cmapp = cmap;
	return nil;
}

