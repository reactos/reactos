#include <fitz.h>
#include <mupdf.h>

/*
 * create xref structure.
 * needs to be initialized by initxref, openxref or repairxref.
 */

fz_error *
pdf_newxref(pdf_xref **xrefp)
{
	pdf_xref *xref;

	xref = fz_malloc(sizeof(pdf_xref));
	if (!xref)
		return fz_outofmem;
	memset(xref, 0, sizeof(pdf_xref));

	pdf_logxref("newxref %p\n", xref);

	xref->file = nil;
	xref->version = 1.3f;
	xref->startxref = 0;
	xref->crypt = nil;

	xref->trailer = nil;
	xref->root = nil;
	xref->info = nil;
	xref->dests = nil;
	xref->store = nil;

	xref->cap = 0;
	xref->len = 0;
	xref->table = nil;

	xref->store = nil;	/* you need to create this if you want to render */

	*xrefp = xref;
	return nil;
}

void
pdf_closexref(pdf_xref *xref)
{
	pdf_logxref("closexref %p\n", xref);

	if (xref->store)
		fz_warn("someone forgot to empty the store before freeing xref!");

	if (xref->table)
	{
		pdf_flushxref(xref, 1);
		fz_free(xref->table);
	}

	if (xref->file)
		fz_dropstream(xref->file);

	if (xref->crypt)
		pdf_dropcrypt(xref->crypt);
	if (xref->trailer)
		fz_dropobj(xref->trailer);
	if (xref->root)
		fz_dropobj(xref->root);
	if (xref->info)
		fz_dropobj(xref->info);
	if (xref->dests)
		fz_dropobj(xref->dests);

	fz_free(xref);
}

fz_error *
pdf_initxref(pdf_xref *xref)
{
	xref->table = fz_malloc(sizeof(pdf_xrefentry) * 128);
	if (!xref->table)
		return fz_outofmem;

	xref->cap = 128;
	xref->len = 1;

	xref->table[0].type = 'f';
	xref->table[0].mark = 0;
	xref->table[0].ofs = 0;
	xref->table[0].gen = 65535;
	xref->table[0].stmbuf = nil;
	xref->table[0].stmofs = 0;
	xref->table[0].obj = nil;

	return nil;
}

void
pdf_flushxref(pdf_xref *xref, int force)
{
	int i;

	pdf_logxref("flushxref %p (%d)\n", xref, force);

	for (i = 0; i < xref->len; i++)
	{
		if (force)
		{
			if (xref->table[i].stmbuf)
			{
				fz_dropbuffer(xref->table[i].stmbuf);
				xref->table[i].stmbuf = nil;
			}
			if (xref->table[i].obj)
			{
				fz_dropobj(xref->table[i].obj);
				xref->table[i].obj = nil;
			}
		}
		else
		{
			if (xref->table[i].stmbuf && xref->table[i].stmbuf->refs == 1)
			{
				fz_dropbuffer(xref->table[i].stmbuf);
				xref->table[i].stmbuf = nil;
			}
			if (xref->table[i].obj && xref->table[i].obj->refs == 1)
			{
				fz_dropobj(xref->table[i].obj);
				xref->table[i].obj = nil;
			}
		}
	}
}

void
pdf_debugxref(pdf_xref *xref)
{
	int i;
	printf("xref\n0 %d\n", xref->len);
	for (i = 0; i < xref->len; i++)
	{
		printf("%010d %05d %c | %d %c%c\n",
			xref->table[i].ofs,
			xref->table[i].gen,
			xref->table[i].type,
			xref->table[i].obj ? xref->table[i].obj->refs : 0,
			xref->table[i].stmofs ? 'f' : '-',
			xref->table[i].stmbuf ? 'b' : '-');
	}
}

/* ICKY! */
fz_error *
pdf_decryptxref(pdf_xref *xref)
{
	fz_error *error;
	fz_obj *encrypt;
	fz_obj *id;

	encrypt = fz_dictgets(xref->trailer, "Encrypt");
	id = fz_dictgets(xref->trailer, "ID");

	if (encrypt && id)
	{
		error = pdf_resolve(&encrypt, xref);
		if (error)
			return error;

		if (fz_isnull(encrypt))
		{
			fz_dropobj(encrypt);
			return nil;
		}

		error = pdf_resolve(&id, xref);
		if (error)
		{
			fz_dropobj(encrypt);
			return error;
		}

		error = pdf_newdecrypt(&xref->crypt, encrypt, id);
		fz_dropobj(encrypt);
		fz_dropobj(id);
		return error;
	}

	return nil;
}

/*
 * mutate objects
 */

static int findprev(pdf_xref *xref, int oid)
{
	int prev;
	for (prev = oid - 1; prev >= 0; prev--)
		if (xref->table[prev].type == 'f' || xref->table[prev].type == 'd')
			return prev;
	return 0;
}

static int findnext(pdf_xref *xref, int oid)
{
	int next;
	for (next = oid + 1; next < xref->len; next++)
		if (xref->table[next].type == 'f' || xref->table[next].type == 'd')
			return next;
	return 0;
}

