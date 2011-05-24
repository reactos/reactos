# Time-stamp: <04/05/01 00:45:03 ptr>
# $Id$

# dependency output parser
#!include ${RULESBASE}/dparser-$(COMPILER_NAME).mak

# if sources disposed in several dirs, calculate
# appropriate rules; here is recursive call!

#DIRS_UNIQUE_SRC := $(dir $(SRC_CPP) $(SRC_CC) $(SRC_C) )
#DIRS_UNIQUE_SRC := $(sort $(DIRS_UNIQUE_SRC) )
#include ${RULESBASE}/dirsrc.mak
!include $(RULESBASE)/$(USE_MAKE)/rules-o.mak

#ALLBASE    := $(basename $(notdir $(SRC_CC) $(SRC_CPP) $(SRC_C)))
ALLBASE    = $(SRC_CC) $(SRC_CPP) $(SRC_C)
#ALLOBJS    := $(addsuffix .o,$(ALLBASE))

# assemble objectfiles by concatenating sourcefiles and replacing extension with .o
# follow tricks to avoid leading space if one of the macro undefined:
# SRC_CC, SRC_CPP or SRC_C
!ifdef SRC_CC
ALLOBJS    = $(SRC_CC:.cc=.o)
!endif
!ifdef SRC_CPP
!ifdef ALLOBJS
ALLOBJS = $(ALLOBJS) $(SRC_CPP:.cpp=.o)
!else
ALLOBJS = $(SRC_CPP:.cpp=.o)
!endif
!endif
!ifdef SRC_C
!ifdef ALLOBJS
ALLOBJS = $(ALLOBJS) $(SRC_C:.c=.o)
!else
ALLOBJS = $(SRC_C:.c=.o)
!endif
!endif

!ifdef SRC_RC
ALLRESS = $(SRC_RC:.rc=.res)
#ALLRESS = $(ALLRESS:../=)
!endif
# ALLOBJS = $(ALLOBJS:somedir/=)

!if EXIST( .\nmake-src-prefix.mak )
# Include strip of path to sources, i.e. macro like
#   ALLOBJS = $(ALLOBJS:..\..\..\..\..\..\explore/../extern/boost/libs/test/src/=)
#   ALLOBJS = $(ALLOBJS:../=)
#   ALLRESS = $(ALLRESS:../=)
# Be careful about path spelling!
# Pay attention the order of this macro! THE ORDER IS SIGNIFICANT!
!include .\nmake-src-prefix.mak
!endif

ALLDEPS    = $(SRC_CC:.cc=.d) $(SRC_CPP:.cpp=.d) $(SRC_C:.c=.d)

#!if [echo ALLOBJS -$(ALLOBJS)-]
#!endif

# Following code adds a marker ('@') everywhere the path needs to be added.
# The code searches for '.o' followed by whitespace and replaces it with '.o @'.
# In a second stage, it removes all whitespace after an '@' sign, to cater for
# the case where more than one whitespace character was separating objectfiles.

# set marker (spaces are significant here!):
OBJ_MARKED=$(ALLOBJS:.o =.o @)
RES_MARKED=$(ALLRESS:.res =.res @)

# remove unwanted space as result of line extending, like
# target: dep1.cpp dep2.cpp \
#         dep3.cpp
# (note, that if write '... dep2.cpp\', no white space happens)
OBJ_MARKED=$(OBJ_MARKED:@ =@)
RES_MARKED=$(RES_MARKED:@ =@)

# unless empty, add marker at the beginning
!if "$(OBJ_MARKED)"!=""
OBJ_MARKED=@$(OBJ_MARKED)
!endif
!if "$(RES_MARKED)"!=""
RES_MARKED=@$(RES_MARKED)
!endif


# second step, insert compiler/CPU part to path
# Transform 'foo.o bar.o baz.o' to 'cc-xy/foo.o cc-xy/bar.o cc-xy/baz.o',
# i.e. to add a prefix path to every objectfile. Now, the problem is that
# nmake can't make substitutions where a string is replaced with the content
# of a variable. IOW, this wont work:
#OBJ=$(OBJ:@=%OUTPUT_DIR%/)
# instead, we have to cater for every possible combination of compiler (and
# target cpu when cross-compiling) by appropriate if/else clauses.

!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc4-arm\@)
RES_MARKED=$(RES_MARKED:@=obj\evc4-arm\@)
!elseif "$(TARGET_PROC)" == "x86"
!if "$(TARGET_PROC_SUBTYPE)" == "emulator"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc4-emulator\@)
RES_MARKED=$(RES_MARKED:@=obj\evc4-emulator\@)
!else
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc4-x86\@)
RES_MARKED=$(RES_MARKED:@=obj\evc4-x86\@)
!endif
!elseif "$(TARGET_PROC)" == "mips"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc4-mips\@)
RES_MARKED=$(RES_MARKED:@=obj\evc4-mips\@)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc4-sh3\@)
RES_MARKED=$(RES_MARKED:@=obj\evc4-sh3\@)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc4-sh4\@)
RES_MARKED=$(RES_MARKED:@=obj\evc4-sh4\@)
!else
!error No target processor configured!
!endif

