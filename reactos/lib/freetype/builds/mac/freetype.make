# Makefile for Apple MPW build environment (currently PPC only)

MAKEFILE     = Makefile
ÿMondoBuildÿ = #{MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
SymÿPPC      = #-sym on
ObjDirÿPPC   = :obj:

CFLAGS  = -i :include -i :src -includes unix {SymÿPPC}

OBJS  = ÿ
		"{ObjDirÿPPC}ftsystem.c.x" ÿ
		"{ObjDirÿPPC}ftdebug.c.x" ÿ
		"{ObjDirÿPPC}ftinit.c.x" ÿ
		"{ObjDirÿPPC}ftbase.c.x" ÿ
		"{ObjDirÿPPC}ftglyph.c.x" ÿ
		"{ObjDirÿPPC}ftmm.c.x" ÿ
		"{ObjDirÿPPC}ftbbox.c.x" ÿ
		"{ObjDirÿPPC}autohint.c.x" ÿ
		"{ObjDirÿPPC}ftcache.c.x" ÿ
		"{ObjDirÿPPC}cff.c.x" ÿ
		"{ObjDirÿPPC}type1cid.c.x" ÿ
		"{ObjDirÿPPC}pcf.c.x" ÿ
		"{ObjDirÿPPC}psaux.c.x" ÿ
		"{ObjDirÿPPC}psmodule.c.x" ÿ
		"{ObjDirÿPPC}raster.c.x" ÿ
		"{ObjDirÿPPC}sfnt.c.x" ÿ
		"{ObjDirÿPPC}smooth.c.x" ÿ
		"{ObjDirÿPPC}truetype.c.x" ÿ
		"{ObjDirÿPPC}type1.c.x" ÿ
		"{ObjDirÿPPC}winfnt.c.x" ÿ
		"{ObjDirÿPPC}ftmac.c.x" ÿ

# Main target - build a library
freetype ÿÿ {ÿMondoBuildÿ} directories freetype.o

# This is used to build the library
freetype.o ÿÿ {ÿMondoBuildÿ} {OBJS}
	PPCLink ÿ
		-o :lib:{Targ} {SymÿPPC} ÿ
		{OBJS} -c '????' -xm l

# This is used to create the directories needed for build
directories ÿ
	if !`Exists obj` ; NewFolder obj ; end
	if !`Exists lib` ; NewFolder lib ; end


"{ObjDirÿPPC}ftsystem.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftsystem.c"
	{PPCC} ":src:base:ftsystem.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftdebug.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftdebug.c"
	{PPCC} ":src:base:ftdebug.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftinit.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftinit.c"
	{PPCC} ":src:base:ftinit.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftbase.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftbase.c"
	{PPCC} ":src:base:ftbase.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftglyph.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftglyph.c"
	{PPCC} ":src:base:ftglyph.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftmm.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftmm.c"
	{PPCC} ":src:base:ftmm.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftbbox.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftbbox.c"
	{PPCC} ":src:base:ftbbox.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}autohint.c.x" ÿ {ÿMondoBuildÿ} ":src:autohint:autohint.c"
	{PPCC} ":src:autohint:autohint.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftcache.c.x" ÿ {ÿMondoBuildÿ} ":src:cache:ftcache.c"
	{PPCC} ":src:cache:ftcache.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}cff.c.x" ÿ {ÿMondoBuildÿ} ":src:cff:cff.c"
	{PPCC} ":src:cff:cff.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}type1cid.c.x" ÿ {ÿMondoBuildÿ} ":src:cid:type1cid.c"
	{PPCC} ":src:cid:type1cid.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}pcf.c.x" ÿ {ÿMondoBuildÿ} ":src:pcf:pcf.c"
	{PPCC} ":src:pcf:pcf.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}psaux.c.x" ÿ {ÿMondoBuildÿ} ":src:psaux:psaux.c"
	{PPCC} ":src:psaux:psaux.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}psmodule.c.x" ÿ {ÿMondoBuildÿ} ":src:psnames:psmodule.c"
	{PPCC} ":src:psnames:psmodule.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}raster.c.x" ÿ {ÿMondoBuildÿ} ":src:raster:raster.c"
	{PPCC} ":src:raster:raster.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}sfnt.c.x" ÿ {ÿMondoBuildÿ} ":src:sfnt:sfnt.c"
	{PPCC} ":src:sfnt:sfnt.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}smooth.c.x" ÿ {ÿMondoBuildÿ} ":src:smooth:smooth.c"
	{PPCC} ":src:smooth:smooth.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}truetype.c.x" ÿ {ÿMondoBuildÿ} ":src:truetype:truetype.c"
	{PPCC} ":src:truetype:truetype.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}type1.c.x" ÿ {ÿMondoBuildÿ} ":src:type1:type1.c"
	{PPCC} ":src:type1:type1.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}winfnt.c.x" ÿ {ÿMondoBuildÿ} ":src:winfonts:winfnt.c"
	{PPCC} ":src:winfonts:winfnt.c" -o {Targ} {CFLAGS}

"{ObjDirÿPPC}ftmac.c.x" ÿ {ÿMondoBuildÿ} ":src:base:ftmac.c"
	{PPCC} ":src:base:ftmac.c" -o {Targ} {CFLAGS}