#include <fitz.h>
#include <mupdf.h>

#ifdef WIN32
#error Compile "fontfilems.c" instead
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

static FT_Library ftlib = nil;
static FcConfig *fclib = nil;

enum
{
	FD_FIXED = 1 << 0,
	FD_SERIF = 1 << 1,
	FD_SYMBOLIC = 1 << 2,
	FD_SCRIPT = 1 << 3,
	FD_NONSYMBOLIC = 1 << 5,
	FD_ITALIC = 1 << 6,
	FD_ALLCAP = 1 << 16,
	FD_SMALLCAP = 1 << 17,
	FD_FORCEBOLD = 1 << 18
};

static char *basenames[14] =
{
	"Courier",
	"Courier-Bold",
	"Courier-Oblique",
	"Courier-BoldOblique",
	"Helvetica",
	"Helvetica-Bold",
	"Helvetica-Oblique",
	"Helvetica-BoldOblique",
	"Times-Roman",
	"Times-Bold",
	"Times-Italic",
	"Times-BoldItalic",
	"Symbol",
	"ZapfDingbats"
};

static char *basepatterns[14] =
{
	"Nimbus Mono L,Courier,Courier New:style=Regular,Roman",
	"Nimbus Mono L,Courier,Courier New:style=Bold",
	"Nimbus Mono L,Courier,Courier New:style=Oblique,Italic",
	"Nimbus Mono L,Courier,Courier New:style=BoldOblique,BoldItalic",
	"Nimbus Sans L,Helvetica,Arial:style=Regular,Roman",
	"Nimbus Sans L,Helvetica,Arial:style=Bold",
	"Nimbus Sans L,Helvetica,Arial:style=Oblique,Italic",
	"Nimbus Sans L,Helvetica,Arial:style=BoldOblique,BoldItalic",
	"Nimbus Roman No9 L,Times,Times New Roman:style=Regular,Roman",
	"Nimbus Roman No9 L,Times,Times New Roman:style=Bold,Medium",
	"Nimbus Roman No9 L,Times,Times New Roman:style=Italic,Regular Italic",
	"Nimbus Roman No9 L,Times,Times New Roman:style=BoldItalic,Medium Italic",
	"Standard Symbols L,Symbol",
	"Zapf Dingbats,Dingbats"
};

static fz_error *initfontlibs(void)
{
	int fterr;
	int maj, min, pat;

	if (ftlib)
		return nil;

	fterr = FT_Init_FreeType(&ftlib);
	if (fterr)
		return fz_throw("freetype failed initialisation: 0x%x", fterr);

	FT_Library_Version(ftlib, &maj, &min, &pat);
	if (maj == 2 && min == 1 && pat < 7)
		return fz_throw("freetype version too old: %d.%d.%d", maj, min, pat);

	fclib = FcInitLoadConfigAndFonts();
	if (!fclib)
		return fz_throw("fontconfig failed initialisation");

	return nil;
}

fz_error *
pdf_loadbuiltinfont(pdf_font *font, char *basefont)
{
	fz_error *error;
	FcResult fcerr;
	int fterr;

	FcPattern *searchpat;
	FcPattern *matchpat;
	FT_Face face;
	char *pattern;
	char *file;
	int index;
	int i;

	error = initfontlibs();
	if (error)
		return error;

	pattern = basefont;
	for (i = 0; i < 14; i++)
		if (!strcmp(basefont, basenames[i]))
			pattern = basepatterns[i];

	pdf_logfont("load builtin %s\n", pattern);

	fcerr = FcResultMatch;
	searchpat = FcNameParse(pattern);
	FcDefaultSubstitute(searchpat);
	FcConfigSubstitute(fclib, searchpat, FcMatchPattern);

	matchpat = FcFontMatch(fclib, searchpat, &fcerr);
	if (fcerr != FcResultMatch)
		return fz_throw("fontconfig could not find font %s", pattern);

	fcerr = FcPatternGetString(matchpat, FC_FILE, 0, (FcChar8**)&file);
	if (fcerr != FcResultMatch)
		return fz_throw("fontconfig could not find font %s", pattern);

	index = 0;
	fcerr = FcPatternGetInteger(matchpat, FC_INDEX, 0, &index);

	pdf_logfont("load font file %s %d\n", file, index);

	fterr = FT_New_Face(ftlib, file, index, &face);
	if (fterr)
		return fz_throw("freetype could not load font file '%s': 0x%x", file, fterr);

	FcPatternDestroy(matchpat);
	FcPatternDestroy(searchpat);

	font->ftface = face;

	return nil;
}

