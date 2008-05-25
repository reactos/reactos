#**********************************************************************
#* Copyright (C) 1999-2007, International Business Machines Corporation
#* and others.  All Rights Reserved.
#**********************************************************************
# nmake file for creating data files on win32
# invoke with
# nmake /f makedata.mak icumake=$(ProjectDir)
#
#	12/10/1999	weiv	Created

##############################################################################
# Keep the following in sync with the version - see common/unicode/uversion.h
U_ICUDATA_NAME=icudt38
##############################################################################
U_ICUDATA_ENDIAN_SUFFIX=l
UNICODE_VERSION=5.0
ICU_LIB_TARGET=$(DLL_OUTPUT)\$(U_ICUDATA_NAME).dll

#  ICUMAKE
#     Must be provided by whoever runs this makefile.
#     Is the directory containing this file (makedata.mak)
#     Is the directory into which most data is built (prior to packaging)
#     Is icu\source\data\build
#
!IF "$(ICUMAKE)"==""
!ERROR Can't find ICUMAKE (ICU Data Make dir, should point to icu\source\data\ )!
!ENDIF
!MESSAGE ICU data make path is $(ICUMAKE)

# Suffixes for data files
.SUFFIXES : .ucm .cnv .dll .dat .res .txt .c

ICUOUT=$(ICUMAKE)\out

#  the prefix "icudt21_" for use in filenames
ICUPKG=$(U_ICUDATA_NAME)$(U_ICUDATA_ENDIAN_SUFFIX)

# need to nuke \\ for .NET...
ICUOUT=$(ICUOUT:\\=\)

ICUBLD=$(ICUOUT)\build
ICUBLD_PKG=$(ICUBLD)\$(ICUPKG)
ICUTMP=$(ICUOUT)\tmp

#  ICUP
#     The root of the ICU source directory tree
#
ICUP=$(ICUMAKE)\..\..
ICUP=$(ICUP:\source\data\..\..=)
# In case the first one didn't do it, try this one.  .NET would do the second one.
ICUP=$(ICUP:\source\data\\..\..=)
!MESSAGE ICU root path is $(ICUP)


#  ICUSRCDATA
#       The data directory in source
#
ICUSRCDATA=$(ICUP)\source\data
ICUSRCDATA_RELATIVE_PATH=..\..\..

#  ICUUCM
#       The directory that contains ucmcore.mk files along with *.ucm files
#
ICUUCM=mappings

#  ICULOC
#       The directory that contains resfiles.mk files along with *.txt locale data files
#
ICULOC=locales

#  ICUCOL
#       The directory that contains colfiles.mk files along with *.txt collation data files
#
ICUCOL=coll

#  ICURBNF
#       The directory that contains rbnffiles.mk files along with *.txt RBNF data files
#
ICURBNF=rbnf

#  ICUTRNS
#       The directory that contains trfiles.mk files along with *.txt transliterator files
#
ICUTRNS=translit

#  ICUBRK
#       The directory that contains resfiles.mk files along with *.txt break iterator files
#
ICUBRK=brkitr

#  ICUUNIDATA
#       The directory that contains Unicode data files
#
ICUUNIDATA=$(ICUP)\source\data\unidata


#  ICUMISC
#       The directory that contains miscfiles.mk along with files that are miscelleneous data
#
ICUMISC=$(ICUP)\source\data\misc
ICUMISC2=misc

#
#  ICUDATA
#     The source directory.  Contains the source files for the common data to be built.
#     WARNING:  NOT THE SAME AS ICU_DATA environment variable.  Confusing.
ICUDATA=$(ICUP)\source\data

#
#  DLL_OUTPUT
#      Destination directory for the common data DLL file.
#      This is the same place that all of the other ICU DLLs go (the code-containing DLLs)
#      The lib file for the data DLL goes in $(DLL_OUTPUT)/../lib/
#
DLL_OUTPUT=$(ICUP)\bin

