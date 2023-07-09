!ifndef __PASS2__
!include $(NTMAKEENV)\makefile.plt

all: tokens.mak
    nmake -l __PASS2__=1 all

tokens.mak: $(BASEDIR)\private\genx\ie\inc\ieverp.h tokens.inc makefile
    @$(TARGET_CPP) -nologo /EP /Tc << | qgrep TOK_ > $@
#include "$(BASEDIR)\private\genx\ie\inc\ieverp.h"
#if 	(VER_PRODUCTBUILD < 10)
#define VER_BPAD 000
#elif	(VER_PRODUCTBUILD < 100)
#define VER_BPAD 00
#elif	(VER_PRODUCTBUILD < 1000)
#define VER_BPAD 0
#else
#undef VER_BPAD
#endif

#ifdef VER_BPAD
#define BLDNUMX1(x, y)  x##y
#define BLDNUMX(x, y)   BLDNUMX1(x, y)

TOK_BUILDNUM = BLDNUMX(VER_BPAD,VER_PRODUCTBUILD)
#else
TOK_BUILDNUM = VER_PRODUCTBUILD
#endif
TOK_QFE = VER_PRODUCTBUILD_QFE
TOK_MAJOR_NUM = VER_MAJOR_PRODUCTVER
TOK_MINOR_NUM = VER_MINOR_PRODUCTVER
#if (VER_PRODUCTBUILD_QFE > 0)
TOK_FULLQFE = .VER_PRODUCTBUILD_QFE
#else
TOK_FULLQFE =
#endif
<<NOKEEP
    type tokens.inc >> tokens.mak


!else
# Pass2 (this time, tokens.mak is built.

!include $(NTMAKEENV)\makefile.plt

!include tokens.mak

!ifndef _NTTREE
!error _NTxxxTREE not defined... Aborting.
!endif

all: $(_NTTREE)\ie.inf $(_NTTREE)\install.ins

PERL_SWITCHES = \
    token.per \
    -tTOK_BUILDNUM=$(TOK_BUILDNUM) \
    -tTOK_QFE=$(TOK_QFE) \
    -tTOK_MAJOR_NUM=$(TOK_MAJOR_NUM) \
    -tTOK_MINOR_NUM=$(TOK_MINOR_NUM) \
    -tTOK_FULLQFE=$(TOK_FULLQFE)

STD_DEP = \
    $(BASEDIR)\public\sdk\inc\ieverp.h \
    tokens.mak \
    makefile

!if [set BUILDTYPE=$(BUILDTYPE)]
!endif

$(_NTTREE)\ie.inf : $(_NTTREE)\iescript\ie.inf $(STD_DEP)
     perl $(PERL_SWITCHES) $(_NTTREE)\iescript\ie.inf > $@

$(_NTTREE)\install.ins : $(_NTTREE)\iescript\install.ins $(STD_DEP)
     perl $(PERL_SWITCHES) $(_NTTREE)\iescript\install.ins > $@

!endif
