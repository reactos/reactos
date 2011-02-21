# Copyright (c) 2001-2006 IBM, Inc. and others
#
#  fortune_resources.mak
#
#      Windows nmake makefile for compiling and packaging the resources
#      for for the ICU sample program "ufortune".
#
#      This makefile is normally invoked by the pre-link step in the
#      MSVC project file for ufortune

#
#  List of resource files to be built.
#    When adding a resource source (.txt) file for a new locale, the corresponding
#    .res file must be added to this list, AND to the file res-file-list.txt
#
RESFILES= root.res es.res 

#
#  ICUDIR   the location of ICU, used to locate the tools for
#           compiling and packaging resources.
#
ICUDIR=..\..\..\..

#
#  File name extensions for inference rule matching.
#    clear out the built-in ones (for .c and the like), and add
#    the definition for .txt to .res.
#
.SUFFIXES : .txt

#
#  Inference rule, for compiling a .txt file into a .res file.
#  -t fools make into thinking there are files such as es.res, etc
#
.txt.res:
	$(ICUDIR)\bin\genrb -d . $*.txt

#
#  all - nmake starts here by default
#
all: fortune_resources.dll

fortune_resources.dll: $(RESFILES)
	$(ICUDIR)\bin\pkgdata --name fortune_resources -v --mode dll -d . res-file-list.txt