#
#  TESTDATA
#     The source directory for data needed for test programs.
TESTDATA=$(ICUP)\source\test\testdata

#
#   TESTDATAOUT
#      The destination directory for the built test data .dat file
TESTDATAOUT=$(ICUP)\source\test\testdata\out

#
#   TESTDATABLD
#		The build directory for test data intermidiate files
#		(Tests are NOT run from this makefile,
#         only the data is put in place.)
TESTDATABLD=$(ICUP)\source\test\testdata\out\build

#
#   ICUTOOLS
#       Directory under which all of the ICU data building tools live.
#
ICUTOOLS=$(ICUP)\source\tools

# The current ICU tools need to be in the path first.
PATH = $(ICUP)\bin;$(PATH)

# This variable can be overridden to "-m static" by the project settings,
# if you want a static data library.
!IF "$(ICU_PACKAGE_MODE)"==""
ICU_PACKAGE_MODE=-m dll
!ENDIF

# If this archive exists, build from that
# instead of building everything from scratch.
ICUDATA_SOURCE_ARCHIVE=$(ICUSRCDATA)\in\$(ICUPKG).dat
!IF !EXISTS("$(ICUDATA_SOURCE_ARCHIVE)")
# Does a big endian version exist either?
ICUDATA_ARCHIVE=$(ICUSRCDATA)\in\$(U_ICUDATA_NAME)b.dat
!IF EXISTS("$(ICUDATA_ARCHIVE)")
ICUDATA_SOURCE_ARCHIVE=$(ICUTMP)\$(ICUPKG).dat
!ELSE
# Nothing was usable for input
!UNDEF ICUDATA_SOURCE_ARCHIVE
!ENDIF
!ENDIF

!IFDEF ICUDATA_SOURCE_ARCHIVE
!MESSAGE ICU data source archive is $(ICUDATA_SOURCE_ARCHIVE)
!ELSE
# We're including a list of .ucm files.
# There are several lists, they are all optional.

# Always build the mapping files for the EBCDIC fallback codepages
# They are necessary on EBCDIC machines, and
# the following logic is much easier if UCM_SOURCE is never empty.
# (They are small.)
UCM_SOURCE=ibm-37_P100-1995.ucm ibm-1047_P100-1995.ucm

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmcore.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmcore.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_CORE)
!ELSE
!MESSAGE Warning: cannot find "ucmcore.mk". Not building core MIME/Unix/Windows converter files.
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmfiles.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_FILES)
!ELSE
!MESSAGE Warning: cannot find "ucmfiles.mk". Not building many converter files.
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmebcdic.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmebcdic.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_EBCDIC)
!ELSE
!MESSAGE Warning: cannot find "ucmebcdic.mk". Not building EBCDIC converter files.
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmlocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmlocal.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "ucmlocal.mk". Not building user-additional converter files.
!ENDIF

CNV_FILES=$(UCM_SOURCE:.ucm=.cnv)

