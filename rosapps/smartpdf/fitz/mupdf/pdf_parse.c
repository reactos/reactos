#include <fitz.h>
#include <mupdf.h>

fz_rect pdf_torect(fz_obj *array)
{
	fz_rect r;
	float a = fz_toreal(fz_arrayget(array, 0));
	float b = fz_toreal(fz_arrayget(array, 1));
	float c = fz_toreal(fz_arrayget(array, 2));
	float d = fz_toreal(fz_arrayget(array, 3));
	r.x0 = MIN(a, c);
	r.y0 = MIN(b, d);
	r.x1 = MAX(a, c);
	r.y1 = MAX(b, d);
	return r;
}

fz_matrix pdf_tomatrix(fz_obj *array)
{
	fz_matrix m;
	m.a = fz_toreal(fz_arrayget(array, 0));
	m.b = fz_toreal(fz_arrayget(array, 1));
	m.c = fz_toreal(fz_arrayget(array, 2));
	m.d = fz_toreal(fz_arrayget(array, 3));
	m.e = fz_toreal(fz_arrayget(array, 4));
	m.f = fz_toreal(fz_arrayget(array, 5));
	return m;
}

fz_error *
pdf_toutf8(char **dstp, fz_obj *src)
{
	unsigned char *srcptr = fz_tostrbuf(src);
	char *dstptr;
	int srclen = fz_tostrlen(src);
	int dstlen = 0;
	int ucs;
	int i;

	if (srclen > 2 && srcptr[0] == 254 && srcptr[1] == 255)
	{
		for (i = 2; i < srclen; i += 2)
		{
			ucs = (srcptr[i] << 8) | srcptr[i+1];
			dstlen += runelen(ucs);
		}

		dstptr = *dstp = fz_malloc(dstlen + 1);
		if (!dstptr)
			return fz_outofmem;

		for (i = 2; i < srclen; i += 2)
		{
			ucs = (srcptr[i] << 8) | srcptr[i+1];
			dstptr += runetochar(dstptr, &ucs);
		}
	}

	else
	{
		for (i = 0; i < srclen; i++)
			dstlen += runelen(pdf_docencoding[srcptr[i]]);

		dstptr = *dstp = fz_malloc(dstlen + 1);
		if (!dstptr)
			return fz_outofmem;

		for (i = 0; i < srclen; i++)
		{
			ucs = pdf_docencoding[srcptr[i]];
			dstptr += runetochar(dstptr, &ucs);
		}
	}

	*dstptr = '\0';
	return nil;
}

fz_error *
pdf_toucs2(unsigned short **dstp, fz_obj *src)
{
	unsigned char *srcptr = fz_tostrbuf(src);
	unsigned short *dstptr;
	int srclen = fz_tostrlen(src);
	int i;

	if (srclen > 2 && srcptr[0] == 254 && srcptr[1] == 255)
	{
		dstptr = *dstp = fz_malloc(((srclen - 2) / 2 + 1) * sizeof(short));
		if (!dstptr)
			return fz_outofmem;
		for (i = 2; i < srclen; i += 2)
			*dstptr++ = (srcptr[i] << 8) | srcptr[i+1];
	}

	else
	{
		dstptr = *dstp = fz_malloc((srclen + 1) * sizeof(short));
		if (!dstptr)
			return fz_outofmem;
		for (i = 0; i < srclen; i++)
			*dstptr++ = pdf_docencoding[srcptr[i]];
	}

	*dstptr = '\0';
	return nil;
}

