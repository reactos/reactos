#include <fitz.h>
#include <mupdf.h>

#include <mupdf/base14.h>

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ftlib = nil;

#ifdef WIN32
#include <io.h>
static int path_exists(const char *path)
{
    if (0 == _access(path, 04))
        return 1;
    return 0;
}

fz_error *initfontlibs_ms(void)
{
    return nil;
}
void deinitfontlibs_ms(void)
{
    if (!ftlib)
        return;
    FT_Done_FreeType(ftlib);
    ftlib = nil;
}
#else
static int path_exists(const char *path)
{
    if (access(path, R_OK) == 0)
        return 1;
    return 0;
}
#endif

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

static const struct
{
	const char *name;
	const unsigned char *cff;
	const unsigned int *len;
} basefonts[15] =
{
	{ "Courier",
		fonts_NimbusMonL_Regu_cff,
		&fonts_NimbusMonL_Regu_cff_len },
	{ "Courier-Bold",
		fonts_NimbusMonL_Bold_cff,
		&fonts_NimbusMonL_Bold_cff_len },
	{ "Courier-Oblique",
		fonts_NimbusMonL_ReguObli_cff,
		&fonts_NimbusMonL_ReguObli_cff_len },
	{ "Courier-BoldOblique",
		fonts_NimbusMonL_BoldObli_cff,
		&fonts_NimbusMonL_BoldObli_cff_len },
	{ "Helvetica",
		fonts_NimbusSanL_Regu_cff,
		&fonts_NimbusSanL_Regu_cff_len },
	{ "Helvetica-Bold",
		fonts_NimbusSanL_Bold_cff,
		&fonts_NimbusSanL_Bold_cff_len },
	{ "Helvetica-Oblique",
		fonts_NimbusSanL_ReguItal_cff,
		&fonts_NimbusSanL_ReguItal_cff_len },
	{ "Helvetica-BoldOblique",
		fonts_NimbusSanL_BoldItal_cff,
		&fonts_NimbusSanL_BoldItal_cff_len },
	{ "Times-Roman",
		fonts_NimbusRomNo9L_Regu_cff,
		&fonts_NimbusRomNo9L_Regu_cff_len },
	{ "Times-Bold",
		fonts_NimbusRomNo9L_Medi_cff,
		&fonts_NimbusRomNo9L_Medi_cff_len },
	{ "Times-Italic",
		fonts_NimbusRomNo9L_ReguItal_cff,
		&fonts_NimbusRomNo9L_ReguItal_cff_len },
	{ "Times-BoldItalic",
		fonts_NimbusRomNo9L_MediItal_cff,
		&fonts_NimbusRomNo9L_MediItal_cff_len },
	{ "Symbol",
		fonts_StandardSymL_cff,
		&fonts_StandardSymL_cff_len },
	{ "ZapfDingbats",
		fonts_Dingbats_cff,
		&fonts_Dingbats_cff_len },
	{ "Chancery",
		fonts_URWChanceryL_MediItal_cff,
		&fonts_URWChanceryL_MediItal_cff_len }
};

enum { CNS, GB, Japan, Korea };
enum { MINCHO, GOTHIC };

struct subent { int csi; int kind; char *name; };

static const struct subent fontsubs[] =
{
	{ CNS, MINCHO, "bkai00mp.ttf" },
	{ CNS, GOTHIC, "bsmi00lp.ttf" },
	{ CNS, MINCHO, "\345\204\267\345\256\213 Pro.ttf" }, /* LiSong Pro */
	{ CNS, GOTHIC, "\345\204\267\351\273\221 Pro.ttf" }, /* LiHei Pro */
	{ CNS, MINCHO, "simsun.ttc" },
	{ CNS, GOTHIC, "simhei.ttf" },

	{ GB, MINCHO, "gkai00mp.ttf" },
	{ GB, GOTHIC, "gbsn00lp.ttf" },
	{ GB, MINCHO, "\345\215\216\346\226\207\345\256\213\344\275\223.ttf" }, /* STSong */
	{ GB, GOTHIC, "\345\215\216\346\226\207\346\245\267\344\275\223.ttf" }, /* STHeiti */
	{ GB, MINCHO, "mingliu.ttc" },
	{ GB, GOTHIC, "mingliu.ttc" },

	{ Japan, MINCHO, "kochi-mincho.ttf" },
	{ Japan, GOTHIC, "kochi-gothic.ttf" },
	{ Japan, MINCHO, "\343\203\222\343\203\251\343\202\255\343\202\231\343\203\216\346\230\216\346\234\235 Pro W3.otf" },
	{ Japan, GOTHIC, "\343\203\222\343\203\251\343\202\255\343\202\231\343\203\216\350\247\222\343\202\263\343\202\231 Pro W3.otf" },
	{ Japan, MINCHO, "msmincho.ttc" },
	{ Japan, GOTHIC, "msgothic.ttc" },

	{ Korea, MINCHO, "batang.ttf" },
	{ Korea, GOTHIC, "dotum.ttf" },
	{ Korea, MINCHO, "AppleMyungjo.dfont" },
	{ Korea, GOTHIC, "AppleGothic.dfont" },
	{ Korea, MINCHO, "batang.ttc" },
	{ Korea, GOTHIC, "dotum.ttc" },
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

	return nil;
}

