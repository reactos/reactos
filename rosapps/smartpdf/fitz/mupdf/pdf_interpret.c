#include <fitz.h>
#include <mupdf.h>

fz_error *
pdf_newcsi(pdf_csi **csip, int maskonly)
{
	fz_error *error;
	pdf_csi *csi;
	fz_node *node;

	csi = *csip = fz_malloc(sizeof(pdf_csi));
	if (!csi)
		return fz_outofmem;

	pdf_initgstate(&csi->gstate[0]);

	csi->gtop = 0;
	csi->top = 0;
	csi->array = nil;
	csi->xbalance = 0;

	error = fz_newpathnode(&csi->path);
	if (error) {
		fz_free(csi);
		return error;
	}

	error = fz_newtree(&csi->tree);
	if (error) {
		fz_dropnode((fz_node*)csi->path);
		fz_free(csi);
		return error;
	}

	error = fz_newovernode(&node);
	csi->tree->root = node;
	csi->gstate[0].head = node;

	if (maskonly)
	{
		csi->gstate[0].fill.kind = PDF_MNONE;
		csi->gstate[0].stroke.kind = PDF_MNONE;
	}

	csi->clip = 0;

	csi->textclip = nil;
	csi->textmode = 0;
	csi->text = nil;
	csi->tm = fz_identity();
	csi->tlm = fz_identity();

	return nil;
}

static void
clearstack(pdf_csi *csi)
{
	int i;
	for (i = 0; i < csi->top; i++)
		fz_dropobj(csi->stack[i]);
	csi->top = 0;
}

static fz_error *
gsave(pdf_csi *csi)
{
	if (csi->gtop == 31)
		return fz_throw("gstate overflow in content stream");

	memcpy(&csi->gstate[csi->gtop + 1], &csi->gstate[csi->gtop], sizeof(pdf_gstate));

	csi->gtop ++;

	if (csi->gstate[csi->gtop].fill.cs)
		fz_keepcolorspace(csi->gstate[csi->gtop].fill.cs);
	if (csi->gstate[csi->gtop].stroke.cs)
		fz_keepcolorspace(csi->gstate[csi->gtop].stroke.cs);

	return nil;
}

static fz_error *
grestore(pdf_csi *csi)
{
	if (csi->gtop == 0)
		return fz_throw("gstate underflow in content stream");

	if (csi->gstate[csi->gtop].fill.cs)
		fz_dropcolorspace(csi->gstate[csi->gtop].fill.cs);
	if (csi->gstate[csi->gtop].stroke.cs)
		fz_dropcolorspace(csi->gstate[csi->gtop].stroke.cs);

	csi->gtop --;

	return nil;
}

void
pdf_dropcsi(pdf_csi *csi)
{
	while (csi->gtop)
		grestore(csi);

	if (csi->gstate[csi->gtop].fill.cs)
		fz_dropcolorspace(csi->gstate[csi->gtop].fill.cs);
	if (csi->gstate[csi->gtop].stroke.cs)
		fz_dropcolorspace(csi->gstate[csi->gtop].stroke.cs);

	if (csi->path) fz_dropnode((fz_node*)csi->path);
	if (csi->clip) fz_dropnode((fz_node*)csi->clip);
	if (csi->textclip) fz_dropnode((fz_node*)csi->textclip);
	if (csi->text) fz_dropnode((fz_node*)csi->text);
	if (csi->array) fz_dropobj(csi->array);

	clearstack(csi);

	fz_free(csi);
}

/*
 * Do some magic to call the xobject subroutine.
 * Push gstate, set transform, clip, run, pop gstate.
 */

