#include <fitz.h>
#include <mupdf.h>

#define noHINT

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_XFREE86_H

static char *basefontnames[14][7] =
{
    { "Courier", "CourierNew", "CourierNewPSMT", 0 },
    { "Courier-Bold", "CourierNew,Bold", "Courier,Bold",
	"CourierNewPS-BoldMT", "CourierNew-Bold", 0 },
    { "Courier-Oblique", "CourierNew,Italic", "Courier,Italic",
	"CourierNewPS-ItalicMT", "CourierNew-Italic", 0 },
    { "Courier-BoldOblique", "CourierNew,BoldItalic", "Courier,BoldItalic",
	"CourierNewPS-BoldItalicMT", "CourierNew-BoldItalic", 0 },
    { "Helvetica", "ArialMT", "Arial", 0 },
    { "Helvetica-Bold", "Arial-BoldMT", "Arial,Bold", "Arial-Bold",
	"Helvetica,Bold", 0 },
    { "Helvetica-Oblique", "Arial-ItalicMT", "Arial,Italic", "Arial-Italic",
	"Helvetica,Italic", "Helvetica-Italic", 0 },
    { "Helvetica-BoldOblique", "Arial-BoldItalicMT",
	"Arial,BoldItalic", "Arial-BoldItalic",
	"Helvetica,BoldItalic", "Helvetica-BoldItalic", 0 },
    { "Times-Roman", "TimesNewRomanPSMT", "TimesNewRoman",
	"TimesNewRomanPS", 0 },
    { "Times-Bold", "TimesNewRomanPS-BoldMT", "TimesNewRoman,Bold",
	"TimesNewRomanPS-Bold", "TimesNewRoman-Bold", 0 },
    { "Times-Italic", "TimesNewRomanPS-ItalicMT", "TimesNewRoman,Italic",
	"TimesNewRomanPS-Italic", "TimesNewRoman-Italic", 0 },
    { "Times-BoldItalic", "TimesNewRomanPS-BoldItalicMT",
	"TimesNewRoman,BoldItalic", "TimesNewRomanPS-BoldItalic",
	"TimesNewRoman-BoldItalic", 0 },
    { "Symbol", 0 },
    { "ZapfDingbats", 0 }
};

/*
 * FreeType and Rendering glue
 */

enum { UNKNOWN, TYPE1, TRUETYPE, CID };

static int ftkind(FT_Face face)
{
    /*const char *kind = FT_Get_X11_Font_Format(face);
    pdf_logfont("ft font format %s\n", kind);
    if (!strcmp(kind, "TrueType"))
        return TRUETYPE;
    if (!strcmp(kind, "Type 1"))
        return TYPE1;
    if (!strcmp(kind, "CFF"))
        return TYPE1;
    if (!strcmp(kind, "CID Type 1"))
        return TYPE1;
    return UNKNOWN;*/
		/* @note: work-around */
		return TYPE1;
}

static inline int ftcidtogid(pdf_font *font, int cid)
{
	if (font->tottfcmap)
	{
		cid = pdf_lookupcmap(font->tottfcmap, cid);
		return FT_Get_Char_Index(font->ftface, cid);
	}

	if (font->cidtogid)
		return font->cidtogid[cid];

	return cid;
}

static int ftwidth(pdf_font *font, int cid)
{
	int e;

	cid = ftcidtogid(font, cid);

	e = FT_Load_Glyph(font->ftface, cid,
			FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM);
	if (e)
		return 0;
	return ((FT_Face)font->ftface)->glyph->advance.x;
}

