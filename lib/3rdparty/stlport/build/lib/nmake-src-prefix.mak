# -*- makefile -*- Time-stamp: <03/10/26 15:42:12 ptr>
# $Id$

ALLOBJS = $(ALLOBJS:../../src/=)
ALLRESS = $(ALLRESS:../../src/=)

#
# rules for .cpp --> .o
#

{../../src}.cpp{$(OUTPUT_DIR)}.o:
	$(COMPILE_cc_REL) $(OUTPUT_OPTION) $<

{../../src}.cpp{$(OUTPUT_DIR_DBG)}.o:
	$(COMPILE_cc_DBG) $(OUTPUT_OPTION_DBG) $<

{../../src}.cpp{$(OUTPUT_DIR_STLDBG)}.o:
	$(COMPILE_cc_STLDBG) $(OUTPUT_OPTION_STLDBG) $<

{../../src}.cpp{$(OUTPUT_DIR_A)}.o:
	$(COMPILE_cc_STATIC_REL) $(OUTPUT_OPTION_STATIC) $<

{../../src}.cpp{$(OUTPUT_DIR_A_DBG)}.o:
	$(COMPILE_cc_STATIC_DBG) $(OUTPUT_OPTION_STATIC_DBG) $<

{../../src}.cpp{$(OUTPUT_DIR_A_STLDBG)}.o:
	$(COMPILE_cc_STATIC_STLDBG) $(OUTPUT_OPTION_STATIC_STLDBG) $<

#
# rules for .c --> .o
#

{../../src}.c{$(OUTPUT_DIR)}.o:
	$(COMPILE_c_REL) $(OUTPUT_OPTION) $<

{../../src}.c{$(OUTPUT_DIR_DBG)}.o:
	$(COMPILE_c_DBG) $(OUTPUT_OPTION_DBG) $<

{../../src}.c{$(OUTPUT_DIR_STLDBG)}.o:
	$(COMPILE_c_STLDBG) $(OUTPUT_OPTION_STLDBG) $<

{../../src}.c{$(OUTPUT_DIR_A)}.o:
	$(COMPILE_c_STATIC_REL) $(OUTPUT_OPTION_STATIC) $<

{../../src}.c{$(OUTPUT_DIR_A_DBG)}.o:
	$(COMPILE_c_STATIC_DBG) $(OUTPUT_OPTION_STATIC_DBG) $<

{../../src}.c{$(OUTPUT_DIR_A_STLDBG)}.o:
	$(COMPILE_c_STATIC_STLDBG) $(OUTPUT_OPTION_STATIC_STLDBG) $<

#
# rules for .rc --> .res
#

{../../src}.rc{$(OUTPUT_DIR)}.res:
	$(COMPILE_rc_REL) $(RC_OUTPUT_OPTION) $<

{../../src}.rc{$(OUTPUT_DIR_DBG)}.res:
	$(COMPILE_rc_DBG) $(RC_OUTPUT_OPTION_DBG) $<

{../../src}.rc{$(OUTPUT_DIR_STLDBG)}.res:
	$(COMPILE_rc_STLDBG) $(RC_OUTPUT_OPTION_STLDBG) $<

{../../src}.rc{$(OUTPUT_DIR_A)}.res:
	$(COMPILE_rc_STATIC_REL) $(RC_OUTPUT_OPTION) $<

{../../src}.rc{$(OUTPUT_DIR_A_DBG)}.res:
	$(COMPILE_rc_STATIC_DBG) $(RC_OUTPUT_OPTION_DBG) $<

{../../src}.rc{$(OUTPUT_DIR_A_STLDBG)}.res:
	$(COMPILE_rc_STATIC_STLDBG) $(RC_OUTPUT_OPTION_STLDBG) $<

