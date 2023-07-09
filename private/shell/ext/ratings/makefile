##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#
#       Global makefile for RATINGS project
#
##########################################################################

!if "$(NTMAKEENV)" != ""
#
# This section is for the NT build ONLY.
# For NT build, change the 'sources' or 'dirs' file, not this makefile
#
clean:

!else
#
# Non-NT build makefile start HERE.
#

MAKE = nmake

DIRS = common. mslocusr.  msrating.

DIRLIST = $(DIRS:.=)
ALLLIST = $(DIRLIST)

all:    $(DIRS:.=.all)

clean:  $(DIRS:.=.cln)

debug:  $(DIRS:.=.dbg)

retail:  $(DIRS:.=.rtl)

depend:  $(DIRS:.=.dpn)

default: $(DIRS:.=.rtl)


$(DIRS:.=):
    cd  $@
    $(MAKE) all
    cd ..

$(DIRS:.=.all):
    cd  $*
    $(MAKE) all
    cd ..

$(DIRS:.=.dpn):
    cd  $*
    $(MAKE) depend
    cd ..

$(DIRS:.=.cln):
    cd  $*
    $(MAKE) clean
    cd ..

$(DIRS:.=.dbg):
    cd  $*
    $(MAKE) debug
    cd ..

$(DIRS:.=.rtl):
    cd  $*
    $(MAKE) retail
    cd ..

!endif    # !if NTMAKEENV
