# Copyright (c) 2001-2006 IBM, Inc. and others
#
# File
#    icu/source/samples/ufortune/resources/Makefile
#
# Usage:
#    See the instructions in the parent Makefile,
#          icu/source/samples/ufortune/Makefile.
#          This subproject builds the ICU resource files for ufortune.
#          It is normally invoked from the parent directory,
#            although the resources can be built from here.
#
#    Two ICU tools are run from this makefile:
#      genrb    compiles a resource source file (.txt) into
#               a binary .res file.
#      pkgdata  combines all of the .res files into a single
#               shared library that can then be linked with the
#               main application.
#
#               pkgdata will recursively generate and run yet
#               another makefile, which in turn runs two more
#               icu tools.  gencmn combines the individual .res
#               files, and genccode emits the data as C source
#               code that can then be built into a .so
#

include ../../defs.mk

# No conventional target - this dir is resources only.
TARGET=
CLEANFILES += *.[co] *.lst $(RESNAME)_*.mak $(RESNAME).dat $(RESFILES) *.ao README*resources.txt

include ../fortunedefs.mk

# target file for resource bundle - this must be set, or 'make all' won't
# build any resources.
ifeq ($(RESMODE),dll)
RESTARGET=lib$(RESNAME).$(SO)
endif
ifeq ($(RESMODE),static)
RESTARGET=lib$(RESNAME).a
endif

# Resource files.  Add new ones for additional locales here.
# keep in sync with the file RESLIST
RESFILES=root.res es.res

# list of resource bundles  - keep in sync with RESFILES
RESLIST=res-file-list.txt

## Include standard rules 
include ../../rules.mk

# for installing the library
install: res-install

es.res: es.txt
	@echo "generating $@"
	$(GENRB) $(GENRBOPT) $^