!IF EXISTS("$(ICUSRCDATA)\$(ICUBRK)\brkfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUBRK)\brkfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUBRK)\brklocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUBRK)\brklocal.mk"
BRK_SOURCE=$(BRK_SOURCE) $(BRK_SOURCE_LOCAL)
BRK_CTD_SOURCE=$(BRK_CTD_SOURCE) $(BRK_CTD_SOURCE_LOCAL)
BRK_RES_SOURCE=$(BRK_RES_SOURCE) $(BRK_RES_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "brklocal.mk". Not building user-additional break iterator files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "brkfiles.mk"
!ENDIF

#
#  Break iterator data files.
#
BRK_FILES=$(ICUBRK)\$(BRK_SOURCE:.txt =.brk brkitr\)
BRK_FILES=$(BRK_FILES:.txt=.brk)
BRK_FILES=$(BRK_FILES:brkitr\ =brkitr\)

!IFDEF BRK_CTD_SOURCE
BRK_CTD_FILES = $(ICUBRK)\$(BRK_CTD_SOURCE:.txt =.ctd brkitr\)
BRK_CTD_FILES = $(BRK_CTD_FILES:.txt=.ctd)
BRK_CTD_FILES = $(BRK_CTD_FILES:brkitr\ =)
!ENDIF

!IFDEF BRK_RES_SOURCE
BRK_RES_FILES = $(BRK_RES_SOURCE:.txt =.res brkitr\)
BRK_RES_FILES = $(BRK_RES_FILES:.txt=.res)
BRK_RES_FILES = $(ICUBRK)\root.res $(ICUBRK)\$(BRK_RES_FILES:brkitr\ =)
ALL_RES = $(ALL_RES) $(ICUBRK)\res_index.res
!ENDIF

# Read list of locale resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICULOC)\resfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICULOC)\resfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICULOC)\reslocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICULOC)\reslocal.mk"
GENRB_SOURCE=$(GENRB_SOURCE) $(GENRB_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "reslocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "resfiles.mk"
!ENDIF

!IFDEF GENRB_SOURCE
RB_FILES = root.res $(GENRB_ALIAS_SOURCE:.txt=.res) $(GENRB_ALIAS_SOURCE_LOCAL:.txt=.res) $(GENRB_SOURCE:.txt=.res)
ALL_RES = $(ALL_RES) res_index.res
!ENDIF


# Read list of locale resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICUCOL)\colfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUCOL)\colfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUCOL)\collocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUCOL)\collocal.mk"
COLLATION_SOURCE=$(COLLATION_SOURCE) $(COLLATION_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "collocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "colfiles.mk"
!ENDIF

!IFDEF COLLATION_SOURCE
COL_FILES = $(ICUCOL)\root.txt $(COLLATION_ALIAS_SOURCE) $(COLLATION_SOURCE)
COL_COL_FILES = $(COL_FILES:.txt =.res coll\)
COL_COL_FILES = $(COL_COL_FILES:.txt=.res)
COL_COL_FILES = $(COL_COL_FILES:coll\ =)
ALL_RES = $(ALL_RES) $(ICUCOL)\res_index.res
!ENDIF

# Read list of RBNF resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICURBNF)\rbnffiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICURBNF)\rbnffiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICURBNF)\rbnflocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICURBNF)\rbnflocal.mk"
RBNF_SOURCE=$(RBNF_SOURCE) $(RBNF_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "rbnflocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "rbnffiles.mk"
!ENDIF

!IFDEF RBNF_SOURCE
RBNF_FILES = $(ICURBNF)\root.txt $(RBNF_ALIAS_SOURCE) $(RBNF_SOURCE)
RBNF_RES_FILES = $(RBNF_FILES:.txt =.res rbnf\)
RBNF_RES_FILES = $(RBNF_RES_FILES:.txt=.res)
RBNF_RES_FILES = $(RBNF_RES_FILES:rbnf\ =rbnf\)
ALL_RES = $(ALL_RES) $(ICURBNF)\res_index.res
!ENDIF

# Read list of transliterator resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICUTRNS)\trnsfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUTRNS)\trnsfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUTRNS)\trnslocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUTRNS)\trnslocal.mk"
TRANSLIT_SOURCE=$(TRANSLIT_SOURCE) $(TRANSLIT_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "trnslocal.mk". Not building user-additional transliterator files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "trnsfiles.mk"
!ENDIF

!IFDEF TRANSLIT_SOURCE
TRANSLIT_FILES = $(ICUTRNS)\$(TRANSLIT_ALIAS_SOURCE) $(TRANSLIT_SOURCE)
TRANSLIT_RES_FILES = $(TRANSLIT_FILES:.txt =.res translit\)
TRANSLIT_RES_FILES = $(TRANSLIT_RES_FILES:.txt=.res)
TRANSLIT_RES_FILES = $(TRANSLIT_RES_FILES:translit\ =translit\)
#ALL_RES = $(ALL_RES) $(ICUTRNS)\res_index.res
!ENDIF

# Read list of miscellaneous resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICUMISC2)\miscfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUMISC2)\miscfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUMISC2)\misclocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUMISC2)\misclocal.mk"
MISC_SOURCE=$(MISC_SOURCE) $(MISC_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "misclocal.mk". Not building user-additional miscellaenous files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "miscfiles.mk"
!ENDIF

MISC_FILES = $(MISC_SOURCE:.txt=.res)

# don't include COL_FILES
ALL_RES = $(ALL_RES) $(RB_FILES) $(MISC_FILES)
!ENDIF

# Common defines for both ways of building ICU's data library.
COMMON_ICUDATA_DEPENDENCIES="$(ICUP)\bin\pkgdata.exe" "$(ICUTMP)\icudata.res" "$(ICUP)\source\stubdata\stubdatabuilt.txt"
COMMON_ICUDATA_ARGUMENTS=-f -e $(U_ICUDATA_NAME) -v $(ICU_PACKAGE_MODE) -M"PKGDATA_LDFLAGS=/base:0x4ad00000" -c -p $(ICUPKG) -T "$(ICUTMP)" -L $(U_ICUDATA_NAME) -d "$(ICUBLD_PKG)" -s .

#############################################################################
#
# ALL
#     This target builds all the data files.  The world starts here.
#			Note: we really want the common data dll to go to $(DLL_OUTPUT), not $(ICUBLD_PKG).  But specifying
#				that here seems to cause confusion with the building of the stub library of the same name.
#				Building the common dll in $(ICUBLD_PKG) unconditionally copies it to $(DLL_OUTPUT) too.
#
#############################################################################
ALL : GODATA "$(ICU_LIB_TARGET)" "$(TESTDATAOUT)\testdata.dat"
	@echo All targets are up to date

# Starting with ICU4C 3.4, the core Unicode properties files (uprops.icu, ucase.icu, ubidi.icu, unorm.icu)
# are hardcoded in the common DLL and therefore not included in the data package any more.
# They are not built by default but need to be built for ICU4J data and for getting the .c source files
# when updating the Unicode data.
# Changed in makedata.mak revision 1.117. See Jitterbug 4497.
# Command line:
#   C:\svn\icuproj\icu\trunk\source\data>nmake -f makedata.mak ICUMAKE=C:\svn\icuproj\icu\trunk\source\data\ CFG=Debug uni-core-data
uni-core-data: GODATA "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\ubidi.icu" "$(ICUBLD_PKG)\unorm.icu"
	@echo Unicode .icu files built to "$(ICUBLD_PKG)"
	@echo Unicode .c source files built to "$(ICUTMP)"

#
# testdata - nmake will invoke pkgdata, which will create testdata.dat
#
"$(TESTDATAOUT)\testdata.dat": "$(TESTDATA)\*" "$(ICUBLD_PKG)\ucadata.icu" $(TRANSLIT_RES_FILES) $(MISC_FILES) $(RB_FILES) {"$(ICUTOOLS)\genrb\$(CFG)"}genrb.exe
	@cd "$(TESTDATA)"
	@echo building testdata...
	nmake /nologo /f "$(TESTDATA)\testdata.mak" TESTDATA=. ICUTOOLS="$(ICUTOOLS)" ICUP="$(ICUP)" CFG=$(CFG) TESTDATAOUT="$(TESTDATAOUT)" TESTDATABLD="$(TESTDATABLD)"

#invoke pkgdata for ICU common data
#  pkgdata will drop all output files (.dat, .dll, .lib) into the target (ICUBLD_PKG) directory.
#  move the .dll and .lib files to their final destination afterwards.
#  The $(U_ICUDATA_NAME).lib and $(U_ICUDATA_NAME).exp should already be in the right place due to stubdata.
#
#  2005-may-05 Removed Unicode properties files (unorm.icu, uprops.icu, ucase.icu, ubidi.icu)
#  from data build. See Jitterbug 4497. (makedata.mak revision 1.117)
#
!IFDEF ICUDATA_SOURCE_ARCHIVE
"$(ICU_LIB_TARGET)" : $(COMMON_ICUDATA_DEPENDENCIES) "$(ICUDATA_SOURCE_ARCHIVE)"
	@echo Building icu data from $(ICUDATA_SOURCE_ARCHIVE)
	cd "$(ICUBLD_PKG)"
	"$(ICUP)\bin\icupkg" -x * --list "$(ICUDATA_SOURCE_ARCHIVE)" > "$(ICUTMP)\icudata.lst"
	"$(ICUP)\bin\pkgdata" $(COMMON_ICUDATA_ARGUMENTS) "$(ICUTMP)\icudata.lst"
	copy "$(U_ICUDATA_NAME).dll" "$(DLL_OUTPUT)"
	-@erase "$(U_ICUDATA_NAME).dll"
	copy "$(ICUTMP)\$(ICUPKG).dat" "$(ICUOUT)\$(U_ICUDATA_NAME)$(U_ICUDATA_ENDIAN_SUFFIX).dat"
	-@erase "$(ICUTMP)\$(ICUPKG).dat"
!ELSE
"$(ICU_LIB_TARGET)" : $(COMMON_ICUDATA_DEPENDENCIES) $(CNV_FILES) "$(ICUBLD_PKG)\unames.icu" "$(ICUBLD_PKG)\pnames.icu" "$(ICUBLD_PKG)\cnvalias.icu" "$(ICUBLD_PKG)\ucadata.icu" "$(ICUBLD_PKG)\invuca.icu" "$(ICUBLD_PKG)\uidna.spp" $(BRK_FILES) $(BRK_CTD_FILES) $(BRK_RES_FILES) $(COL_COL_FILES) $(RBNF_RES_FILES) $(TRANSLIT_RES_FILES) $(ALL_RES)
	@echo Building icu data
	cd "$(ICUBLD_PKG)"
	"$(ICUP)\bin\pkgdata" $(COMMON_ICUDATA_ARGUMENTS) <<"$(ICUTMP)\icudata.lst"
pnames.icu
unames.icu
ucadata.icu
invuca.icu
uidna.spp
cnvalias.icu
$(CNV_FILES:.cnv =.cnv
)
$(ALL_RES:.res =.res
)
$(COL_COL_FILES:.res =.res
)
$(RBNF_RES_FILES:.res =.res
)
$(TRANSLIT_RES_FILES:.res =.res
)
$(BRK_FILES:.brk =.brk
)
$(BRK_CTD_FILES:.ctd =.ctd
)
$(BRK_RES_FILES:.res =.res
)
<<KEEP
	-@erase "$(ICU_LIB_TARGET)"
	copy "$(U_ICUDATA_NAME).dll" "$(ICU_LIB_TARGET)"
	-@erase "$(U_ICUDATA_NAME).dll"
	copy "$(ICUTMP)\$(ICUPKG).dat" "$(ICUOUT)\$(U_ICUDATA_NAME)$(U_ICUDATA_ENDIAN_SUFFIX).dat"
	-@erase "$(ICUTMP)\$(ICUPKG).dat"
!ENDIF

# utility target to create missing directories
CREATE_DIRS :
	@if not exist "$(ICUOUT)\$(NULL)" mkdir "$(ICUOUT)"
	@if not exist "$(ICUTMP)\$(NULL)" mkdir "$(ICUTMP)"
	@if not exist "$(ICUOUT)\build\$(NULL)" mkdir "$(ICUOUT)\build"
	@if not exist "$(ICUBLD_PKG)\$(NULL)" mkdir "$(ICUBLD_PKG)"
	@if not exist "$(ICUBLD_PKG)\$(ICUBRK)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICUBRK)"
	@if not exist "$(ICUBLD_PKG)\$(ICUCOL)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICUCOL)"
	@if not exist "$(ICUBLD_PKG)\$(ICURBNF)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICURBNF)"
	@if not exist "$(ICUBLD_PKG)\$(ICUTRNS)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICUTRNS)"
	@if not exist "$(TESTDATAOUT)\$(NULL)" mkdir "$(TESTDATAOUT)"
	@if not exist "$(TESTDATABLD)\$(NULL)" mkdir "$(TESTDATABLD)"
	@if not exist "$(TESTDATAOUT)\testdata\$(NULL)" mkdir "$(TESTDATAOUT)\testdata"

# utility target to send us to the right dir
GODATA : CREATE_DIRS
	@cd "$(ICUBLD_PKG)"

# This is to remove all the data files
CLEAN : GODATA
	@echo Cleaning up the data files.
	@cd "$(ICUBLD_PKG)"
	-@erase "*.cnv"
	-@erase "*.exp"
	-@erase "*.icu"
	-@erase "*.lib"
	-@erase "*.res"
	-@erase "*.spp"
	-@erase "*.txt"
	@cd "$(ICUBLD_PKG)\$(ICUBRK)"
	-@erase "*.brk"
	-@erase "*.ctd"
	-@erase "*.res"
	-@erase "*.txt"
	@cd "$(ICUBLD_PKG)\$(ICUCOL)"
	-@erase "*.res"
	-@erase "*.txt"
	@cd "$(ICUBLD_PKG)\$(ICURBNF)"
	-@erase "*.res"
	-@erase "*.txt"
    @cd "$(ICUBLD_PKG)\$(ICUTRNS)"
	-@erase "*.res"
	@cd "$(ICUOUT)"
	-@erase "*.dat"
	@cd "$(ICUTMP)"
	-@erase "*.html"
	-@erase "*.lst"
	-@erase "*.mak"
	-@erase "*.obj"
	-@erase "*.res"
	@cd "$(TESTDATABLD)"
	-@erase "*.cnv"
	-@erase "*.icu"
	-@erase "*.mak"
	-@erase "*.res"
	-@erase "*.spp"
	-@erase "*.txt"
	@cd "$(TESTDATAOUT)"
	-@erase "*.dat"
	@cd "$(TESTDATAOUT)\testdata"
	-@erase "*.typ"
	@cd "$(ICUBLD_PKG)"


# RBBI .brk file generation.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUBRK)}.txt.brk:
	@echo Creating $@
	@"$(ICUTOOLS)\genbrk\$(CFG)\genbrk" -c -r $< -o $@ -d"$(ICUBLD_PKG)" -i "$(ICUBLD_PKG)"

# RBBI .ctd file generation.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUBRK)}.txt.ctd:
	@echo Creating $@
	@"$(ICUTOOLS)\genctd\$(CFG)\genctd" -c -o $@ -d"$(ICUBLD_PKG)" -i "$(ICUBLD_PKG)" $<

# Batch inference rule for creating converters
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUUCM)}.ucm.cnv::
	@echo Making Charset Conversion tables
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -c -d"$(ICUBLD_PKG)" $<

# Batch inference rule for creating miscellaneous resource files
# TODO: -q option is specified to squelch the 120+ warnings about
#       empty intvectors and binary elements.  Unfortunately, this may
#       squelch other legitimate warnings.  When there is a better
#       way, remove the -q.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUMISC2)}.txt.res::
	@echo Making Miscellaneous Resource Bundle files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -q -d"$(ICUBLD_PKG)" $<

# Inference rule for creating resource bundle files
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICULOC)}.txt.res::
	@echo Making Locale Resource Bundle files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)" $<

res_index.res:
	@echo Generating <<res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(GENRB_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)" .\res_index.txt
	
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUCOL)}.txt{$(ICUCOL)}.res::
	@echo Making Collation files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICUCOL)" $<

