#
# Makefile for FreeType2 link library using ppc-morphos-gcc-2.95.3-bin.tgz
# (gcc 2.95.3 hosted on 68k-Amiga producing MorphOS-PPC-binaries from
# http://www.morphos.de)
#
# to build from the builds/amiga directory call
#
#  make assign
#  make
#
# Your programs source code should start with this
# (uncomment the parts you do not need to keep the program small):
# ---8<---
#define FT_USE_AUTOHINT // autohinter
#define FT_USE_RASTER   // monochrome rasterizer
#define FT_USE_SMOOTH   // anti-aliasing rasterizer
#define FT_USE_TT       // truetype font driver
#define FT_USE_T1       // type1 font driver
#define FT_USE_T42      // type42 font driver
#define FT_USE_T1CID    // cid-keyed type1 font driver
#define FT_USE_CFF      // opentype font driver
#define FT_USE_BDF      // bdf bitmap font driver
#define FT_USE_PCF      // pcf bitmap font driver
#define FT_USE_PFR      // pfr font driver
#define FT_USE_WINFNT   // windows .fnt|.fon bitmap font driver
#include "FT:src/base/ftinit.c"
# ---8<---
#
# link your programs with libft2_ppc.a and either ftsystem.ppc.o or ftsystempure.ppc.o
# (and either ftdebug.ppc.o or ftdebugpure.ppc.o if you enabled FT_DEBUG_LEVEL_ERROR or
# FT_DEBUG_LEVEL_TRACE in include/freetype/config/ftoption.h).

all:	libft2_ppc.a ftsystem.ppc.o ftsystempure.ppc.o

assign:
	assign FT: //

FTSRC = /FT/src

CC =     ppc-morphos-gcc
AR =     ppc-morphos-ar rc
RANLIB = ppc-morphos-ranlib
LD =     ppc-morphos-ld
CFLAGS = -O2 -I/emu/emulinclude/includegcc -I/emu/include -Iinclude -I$(FTSRC) -I/FT/include

#
# FreeType2 library base
#
ftbase.ppc.o: $(FTSRC)/base/ftbase.c
	$(CC) -c $(CFLAGS) -o $@ $<

ftinit.ppc.o: $(FTSRC)/base/ftinit.c
	$(CC) -c $(CFLAGS) -o $@ $<

ftsystem.ppc.o: $(FTSRC)/base/ftsystem.c
	$(CC) -c $(CFLAGS) -o $@ $<

# pure version for use in run-time library etc
ftsystempure.ppc.o: src/base/ftsystem.c
	$(CC) -c $(CFLAGS) -o $@ $<

ftdebug.ppc.o: $(FTSRC)/base/ftdebug.c
	$(CC) -c $(CFLAGS) -o $@ $<

# pure version for use in run-time library etc
ftdebugpure.ppc.o: src/base/ftdebug.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library base extensions
#
ftglyph.ppc.o: $(FTSRC)/base/ftglyph.c
	$(CC) -c $(CFLAGS) -o $@ $<

ftbbox.ppc.o: $(FTSRC)/base/ftbbox.c
	$(CC) -c $(CFLAGS) -o $@ $<

ftmm.ppc.o: $(FTSRC)/base/ftmm.c
	$(CC) -c $(CFLAGS) -o $@ $<

ftsynth.ppc.o: $(FTSRC)/base/ftsynth.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library autohinting module
#
autohint.ppc.o: $(FTSRC)/autohint/autohint.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library autohinting module extensions
#
ahoptim.ppc.o: $(FTSRC)/autohint/ahoptim.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library postscript hinting module
#
pshinter.ppc.o: $(FTSRC)/pshinter/pshinter.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library PS support module
#
psaux.ppc.o: $(FTSRC)/psaux/psaux.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library PS glyph names module
#
psnames.ppc.o: $(FTSRC)/psnames/psnames.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library monochrome raster module
#
raster.ppc.o: $(FTSRC)/raster/raster.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library anti-aliasing raster module
#
smooth.ppc.o: $(FTSRC)/smooth/smooth.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library 'sfnt' module
#
sfnt.ppc.o: $(FTSRC)/sfnt/sfnt.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library glyph and image caching system (still experimental)
#
ftcache.ppc.o: $(FTSRC)/cache/ftcache.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library OpenType font driver
#
cff.ppc.o: $(FTSRC)/cff/cff.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library TrueType font driver
#
truetype.ppc.o: $(FTSRC)/truetype/truetype.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library Type1 font driver
#
type1.ppc.o: $(FTSRC)/type1/type1.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library Type42 font driver
#
type42.ppc.o: $(FTSRC)/type42/type42.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library CID-keyed Type1 font driver
#
type1cid.ppc.o: $(FTSRC)/cid/type1cid.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library BDF bitmap font driver
#
bdf.ppc.o: $(FTSRC)/bdf/bdf.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library PCF bitmap font driver
#
pcf.ppc.o: $(FTSRC)/pcf/pcf.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library PFR font driver
#
pfr.ppc.o: $(FTSRC)/pfr/pfr.c
	$(CC) -c $(CFLAGS) -o $@ $<

#
# FreeType2 library Windows FNT/FON bitmap font driver
#
winfnt.ppc.o: $(FTSRC)/winfonts/winfnt.c
	$(CC) -c $(CFLAGS) -o $@ $<

BASEPPC = ftbase.ppc.o ftglyph.ppc.o ftbbox.ppc.o ftmm.ppc.o ftsynth.ppc.o

DEBUGPPC = ftdebug.ppc.o ftdebugpure.ppc.o

AHINTPPC = autohint.ppc.o ahoptim.ppc.o

PSPPC = psaux.ppc.o psnames.ppc.o pshinter.ppc.o

RASTERPPC = raster.ppc.o smooth.ppc.o

FONTDPPC = cff.ppc.o type1.ppc.o type42.ppc.o type1cid.ppc.o truetype.ppc.o\
	   bdf.ppc.o pcf.ppc.o pfr.ppc.o winfnt.ppc.o

libft2_ppc.a:    $(BASEPPC) $(AHINTPPC) $(PSPPC) $(RASTERPPC) sfnt.ppc.o ftcache.ppc.o $(FONTDPPC)
	$(AR) $@ $(BASEPPC) $(AHINTPPC) $(PSPPC) $(RASTERPPC) sfnt.ppc.o ftcache.ppc.o $(FONTDPPC)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1