static fz_error *
runxobject(pdf_csi *csi, pdf_xref *xref, pdf_xobject *xobj)
{
	fz_error *error;
	fz_node *transform;
	fz_stream *file;

	/* gsave */
	error = gsave(csi);
	if (error)
		return error;

	/* push transform */

	error = fz_newtransformnode(&transform, xobj->matrix);
	if (error)
		return error;

	error = pdf_addtransform(csi->gstate + csi->gtop, transform);
	if (error)
	{
		fz_dropnode(transform);
		return error;
	}

	/* run contents */

	xobj->contents->rp = xobj->contents->bp;

	error = fz_openrbuffer(&file, xobj->contents);
	if (error)
		return error;

	error = pdf_runcsi(csi, xref, xobj->resources, file);

	fz_dropstream(file);

	if (error)
		return error;

	/* grestore */
	error = grestore(csi);
	if (error)
		return error;

	return nil;
}

/*
 * Decode inline image and insert into page.
 */

static fz_error *
runinlineimage(pdf_csi *csi, pdf_xref *xref, fz_obj *rdb, fz_stream *file, fz_obj *dict)
{
	fz_error *error;
	pdf_image *img = NULL;
	char buf[256];
	int token;
	int len;

	error = pdf_loadinlineimage(&img, xref, rdb, dict, file);
	if (error)
		return error;

	token = pdf_lex(file, buf, sizeof buf, &len);
	if (token != PDF_TKEYWORD || strcmp("EI", buf))
		fz_warn("syntaxerror: corrupt inline image");

	error = pdf_showimage(csi, img);
	fz_dropimage((fz_image*)img);
	return error;
}

/*
 * Set gstate params from an ExtGState dictionary.
 */

static fz_error *
runextgstate(pdf_gstate *gstate, pdf_xref *xref, fz_obj *extgstate)
{
	int i, k;

	for (i = 0; i < fz_dictlen(extgstate); i++)
	{
		fz_obj *key = fz_dictgetkey(extgstate, i);
		fz_obj *val = fz_dictgetval(extgstate, i);
		char *s = fz_toname(key);

		if (!strcmp(s, "Font"))
		{
			if (fz_isarray(val) && fz_arraylen(val) == 2)
			{
				gstate->font = pdf_finditem(xref->store, PDF_KFONT, fz_arrayget(val, 0));
				if (!gstate->font)
					return fz_throw("syntaxerror: missing font resource");
				gstate->size = fz_toreal(fz_arrayget(val, 1));
			}
			else
				return fz_throw("syntaxerror in ExtGState/Font");
		}

		else if (!strcmp(s, "LW"))
			gstate->linewidth = fz_toreal(val);
		else if (!strcmp(s, "LC"))
			gstate->linecap = fz_toint(val);
		else if (!strcmp(s, "LJ"))
			gstate->linejoin = fz_toint(val);
		else if (!strcmp(s, "ML"))
			gstate->miterlimit = fz_toreal(val);
	
		else if (!strcmp(s, "D"))
		{
			if (fz_isarray(val) && fz_arraylen(val) == 2)
			{
				fz_obj *dashes = fz_arrayget(val, 0);
				gstate->dashlen = MAX(fz_arraylen(dashes), 32);
				for (k = 0; k < gstate->dashlen; k++)
					gstate->dashlist[k] = fz_toreal(fz_arrayget(dashes, k));
				gstate->dashphase = fz_toreal(fz_arrayget(val, 1));
			}
			else
				return fz_throw("syntaxerror in ExtGState/D");
		}
	}

	return nil;
}

/*
 * The meat of the interpreter...
 */