$(ICUCOL)\res_index.res:
	@echo Generating <<$(ICUCOL)\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(COLLATION_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\$(ICUCOL)" .\$(ICUCOL)\res_index.txt

{$(ICUSRCDATA_RELATIVE_PATH)\$(ICURBNF)}.txt{$(ICURBNF)}.res::
	@echo Making RBNF files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICURBNF)" $<

$(ICURBNF)\res_index.res:
	@echo Generating <<$(ICURBNF)\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(RBNF_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\$(ICURBNF)" .\$(ICURBNF)\res_index.txt

$(ICUBRK)\res_index.res:
	@echo Generating <<$(ICUBRK)\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(BRK_RES_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\$(ICUBRK)" .\$(ICUBRK)\res_index.txt

{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUBRK)}.txt{$(ICUBRK)}.res::
	@echo Making Break Iterator Resource files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICUBRK)" $<

{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUTRNS)}.txt{$(ICUTRNS)}.res::
	@echo Making Transliterator files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICUTRNS)" $<


# DLL version information
# If you modify this, modify winmode.c in pkgdata.
"$(ICUTMP)\icudata.res": "$(ICUMISC)\icudata.rc"
	@echo Creating data DLL version information from $**
	@rc.exe /i "..\..\..\..\common" /r /fo $@ $**

