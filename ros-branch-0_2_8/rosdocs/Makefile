#
# Makefile for rosdocs module
#

PATH_TO_TOP = .

HHC=hhc
XSLTPROC=xsltproc

XSLSTYLESHEET_HTMLBIG=./xsl/html/docbook.xsl
XSLSTYLESHEET_HTMLCHUNK=./xsl/html/chunk.xsl
XSLSTYLESHEET_HTMLHELP=./xsl/htmlhelp/htmlhelp.xsl

OUTPUT_DIR=./output
OUTPUT_DIR_HTMLBIG=$(OUTPUT_DIR)/htmlbig
OUTPUT_DIR_HTMLCHUNK=$(OUTPUT_DIR)/htmlchunk
OUTPUT_DIR_HTMLHELP=$(OUTPUT_DIR)/htmlhelp

# hhc is not available on linux
ifeq ($(HOST),mingw32-linux)
all: tools htmlbig htmlchunk
	
else
all: tools htmlbig htmlchunk # htmlhelp
	
endif
	

#
# Tools
#
tools:
	$(MAKE) -C tools

tools_implib:

tools_clean:
	$(MAKE) -C tools clean

tools_install:

tools_dist:

.PHONY: tools tools_clean tools_install tools_dist



htmlchunk: $(OUTPUT_DIR_HTMLCHUNK)
	- $(XSLTPROC) -o $(OUTPUT_DIR_HTMLCHUNK)/tutorials.html $(XSLSTYLESHEET_HTMLCHUNK) tutorials/tutorials.xml

htmlbig: $(OUTPUT_DIR_HTMLBIG)
	- $(XSLTPROC) -o $(OUTPUT_DIR_HTMLBIG)/tutorials.html $(XSLSTYLESHEET_HTMLBIG) tutorials/tutorials.xml

htmlhelp: $(OUTPUT_DIR_HTMLHELP)
	- $(XSLTPROC) -o $(OUTPUT_DIR_HTMLHELP)/tutorials.html $(XSLSTYLESHEET_HTMLHELP) tutorials/tutorials.xml
	- $(HHC) $(OUTPUT_DIR_HTMLHELP)/htmlhelp.hhp

cleanoutput: tools
	- $(RM) $(OUTPUT_DIR_HTMLBIG)/*.html
	- $(RM) $(OUTPUT_DIR_HTMLCHUNK)/*.html
	- $(RM) $(OUTPUT_DIR_HTMLHELP)/*.html
	- $(RM) $(OUTPUT_DIR_HTMLHELP)/*.hhp
	- $(RM) $(OUTPUT_DIR_HTMLHELP)/*.chm
	- $(RM) $(OUTPUT_DIR_HTMLHELP)/*.hhc
	- $(RMDIR) $(OUTPUT_DIR_HTMLHELP)
	- $(RMDIR) $(OUTPUT_DIR_HTMLBIG)
	- $(RMDIR) $(OUTPUT_DIR_HTMLCHUNK)
	- $(RMDIR) $(OUTPUT_DIR)

clean: cleanoutput tools_clean
	

$(OUTPUT_DIR_HTMLBIG): $(OUTPUT_DIR)
	- $(RMKDIR) $(OUTPUT_DIR_HTMLBIG)

$(OUTPUT_DIR_HTMLCHUNK): $(OUTPUT_DIR)
	- $(RMKDIR) $(OUTPUT_DIR_HTMLCHUNK)

$(OUTPUT_DIR_HTMLHELP): $(OUTPUT_DIR)
	- $(RMKDIR) $(OUTPUT_DIR_HTMLHELP)

$(OUTPUT_DIR):
	- $(RMKDIR) $(OUTPUT_DIR)

.PHONY: all htmlbig htmlchunk htmlhelp cleanoutput clean

include rules.mk