fz_error *
pdf_parsearray(fz_obj **op, fz_stream *file, char *buf, int cap)
{
	fz_error *error = nil;
	fz_obj *ary = nil;
	fz_obj *obj = nil;
	int a = 0, b = 0, n = 0;
	int tok, len;

	error = fz_newarray(op, 4);
	if (error) return error;
	ary = *op;

	while (1)
	{
		tok = pdf_lex(file, buf, cap, &len);

		if (tok != PDF_TINT && tok != PDF_TR)
		{
			if (n > 0)
			{
				error = fz_newint(&obj, a);
				if (error) goto cleanup;
				error = fz_arraypush(ary, obj);
				if (error) goto cleanup;
				fz_dropobj(obj);
				obj = nil;
			}
			if (n > 1)
			{
				error = fz_newint(&obj, b);
				if (error) goto cleanup;
				error = fz_arraypush(ary, obj);
				if (error) goto cleanup;
				fz_dropobj(obj);
				obj = nil;
			}
			n = 0;
		}

		if (tok == PDF_TINT && n == 2)
		{
			error = fz_newint(&obj, a);
			if (error) goto cleanup;
			error = fz_arraypush(ary, obj);
			if (error) goto cleanup;
			fz_dropobj(obj);
			obj = nil;
			a = b;
			n --;
		}

		switch (tok)
		{
		case PDF_TCARRAY:
			return nil;
		case PDF_TINT:
			if (n == 0)
				a = atoi(buf);
			if (n == 1)
				b = atoi(buf);
			n ++;
			break;
		case PDF_TR:
			if (n != 2)
				goto cleanup;
			error = fz_newindirect(&obj, a, b);
			if (error) goto cleanup;
			n = 0;
			break;
		case PDF_TOARRAY:	error = pdf_parsearray(&obj, file, buf, cap); break;
		case PDF_TODICT:	error = pdf_parsedict(&obj, file, buf, cap); break;
		case PDF_TNAME:		error = fz_newname(&obj, buf); break;
		case PDF_TREAL:		error = fz_newreal(&obj, atof(buf)); break;
		case PDF_TSTRING:	error = fz_newstring(&obj, buf, len); break;
		case PDF_TTRUE:		error = fz_newbool(&obj, 1); break;
		case PDF_TFALSE:	error = fz_newbool(&obj, 0); break;
		case PDF_TNULL:		error = fz_newnull(&obj); break;
		default:		goto cleanup;
		}
		if (error) goto cleanup;

		if (obj)
		{
			error = fz_arraypush(ary, obj);
			if (error) goto cleanup;
			fz_dropobj(obj);
		}

		obj = nil;
	}

cleanup:
	if (obj) fz_dropobj(obj);
	if (ary) fz_dropobj(ary);
	if (error) return error;
	return fz_throw("syntaxerror: corrupt array");
}

fz_error *
pdf_parsedict(fz_obj **op, fz_stream *file, char *buf, int cap)
{
	fz_error *error = nil;
	fz_obj *dict = nil;
	fz_obj *key = nil;
	fz_obj *val = nil;
	int tok, len;
	int a, b;

	error = fz_newdict(op, 8);
	if (error) return error;
	dict = *op;

	while (1)
	{
		tok = pdf_lex(file, buf, cap, &len);

skip:
		if (tok == PDF_TCDICT)
			return nil;

		/* for BI .. ID .. EI in content streams */
		if (tok == PDF_TKEYWORD && !strcmp(buf, "ID"))
			return nil;

		if (tok != PDF_TNAME)
			goto cleanup;

		error = fz_newname(&key, buf);
		if (error) 
            goto cleanup;

		tok = pdf_lex(file, buf, cap, &len);

		switch (tok)
		{
		case PDF_TOARRAY:	error = pdf_parsearray(&val, file, buf, cap); break;
		case PDF_TODICT:	error = pdf_parsedict(&val, file, buf, cap); break;
		case PDF_TNAME:		error = fz_newname(&val, buf); break;
		case PDF_TREAL:		error = fz_newreal(&val, atof(buf)); break;
		case PDF_TSTRING:	error = fz_newstring(&val, buf, len); break;
		case PDF_TTRUE:		error = fz_newbool(&val, 1); break;
		case PDF_TFALSE:	error = fz_newbool(&val, 0); break;
		case PDF_TNULL:		error = fz_newnull(&val); break;
		case PDF_TINT:
			a = atoi(buf);
			tok = pdf_lex(file, buf, cap, &len);
			if (tok == PDF_TCDICT || tok == PDF_TNAME ||
				(tok == PDF_TKEYWORD && !strcmp(buf, "ID")))
			{
				error = fz_newint(&val, a);
				if (error) goto cleanup;
				error = fz_dictput(dict, key, val);
				if (error) 
                    goto cleanup;
				fz_dropobj(val);
				fz_dropobj(key);
				key = val = nil;
				goto skip;
			}
			if (tok == PDF_TINT)
			{
				b = atoi(buf);
				tok = pdf_lex(file, buf, cap, &len);
				if (tok == PDF_TR)
				{
					error = fz_newindirect(&val, a, b);
					break;
				}
			}
			goto cleanup;
		default:
			goto cleanup;
		}

		if (error) 
            goto cleanup;

		error = fz_dictput(dict, key, val);
		if (error) 
            goto cleanup;

		fz_dropobj(val);
		fz_dropobj(key);
		key = val = nil;
	}

cleanup:
	if (key) 
        fz_dropobj(key);
	if (val)
        fz_dropobj(val);
	if (dict) 
        fz_dropobj(dict);
    *op = nil;
    if (error) return error;
	return fz_throw("syntaxerror: corrupt dictionary");
}