# Targets for unames.icu
"$(ICUBLD_PKG)\unames.icu": "$(ICUUNIDATA)\*.txt" "$(ICUTOOLS)\gennames\$(CFG)\gennames.exe"
	@echo Creating data file for Unicode Names
	@"$(ICUTOOLS)\gennames\$(CFG)\gennames" -1 -u $(UNICODE_VERSION) -d "$(ICUBLD_PKG)" "$(ICUUNIDATA)\UnicodeData.txt"

# Targets for pnames.icu
# >> Depends on the Unicode data as well as uchar.h and uscript.h <<
"$(ICUBLD_PKG)\pnames.icu": "$(ICUUNIDATA)\*.txt" "$(ICUTOOLS)\genpname\$(CFG)\genpname.exe" "$(ICUP)\source\common\unicode\uchar.h" "$(ICUP)\source\common\unicode\uscript.h"
	@echo Creating data file for Unicode Property Names
	@"$(ICUTOOLS)\genpname\$(CFG)\genpname" -d "$(ICUBLD_PKG)"

# Targets for uprops.icu
"$(ICUBLD_PKG)\uprops.icu": "$(ICUUNIDATA)\*.txt" "$(ICUTOOLS)\genprops\$(CFG)\genprops.exe" "$(ICUBLD_PKG)\pnames.icu"
	@echo Creating data file for Unicode Character Properties
	@"$(ICUTOOLS)\genprops\$(CFG)\genprops" -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUBLD_PKG)"
	@"$(ICUTOOLS)\genprops\$(CFG)\genprops" --csource -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUTMP)"

