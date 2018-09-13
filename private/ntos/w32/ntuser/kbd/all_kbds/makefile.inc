#############################################################################
#
# This is the first phase of building keyboard layouts
#
# For each *.txt layout definition (eg: kbdbe.txt kbdbtr.txt etc.):
#  - make the ..\tmp\* directory
#  - generate "sources", "makefile" and "makefile.inc" files in ..\tmp\*
#
# Also generate the ..\tmp\dirs files
#
# THIS FILE IS INCLUDED BY ..\us_kbd\makefile.inc
#
#############################################################################

#
# Layout names (eg: kbdbe)
#
TARGETNAMES=$(SOURCES:.c=)

#
# Directories in which layout will be built (eg: ..\tmp\kbdbe)
#
TARGETDIRS=$(TARGETNAMES:kbd=..\tmp\kbd)

#
# "sources" files used by BUILD to compile layouts (eg: kbdbe\sources)
#
TARGETSOURCEFILES=$(SOURCES:.c=\sources)

#############################################################################
#
# Build all
# builds the "dirs" file that steers the compile and link phase
#
#############################################################################
all: always_build_dirs $(TARGETSOURCEFILES:kbd=..\tmp\kbd)

#
# build -clean does this
#
clean:
    delnode /q ..\tmp\*

#
# Build the DIRS file in ..\tmp
# do this always
#
always_build_dirs: ..\tmp\dirs
    type << > ..\tmp\dirs
DIRS=\
$(TARGETNAMES)
<<


#
# If the "sources" or (this) "makefile.inc" file in this directory have
# changed, make the TARGETDIRS out of date (by deleting)
#
..\tmp\dirs: ..\all_kbds\sources ..\us_kbd\sources makefile.inc
    -md ..\tmp
    type << > ..\tmp\dirs
DIRS=\
$(TARGETNAMES)
<<
    delnode /q ..\tmp\kbd*

#
# Dependencies to make sure the "sources" files are built
# (eg: ..\tmp\kbdbe\sources: ..\tmp\kbdbe)
#
$(TARGETSOURCEFILES:kbd=..\tmp\kbd): $$(@D)


#
# build the SOURCES, MAKEFILE and MAKEFILE.INC in each ..\tmp\kbd*
#   The target of this rule is a directory, which is a problem, since the
#   timestamp of these can only change if the directory is deleted and
#   recreated: hence the delnode /q ..\tmp\kbd* line is the ..\tmp\dirs
#   rule (above).  This deletes the _objects.mac files so you have to run
#   build twice is either the sources or makefile.inc files are updated.
#   To get around this by changing the
#
$(TARGETDIRS): sources ..\all_kbds\makefile.inc
    -md $@
    copy ..\all_kbds\makefile.tpl $@\makefile
    type << > $@\makefile.inc
$(@B).c $(@B).h $(@B).rc $(@B).def: ..\..\all_kbds\$(@B).txt
    kbdtool ..\..\all_kbds\$(@B).txt
<<
    type << > $@\sources._xx
^!IF 0
Copyright (c) 1995  Microsoft Corporation
^!ENDIF

NOLINK=
MAJORCOMP=user
MINORCOMP=$(@B)
TARGETNAME=$(@B)
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=
INCLUDES=..\..\..\inc
SOURCES=$(@B).c $(@B).rc
C_DEFINES=
UMTYPE=nt
UMTEST=
UMAPPL=
UMBASE=0x1000000
UMLIBS=
^!if $$(IA64)
SECTION_ALIGNMENT=0x4000
LINKER_FLAGS = -merge:.edata=.data -merge:.srdata=.data -merge:.text=.data -merge:.bss=.data -section:.data,re
^!else
LINKER_FLAGS = -merge:.edata=.data -merge:.rdata=.data -merge:.text=.data -merge:.bss=.data -section:.data,re
^!endif

NTTARGETFILE0=$(@B).h $(@B).rc $(@B).def
<< 
    sed "s/^\^//" $@\sources._xx > $@\sources
    del /f $@\sources._xx
