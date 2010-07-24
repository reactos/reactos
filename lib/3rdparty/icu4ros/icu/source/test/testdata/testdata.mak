#**********************************************************************
#* Copyright (C) 1999-2007, International Business Machines Corporation
#* and others.  All Rights Reserved.
#**********************************************************************
#
#   03/19/2001  weiv, schererm  Created

.SUFFIXES : .res .txt

TESTPKG=testdata
TESTDT=$(TESTPKG)


ALL : "$(TESTDATAOUT)\testdata.dat" 
	@echo Test data is built.

# icu26_testtypes.res is there for cintltst/udatatst.c/TestSwapData()
# I generated it with an ICU 2.6.1 build on Windows after removing
# testincludeUTF (which made it large, unnecessarily for this test)
# and CollationElements (which will not work with a newer swapper)
# markus 2003nov19

# icu26e_testtypes.res is the same, but icuswapped to big-endian EBCDIC
# markus 2003nov21

# the following file has $(TEST_RES_SOURCE)
!INCLUDE "$(TESTDATA)\tstfiles.mk"

TEST_RES_FILES = $(TEST_RES_SOURCE:.txt=.res)

"$(TESTDATAOUT)\testdata.dat" : $(TEST_RES_FILES) "$(TESTDATABLD)\casing.res" "$(TESTDATABLD)\conversion.res" "$(TESTDATABLD)\icuio.res" "$(TESTDATABLD)\mc.res" "$(TESTDATABLD)\structLocale.res" "$(TESTDATABLD)\root.res" "$(TESTDATABLD)\sh.res" "$(TESTDATABLD)\sh_YU.res"  "$(TESTDATABLD)\te.res" "$(TESTDATABLD)\te_IN.res" "$(TESTDATABLD)\te_IN_REVISED.res" "$(TESTDATABLD)\testaliases.res" "$(TESTDATABLD)\testtypes.res" "$(TESTDATABLD)\testempty.res" "$(TESTDATABLD)\iscii.res" "$(TESTDATABLD)\idna_rules.res" "$(TESTDATABLD)\DataDrivenCollationTest.res" "$(TESTDATABLD)\test.icu" "$(TESTDATABLD)\testtable32.res" "$(TESTDATABLD)\test1.cnv" "$(TESTDATABLD)\test3.cnv" "$(TESTDATABLD)\test4.cnv" "$(TESTDATABLD)\test4x.cnv" "$(TESTDATABLD)\ibm9027.cnv" "$(TESTDATABLD)\nfscsi.spp" "$(TESTDATABLD)\nfscss.spp" "$(TESTDATABLD)\nfscis.spp" "$(TESTDATABLD)\nfsmxs.spp" "$(TESTDATABLD)\nfsmxp.spp"
	@echo Building test data
	@copy "$(TESTDATABLD)\te.res" "$(TESTDATAOUT)\$(TESTDT)\nam.typ"
	@copy "$(TESTDATA)\icu26_testtypes.res" "$(TESTDATABLD)"
	@copy "$(TESTDATA)\icu26e_testtypes.res" "$(TESTDATABLD)"
	"$(ICUP)\bin\pkgdata" -f -v -m common -c -p"$(TESTPKG)" -d "$(TESTDATAOUT)" -T "$(TESTDATABLD)" -s "$(TESTDATABLD)" <<
casing.res
conversion.res
mc.res
root.res
testtable32.res
sh.res
sh_YU.res
te.res
te_IN.res
te_IN_REVISED.res
testtypes.res
icu26_testtypes.res
icu26e_testtypes.res
testempty.res
testaliases.res
structLocale.res
icuio.res
iscii.res
test.icu
test1.cnv
test3.cnv
test4.cnv
test4x.cnv
ibm9027.cnv
idna_rules.res
nfscsi.spp
nfscss.spp
nfscis.spp
nfsmxs.spp
nfsmxp.spp
$(TEST_RES_FILES:.res =.res
)
<<


# Inference rule for creating resource bundles
# Some test data resource bundles are known to have warnings and bad data.
# The -q option is there on purpose, so we don't see it normally.
{$(TESTDATA)}.txt.res:: 
	@echo Making Test Resource Bundle files $<
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -q -s"$(TESTDATA)" -d"$(TESTDATABLD)" $<

"$(TESTDATABLD)\iscii.res": "$(TESTDATA)\iscii.bin"
	@echo Making Test Resource Bundle file with encoding ISCII,version=0
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -s"$(TESTDATA)" -eISCII,version=0 -d"$(TESTDATABLD)" iscii.bin