# Targets for ubidi.icu
"$(ICUBLD_PKG)\ubidi.icu": "$(ICUUNIDATA)\*.txt" "$(ICUTOOLS)\genbidi\$(CFG)\genbidi.exe"
	@echo Creating data file for Unicode BiDi/Shaping Properties
	@"$(ICUTOOLS)\genbidi\$(CFG)\genbidi" -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUBLD_PKG)"
	@"$(ICUTOOLS)\genbidi\$(CFG)\genbidi" --csource -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUTMP)"

# Targets for ucase.icu
"$(ICUBLD_PKG)\ucase.icu": "$(ICUUNIDATA)\*.txt" "$(ICUTOOLS)\gencase\$(CFG)\gencase.exe"
	@echo Creating data file for Unicode Case Mapping Properties
	@"$(ICUTOOLS)\gencase\$(CFG)\gencase" -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUBLD_PKG)"
	@"$(ICUTOOLS)\gencase\$(CFG)\gencase" --csource -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUTMP)"

# Targets for unorm.icu
"$(ICUBLD_PKG)\unorm.icu": "$(ICUUNIDATA)\*.txt" "$(ICUTOOLS)\gennorm\$(CFG)\gennorm.exe" "$(ICUBLD_PKG)\pnames.icu" "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu"
	@echo Creating data file for Unicode Normalization
	@"$(ICUTOOLS)\gennorm\$(CFG)\gennorm" -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUBLD_PKG)"
	@"$(ICUTOOLS)\gennorm\$(CFG)\gennorm" --csource -u $(UNICODE_VERSION) -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)" -d "$(ICUTMP)"

