# Copyright (c) 2001-2002 IBM, Inc. and others
# common makefile between ufortune and ufortune/resources

# mode of resource bundle -
# you can change this to:
#   dll    -  will create a dynamically linked library 
#                 (may require 'make install' in resources subdir for
#                  proper library installation)
#
#   static - will statically link data into ufortune
#
#   common - will create fortune_resources.dat in the resources subdir
#                (must be locatable by ICU_PATH - use 'make check')
#
#   files  - will use separate files, such as es.res, fi.res, etc.
#                (use 'make check')
#
RESMODE=static

# Resource shortname
RESNAME=fortune_resources

RESLDFLAGS=
# Don't call udata_setAppData unless we are linked with the data
RESCPPFLAGS=-DUFORTUNE_NOSETAPPDATA
CHECK_VARS= ICU_DATA=$(RESDIR)

# DLL and static modes are identical here
ifeq ($(RESMODE),dll)
RESLDFLAGS= -L$(RESDIR) -l$(RESNAME)
RESCPPFLAGS=
CHECK_VARS=
endif

ifeq ($(RESMODE),static)
RESLDFLAGS= -L$(RESDIR) -l$(RESNAME)
RESCPPFLAGS=
CHECK_VARS=
endif