static fz_error *
runkeyword(pdf_csi *csi, pdf_xref *xref, fz_obj *rdb, char *buf)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_error *error;
	float a, b, c, d, e, f;
	float x, y, w, h;
	fz_matrix m;
	float v[FZ_MAXCOLORS];
	int what;
	int i;

	if (strlen(buf) > 1)
	{
		if (!strcmp(buf, "BX"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			csi->xbalance ++;
		}

		else if (!strcmp(buf, "EX"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			csi->xbalance --;
		}

		else if (!strcmp(buf, "MP"))
		{
			if (csi->top != 1)
				goto syntaxerror;
		}

		else if (!strcmp(buf, "DP"))
		{
			if (csi->top != 2)
				goto syntaxerror;
		}

		else if (!strcmp(buf, "BMC"))
		{
			if (csi->top != 1)
				goto syntaxerror;
		}

		else if (!strcmp(buf, "BDC"))
		{
			if (csi->top != 2)
				goto syntaxerror;
		}

		else if (!strcmp(buf, "EMC"))
		{
			if (csi->top != 0)
				goto syntaxerror;
		}

		else if (!strcmp(buf, "cm"))
		{
			fz_matrix m;
			fz_node *transform;

			if (csi->top != 6)
				goto syntaxerror;

			m.a = fz_toreal(csi->stack[0]);
			m.b = fz_toreal(csi->stack[1]);
			m.c = fz_toreal(csi->stack[2]);
			m.d = fz_toreal(csi->stack[3]);
			m.e = fz_toreal(csi->stack[4]);
			m.f = fz_toreal(csi->stack[5]);

			error = fz_newtransformnode(&transform, m);
			if (error) return error;
			error = pdf_addtransform(gstate, transform);
			if (error)
			{
				fz_dropnode(transform);
				return error;
			}
		}

		else if (!strcmp(buf, "ri"))
		{
			if (csi->top != 1)
				goto syntaxerror;
		}

		else if (!strcmp(buf, "gs"))
		{
			fz_obj *dict;
			fz_obj *obj;

			if (csi->top != 1)
				goto syntaxerror;

			dict = fz_dictgets(rdb, "ExtGState");
			if (!dict)
				return fz_throw("syntaxerror: missing extgstate resource");

			obj = fz_dictget(dict, csi->stack[0]);
			if (!obj)
				return fz_throw("syntaxerror: missing extgstate resource");

			runextgstate(gstate, xref, obj);
		}

		else if (!strcmp(buf, "re"))
		{
			if (csi->top != 4)
				goto syntaxerror;
			x = fz_toreal(csi->stack[0]);
			y = fz_toreal(csi->stack[1]);
			w = fz_toreal(csi->stack[2]);
			h = fz_toreal(csi->stack[3]);
			error = fz_moveto(csi->path, x, y);
			if (error) return error;
			error = fz_lineto(csi->path, x + w, y);
			if (error) return error;
			error = fz_lineto(csi->path, x + w, y + h);
			if (error) return error;
			error = fz_lineto(csi->path, x, y + h);
			if (error) return error;
			error = fz_closepath(csi->path);
			if (error) return error;
		}

		else if (!strcmp(buf, "f*"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			error = pdf_showpath(csi, 0, 1, 0, 1);
			if (error) return error;
		}

		else if (!strcmp(buf, "B*"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			error = pdf_showpath(csi, 0, 1, 1, 1);
			if (error) return error;
		}

		else if (!strcmp(buf, "b*"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			error = pdf_showpath(csi, 1, 1, 1, 1);
			if (error) return error;
		}

		else if (!strcmp(buf, "W*"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			csi->clip = 1;
		}

		else if (!strcmp(buf, "cs"))
		{
			what = PDF_MFILL;
			goto Lsetcolorspace;
		}

		else if (!strcmp(buf, "CS"))
		{
			fz_colorspace *cs;
			fz_obj *obj;

			what = PDF_MSTROKE;

Lsetcolorspace:
			if (csi->top != 1)
				goto syntaxerror;

			obj = csi->stack[0];

			if (!strcmp(fz_toname(obj), "Pattern"))
			{
				error = pdf_setpattern(csi, what, nil, nil);
				if (error) return error;
			}

			else 
			{
				if (!strcmp(fz_toname(obj), "DeviceGray"))
					cs = pdf_devicegray;
				else if (!strcmp(fz_toname(obj), "DeviceRGB"))
					cs = pdf_devicergb;
				else if (!strcmp(fz_toname(obj), "DeviceCMYK"))
					cs = pdf_devicecmyk;
				else
				{
					fz_obj *dict = fz_dictgets(rdb, "ColorSpace");
					if (!dict)
						return fz_throw("syntaxerror: missing colorspace resource");
					obj = fz_dictget(dict, obj);
					if (!obj)
						return fz_throw("syntaxerror: missing colorspace resource");

					cs = pdf_finditem(xref->store, PDF_KCOLORSPACE, obj);
					if (!cs)
						return fz_throw("syntaxerror: missing colorspace resource");
				}

				error = pdf_setcolorspace(csi, what, cs);
				if (error) return error;
			}
		}

		else if (!strcmp(buf, "sc") || !strcmp(buf, "scn"))
		{
			what = PDF_MFILL;
			goto Lsetcolor;
		}

		else if (!strcmp(buf, "SC") || !strcmp(buf, "SCN"))
		{
			pdf_material *mat;
			pdf_pattern *pat;
			fz_shade *shd;
			fz_obj *dict;
			fz_obj *obj;

			what = PDF_MSTROKE;

Lsetcolor:
			mat = what == PDF_MSTROKE ? &gstate->stroke : &gstate->fill;

			if (fz_isname(csi->stack[csi->top - 1]))
				mat->kind = PDF_MPATTERN;

			switch (mat->kind)
			{
			case PDF_MNONE:
				return fz_throw("syntaxerror: cannot set color in mask objects");

			case PDF_MINDEXED:
				if (csi->top != 1)
					goto syntaxerror;
				v[0] = fz_toreal(csi->stack[0]);
				error = pdf_setcolor(csi, what, v);
				if (error) return error;
				break;

			case PDF_MCOLOR:
			case PDF_MLAB:
				if (csi->top != mat->cs->n)
					goto syntaxerror;
				for (i = 0; i < csi->top; i++)
					v[i] = fz_toreal(csi->stack[i]);
				error = pdf_setcolor(csi, what, v);
				if (error) return error;
				break;

			case PDF_MPATTERN:
				for (i = 0; i < csi->top - 1; i++)
					v[i] = fz_toreal(csi->stack[i]);

				dict = fz_dictgets(rdb, "Pattern");
				if (!dict)
					return fz_throw("syntaxerror: missing pattern resource");

				obj = fz_dictget(dict, csi->stack[csi->top - 1]);
				if (!obj)
					return fz_throw("syntaxerror: missing pattern resource");

				pat = pdf_finditem(xref->store, PDF_KPATTERN, obj);
				if (pat)
				{
					error = pdf_setpattern(csi, what, pat, csi->top == 1 ? nil : v);
					if (error) return error;
				}

				shd = pdf_finditem(xref->store, PDF_KSHADE, obj);
				if (shd)
				{
					error = pdf_setshade(csi, what, shd);
					if (error) return error;
				}

				if (!pat && !shd)
					return fz_throw("syntaxerror: missing pattern resource");

				break;

			case PDF_MSHADE:
				return fz_throw("syntaxerror: cannot set color in shade objects");
			}
		}

		else if (!strcmp(buf, "rg"))
		{
			if (csi->top != 3)
				goto syntaxerror;

			v[0] = fz_toreal(csi->stack[0]);
			v[1] = fz_toreal(csi->stack[1]);
			v[2] = fz_toreal(csi->stack[2]);

			error = pdf_setcolorspace(csi, PDF_MFILL, pdf_devicergb);
			if (error) return error;
			error = pdf_setcolor(csi, PDF_MFILL, v);
			if (error) return error;
		}

		else if (!strcmp(buf, "RG"))
		{
			if (csi->top != 3)
				goto syntaxerror;

			v[0] = fz_toreal(csi->stack[0]);
			v[1] = fz_toreal(csi->stack[1]);
			v[2] = fz_toreal(csi->stack[2]);

			error = pdf_setcolorspace(csi, PDF_MSTROKE, pdf_devicergb);
			if (error) return error;
			error = pdf_setcolor(csi, PDF_MSTROKE, v);
			if (error) return error;
		}

		else if (!strcmp(buf, "BT"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			csi->tm = fz_identity();
			csi->tlm = fz_identity();
		}

		else if (!strcmp(buf, "ET"))
		{
			if (csi->top != 0)
				goto syntaxerror;

			error = pdf_flushtext(csi);
			if (error)
				return error;

			if (csi->textclip)
			{
				error = pdf_addclipmask(gstate, csi->textclip);
				if (error) return error;
				csi->textclip = nil;
			}
		}

		else if (!strcmp(buf, "Tc"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			gstate->charspace = fz_toreal(csi->stack[0]);
		}

		else if (!strcmp(buf, "Tw"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			gstate->wordspace = fz_toreal(csi->stack[0]);
		}

		else if (!strcmp(buf, "Tz"))
		{
			if (csi->top != 1)
				goto syntaxerror;

			error = pdf_flushtext(csi);
			if (error) return error;

			gstate->scale = fz_toreal(csi->stack[0]) / 100.0;
		}

		else if (!strcmp(buf, "TL"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			gstate->leading = fz_toreal(csi->stack[0]);
		}

		else if (!strcmp(buf, "Tf"))
		{
			fz_obj *dict;
			fz_obj *obj;

			if (csi->top != 2)
				goto syntaxerror;

			dict = fz_dictgets(rdb, "Font");
			if (!dict)
				return fz_throw("syntaxerror: missing font resource");

			obj = fz_dictget(dict, csi->stack[0]);
			if (!obj)
				return fz_throw("syntaxerror: missing font resource");

			gstate->font = pdf_finditem(xref->store, PDF_KFONT, obj);
			if (!gstate->font)
				return fz_throw("syntaxerror: missing font resource");

			gstate->size = fz_toreal(csi->stack[1]);
		}

		else if (!strcmp(buf, "Tr"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			gstate->render = fz_toint(csi->stack[0]);
		}

		else if (!strcmp(buf, "Ts"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			gstate->rise = fz_toreal(csi->stack[0]);
		}

		else if (!strcmp(buf, "Td"))
		{
			if (csi->top != 2)
				goto syntaxerror;
			m = fz_translate(fz_toreal(csi->stack[0]), fz_toreal(csi->stack[1]));
			csi->tlm = fz_concat(m, csi->tlm);
			csi->tm = csi->tlm;
		}

		else if (!strcmp(buf, "TD"))
		{
			if (csi->top != 2)
				goto syntaxerror;
			gstate->leading = -fz_toreal(csi->stack[1]);
			m = fz_translate(fz_toreal(csi->stack[0]), fz_toreal(csi->stack[1]));
			csi->tlm = fz_concat(m, csi->tlm);
			csi->tm = csi->tlm;
		}

		else if (!strcmp(buf, "Tm"))
		{
			if (csi->top != 6)
				goto syntaxerror;

			error = pdf_flushtext(csi);
			if (error) return error;

			csi->tm.a = fz_toreal(csi->stack[0]);
			csi->tm.b = fz_toreal(csi->stack[1]);
			csi->tm.c = fz_toreal(csi->stack[2]);
			csi->tm.d = fz_toreal(csi->stack[3]);
			csi->tm.e = fz_toreal(csi->stack[4]);
			csi->tm.f = fz_toreal(csi->stack[5]);
			csi->tlm = csi->tm;
		}

		else if (!strcmp(buf, "T*"))
		{
			if (csi->top != 0)
				goto syntaxerror;
			m = fz_translate(0, -gstate->leading);
			csi->tlm = fz_concat(m, csi->tlm);
			csi->tm = csi->tlm;
		}

		else if (!strcmp(buf, "Tj"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			error = pdf_showtext(csi, csi->stack[0]);
			if (error) return error;
		}

		else if (!strcmp(buf, "TJ"))
		{
			if (csi->top != 1)
				goto syntaxerror;
			error = pdf_showtext(csi, csi->stack[0]);
			if (error) return error;
		}

		else if (!strcmp(buf, "Do"))
		{
			fz_obj *dict;
			fz_obj *obj;
			pdf_image *img;
			pdf_xobject *xobj;

			if (csi->top != 1)
				goto syntaxerror;

			dict = fz_dictgets(rdb, "XObject");
			if (!dict)
{
fz_debugobj(rdb);
				return fz_throw("syntaxerror: missing xobject resource");
}

			obj = fz_dictget(dict, csi->stack[0]);
			if (!obj)
				return fz_throw("syntaxerror: missing xobject resource");

			img = pdf_finditem(xref->store, PDF_KIMAGE, obj);
			xobj = pdf_finditem(xref->store, PDF_KXOBJECT, obj);

			if (!img && !xobj)
				return fz_throw("syntaxerror: missing xobject resource");

			if (img)
			{
				error = pdf_showimage(csi, img);
				if (error)
					return error;
			}

			if (xobj)
			{
				clearstack(csi);
				error = runxobject(csi, xref, xobj);
				if (error)
					return error;
			}
		}

		else if (!strcmp(buf, "sh"))
		{
			fz_obj *dict;
			fz_obj *obj;
			fz_shade *shd;

			if (csi->top != 1)
				goto syntaxerror;

			dict = fz_dictgets(rdb, "Shading");
			if (!dict)
				return fz_throw("syntaxerror: missing shading resource");

			obj = fz_dictget(dict, csi->stack[csi->top - 1]);
			if (!obj)
				return fz_throw("syntaxerror: missing shading resource");

			shd = pdf_finditem(xref->store, PDF_KSHADE, obj);
			if (!shd)
				return fz_throw("syntaxerror: missing shading resource");

			error = pdf_addshade(gstate, shd);
			if (error) return error;
		}

		else if (!strcmp(buf, "d0"))
		{
			fz_warn("unimplemented: d0 charprocs");
		}

		else if (!strcmp(buf, "d1"))
		{
		}

		else
			if (!csi->xbalance) goto syntaxerror;
	}

	else switch (buf[0])
	{

	case 'q':
		if (csi->top != 0)
			goto syntaxerror;
		error = gsave(csi);
		if (error)
			return error;
		break;

	case 'Q':
		if (csi->top != 0)
			goto syntaxerror;
		error = grestore(csi);
		if (error)
			return error;
		break;

	case 'w':
		if (csi->top != 1)
			goto syntaxerror;
		gstate->linewidth = fz_toreal(csi->stack[0]);
		break;

	case 'J':
		if (csi->top != 1)
			goto syntaxerror;
		gstate->linecap = fz_toint(csi->stack[0]);
		break;

	case 'j':
		if (csi->top != 1)
			goto syntaxerror;
		gstate->linejoin = fz_toint(csi->stack[0]);
		break;

	case 'M':
		if (csi->top != 1)
			goto syntaxerror;
		gstate->miterlimit = fz_toreal(csi->stack[0]);
		break;

	case 'd':
		if (csi->top != 2)
			goto syntaxerror;
		{
			int i;
			fz_obj *array = csi->stack[0];
			gstate->dashlen = fz_arraylen(array);
			if (gstate->dashlen > 32)
				return fz_throw("rangecheck: too large dash pattern");
			for (i = 0; i < gstate->dashlen; i++)
				gstate->dashlist[i] = fz_toreal(fz_arrayget(array, i));
			gstate->dashphase = fz_toreal(csi->stack[1]);
		}
		break;

	case 'i':
		if (csi->top != 1)
			goto syntaxerror;
		/* flatness */
		break;

	case 'm':
		if (csi->top != 2)
			goto syntaxerror;
		a = fz_toreal(csi->stack[0]);
		b = fz_toreal(csi->stack[1]);
		return fz_moveto(csi->path, a, b);

	case 'l':
		if (csi->top != 2)
			goto syntaxerror;
		a = fz_toreal(csi->stack[0]);
		b = fz_toreal(csi->stack[1]);
		return fz_lineto(csi->path, a, b);

	case 'c':
		if (csi->top != 6)
			goto syntaxerror;
		a = fz_toreal(csi->stack[0]);
		b = fz_toreal(csi->stack[1]);
		c = fz_toreal(csi->stack[2]);
		d = fz_toreal(csi->stack[3]);
		e = fz_toreal(csi->stack[4]);
		f = fz_toreal(csi->stack[5]);
		return fz_curveto(csi->path, a, b, c, d, e, f);

	case 'v':
		if (csi->top != 4)
			goto syntaxerror;
		a = fz_toreal(csi->stack[0]);
		b = fz_toreal(csi->stack[1]);
		c = fz_toreal(csi->stack[2]);
		d = fz_toreal(csi->stack[3]);
		return fz_curvetov(csi->path, a, b, c, d);

	case 'y':
		if (csi->top != 4)
			goto syntaxerror;
		a = fz_toreal(csi->stack[0]);
		b = fz_toreal(csi->stack[1]);
		c = fz_toreal(csi->stack[2]);
		d = fz_toreal(csi->stack[3]);
		return fz_curvetoy(csi->path, a, b, c, d);

	case 'h':
		if (csi->top != 0)
			goto syntaxerror;
		return fz_closepath(csi->path);

	case 'S':
		if (csi->top != 0)
			goto syntaxerror;
		error = pdf_showpath(csi, 0, 0, 1, 0);
		if (error) return error;
		break;

	case 's':
		if (csi->top != 0)
			goto syntaxerror;
		error = pdf_showpath(csi, 1, 0, 1, 0);
		if (error) return error;
		break;

	case 'F':
	case 'f':
		if (csi->top != 0)
			goto syntaxerror;
		error = pdf_showpath(csi, 0, 1, 0, 0);
		if (error) return error;
		break;

	case 'B':
		if (csi->top != 0)
			goto syntaxerror;
		error = pdf_showpath(csi, 0, 1, 1, 0);
		if (error) return error;
		break;

	case 'b':
		if (csi->top != 0)
			goto syntaxerror;
		error = pdf_showpath(csi, 1, 1, 1, 0);
		if (error) return error;
		break;

	case 'n':
		if (csi->top != 0)
			goto syntaxerror;
		error = pdf_showpath(csi, 0, 0, 0, 0);
		if (error) return error;
		break;

	case 'W':
		if (csi->top != 0)
			goto syntaxerror;
		csi->clip = 1;
		break;

	case 'g':	
		if (csi->top != 1)
			goto syntaxerror;

		v[0] = fz_toreal(csi->stack[0]);
		error = pdf_setcolorspace(csi, PDF_MFILL, pdf_devicegray);
		if (error) return error;
		error = pdf_setcolor(csi, PDF_MFILL, v);
		if (error) return error;
		break;
		
	case 'G':
		if (csi->top != 1)
			goto syntaxerror;

		v[0] = fz_toreal(csi->stack[0]);
		error = pdf_setcolorspace(csi, PDF_MSTROKE, pdf_devicegray);
		if (error) return error;
		error = pdf_setcolor(csi, PDF_MSTROKE, v);
		if (error) return error;
		break;

	case 'k':
		if (csi->top != 4)
			goto syntaxerror;

		v[0] = fz_toreal(csi->stack[0]);
		v[1] = fz_toreal(csi->stack[1]);
		v[2] = fz_toreal(csi->stack[2]);
		v[3] = fz_toreal(csi->stack[3]);

		error = pdf_setcolorspace(csi, PDF_MFILL, pdf_devicecmyk);
		if (error) return error;
		error = pdf_setcolor(csi, PDF_MFILL, v);
		if (error) return error;
		break;

	case 'K':
		if (csi->top != 4)
			goto syntaxerror;

		v[0] = fz_toreal(csi->stack[0]);
		v[1] = fz_toreal(csi->stack[1]);
		v[2] = fz_toreal(csi->stack[2]);
		v[3] = fz_toreal(csi->stack[3]);

		error = pdf_setcolorspace(csi, PDF_MSTROKE, pdf_devicecmyk);
		if (error) return error;
		error = pdf_setcolor(csi, PDF_MSTROKE, v);
		if (error) return error;
		break;

	case '\'':
		if (csi->top != 1)
			goto syntaxerror;

		m = fz_translate(0, -gstate->leading);
		csi->tlm = fz_concat(m, csi->tlm);
		csi->tm = csi->tlm;

		error = pdf_showtext(csi, csi->stack[0]);
		if (error) return error;
		break;

	case '"':
		if (csi->top != 3)
			goto syntaxerror;

		gstate->wordspace = fz_toreal(csi->stack[0]);
		gstate->charspace = fz_toreal(csi->stack[1]);

		m = fz_translate(0, -gstate->leading);
		csi->tlm = fz_concat(m, csi->tlm);
		csi->tm = csi->tlm;

		error = pdf_showtext(csi, csi->stack[2]);
		if (error) return error;
		break;

	default:
		if (!csi->xbalance) goto syntaxerror;
	}

	return nil;

syntaxerror:
	return fz_throw("syntaxerror in content stream: '%s'", buf);
}

fz_error *
pdf_runcsi(pdf_csi *csi, pdf_xref *xref, fz_obj *rdb, fz_stream *file)
{
	fz_error *error;
	char buf[65536];
	int token, len;
	fz_obj *obj;

	while (1)
	{
		if (csi->top == 31)
			return fz_throw("stack overflow in content stream");

		token = pdf_lex(file, buf, sizeof buf, &len);

		if (csi->array)
		{
			if (token == PDF_TCARRAY)
			{
				csi->stack[csi->top] = csi->array;
				csi->array = nil;
				csi->top ++;
			}
			else if (token == PDF_TINT || token == PDF_TREAL)
			{
				error = fz_newreal(&obj, atof(buf));
				if (error) return error;
				error = fz_arraypush(csi->array, obj);
				fz_dropobj(obj);
				if (error) return error;
			}
			else if (token == PDF_TSTRING)
			{
				error = fz_newstring(&obj, buf, len);
				if (error) return error;
				error = fz_arraypush(csi->array, obj);
				fz_dropobj(obj);
				if (error) return error;
			}
			else if (token == PDF_TEOF)
			{
				return nil;
			}
			else
			{
				clearstack(csi);
				return fz_throw("syntaxerror in content stream");
			}
		}

		else switch (token)
		{
		case PDF_TEOF:
			return nil;

		/* optimize text-object array parsing */
		case PDF_TOARRAY:
			error = fz_newarray(&csi->array, 8);
			if (error) return error;
			break;

		case PDF_TODICT:
			error = pdf_parsedict(&csi->stack[csi->top], file, buf, sizeof buf);
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TNAME:
			error = fz_newname(&csi->stack[csi->top], buf);
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TINT:
			error = fz_newint(&csi->stack[csi->top], atoi(buf));
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TREAL:
			error = fz_newreal(&csi->stack[csi->top], atof(buf));
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TSTRING:
			error = fz_newstring(&csi->stack[csi->top], buf, len);
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TTRUE:
			error = fz_newbool(&csi->stack[csi->top], 1);
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TFALSE:
			error = fz_newbool(&csi->stack[csi->top], 0);
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TNULL:
			error = fz_newnull(&csi->stack[csi->top]);
			if (error) return error;
			csi->top ++;
			break;

		case PDF_TKEYWORD:
			if (!strcmp(buf, "BI"))
			{
				fz_obj *obj;

				error = pdf_parsedict(&obj, file, buf, sizeof buf);
				if (error)
					return error;

				/* read whitespace after ID keyword */
				fz_readbyte(file);

				error = runinlineimage(csi, xref, rdb, file, obj);
				fz_dropobj(obj);
				if (error)
					return error;
			}
			else
			{
				error = runkeyword(csi, xref, rdb, buf);
				if (error) return error;
				clearstack(csi);
			}
			break;

		default:
			clearstack(csi);
			return fz_throw("syntaxerror in content stream");
		}
	}
}