# Targets for converters
"$(ICUBLD_PKG)\cnvalias.icu" : {"$(ICUSRCDATA)\$(ICUUCM)"}\convrtrs.txt "$(ICUTOOLS)\gencnval\$(CFG)\gencnval.exe"
	@echo Creating data file for Converter Aliases
	@"$(ICUTOOLS)\gencnval\$(CFG)\gencnval" -d "$(ICUBLD_PKG)" "$(ICUSRCDATA)\$(ICUUCM)\convrtrs.txt"

# Targets for ucadata.icu & invuca.icu
# used to depend on "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\unorm.icu"
# see Jitterbug 4497
"$(ICUBLD_PKG)\invuca.icu" "$(ICUBLD_PKG)\ucadata.icu": "$(ICUUNIDATA)\FractionalUCA.txt" "$(ICUTOOLS)\genuca\$(CFG)\genuca.exe"
	@echo Creating UCA data files
	@"$(ICUTOOLS)\genuca\$(CFG)\genuca" -d "$(ICUBLD_PKG)" -i "$(ICUBLD_PKG)" -s "$(ICUUNIDATA)"

# Targets for uidna.spp
"$(ICUBLD_PKG)\uidna.spp" : "$(ICUUNIDATA)\*.txt" "$(ICUMISC)\NamePrepProfile.txt"
	"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s "$(ICUMISC)" -d "$(ICUBLD_PKG)\\" -b uidna -n "$(ICUUNIDATA)" -k -u 3.2.0 NamePrepProfile.txt

