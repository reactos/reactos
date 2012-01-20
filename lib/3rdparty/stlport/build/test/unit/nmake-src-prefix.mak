# -*- makefile -*- Time-stamp: <04/03/29 22:25:01 ptr>
# $Id$

ALLOBJS = $(ALLOBJS:../../../test/unit/=)
ALLOBJS = $(ALLOBJS:cppunit/=)

#
# rules for .cpp --> .o
#

{../../../test/unit}.cpp{$(OUTPUT_DIR)}.o:
	$(COMPILE_cc_REL) $(OUTPUT_OPTION) $<

{../../../test/unit}.cpp{$(OUTPUT_DIR_DBG)}.o:
	$(COMPILE_cc_DBG) $(OUTPUT_OPTION_DBG) $<

{../../../test/unit}.cpp{$(OUTPUT_DIR_STLDBG)}.o:
	$(COMPILE_cc_STLDBG) $(OUTPUT_OPTION_STLDBG) $<

{../../../test/unit}.cpp{$(OUTPUT_DIR_A)}.o:
	$(COMPILE_cc_STATIC_REL) $(OUTPUT_OPTION_STATIC) $<

{../../../test/unit}.cpp{$(OUTPUT_DIR_A_DBG)}.o:
	$(COMPILE_cc_STATIC_DBG) $(OUTPUT_OPTION_STATIC_DBG) $<

{../../../test/unit}.cpp{$(OUTPUT_DIR_A_STLDBG)}.o:
	$(COMPILE_cc_STATIC_STLDBG) $(OUTPUT_OPTION_STATIC_STLDBG) $<

#
# rules for .c --> .o
#

{../../../test/unit}.c{$(OUTPUT_DIR)}.o:
	$(COMPILE_c_REL) $(OUTPUT_OPTION) $<

{../../../test/unit}.c{$(OUTPUT_DIR_DBG)}.o:
	$(COMPILE_c_DBG) $(OUTPUT_OPTION_DBG) $<

{../../../test/unit}.c{$(OUTPUT_DIR_STLDBG)}.o:
	$(COMPILE_c_STLDBG) $(OUTPUT_OPTION_STLDBG) $<

{../../../test/unit}.c{$(OUTPUT_DIR_A)}.o:
	$(COMPILE_c_STATIC_REL) $(OUTPUT_OPTION_STATIC) $<

{../../../test/unit}.c{$(OUTPUT_DIR_A_DBG)}.o:
	$(COMPILE_c_STATIC_DBG) $(OUTPUT_OPTION_STATIC_DBG) $<

{../../../test/unit}.c{$(OUTPUT_DIR_A_STLDBG)}.o:
	$(COMPILE_c_STATIC_STLDBG) $(OUTPUT_OPTION_STATIC_STLDBG) $<

#
# rules for cppunit/.cpp --> .o
#

{../../../test/unit/cppunit}.cpp{$(OUTPUT_DIR)}.o:
	$(COMPILE_cc_REL) $(OUTPUT_OPTION) $<

{../../../test/unit/cppunit/}.cpp{$(OUTPUT_DIR_DBG)}.o:
	$(COMPILE_cc_DBG) $(OUTPUT_OPTION_DBG) $<

{../../../test/unit/cppunit}.cpp{$(OUTPUT_DIR_STLDBG)}.o:
	$(COMPILE_cc_STLDBG) $(OUTPUT_OPTION_STLDBG) $<

{../../../test/unit/cppunit}.cpp{$(OUTPUT_DIR_A)}.o:
	$(COMPILE_cc_STATIC_REL) $(OUTPUT_OPTION_STATIC) $<

{../../../test/unit/cppunit}.cpp{$(OUTPUT_DIR_A_DBG)}.o:
	$(COMPILE_cc_STATIC_DBG) $(OUTPUT_OPTION_STATIC_DBG) $<

{../../../test/unit/cppunit}.cpp{$(OUTPUT_DIR_A_STLDBG)}.o:
	$(COMPILE_cc_STATIC_STLDBG) $(OUTPUT_OPTION_STATIC_STLDBG) $<