fz_error *
pdf_parsestmobj(fz_obj **op, fz_stream *file, char *buf, int cap)
{
	int tok, len;

	tok = pdf_lex(file, buf, cap, &len);

	switch (tok)
	{
		case PDF_TOARRAY:	return pdf_parsearray(op, file, buf, cap);
		case PDF_TODICT:	return pdf_parsedict(op, file, buf, cap);
		case PDF_TNAME:		return fz_newname(op, buf);
		case PDF_TREAL:		return fz_newreal(op, atof(buf));
		case PDF_TSTRING:	return fz_newstring(op, buf, len);
		case PDF_TTRUE:		return fz_newbool(op, 1);
		case PDF_TFALSE:	return fz_newbool(op, 0);
		case PDF_TNULL:		return fz_newnull(op);
		case PDF_TINT:		return fz_newint(op, atoi(buf));
	}

	return fz_throw("syntaxerror: corrupt object stream");
}

fz_error *
pdf_parseindobj(fz_obj **op, fz_stream *file, char *buf, int cap,
		int *ooid, int *ogid, int *ostmofs)
{
	fz_error *error = nil;
	fz_obj *obj = nil;
	int oid = 0, gid = 0, stmofs;
	int tok, len;
	int a, b;

	tok = pdf_lex(file, buf, cap, &len);
	if (tok != PDF_TINT)
		goto cleanup;
	oid = atoi(buf);

	tok = pdf_lex(file, buf, cap, &len);
	if (tok != PDF_TINT)
		goto cleanup;
	gid = atoi(buf);

	tok = pdf_lex(file, buf, cap, &len);
	if (tok != PDF_TOBJ)
		goto cleanup;

	tok = pdf_lex(file, buf, cap, &len);
	switch (tok)
	{
		case PDF_TOARRAY:	error = pdf_parsearray(&obj, file, buf, cap); break;
		case PDF_TODICT:	error = pdf_parsedict(&obj, file, buf, cap); break;
		case PDF_TNAME:		error = fz_newname(&obj, buf); break;
		case PDF_TREAL:		error = fz_newreal(&obj, atof(buf)); break;
		case PDF_TSTRING:	error = fz_newstring(&obj, buf, len); break;
		case PDF_TTRUE:		error = fz_newbool(&obj, 1); break;
		case PDF_TFALSE:	error = fz_newbool(&obj, 0); break;
		case PDF_TNULL:		error = fz_newnull(&obj); break;
		case PDF_TINT:
			a = atoi(buf);
			tok = pdf_lex(file, buf, cap, &len);
			if (tok == PDF_TSTREAM || tok == PDF_TENDOBJ)
			{
				error = fz_newint(&obj, a);
				if (error) goto cleanup;
				goto skip;
			}
			if (tok == PDF_TINT)
			{
				b = atoi(buf);
				tok = pdf_lex(file, buf, cap, &len);
				if (tok == PDF_TR)
				{
					error = fz_newindirect(&obj, a, b);
					break;
				}
			}
			goto cleanup;
		default:
			goto cleanup;
	}
	if (error) goto cleanup;

	tok = pdf_lex(file, buf, cap, &len);

skip:
	if (tok == PDF_TSTREAM)
	{
		int c = fz_readbyte(file);
		if (c == '\r')
		{
			c = fz_peekbyte(file);
			if (c != '\n')
				fz_warn("syntaxerror: DOS format line ending after stream keyword (%d %d)\n", oid, gid);
			else
				c = fz_readbyte(file);
		}
		stmofs = fz_tell(file);
	}
	else if (tok == PDF_TENDOBJ)
		stmofs = 0;
	else
		goto cleanup;

	if (ooid) *ooid = oid;
	if (ogid) *ogid = gid;
	if (ostmofs) *ostmofs = stmofs;
	*op = obj;
	return nil;

cleanup:
	if (obj) fz_dropobj(obj);
	if (error) return error;
	return fz_throw("syntaxerror: corrupt indirect object (%d %d)", oid, gid);
}