!IFDEF ICUDATA_ARCHIVE
"$(ICUDATA_SOURCE_ARCHIVE)": CREATE_DIRS $(ICUDATA_ARCHIVE) "$(ICUTOOLS)\icupkg\$(CFG)\icupkg.exe"
	"$(ICUTOOLS)\icupkg\$(CFG)\icupkg" -t$(U_ICUDATA_ENDIAN_SUFFIX) "$(ICUDATA_ARCHIVE)" "$(ICUDATA_SOURCE_ARCHIVE)"
!ENDIF

# Dependencies on the tools for the batch inference rules

!IFNDEF ICUDATA_SOURCE_ARCHIVE
$(UCM_SOURCE) : {"$(ICUTOOLS)\makeconv\$(CFG)"}makeconv.exe

# This used to depend on "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\ubidi.icu" "$(ICUBLD_PKG)\unorm.icu"
# This data is now hard coded as a part of the library.
# See Jitterbug 4497 for details.
$(MISC_SOURCE) $(RB_FILES) $(COL_COL_FILES) $(RBNF_RES_FILES) $(BRK_RES_FILES) $(TRANSLIT_RES_FILES): {"$(ICUTOOLS)\genrb\$(CFG)"}genrb.exe "$(ICUBLD_PKG)\ucadata.icu"

# This used to depend on "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\ubidi.icu" "$(ICUBLD_PKG)\unorm.icu"
# This data is now hard coded as a part of the library.
# See Jitterbug 4497 for details.
$(BRK_SOURCE) : "$(ICUBLD_PKG)\unames.icu" "$(ICUBLD_PKG)\pnames.icu"
!ENDIF