static fz_error *
ftrender(fz_glyph *glyph, fz_font *fzfont, int cid, fz_matrix trm)
{
	pdf_font *font = (pdf_font*)fzfont;
	FT_Face face = font->ftface;
	FT_Matrix m;
	FT_Vector v;
	FT_Error fterr;
	float scale;
	int gid;
	int x, y;

	gid = ftcidtogid(font, cid);

	if (font->substitute && fzfont->wmode == 0)
	{
		fz_hmtx subw;
		int realw;

		FT_Set_Char_Size(face, 1000, 1000, 72, 72);

		fterr = FT_Load_Glyph(font->ftface, gid,
					FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM);
		if (fterr)
			return fz_throw("freetype failed to load glyph: 0x%x", fterr);

		realw = ((FT_Face)font->ftface)->glyph->advance.x;
		subw = fz_gethmtx(fzfont, cid);
		if (realw)
			scale = (float) subw.w / realw;
		else
			scale = 1.0;

		trm = fz_concat(fz_scale(scale, 1.0), trm);
	}

	glyph->w = 0;
	glyph->h = 0;
	glyph->x = 0;
	glyph->y = 0;
	glyph->samples = nil;

	/* freetype mutilates complex glyphs if they are loaded
	 * with FT_Set_Char_Size 1.0. it rounds the coordinates
	 * before applying transformation. to get more precision in
	 * freetype, we shift part of the scale in the matrix
	 * into FT_Set_Char_Size instead
	 */

#ifdef HINT
	scale = fz_matrixexpansion(trm);
	m.xx = trm.a * 65536 / scale;
	m.yx = trm.b * 65536 / scale;
	m.xy = trm.c * 65536 / scale;
	m.yy = trm.d * 65536 / scale;
	v.x = 0;
	v.y = 0;

	FT_Set_Char_Size(face, 64 * scale, 64 * scale, 72, 72);
	FT_Set_Transform(face, &m, &v);

	fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP);
	if (fterr)
		fz_warn("freetype load glyph: 0x%x", fterr);

#else

	m.xx = trm.a * 64;	/* should be 65536 */
	m.yx = trm.b * 64;
	m.xy = trm.c * 64;
	m.yy = trm.d * 64;
	v.x = trm.e * 64;
	v.y = trm.f * 64;

	FT_Set_Char_Size(face, 65536, 65536, 72, 72); /* should be 64, 64 */
	FT_Set_Transform(face, &m, &v);

	fterr = FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
	if (fterr)
		fz_warn("freetype load glyph: 0x%x", fterr);

#endif

	fterr = FT_Render_Glyph(face->glyph, ft_render_mode_normal);
	if (fterr)
		fz_warn("freetype render glyph: 0x%x", fterr);

	glyph->w = face->glyph->bitmap.width;
	glyph->h = face->glyph->bitmap.rows;
	glyph->x = face->glyph->bitmap_left;
	glyph->y = face->glyph->bitmap_top - glyph->h;
	glyph->samples = face->glyph->bitmap.buffer;

	for (y = 0; y < glyph->h / 2; y++)
	{
		for (x = 0; x < glyph->w; x++)
		{
			unsigned char a = glyph->samples[y * glyph->w + x ];
			unsigned char b = glyph->samples[(glyph->h - y - 1) * glyph->w + x];
			glyph->samples[y * glyph->w + x ] = b;
			glyph->samples[(glyph->h - y - 1) * glyph->w + x] = a;
		}
	}

	return nil;
}

/*
 * Basic encoding tables
 */

static char *cleanfontname(char *fontname)
{
	int i, k;
	for (i = 0; i < 14; i++)
		for (k = 0; basefontnames[i][k]; k++)
			if (!strcmp(basefontnames[i][k], fontname))
				return basefontnames[i][0];
	return fontname;
}

static int mrecode(char *name)
{
	int i;
	for (i = 0; i < 256; i++)
		if (pdf_macroman[i] && !strcmp(name, pdf_macroman[i]))
			return i;
	return -1;
}

/*
 * Create and destroy
 */

static void ftdropfont(fz_font *font)
{
	pdf_font *pfont = (pdf_font*)font;
	if (pfont->encoding)
		pdf_dropcmap(pfont->encoding);
	if (pfont->tottfcmap)
		pdf_dropcmap(pfont->tottfcmap);
	if (pfont->tounicode)
		pdf_dropcmap(pfont->tounicode);
	fz_free(pfont->cidtogid);
	fz_free(pfont->cidtoucs);
	if (pfont->ftface)
		FT_Done_Face((FT_Face)pfont->ftface);
	if (pfont->fontdata)
		fz_dropbuffer(pfont->fontdata);
}

