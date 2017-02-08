# -*- makefile -*- Time-stamp: <04/03/29 22:25:01 ptr>
# $Id$

ALLOBJS = $(ALLOBJS:../../../test/eh/=)

#
# rules for .cpp --> .o
#

{../../../test/eh}.cpp{$(OUTPUT_DIR)}.o:
	$(COMPILE_cc_REL) $(OUTPUT_OPTION) $<

{../../../test/eh}.cpp{$(OUTPUT_DIR_DBG)}.o:
	$(COMPILE_cc_DBG) $(OUTPUT_OPTION_DBG) $<

{../../../test/eh}.cpp{$(OUTPUT_DIR_STLDBG)}.o:
	$(COMPILE_cc_STLDBG) $(OUTPUT_OPTION_STLDBG) $<

{../../../test/eh}.cpp{$(OUTPUT_DIR_A)}.o:
	$(COMPILE_cc_STATIC_REL) $(OUTPUT_OPTION_STATIC) $<

{../../../test/eh}.cpp{$(OUTPUT_DIR_A_DBG)}.o:
	$(COMPILE_cc_STATIC_DBG) $(OUTPUT_OPTION_STATIC_DBG) $<

{../../../test/eh}.cpp{$(OUTPUT_DIR_A_STLDBG)}.o:
	$(COMPILE_cc_STATIC_STLDBG) $(OUTPUT_OPTION_STATIC_STLDBG) $<