fz_error *
pdf_loadbuiltinfont(pdf_font *font, char *fontname)
{
	fz_error *error;
	unsigned char *data;
	unsigned int len;
	FT_Error e;
	int i;

	error = initfontlibs();
	if (error)
		return error;

	for (i = 0; i < 15; i++)
		if (!strcmp(fontname, basefonts[i].name))
			goto found;

	return fz_throw("font not found: %s", fontname);

found:
	pdf_logfont("load builtin font %s\n", fontname);

	data = (unsigned char *) basefonts[i].cff;
	len = *basefonts[i].len;

	e = FT_New_Memory_Face(ftlib, data, len, 0, (FT_Face*)&font->ftface);
	if (e)
		return fz_throw("freetype: could not load font: 0x%x", e);

	return nil;
}

static int
findcidfont(char *filename, char *path, int pathlen)
{
	static const char *dirs[] =
	{
		"$/.fonts",
		"$/Library/Fonts",
		"/usr/X11R6/lib/X11/fonts/TTF",
		"/usr/X11R6/lib/X11/fonts/TrueType",
		"/usr/share/fonts/arphic",
		"/usr/share/fonts/baekmuk",
		"/usr/share/fonts/kochi",
		"/System/Library/Fonts",
		"/Library/Fonts",
		nil
	};

	char **dirp;
	char *home;
	char *dir;

	dir = getenv("FONTDIR");
	if (dir)
	{
		strlcpy(path, dir, pathlen);
		strlcat(path, "/", pathlen);
		strlcat(path, filename, pathlen);
		if (path_exists(path))
			return 1;
	}

	dir = getenv("WINDIR");
	if (dir)
	{
		strlcpy(path, dir, pathlen);
		strlcat(path, "/Fonts/", pathlen);
		strlcat(path, filename, pathlen);
		if (path_exists(path))
			return 1;
	}

	home = getenv("HOME");
	if (!home)
		home = "/";

	for (dirp = (char**)dirs; *dirp; dirp++)
	{
		dir = *dirp;
		if (dir[0] == '$')
		{
			strlcpy(path, home, pathlen);
			strlcat(path, "/", pathlen);
			strlcat(path, dir + 1, pathlen);
		}
		else
		{
			strlcpy(path, dir, pathlen);
		}
		strlcat(path, "/", pathlen);
		strlcat(path, filename, pathlen);
		if (path_exists(path))
			return 1;
	}

	return 0;
}

static fz_error *
loadcidfont(pdf_font *font, int csi, int kind)
{
	char path[1024];
	int e;
	int i;

	for (i = 0; i < nelem(fontsubs); i++)
	{
		if (fontsubs[i].csi == csi && fontsubs[i].kind == kind)
		{
			if (findcidfont(fontsubs[i].name, path, sizeof path))
			{
				pdf_logfont("load system font '%s'\n", fontsubs[i].name);
				e = FT_New_Face(ftlib, path, 0, (FT_Face*)&font->ftface);
				if (e)
					return fz_throw("freetype: could not load font: 0x%x", e);
				return nil;
			}
		}
	}

	return fz_throw("could not find cid font file");
}

fz_error *
pdf_loadsystemfont(pdf_font *font, char *fontname, char *collection)
{
	fz_error *error;
	char *name;

	int isbold = 0;
	int isitalic = 0;
	int isserif = 0;
	int isscript = 0;
	int isfixed = 0;

	error = initfontlibs();
	if (error)
		return error;

	font->substitute = 1;

	if (strstr(fontname, "Bold"))
		isbold = 1;
	if (strstr(fontname, "Italic"))
		isitalic = 1;
	if (strstr(fontname, "Oblique"))
		isitalic = 1;

	if (font->flags & FD_FIXED)
		isfixed = 1;
	if (font->flags & FD_SERIF)
		isserif = 1;
	if (font->flags & FD_ITALIC)
		isitalic = 1;
	if (font->flags & FD_SCRIPT)
		isscript = 1;
	if (font->flags & FD_FORCEBOLD)
		isbold = 1;

	pdf_logfont("fixed-%d serif-%d italic-%d script-%d bold-%d\n",
		isfixed, isserif, isitalic, isscript, isbold);

	if (collection)
	{
		int kind;

		if (isserif)
			kind = MINCHO;
		else
			kind = GOTHIC;

		if (!strcmp(collection, "Adobe-CNS1"))
			return loadcidfont(font, CNS, kind);
		else if (!strcmp(collection, "Adobe-GB1"))
			return loadcidfont(font, GB, kind);
		else if (!strcmp(collection, "Adobe-Japan1"))
			return loadcidfont(font, Japan, kind);
		else if (!strcmp(collection, "Adobe-Japan2"))
			return loadcidfont(font, Japan, kind);
		else if (!strcmp(collection, "Adobe-Korea1"))
			return loadcidfont(font, Korea, kind);

		fz_warn("unknown cid collection: %s", collection);
	}

	if (isscript)
		name = "Chancery";

	else if (isfixed)
	{
		if (isitalic) {
			if (isbold) name = "Courier-BoldOblique";
			else name = "Courier-Oblique";
		}
		else {
			if (isbold) name = "Courier-Bold";
			else name = "Courier";
		}
	}

	else if (isserif)
	{
		if (isitalic) {
			if (isbold) name = "Times-BoldItalic";
			else name = "Times-Italic";
		}
		else {
			if (isbold) name = "Times-Bold";
			else name = "Times-Roman";
		}
	}

	else
	{
		if (isitalic) {
			if (isbold) name = "Helvetica-BoldOblique";
			else name = "Helvetica-Oblique";
		}
		else {
			if (isbold) name = "Helvetica-Bold";
			else name = "Helvetica";
		}
	}

	return pdf_loadbuiltinfont(font, name);
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