pdf_font *
pdf_newfont(char *name)
{
	pdf_font *font;
	int i;

	font = fz_malloc(sizeof (pdf_font));
	if (!font)
		return nil;

	fz_initfont((fz_font*)font, name);
	font->super.render = ftrender;
	font->super.drop = (void(*)(fz_font*)) ftdropfont;

	font->ftface = nil;
	font->substitute = 0;

	font->flags = 0;
	font->italicangle = 0;
	font->ascent = 0;
	font->descent = 0;
	font->capheight = 0;
	font->xheight = 0;
	font->missingwidth = 0;

	font->encoding = nil;
	font->tottfcmap = 0;
	font->ncidtogid = 0;
	font->cidtogid = nil;

	font->tounicode = nil;
	font->ncidtoucs = 0;
	font->cidtoucs = nil;

	font->filename = nil;
	font->fontdata = nil;

	for (i = 0; i < 256; i++)
		font->charprocs[i] = nil;

	return font;
}

/*
 * Simple fonts (Type1 and TrueType)
 */

static fz_error *
loadsimplefont(pdf_font **fontp, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	fz_obj *descriptor = nil;
	fz_obj *encoding = nil;
	fz_obj *widths = nil;
	unsigned short *etable = nil;
	pdf_font *font;
	fz_irect bbox;
	FT_Face face;
	FT_CharMap cmap;
	int kind;
	int symbolic;

	char *basefont;
	char *fontname;
	char *estrings[256];
	char ebuffer[256][32];
	int i, k, n, e;

	basefont = fz_toname(fz_dictgets(dict, "BaseFont"));
	fontname = cleanfontname(basefont);

	/*
	 * Load font file
	 */

	font = pdf_newfont(fontname);
	if (!font)
		return fz_outofmem;

	pdf_logfont("load simple font %d %d (%p) {\n", fz_tonum(ref), fz_togen(ref), font);
	pdf_logfont("basefont0 %s\n", basefont);
	pdf_logfont("basefont1 %s\n", fontname);

	descriptor = fz_dictgets(dict, "FontDescriptor");
	if (descriptor && basefont == fontname)
		error = pdf_loadfontdescriptor(font, xref, descriptor, nil);
	else
		error = pdf_loadbuiltinfont(font, fontname);
	if (error)
		goto cleanup;

	face = font->ftface;
	kind = ftkind(face);

	pdf_logfont("ft name '%s' '%s'\n", face->family_name, face->style_name);

	bbox.x0 = (face->bbox.xMin * 1000) / face->units_per_EM;
	bbox.y0 = (face->bbox.yMin * 1000) / face->units_per_EM;
	bbox.x1 = (face->bbox.xMax * 1000) / face->units_per_EM;
	bbox.y1 = (face->bbox.yMax * 1000) / face->units_per_EM;

	pdf_logfont("ft bbox [%d %d %d %d]\n", bbox.x0, bbox.y0, bbox.x1, bbox.y1);

	if (bbox.x0 == bbox.x1)
		fz_setfontbbox((fz_font*)font, -1000, -1000, 2000, 2000);
	else
		fz_setfontbbox((fz_font*)font, bbox.x0, bbox.y0, bbox.x1, bbox.y1);

	/*
	 * Encoding
	 */

	symbolic = font->flags & 4;

	if (face->num_charmaps > 0)
		cmap = face->charmaps[0];
	else
		cmap = nil;

	for (i = 0; i < face->num_charmaps; i++)
	{
		FT_CharMap test = face->charmaps[i];

		if (kind == TYPE1)
		{
			if (test->platform_id == 7)
				cmap = test;
		}

		if (kind == TRUETYPE)
		{
			if (test->platform_id == 1 && test->encoding_id == 0)
				cmap = test;
			if (test->platform_id == 3 && test->encoding_id == 1)
				cmap = test;
		}
	}

	if (cmap)
	{
		e = FT_Set_Charmap(face, cmap);
		if (e)
		{
			error = fz_throw("freetype could not set cmap: 0x%x", e);
			goto cleanup;
		}
	}
	else
		fz_warn("freetype could not find any cmaps");

	etable = fz_malloc(sizeof(unsigned short) * 256);
	if (!etable)
		goto cleanup;

	for (i = 0; i < 256; i++)
	{
		estrings[i] = nil;
		etable[i] = 0;
	}

	encoding = fz_dictgets(dict, "Encoding");
	if (encoding && !(kind == TRUETYPE && symbolic))
	{
		error = pdf_resolve(&encoding, xref);
		if (error)
			goto cleanup;

		if (fz_isname(encoding))
			pdf_loadencoding(estrings, fz_toname(encoding));

		if (fz_isdict(encoding))
		{
			fz_obj *base, *diff, *item;

			base = fz_dictgets(encoding, "BaseEncoding");
			if (fz_isname(base))
				pdf_loadencoding(estrings, fz_toname(base));

			diff = fz_dictgets(encoding, "Differences");
			if (fz_isarray(diff))
			{
				n = fz_arraylen(diff);
				k = 0;
				for (i = 0; i < n; i++)
				{
					item = fz_arrayget(diff, i);
					if (fz_isint(item))
						k = fz_toint(item);
					if (fz_isname(item))
						estrings[k++] = fz_toname(item);
					if (k < 0) k = 0;
					if (k > 255) k = 255;
				}
			}
		}

		if (kind == TYPE1)
		{
			pdf_logfont("encode type1/cff by strings\n");
			for (i = 0; i < 256; i++)
				if (estrings[i])
					etable[i] = FT_Get_Name_Index(face, estrings[i]);
				else
					etable[i] = FT_Get_Char_Index(face, i);
		}

		if (kind == TRUETYPE)
		{
			/* Unicode cmap */
			if (face->charmap->platform_id == 3)
			{
				pdf_logfont("encode truetype via unicode\n");
				for (i = 0; i < 256; i++)
					if (estrings[i])
					{
						int aglbuf[256];
						int aglnum;
						aglnum = pdf_lookupagl(estrings[i], aglbuf, nelem(aglbuf));
						if (aglnum != 1)
							etable[i] = FT_Get_Name_Index(face, estrings[i]);
						else
							etable[i] = FT_Get_Char_Index(face, aglbuf[0]);
					}
					else
						etable[i] = FT_Get_Char_Index(face, i);
			}

			/* MacRoman cmap */
			else if (face->charmap->platform_id == 1)
			{
				pdf_logfont("encode truetype via macroman\n");
				for (i = 0; i < 256; i++)
					if (estrings[i])
					{
						k = mrecode(estrings[i]);
						if (k <= 0)
							etable[i] = FT_Get_Name_Index(face, estrings[i]);
						else
							etable[i] = FT_Get_Char_Index(face, k);
					}
					else
						etable[i] = FT_Get_Char_Index(face, i);
			}

			/* Symbolic cmap */
			else
			{
				pdf_logfont("encode truetype symbolic\n");
				for (i = 0; i < 256; i++)
				{
					etable[i] = FT_Get_Char_Index(face, i);
					FT_Get_Glyph_Name(face, etable[i], ebuffer[i], 32);
					if (ebuffer[i][0])
						estrings[i] = ebuffer[i];
				}
			}
		}

		fz_dropobj(encoding);
	}

	else
	{
		pdf_logfont("encode builtin\n");
		for (i = 0; i < 256; i++)
		{
			etable[i] = FT_Get_Char_Index(face, i);
			FT_Get_Glyph_Name(face, etable[i], ebuffer[i], 32);
			if (ebuffer[i][0])
				estrings[i] = ebuffer[i];
		}
	}

	error = pdf_newidentitycmap(&font->encoding, 0, 1);
	if (error)
		goto cleanup;

	font->ncidtogid = 256;
	font->cidtogid = etable;

	error = pdf_loadtounicode(font, xref,
				estrings, nil, fz_dictgets(dict, "ToUnicode"));
	if (error)
		goto cleanup;

	/*
	 * Widths
	 */

	fz_setdefaulthmtx((fz_font*)font, font->missingwidth);

	widths = fz_dictgets(dict, "Widths");
	if (widths)
	{
		int first, last;

		error = pdf_resolve(&widths, xref);
		if (error)
			goto cleanup;

		first = fz_toint(fz_dictgets(dict, "FirstChar"));
		last = fz_toint(fz_dictgets(dict, "LastChar"));

		if (first < 0 || last > 255 || first > last)
			first = last = 0;

		for (i = 0; i < last - first + 1; i++)
		{
			int wid = fz_toint(fz_arrayget(widths, i));
			error = fz_addhmtx((fz_font*)font, i + first, i + first, wid);
			if (error)
				goto cleanup;
		}

		fz_dropobj(widths);
	}
	else
	{
		FT_Set_Char_Size(face, 1000, 1000, 72, 72);
		for (i = 0; i < 256; i++)
		{
			error = fz_addhmtx((fz_font*)font, i, i, ftwidth(font, i));
			if (error)
				goto cleanup;
		}
	}

	error = fz_endhmtx((fz_font*)font);
	if (error)
		goto cleanup;

	pdf_logfont("}\n");

	*fontp = font;
	return nil;

cleanup:
	fz_free(etable);
	if (widths)
		fz_dropobj(widths);
	fz_dropfont((fz_font*)font);
	return error;
}