fz_error *
pdf_loadsystemfont(pdf_font *font, char *basefont, char *collection)
{
	fz_error *error;
	FcResult fcerr;
	int fterr;

	char fontname[200];
	FcPattern *searchpat;
	FcPattern *matchpat;
	FT_Face face;
	char *style;
	char *file;
	int index;

	error = initfontlibs();
	if (error)
		return error;

	/* parse windows-style font name descriptors Font,Style */
	/* TODO: reliable way to split style from Font-Style type names */
	strlcpy(fontname, basefont, sizeof fontname);
	style = strchr(fontname, ',');
	if (style)
		*style++ = 0;

	searchpat = FcPatternCreate();
	if (!searchpat)
		return fz_outofmem;

	error = fz_outofmem;

	if (!FcPatternAddString(searchpat, FC_FAMILY, fontname))
		goto cleanup;

	if (collection)
	{
		if (!strcmp(collection, "Adobe-GB1"))
			if (!FcPatternAddString(searchpat, FC_LANG, "zh-TW"))
				goto cleanup;
		if (!strcmp(collection, "Adobe-CNS1"))
			if (!FcPatternAddString(searchpat, FC_LANG, "zh-CN"))
				goto cleanup;
		if (!strcmp(collection, "Adobe-Japan1"))
			if (!FcPatternAddString(searchpat, FC_LANG, "ja"))
				goto cleanup;
		if (!strcmp(collection, "Adobe-Japan2"))
			if (!FcPatternAddString(searchpat, FC_LANG, "ja"))
				goto cleanup;
		if (!strcmp(collection, "Adobe-Korea1"))
			if (!FcPatternAddString(searchpat, FC_LANG, "ko"))
				goto cleanup;
	}

	if (style)
		if (!FcPatternAddString(searchpat, FC_STYLE, style))
			goto cleanup;

	if (font->flags & FD_SERIF)
		FcPatternAddString(searchpat, FC_FAMILY, "serif");
	else
		FcPatternAddString(searchpat, FC_FAMILY, "sans-serif");
	if (font->flags & FD_ITALIC)
		FcPatternAddString(searchpat, FC_STYLE, "Italic");
	if (font->flags & FD_FORCEBOLD)
		FcPatternAddString(searchpat, FC_STYLE, "Bold");

	if (!FcPatternAddBool(searchpat, FC_OUTLINE, 1))
		goto cleanup;

	file = FcNameUnparse(searchpat);
	pdf_logfont("fontconfig0 %s\n", file);
	free(file);

	fcerr = FcResultMatch;
/*	FcDefaultSubstitute(searchpat); */
	FcConfigSubstitute(fclib, searchpat, FcMatchPattern);

	file = FcNameUnparse(searchpat);
	pdf_logfont("fontconfig1 %s\n", file);
	free(file);

	matchpat = FcFontMatch(fclib, searchpat, &fcerr);
	if (fcerr != FcResultMatch)
		return fz_throw("fontconfig could not find font %s", basefont);

	fcerr = FcPatternGetString(matchpat, FC_FAMILY, 0, (FcChar8**)&file);
	if (file && strcmp(fontname, file))
		font->substitute = 1;

	fcerr = FcPatternGetString(matchpat, FC_STYLE, 0, (FcChar8**)&file);
	if (file && style && strcmp(style, file))
		font->substitute = 1;

	fcerr = FcPatternGetString(matchpat, FC_FILE, 0, (FcChar8**)&file);
	if (fcerr != FcResultMatch)
		return fz_throw("fontconfig could not find font %s", basefont);

	index = 0;
	fcerr = FcPatternGetInteger(matchpat, FC_INDEX, 0, &index);

	pdf_logfont("load font file %s %d\n", file, index);

	fterr = FT_New_Face(ftlib, file, index, &face);
	if (fterr) {
		FcPatternDestroy(matchpat);
		FcPatternDestroy(searchpat);
		return fz_throw("freetype could not load font file '%s': 0x%x", file, fterr);
	}

	FcPatternDestroy(matchpat);
	FcPatternDestroy(searchpat);

	font->ftface = face;

	return nil;

cleanup:
	FcPatternDestroy(searchpat);
	return error;
}

fz_error *
pdf_loadembeddedfont(pdf_font *font, pdf_xref *xref, fz_obj *stmref)
{
	fz_error *error;
	int fterr;
	FT_Face face;
	fz_buffer *buf;

	error = initfontlibs();
	if (error)
		return error;

	pdf_logfont("load embedded font\n");

	error = pdf_loadstream(&buf, xref, fz_tonum(stmref), fz_togen(stmref));
	if (error)
		return error;

	fterr = FT_New_Memory_Face(ftlib, buf->rp, buf->wp - buf->rp, 0, &face);

	if (fterr) {
		fz_free(buf);
		return fz_throw("freetype could not load embedded font: 0x%x", fterr);
	}

	font->ftface = face;
	font->fontdata = buf;

	return nil;
}