fz_error *
pdf_allocobject(pdf_xref *xref, int *oidp, int *genp)
{
	pdf_xrefentry *x;
	int prev, next;
	int oid = 0;

	pdf_logxref("allocobj");

	while (1)
	{
		x = xref->table + oid;

		if (x->type == 'f' || x->type == 'd')
		{
			if (x->gen < 65535)
			{
				*oidp = oid;
				*genp = x->gen;

				pdf_logxref(" reuse %d %d\n", *oidp, *genp);

				x->type = 'a';
				x->ofs = 0;

				prev = findprev(xref, oid);
				next = findnext(xref, oid);
				xref->table[prev].type = 'd';
				xref->table[prev].ofs = next;

				return nil;
			}
		}

		oid = x->ofs;

		if (oid == 0)
			break;
	}

	if (xref->len + 1 >= xref->cap)
	{
		int newcap = xref->cap + 256;
		pdf_xrefentry *newtable;

		newtable = fz_realloc(xref->table, sizeof(pdf_xrefentry) * newcap);
		if (!newtable)
			return fz_outofmem;

		xref->table = newtable;
		xref->cap = newcap;
	}

	oid = xref->len ++;

	xref->table[oid].type = 'a';
	xref->table[oid].mark = 0;
	xref->table[oid].ofs = 0;
	xref->table[oid].gen = 0;
	xref->table[oid].stmbuf = nil;
	xref->table[oid].stmofs = 0;
	xref->table[oid].obj = nil;

	*oidp = oid;
	*genp = 0;

	pdf_logxref(" %d %d\n", *oidp, *genp);

	prev = findprev(xref, oid);
	next = findnext(xref, oid);
	xref->table[prev].type = 'd';
	xref->table[prev].ofs = next;

	return nil;
}

fz_error *
pdf_deleteobject(pdf_xref *xref, int oid, int gen)
{
	pdf_xrefentry *x;
	int prev;

	if (oid < 0 || oid >= xref->len)
		return fz_throw("rangecheck: object number out of range: %d", oid);

	pdf_logxref("deleteobj %d %d\n", oid, gen);

	x = xref->table + oid;

	x->type = 'd';
	x->ofs = findnext(xref, oid);
	x->gen ++;

	if (x->stmbuf)
		fz_dropbuffer(x->stmbuf);
	x->stmbuf = nil;

	if (x->obj)
		fz_dropobj(x->obj);
	x->obj = nil;

	prev = findprev(xref, oid);
	xref->table[prev].type = 'd';
	xref->table[prev].ofs = oid;

	return nil;
}

fz_error *
pdf_updateobject(pdf_xref *xref, int oid, int gen, fz_obj *obj)
{
	pdf_xrefentry *x;

	if (oid < 0 || oid >= xref->len)
		return fz_throw("rangecheck: object number out of range: %d", oid);

	pdf_logxref("updateobj %d %d (%p)\n", oid, gen, obj);

	x = xref->table + oid;

	if (x->obj)
		fz_dropobj(x->obj);
	x->obj = fz_keepobj(obj);

	if (x->type == 'f' || x->type == 'd')
	{
		int prev = findprev(xref, oid);
		int next = findnext(xref, oid);
		xref->table[prev].type = 'd';
		xref->table[prev].ofs = next;
	}

	x->type = 'a';

	return nil;
}

fz_error *
pdf_updatestream(pdf_xref *xref, int oid, int gen, fz_buffer *stm)
{
	pdf_xrefentry *x;

	if (oid < 0 || oid >= xref->len)
		return fz_throw("rangecheck: object number out of range: %d", oid);

	pdf_logxref("updatestm %d %d (%p)\n", oid, gen, stm);

	x = xref->table + oid;

	if (x->stmbuf)
		fz_dropbuffer(x->stmbuf);
	x->stmbuf = fz_keepbuffer(stm);

	return nil;
}

/*
 * object loading
 */

fz_error *
pdf_cacheobject(pdf_xref *xref, int oid, int gen)
{
	char buf[65536];	/* yeowch! */

	fz_error *error;
	pdf_xrefentry *x;
	int roid, rgen;
	int n;

	if (oid < 0 || oid >= xref->len)
		return fz_throw("rangecheck: object number out of range: %d", oid);

	x = &xref->table[oid];

	if (x->obj)
		return nil;

	if (x->type == 'f' || x->type == 'd')
	{
		error = fz_newnull(&x->obj);
		if (error)
			return error;
		return nil;
	}

	if (x->type == 'n')
	{
		n = fz_seek(xref->file, x->ofs, 0);
		if (n < 0)
			return fz_ioerror(xref->file);

		error = pdf_parseindobj(&x->obj, xref->file, buf, sizeof buf, &roid, &rgen, &x->stmofs);
		if (error)
			return error;

		if (roid != oid || rgen != gen)
			return fz_throw("syntaxerror: found wrong object");

		if (xref->crypt)
			pdf_cryptobj(xref->crypt, x->obj, oid, gen);
	}

	else if (x->type == 'o')
	{
		if (!x->obj)
		{
			error = pdf_loadobjstm(xref, x->ofs, 0, buf, sizeof buf);
			if (error)
				return error;
		}
	}

	return nil;
}

fz_error *
pdf_loadobject(fz_obj **objp, pdf_xref *xref, int oid, int gen)
{
	fz_error *error;

	error = pdf_cacheobject(xref, oid, gen);
	if (error)
		return error;

	*objp = fz_keepobj(xref->table[oid].obj);

	return nil;
}

fz_error *
pdf_loadindirect(fz_obj **objp, pdf_xref *xref, fz_obj *ref)
{
	assert(ref != nil);
	return pdf_loadobject(objp, xref, fz_tonum(ref), fz_togen(ref));
}

fz_error *
pdf_resolve(fz_obj **objp, pdf_xref *xref)
{
	if (fz_isindirect(*objp))
		return pdf_loadindirect(objp, xref, *objp);
	fz_keepobj(*objp);
	return nil;
}