/*
 * CID Fonts
 */

static fz_error *
loadcidfont(pdf_font **fontp, pdf_xref *xref, fz_obj *dict, fz_obj *ref, fz_obj *encoding, fz_obj *tounicode)
{
	fz_error *error;
	fz_obj *widths = nil;
	fz_obj *descriptor;
	pdf_font *font;
	FT_Face face;
	fz_irect bbox;
	int kind;
	char collection[256];
	char *basefont;
	int i, k;

	/*
	 * Get font name and CID collection
	 */

	basefont = fz_toname(fz_dictgets(dict, "BaseFont"));

	{
		fz_obj *cidinfo;
		fz_obj *obj;
		char tmpstr[64];
		int tmplen;

		cidinfo = fz_dictgets(dict, "CIDSystemInfo");

		error = pdf_resolve(&cidinfo, xref);
		if (error)
			return error;

		obj = fz_dictgets(cidinfo, "Registry");
		tmplen = MIN(sizeof tmpstr - 1, fz_tostrlen(obj));
		memcpy(tmpstr, fz_tostrbuf(obj), tmplen);
		tmpstr[tmplen] = '\0';
		strlcpy(collection, tmpstr, sizeof collection);

		strlcat(collection, "-", sizeof collection);

		obj = fz_dictgets(cidinfo, "Ordering");
		tmplen = MIN(sizeof tmpstr - 1, fz_tostrlen(obj));
		memcpy(tmpstr, fz_tostrbuf(obj), tmplen);
		tmpstr[tmplen] = '\0';
		strlcat(collection, tmpstr, sizeof collection);

		fz_dropobj(cidinfo);
	}

	/*
	 * Load font file
	 */

	font = pdf_newfont(basefont);
	if (!font)
		return fz_outofmem;

	pdf_logfont("load cid font %d %d (%p) {\n", fz_tonum(ref), fz_togen(ref), font);
	pdf_logfont("basefont %s\n", basefont);
	pdf_logfont("collection %s\n", collection);

	descriptor = fz_dictgets(dict, "FontDescriptor");
	if (descriptor)
		error = pdf_loadfontdescriptor(font, xref, descriptor, collection);
	else
		error = fz_throw("syntaxerror: missing font descriptor");
	if (error)
		goto cleanup;

	face = font->ftface;
	kind = ftkind(face);

	bbox.x0 = (face->bbox.xMin * 1000) / face->units_per_EM;
	bbox.y0 = (face->bbox.yMin * 1000) / face->units_per_EM;
	bbox.x1 = (face->bbox.xMax * 1000) / face->units_per_EM;
	bbox.y1 = (face->bbox.yMax * 1000) / face->units_per_EM;

	pdf_logfont("ft bbox [%d %d %d %d]\n", bbox.x0, bbox.y0, bbox.x1, bbox.y1);

	if (bbox.x0 == bbox.x1)
		fz_setfontbbox((fz_font*)font, -1000, -1000, 2000, 2000);
	else
		fz_setfontbbox((fz_font*)font, bbox.x0, bbox.y0, bbox.x1, bbox.y1);

	/*
	 * Encoding
	 */

	if (fz_isname(encoding))
	{
		pdf_logfont("encoding /%s\n", fz_toname(encoding));
		if (!strcmp(fz_toname(encoding), "Identity-H"))
			error = pdf_newidentitycmap(&font->encoding, 0, 2);
		else if (!strcmp(fz_toname(encoding), "Identity-V"))
			error = pdf_newidentitycmap(&font->encoding, 1, 2);
		else
			error = pdf_loadsystemcmap(&font->encoding, fz_toname(encoding));
	}
	else if (fz_isindirect(encoding))
	{
		pdf_logfont("encoding %d %d R\n", fz_tonum(encoding), fz_togen(encoding));
		error = pdf_loadembeddedcmap(&font->encoding, xref, encoding);
	}
	else
	{
		error = fz_throw("syntaxerror: font missing encoding");
	}
	if (error)
		goto cleanup;

	fz_setfontwmode((fz_font*)font, pdf_getwmode(font->encoding));
	pdf_logfont("wmode %d\n", pdf_getwmode(font->encoding));

	if (kind == TRUETYPE)
	{
		fz_obj *cidtogidmap;

		cidtogidmap = fz_dictgets(dict, "CIDToGIDMap");
		if (fz_isindirect(cidtogidmap))
		{
			unsigned short *map;
			fz_buffer *buf;
			int len;

			pdf_logfont("cidtogidmap stream\n");

			error = pdf_loadstream(&buf, xref, fz_tonum(cidtogidmap), fz_togen(cidtogidmap));
			if (error)
				goto cleanup;

			len = (buf->wp - buf->rp) / 2;

			map = fz_malloc(len * sizeof(unsigned short));
			if (!map) {
				fz_dropbuffer(buf);
				error = fz_outofmem;
				goto cleanup;
			}

			for (i = 0; i < len; i++)
				map[i] = (buf->rp[i * 2] << 8) + buf->rp[i * 2 + 1];

			font->ncidtogid = len;
			font->cidtogid = map;

			fz_dropbuffer(buf);
		}

		/* if truetype font is external, cidtogidmap should not be identity */
		/* so we map from cid to unicode and then map that through the (3 1) */
		/* unicode cmap to get a glyph id */
		else if (font->substitute)
		{
			int e;

			pdf_logfont("emulate ttf cidfont\n");

			e = FT_Select_Charmap(face, ft_encoding_unicode);
			if (e)
				return fz_throw("fonterror: no unicode cmap when emulating CID font");

			if (!strcmp(collection, "Adobe-CNS1"))
				error = pdf_loadsystemcmap(&font->tottfcmap, "Adobe-CNS1-UCS2");
			else if (!strcmp(collection, "Adobe-GB1"))
				error = pdf_loadsystemcmap(&font->tottfcmap, "Adobe-GB1-UCS2");
			else if (!strcmp(collection, "Adobe-Japan1"))
				error = pdf_loadsystemcmap(&font->tottfcmap, "Adobe-Japan1-UCS2");
			else if (!strcmp(collection, "Adobe-Japan2"))
				error = pdf_loadsystemcmap(&font->tottfcmap, "Adobe-Japan2-UCS2");
			else if (!strcmp(collection, "Adobe-Korea1"))
				error = pdf_loadsystemcmap(&font->tottfcmap, "Adobe-Korea1-UCS2");
			else
				error = nil;

			if (error)
				return error;
		}
	}

	error = pdf_loadtounicode(font, xref, nil, collection, tounicode);
	if (error)
		goto cleanup;

	/*
	 * Horizontal
	 */

	fz_setdefaulthmtx((fz_font*)font, fz_toint(fz_dictgets(dict, "DW")));

	widths = fz_dictgets(dict, "W");
	if (widths)
	{
		int c0, c1, w;
		fz_obj *obj;

		error = pdf_resolve(&widths, xref);
		if (error)
			goto cleanup;

		for (i = 0; i < fz_arraylen(widths); )
		{
			c0 = fz_toint(fz_arrayget(widths, i));
			obj = fz_arrayget(widths, i + 1);
			if (fz_isarray(obj))
			{
				for (k = 0; k < fz_arraylen(obj); k++)
				{
					w = fz_toint(fz_arrayget(obj, k));
					error = fz_addhmtx((fz_font*)font, c0 + k, c0 + k, w);
					if (error)
						goto cleanup;
				}
				i += 2;
			}
			else
			{
				c1 = fz_toint(obj);
				w = fz_toint(fz_arrayget(widths, i + 2));
				error = fz_addhmtx((fz_font*)font, c0, c1, w);
				if (error)
					goto cleanup;
				i += 3;
			}
		}

		fz_dropobj(widths);
	}

	error = fz_endhmtx((fz_font*)font);
	if (error)
		goto cleanup;

	/*
	 * Vertical
	 */

	if (pdf_getwmode(font->encoding) == 1)
	{
		fz_obj *obj;
		int dw2y = 880;
		int dw2w = -1000;

		obj = fz_dictgets(dict, "DW2");
		if (obj)
		{
			dw2y = fz_toint(fz_arrayget(obj, 0));
			dw2w = fz_toint(fz_arrayget(obj, 1));
		}

		fz_setdefaultvmtx((fz_font*)font, dw2y, dw2w);

		widths = fz_dictgets(dict, "W2");
		if (widths)
		{
			int c0, c1, w, x, y, k;

			error = pdf_resolve(&widths, xref);
			if (error)
				goto cleanup;

			for (i = 0; i < fz_arraylen(widths); )
			{
				c0 = fz_toint(fz_arrayget(widths, i));
				obj = fz_arrayget(widths, i + 1);
				if (fz_isarray(obj))
				{
					for (k = 0; k < fz_arraylen(obj); k += 3)
					{
						w = fz_toint(fz_arrayget(obj, k + 0));
						x = fz_toint(fz_arrayget(obj, k + 1));
						y = fz_toint(fz_arrayget(obj, k + 2));
						error = fz_addvmtx((fz_font*)font, c0 + k, c0 + k, x, y, w);
						if (error)
							goto cleanup;
					}
					i += 2;
				}
				else
				{
					c1 = fz_toint(obj);
					w = fz_toint(fz_arrayget(widths, i + 2));
					x = fz_toint(fz_arrayget(widths, i + 3));
					y = fz_toint(fz_arrayget(widths, i + 4));
					error = fz_addvmtx((fz_font*)font, c0, c1, x, y, w);
					if (error)
						goto cleanup;
					i += 5;
				}
			}

			fz_dropobj(widths);
		}

		error = fz_endvmtx((fz_font*)font);
		if (error)
			goto cleanup;
	}

	pdf_logfont("}\n");

	*fontp = font;
	return nil;

cleanup:
	if (widths)
		fz_dropobj(widths);
	fz_dropfont((fz_font*)font);
	return error;
}

