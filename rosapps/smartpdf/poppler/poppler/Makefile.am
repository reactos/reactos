if BUILD_SPLASH_OUTPUT

splash_sources =				\
	SplashOutputDev.cc

splash_headers =				\
	SplashOutputDev.h

splash_includes =				\
	$(SPLASH_CFLAGS)

splash_libs =					\
	$(SPLASH_LIBS)				\
	$(top_builddir)/splash/libsplash.la

endif

if BUILD_POPPLER_QT4

poppler_arthur = libpoppler-arthur.la

libpoppler_arthur_la_SOURCES =			\
	ArthurOutputDev.h			\
	ArthurOutputDev.cc

arthur_includes =				\
	$(POPPLER_QT4_CXXFLAGS)

arthur_libs =					\
	$(POPPLER_QT4_LIBS)

endif


if BUILD_CAIRO_OUTPUT

poppler_cairo = libpoppler-cairo.la

cairo_includes =				\
	$(CAIRO_CFLAGS)

cairo_libs =					\
	$(CAIRO_LIBS)

libpoppler_cairo_la_SOURCES =			\
	CairoFontEngine.cc			\
	CairoFontEngine.h			\
	CairoOutputDev.cc			\
	CairoOutputDev.h

endif

if BUILD_LIBJPEG

libjpeg_sources =				\
	DCTStream.h				\
	DCTStream.cc

libjpeg_libs =					\
	$(LIBJPEG_LIBS)

endif

if BUILD_ZLIB

zlib_sources =					\
	FlateStream.h				\
	FlateStream.cc

zlib_libs = 					\
	$(ZLIB_LIBS)

endif

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/goo			\
	$(splash_includes)			\
	$(cairo_includes)			\
	$(arthur_includes)			\
	$(FREETYPE_CFLAGS)			\
	$(FONTCONFIG_CFLAGS)

lib_LTLIBRARIES = libpoppler.la

noinst_LTLIBRARIES = $(poppler_cairo)		\
	$(poppler_arthur)

libpoppler_la_LIBADD =				\
	$(top_builddir)/goo/libgoo.la		\
	$(top_builddir)/fofi/libfofi.la		\
	$(splash_libs)				\
	$(cairo_libs)				\
	$(libjpeg_libs)				\
	$(zlib_libs)				\
	$(FREETYPE_LIBS)			\
	$(FONTCONFIG_LIBS)

libpoppler_la_LDFLAGS = -version-info 1:0:0

if ENABLE_XPDF_HEADERS

poppler_includedir = $(includedir)/poppler
poppler_include_HEADERS =	\
	$(splash_headers)	\
	Annot.h			\
	Array.h			\
	BaseFile.h		\
	BuiltinFont.h		\
	BuiltinFontTables.h	\
	Catalog.h		\
	CharCodeToUnicode.h	\
	CMap.h			\
	Decrypt.h		\
	Dict.h			\
	Error.h			\
	FontEncodingTables.h	\
	FontInfo.h		\
	Function.cc		\
	Function.h		\
	Gfx.h			\
	GfxFont.h		\
	GfxState.h		\
	GlobalParams.h		\
	JArithmeticDecoder.h	\
	JBIG2Stream.h		\
	JPXStream.h		\
	Lexer.h			\
	Link.h			\
	NameToCharCode.h	\
	Object.h		\
	Outline.h		\
	OutputDev.h		\
	Page.h			\
	Parser.h		\
	PDFDoc.h		\
	PDFDocEncoding.h	\
	ProfileData.h		\
	PSTokenizer.h		\
	Stream-CCITT.h		\
	Stream.h		\
	UnicodeMap.h		\
	UnicodeMapTables.h	\
	UnicodeTypeTable.h	\
	UnicodeCClassTables.h	\
	UnicodeCompTables.h	\
	UnicodeDecompTables.h	\
	XRef.h			\
	CharTypes.h		\
	CompactFontTables.h	\
	ErrorCodes.h		\
	NameToUnicodeTable.h	\
	PSOutputDev.h		\
	TextOutputDev.h		\
	SecurityHandler.h	\
	UGooString.h		\
	UTF8.h			\
	XpdfPluginAPI.h		\
	Sound.h			\
	poppler-config.h

endif

libpoppler_la_SOURCES =		\
	$(splash_sources)	\
	$(cairo_sources)	\
	$(arthur_sources)	\
	$(libjpeg_sources)	\
	$(zlib_sources)		\
	Annot.cc		\
	Array.cc 		\
	BuiltinFont.cc		\
	BuiltinFontTables.cc	\
	Catalog.cc 		\
	CharCodeToUnicode.cc	\
	CMap.cc			\
	Decrypt.cc		\
	Dict.cc 		\
	Error.cc 		\
	FontEncodingTables.cc	\
	FontInfo.cc		\
	Function.cc		\
	Gfx.cc 			\
	GfxFont.cc 		\
	GfxState.cc		\
	GlobalParams.cc		\
	JArithmeticDecoder.cc	\
	JBIG2Stream.cc		\
	JPXStream.cc		\
	Lexer.cc 		\
	Link.cc 		\
	NameToCharCode.cc	\
	Object.cc 		\
	Outline.cc		\
	OutputDev.cc 		\
	Page.cc 		\
	Parser.cc 		\
	PDFDoc.cc 		\
	PDFDocEncoding.cc	\
	ProfileData.cc		\
	PSTokenizer.cc		\
	Stream.cc 		\
	UnicodeMap.cc		\
	UnicodeTypeTable.cc	\
	XRef.cc			\
	PSOutputDev.cc		\
	TextOutputDev.cc	\
	PageLabelInfo.h		\
	PageLabelInfo.cc	\
	SecurityHandler.cc	\
	UGooString.cc	 	\
	Sound.cc		\
	XpdfPluginAPI.cc

EXTRA_DIST = gen-unicode-tables.py