"$(TESTDATABLD)\idna_rules.res": "$(TESTDATA)\idna_rules.txt"
	@echo Making Test Resource Bundle file for IDNA reference implementation
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -s"$(TESTDATA)" -d"$(TESTDATABLD)" idna_rules.txt


"$(TESTDATABLD)\test.icu" : {"$(ICUTOOLS)\gentest\$(CFG)"}gentest.exe
	"$(ICUTOOLS)\gentest\$(CFG)\gentest" -d"$(TESTDATABLD)"

# testtable32 resource file
"$(TESTDATABLD)\testtable32.txt" : {"$(ICUTOOLS)\gentest\$(CFG)"}gentest.exe
	"$(ICUTOOLS)\gentest\$(CFG)\gentest" -r -d"$(TESTDATABLD)"

"$(TESTDATABLD)\testtable32.res": "$(TESTDATABLD)\testtable32.txt"
	"$(ICUTOOLS)\genrb\$(CFG)\genrb" -s"$(TESTDATABLD)" -d"$(TESTDATABLD)" testtable32.txt

# Targets for nfscsi.spp
"$(TESTDATABLD)\nfscsi.spp" : {"$(ICUTOOLS)\gensprep\$(CFG)"}gensprep.exe "$(TESTDATA)\nfs4_cs_prep_ci.txt"
	@echo Building $@
	@"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s "$(TESTDATA)" -d "$(TESTDATABLD)\\" -b nfscsi -u 3.2.0 nfs4_cs_prep_ci.txt

# Targets for nfscss.spp
"$(TESTDATABLD)\nfscss.spp" : {"$(ICUTOOLS)\gensprep\$(CFG)"}gensprep.exe "$(TESTDATA)\nfs4_cs_prep_cs.txt"
	@echo Building $@
	@"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s "$(TESTDATA)" -d "$(TESTDATABLD)\\" -b nfscss -u 3.2.0 nfs4_cs_prep_cs.txt

# Targets for nfscis.spp
"$(TESTDATABLD)\nfscis.spp" : {"$(ICUTOOLS)\gensprep\$(CFG)"}gensprep.exe "$(TESTDATA)\nfs4_cis_prep.txt"
	@echo Building $@
	@"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s "$(TESTDATA)" -d "$(TESTDATABLD)\\" -b nfscis -u 3.2.0 -k -n "$(ICUTOOLS)\..\data\unidata" nfs4_cis_prep.txt

# Targets for nfsmxs.spp
"$(TESTDATABLD)\nfsmxs.spp" : {"$(ICUTOOLS)\gensprep\$(CFG)"}gensprep.exe "$(TESTDATA)\nfs4_mixed_prep_s.txt"
	@echo Building $@
	@"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s "$(TESTDATA)" -d "$(TESTDATABLD)\\" -b nfsmxs -u 3.2.0 -k -n "$(ICUTOOLS)\..\data\unidata" nfs4_mixed_prep_s.txt

# Targets for nfsmxp.spp
"$(TESTDATABLD)\nfsmxp.spp" : {"$(ICUTOOLS)\gensprep\$(CFG)"}gensprep.exe "$(TESTDATA)\nfs4_mixed_prep_p.txt"
	@echo Building $@
	@"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s "$(TESTDATA)" -d "$(TESTDATABLD)\\" -b nfsmxp -u 3.2.0 -k -n "$(ICUTOOLS)\..\data\unidata" nfs4_mixed_prep_p.txt


# Targets for test converter data
"$(TESTDATABLD)\test1.cnv": "$(TESTDATA)\test1.ucm"
	@echo Building $@
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -d"$(TESTDATABLD)" $**

"$(TESTDATABLD)\test3.cnv": "$(TESTDATA)\test3.ucm"
	@echo Building $@
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -d"$(TESTDATABLD)" $**

"$(TESTDATABLD)\test4.cnv": "$(TESTDATA)\test4.ucm"
	@echo Building $@
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -d"$(TESTDATABLD)" $**

"$(TESTDATABLD)\test4x.cnv": "$(TESTDATA)\test4x.ucm"
	@echo Building $@
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -d"$(TESTDATABLD)" $**

"$(TESTDATABLD)\ibm9027.cnv": "$(TESTDATA)\ibm9027.ucm"
	@echo Building $@
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -d"$(TESTDATABLD)" $**

