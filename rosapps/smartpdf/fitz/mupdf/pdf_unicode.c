#include <fitz.h>
#include <mupdf.h>

/*
 * ToUnicode map for fonts
 */

fz_error *
pdf_loadtounicode(pdf_font *font, pdf_xref *xref,
	char **strings, char *collection, fz_obj *cmapstm)
{
	fz_error *error;
	pdf_cmap *cmap;
	int cid;
	int ucs;
	int i;

	if (fz_isindirect(cmapstm))
	{
		pdf_logfont("tounicode embedded cmap\n");

		error = pdf_loadembeddedcmap(&cmap, xref, cmapstm);
		if (error)
			return error;

		error = pdf_newcmap(&font->tounicode);
		if (error)
			goto cleanup;

		for (i = 0; i < (strings ? 256 : 65536); i++)
		{
			cid = pdf_lookupcmap(font->encoding, i);
			if (cid > 0)
			{
				ucs = pdf_lookupcmap(cmap, i);
				if (ucs > 0)
				{
					error = pdf_maprangetorange(font->tounicode, cid, cid, ucs);
					if (error)
						goto cleanup;
				}
			}
		}

		error = pdf_sortcmap(font->tounicode);
		if (error)
			goto cleanup;

	cleanup:
		pdf_dropcmap(cmap);
		return error;
	}

	else if (collection)
	{
		pdf_logfont("tounicode cid collection\n");

		if (!strcmp(collection, "Adobe-CNS1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-CNS1-UCS2");
		else if (!strcmp(collection, "Adobe-GB1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-GB1-UCS2");
		else if (!strcmp(collection, "Adobe-Japan1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-Japan1-UCS2");
		else if (!strcmp(collection, "Adobe-Japan2"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-Japan2-UCS2");
		else if (!strcmp(collection, "Adobe-Korea1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-Korea1-UCS2");
	}

	if (strings)
	{
		pdf_logfont("tounicode strings\n");

		/* TODO use tounicode cmap here ... for one-to-many mappings */

		font->ncidtoucs = 256;
		font->cidtoucs = fz_malloc(256 * sizeof(unsigned short));
		if (!font->cidtoucs)
			return fz_outofmem;

		for (i = 0; i < 256; i++)
		{
			if (strings[i])
			{
				int aglbuf[256];
				int aglnum;
				aglnum = pdf_lookupagl(strings[i], aglbuf, nelem(aglbuf));
				if (aglnum > 0)
					font->cidtoucs[i] = aglbuf[0];
				else
					font->cidtoucs[i] = '?';
			}
			else
				font->cidtoucs[i] = '?';
		}

		return nil;
	}

	pdf_logfont("tounicode impossible");
	return nil;
}

/*
 * Extract lines of text from display tree
 */

fz_error *
pdf_newtextline(pdf_textline **linep)
{
	pdf_textline *line;
	line = *linep = fz_malloc(sizeof(pdf_textline));
	if (!line)
		return fz_outofmem;
	line->len = 0;
	line->cap = 0;
	line->text = nil;
	line->next = nil;
	return nil;
}

void
pdf_droptextline(pdf_textline *line)
{
	if (line->next)
		pdf_droptextline(line->next);
	fz_free(line->text);
	fz_free(line);
}

static fz_error *
addtextchar(pdf_textline *line, fz_irect bbox, int c)
{
	pdf_textchar *newtext;
	int newcap;

	if (line->len + 1 >= line->cap)
	{
		newcap = line->cap ? line->cap * 2 : 80;
		newtext = fz_realloc(line->text, sizeof(pdf_textchar) * newcap);
		if (!newtext)
			return fz_outofmem;
		line->cap = newcap;
		line->text = newtext;
	}

	line->text[line->len].bbox = bbox;
	line->text[line->len].c = c;
	line->len ++;

	return nil;
}

/* XXX global! not reentrant! */
static fz_point oldpt = { 0, 0 };

static fz_error *
extracttext(pdf_textline **line, fz_node *node, fz_matrix ctm)
{
	fz_error *error;

	if (fz_istextnode(node))
	{
		fz_textnode *text = (fz_textnode*)node;
		pdf_font *font = (pdf_font*)text->font;
		fz_matrix inv = fz_invertmatrix(text->trm);
		fz_matrix tm = text->trm;
		fz_matrix trm;
		float dx, dy, t;
		fz_point p;
		fz_point vx;
		fz_point vy;
		fz_vmtx v;
		fz_hmtx h;
		int i, g;
		int x, y;
		fz_irect box;
		int c;

		for (i = 0; i < text->len; i++)
		{
			g = text->els[i].cid;

			tm.e = text->els[i].x;
			tm.f = text->els[i].y;
			trm = fz_concat(tm, ctm);
			x = trm.e;
			y = trm.f;
			trm.e = 0;
			trm.f = 0;

			p.x = text->els[i].x;
			p.y = text->els[i].y;
			p = fz_transformpoint(inv, p);
			dx = oldpt.x - p.x;
			dy = oldpt.y - p.y;
			oldpt = p;

			if (text->font->wmode == 0)
			{
				h = fz_gethmtx(text->font, g);
				oldpt.x += h.w * 0.001;

				vx.x = h.w * 0.001; vx.y = 0;
				vy.x = 0; vy.y = 1;
			}
			else
			{
				v = fz_getvmtx(text->font, g);
				oldpt.y += v.w * 0.001;
				t = dy; dy = dx; dx = t;

				vx.x = 0.5; vx.y = 0;
				vy.x = 0; vy.y = v.w * 0.001;
			}

			if (fabs(dy) > 0.2)
			{
				pdf_textline *newline;
				error = pdf_newtextline(&newline);
				if (error)
					return error;
				(*line)->next = newline;
				*line = newline;
			}
			else if (fabs(dx) > 0.2)
			{
				box.x0 = x; box.x1 = x;
				box.y0 = y; box.y1 = y;
				error = addtextchar(*line, box, ' ');
				if (error)
					return error;
			}

			vx = fz_transformpoint(trm, vx);
			vy = fz_transformpoint(trm, vy);
			box.x0 = MIN(0, MIN(vx.x, vy.x)) + x;
			box.x1 = MAX(0, MAX(vx.x, vy.x)) + x;
			box.y0 = MIN(0, MIN(vx.y, vy.y)) + y;
			box.y1 = MAX(0, MAX(vx.y, vy.y)) + y;

			if (font->tounicode)
				c = pdf_lookupcmap(font->tounicode, g);
			else if (g < font->ncidtoucs)
				c = font->cidtoucs[g];
			else
				c = g;

			error = addtextchar(*line, box, c);
			if (error)
				return error;
		}
	}

	if (fz_istransformnode(node))
		ctm = fz_concat(((fz_transformnode*)node)->m, ctm);

	for (node = node->first; node; node = node->next)
	{
		error = extracttext(line, node, ctm);
		if (error)
			return error;
	}

	return nil;
}

fz_error *
pdf_loadtextfromtree(pdf_textline **outp, fz_tree *tree, fz_matrix ctm)
{
	pdf_textline *root;
	pdf_textline *line;
	fz_error *error;

	oldpt.x = -1;
	oldpt.y = -1;

	error = pdf_newtextline(&root);
	if (error)
		return error;

	line = root;

	error = extracttext(&line, tree->root, ctm);
	if (error)
	{
		pdf_droptextline(root);
		return error;
	}

	*outp = root;
	return nil;
}

void
pdf_debugtextline(pdf_textline *line)
{
	char buf[10];
	int c, n, k, i;

	for (i = 0; i < line->len; i++)
	{
		c = line->text[i].c;
		if (c < 128)
			putchar(c);
		else
		{
			n = runetochar(buf, &c);
			for (k = 0; k < n; k++)
				putchar(buf[k]);
		}
	}
	putchar('\n');

	if (line->next)
		pdf_debugtextline(line->next);
}