static fz_error *
loadtype0(pdf_font **fontp, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	fz_obj *dfonts;
	fz_obj *dfont;
	fz_obj *subtype;
	fz_obj *encoding;
	fz_obj *tounicode;

	dfonts = fz_dictgets(dict, "DescendantFonts");
	error = pdf_resolve(&dfonts, xref);
	if (error)
		return error;

	dfont = fz_arrayget(dfonts, 0);
	error = pdf_resolve(&dfont, xref);
	if (error)
		return fz_dropobj(dfonts), error;

	subtype = fz_dictgets(dfont, "Subtype");
	encoding = fz_dictgets(dict, "Encoding");
	tounicode = fz_dictgets(dict, "ToUnicode");

	if (!strcmp(fz_toname(subtype), "CIDFontType0"))
		error = loadcidfont(fontp, xref, dfont, ref, encoding, tounicode);
	else if (!strcmp(fz_toname(subtype), "CIDFontType2"))
		error = loadcidfont(fontp, xref, dfont, ref, encoding, tounicode);
	else
		error = fz_throw("syntaxerror: unknown cid font type");

	fz_dropobj(dfont);
	fz_dropobj(dfonts);

	if (error)
		return error;

	return nil;
}

/*
 * FontDescriptor
 */

fz_error *
pdf_loadfontdescriptor(pdf_font *font, pdf_xref *xref, fz_obj *desc, char *collection)
{
	fz_error *error;
	fz_obj *obj1, *obj2, *obj3, *obj;
	fz_rect bbox;
	char *fontname;

	error = pdf_resolve(&desc, xref);
	if (error)
		return error;

	pdf_logfont("load fontdescriptor {\n");

	fontname = fz_toname(fz_dictgets(desc, "FontName"));

	pdf_logfont("fontname %s\n", fontname);

	font->flags = fz_toint(fz_dictgets(desc, "Flags"));
	font->italicangle = fz_toreal(fz_dictgets(desc, "ItalicAngle"));
	font->ascent = fz_toreal(fz_dictgets(desc, "Ascent"));
	font->descent = fz_toreal(fz_dictgets(desc, "Descent"));
	font->capheight = fz_toreal(fz_dictgets(desc, "CapHeight"));
	font->xheight = fz_toreal(fz_dictgets(desc, "XHeight"));
	font->missingwidth = fz_toreal(fz_dictgets(desc, "MissingWidth"));

	bbox = pdf_torect(fz_dictgets(desc, "FontBBox"));
	pdf_logfont("bbox [%g %g %g %g]\n",
		bbox.x0, bbox.y0,
		bbox.x1, bbox.y1);

	pdf_logfont("flags %d\n", font->flags);

	obj1 = fz_dictgets(desc, "FontFile");
	obj2 = fz_dictgets(desc, "FontFile2");
	obj3 = fz_dictgets(desc, "FontFile3");
	obj = obj1 ? obj1 : obj2 ? obj2 : obj3;

	if (getenv("NOFONT"))
		obj = nil;

	if (fz_isindirect(obj))
	{
		error = pdf_loadembeddedfont(font, xref, obj);
		if (error)
			goto cleanup;
	}
	else
	{
		error = pdf_loadsystemfont(font, fontname, collection);
		if (error)
			goto cleanup;
	}

	fz_dropobj(desc);

	pdf_logfont("}\n");

	return nil;

cleanup:
	fz_dropobj(desc);
	return error;
}

fz_error *
pdf_loadfont(pdf_font **fontp, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	char *subtype;

	if ((*fontp = pdf_finditem(xref->store, PDF_KFONT, ref)))
	{
		fz_keepfont((fz_font*)*fontp);
		return nil;
	}

	subtype = fz_toname(fz_dictgets(dict, "Subtype"));
	if (!strcmp(subtype, "Type0"))
		error = loadtype0(fontp, xref, dict, ref);
	else if (!strcmp(subtype, "Type1") || !strcmp(subtype, "MMType1"))
		error = loadsimplefont(fontp, xref, dict, ref);
	else if (!strcmp(subtype, "TrueType"))
		error = loadsimplefont(fontp, xref, dict, ref);
	else if (!strcmp(subtype, "Type3"))
		error = pdf_loadtype3font(fontp, xref, dict, ref);
	else
		error = fz_throw("unimplemented: %s fonts", subtype);

	if (error)
		return error;

	error = pdf_storeitem(xref->store, PDF_KFONT, ref, *fontp);
	if (error)
		return error;

	return nil;
}