!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc3-arm\@)
RES_MARKED=$(RES_MARKED:@=obj\evc3-arm\@)
!elseif "$(TARGET_PROC)" == "x86"
!if "$(TARGET_PROC_SUBTYPE)" == "emulator"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc3-emulator\@)
RES_MARKED=$(RES_MARKED:@=obj\evc3-emulator\@)
!else
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc3-x86\@)
RES_MARKED=$(RES_MARKED:@=obj\evc3-x86\@)
!endif
!elseif "$(TARGET_PROC)" == "mips"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc3-mips\@)
RES_MARKED=$(RES_MARKED:@=obj\evc3-mips\@)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc3-sh3\@)
RES_MARKED=$(RES_MARKED:@=obj\evc3-sh3\@)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc3-sh4\@)
RES_MARKED=$(RES_MARKED:@=obj\evc3-sh4\@)
!else
!error No target processor configured!
!endif

!elseif "$(COMPILER_NAME)" == "evc8"
!if "$(TARGET_PROC)" == ""
!error No target processor configured!
!elseif "$(TARGET_PROC)" == "arm"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc8-arm\@)
RES_MARKED=$(RES_MARKED:@=obj\evc8-arm\@)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc8-x86\@)
RES_MARKED=$(RES_MARKED:@=obj\evc8-x86\@)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc8-mips\@)
RES_MARKED=$(RES_MARKED:@=obj\evc8-mips\@)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc8-sh4\@)
RES_MARKED=$(RES_MARKED:@=obj\evc8-sh4\@)
!else
!error Unknown target processor configured!
!endif

!elseif "$(COMPILER_NAME)" == "evc9"
!if "$(TARGET_PROC)" == ""
!error No target processor configured!
!elseif "$(TARGET_PROC)" == "arm"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc9-arm\@)
RES_MARKED=$(RES_MARKED:@=obj\evc9-arm\@)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc9-x86\@)
RES_MARKED=$(RES_MARKED:@=obj\evc9-x86\@)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc9-mips\@)
RES_MARKED=$(RES_MARKED:@=obj\evc9-mips\@)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_MARKED=$(OBJ_MARKED:@=obj\evc9-sh4\@)
RES_MARKED=$(RES_MARKED:@=obj\evc9-sh4\@)
!else
!error Unknown target processor configured!
!endif

!elseif "$(COMPILER_NAME)" == "vc6"
OBJ_MARKED=$(OBJ_MARKED:@=obj\vc6\@)
RES_MARKED=$(RES_MARKED:@=obj\vc6\@)

!elseif "$(COMPILER_NAME)" == "vc70"
OBJ_MARKED=$(OBJ_MARKED:@=obj\vc70\@)
RES_MARKED=$(RES_MARKED:@=obj\vc70\@)

!elseif "$(COMPILER_NAME)" == "vc71"
OBJ_MARKED=$(OBJ_MARKED:@=obj\vc71\@)
RES_MARKED=$(RES_MARKED:@=obj\vc71\@)

!elseif "$(COMPILER_NAME)" == "vc8"
OBJ_MARKED=$(OBJ_MARKED:@=obj\vc8\@)
RES_MARKED=$(RES_MARKED:@=obj\vc8\@)

!elseif "$(COMPILER_NAME)" == "vc9"
OBJ_MARKED=$(OBJ_MARKED:@=obj\vc9\@)
RES_MARKED=$(RES_MARKED:@=obj\vc9\@)

!elseif "$(COMPILER_NAME)" == "icl"
OBJ_MARKED=$(OBJ_MARKED:@=obj\icl\@)
RES_MARKED=$(RES_MARKED:@=obj\icl\@)
!else
!error No compiler configured
!endif

# last step, insert the linkage (shared/static) and release mode
# (release/debug/stldebug) into the path
OBJ=$(OBJ_MARKED:@=shared\)
OBJ_DBG=$(OBJ_MARKED:@=shared-g\)
OBJ_STLDBG=$(OBJ_MARKED:@=shared-stlg\)
OBJ_A=$(OBJ_MARKED:@=static\)
OBJ_A_DBG=$(OBJ_MARKED:@=static-g\)
OBJ_A_STLDBG=$(OBJ_MARKED:@=static-stlg\)
RES=$(RES_MARKED:@=shared\)
RES_DBG=$(RES_MARKED:@=shared-g\)
RES_STLDBG=$(RES_MARKED:@=shared-stlg\)


