#include <fitz.h>
#include <mupdf.h>

/*
 * open pdf and scan objects to reconstruct xref table
 */

struct entry
{
	int oid;
	int gen;
	int ofs;
	int stmofs;
	int stmlen;
};

static fz_error *
parseobj(fz_stream *file, char *buf, int cap, int *stmofs, int *stmlen,
	int *isroot, int *isinfo)
{
	fz_error *error;
	fz_obj *dict = nil;
	fz_obj *length;
	fz_obj *filter;
	fz_obj *type;
	int tok, len;

	*stmlen = -1;
	*isroot = 0;
	*isinfo = 0;

	tok = pdf_lex(file, buf, cap, &len);
	if (tok == PDF_TODICT)
	{
		error = pdf_parsedict(&dict, file, buf, cap);
		if (error)
			return error;
	}

	if (fz_isdict(dict))
	{
		type = fz_dictgets(dict, "Type");
		if (fz_isname(type) && !strcmp(fz_toname(type), "Catalog"))
			*isroot = 1;

		filter = fz_dictgets(dict, "Filter");
		if (fz_isname(filter) && !strcmp(fz_toname(filter), "Standard"))
			return fz_throw("cannot repair encrypted files");

		if (fz_dictgets(dict, "Producer"))
			if (fz_dictgets(dict, "Creator"))
				if (fz_dictgets(dict, "Title"))
					*isinfo = 1;
	}

	while (	tok != PDF_TSTREAM &&
			tok != PDF_TENDOBJ &&
			tok != PDF_TERROR &&
			tok != PDF_TEOF )
		tok = pdf_lex(file, buf, cap, &len);

	if (tok == PDF_TSTREAM)
	{
		int c = fz_readbyte(file);
		if (c == '\r') {
			c = fz_peekbyte(file);
			if (c == '\n')
				fz_readbyte(file);
		}

		*stmofs = fz_tell(file);

		length = fz_dictgets(dict, "Length");
		if (fz_isint(length))
		{
			fz_seek(file, *stmofs + fz_toint(length), 0);
			tok = pdf_lex(file, buf, cap, &len);
			if (tok == PDF_TENDSTREAM)
				goto atobjend;
			fz_seek(file, *stmofs, 0);
		}

		fz_read(file, buf, 9);
		while (memcmp(buf, "endstream", 9) != 0)
		{
			c = fz_readbyte(file);
			if (c == EOF)
				break;
			memmove(buf, buf + 1, 8);
			buf[8] = c;
		}

		*stmlen = fz_tell(file) - *stmofs - 9;

atobjend:
		tok = pdf_lex(file, buf, cap, &len);
		if (tok == PDF_TENDOBJ)
			;
	}

	if (dict)
		fz_dropobj(dict);

	return nil;
}

fz_error *
pdf_repairxref(pdf_xref *xref, char *filename)
{
	fz_error *error;
	fz_stream *file;

	struct entry *list = nil;
	int listlen;
	int listcap;
	int maxoid = 0;

	char buf[65536];

	int oid = 0;
	int gen = 0;
	int tmpofs, oidofs = 0, genofs = 0;
	int isroot, rootoid = 0, rootgen = 0;
	int isinfo, infooid = 0, infogen = 0;
	int stmofs, stmlen;
	int tok, len;
	int next;
	int i;

	error = fz_openrfile(&file, filename);
	if (error)
		return error;

	pdf_logxref("repairxref '%s' %p\n", filename, xref);

	xref->file = file;

	/* TODO: extract version */

	listlen = 0;
	listcap = 1024;
	list = fz_malloc(listcap * sizeof(struct entry));
	if (!list)
		goto cleanup;

	while (1)
	{
		tmpofs = fz_tell(file);

		tok = pdf_lex(file, buf, sizeof buf, &len);
		if (tok == PDF_TINT)
		{
			oidofs = genofs;
			oid = gen;
			genofs = tmpofs;
			gen = atoi(buf);
		}

		if (tok == PDF_TOBJ)
		{
			error = parseobj(file, buf, sizeof buf, &stmofs, &stmlen, &isroot, &isinfo);
			if (error)
				goto cleanup;

			if (isroot) {
				pdf_logxref("found catalog: %d %d\n", oid, gen);
				rootoid = oid;
				rootgen = gen;
			}

			if (isinfo) {
				pdf_logxref("found info: %d %d\n", oid, gen);
				infooid = oid;
				infogen = gen;
			}

			if (listlen + 1 == listcap)
			{
				struct entry *newlist;
				listcap = listcap * 2;
				newlist = fz_realloc(list, listcap * sizeof(struct entry));
				if (!newlist) {
					error = fz_outofmem;
					goto cleanup;
				}
				list = newlist;
			}

			list[listlen].oid = oid;
			list[listlen].gen = gen;
			list[listlen].ofs = oidofs;
			list[listlen].stmofs = stmofs;
			list[listlen].stmlen = stmlen;
			listlen ++;

			if (oid > maxoid)
				maxoid = oid;
		}

		if (tok == PDF_TERROR)
			fz_readbyte(file);

		if (tok == PDF_TEOF)
			break;
	}

	if (rootoid == 0)
	{
		error = fz_throw("syntaxerror: could not find catalog");
		goto cleanup;
	}

	error = fz_packobj(&xref->trailer,
					"<< /Size %i /Root %r >>",
					maxoid + 1, rootoid, rootgen);
	if (error)
		goto cleanup;

	xref->len = maxoid + 1;
	xref->cap = xref->len;
	xref->table = fz_malloc(xref->cap * sizeof(pdf_xrefentry));
	if (!xref->table)
	{
		error = fz_outofmem;
		goto cleanup;
	}

	xref->table[0].type = 'f';
	xref->table[0].mark = 0;
	xref->table[0].ofs = 0;
	xref->table[0].gen = 65535;
	xref->table[0].stmbuf = nil;
	xref->table[0].stmofs = 0;
	xref->table[0].obj = nil;

	for (i = 1; i < xref->len; i++)
	{
		xref->table[i].type = 'f';
		xref->table[i].mark = 0;
		xref->table[i].ofs = 0;
		xref->table[i].gen = 0;
		xref->table[i].stmbuf = nil;
		xref->table[i].stmofs = 0;
		xref->table[i].obj = nil;
	}

	for (i = 0; i < listlen; i++)
	{
		xref->table[list[i].oid].type = 'n';
		xref->table[list[i].oid].ofs = list[i].ofs;
		xref->table[list[i].oid].gen = list[i].gen;
		xref->table[list[i].oid].mark = 0;

		xref->table[list[i].oid].stmofs = list[i].stmofs;

		/* corrected stream length */
		if (list[i].stmlen >= 0)
		{
			fz_obj *dict, *length;

			pdf_logxref("correct stream length %d %d = %d\n",
				list[i].oid, list[i].gen, list[i].stmlen);

			error = pdf_loadobject(&dict, xref, list[i].oid, list[i].gen);
			if (error)
				goto cleanup;

			error = fz_newint(&length, list[i].stmlen);
			if (error)
				goto cleanup;
			error = fz_dictputs(dict, "Length", length);
			if (error)
				goto cleanup;

			pdf_updateobject(xref, list[i].oid, list[i].gen, dict);

			fz_dropobj(dict);
		}
	}

	next = 0;
	for (i = xref->len - 1; i >= 0; i--)
	{
		if (xref->table[i].type == 'f')
		{
			xref->table[i].ofs = next;
			if (xref->table[i].gen < 65535)
				xref->table[i].gen ++;
			next = i;
		}
	}

	fz_free(list);

	return nil;

cleanup:
	assert(1 == file->refs);
	fz_dropstream(file);
	xref->file = NULL;
	fz_free(list);
	return error;
}

