# src/mesa/shader/slang/library/Makefile

TOP = ../../../../..

include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

#
# targets
#

.PHONY: default clean

default: syntax builtin

clean:
	-rm -f syn_to_c gc_to_bin *_syn.h *_gc.h

syntax: slang_pp_directives_syn.h slang_pp_expression_syn.h slang_shader_syn.h slang_pp_version_syn.h

builtin: builtin_110 builtin_120

#
# executables
#

syn_to_c: syn_to_c.c
	$(CC) syn_to_c.c -o syn_to_c

gc_to_bin: gc_to_bin.c slang_shader_syn.h
	$(CC) gc_to_bin.c -o gc_to_bin

#
# syntax scripts
#

slang_pp_directives_syn.h: syn_to_c slang_pp_directives.syn
	./syn_to_c slang_pp_directives.syn > slang_pp_directives_syn.h

slang_pp_expression_syn.h: syn_to_c slang_pp_expression.syn
	./syn_to_c slang_pp_expression.syn > slang_pp_expression_syn.h

slang_shader_syn.h: syn_to_c slang_shader.syn
	./syn_to_c slang_shader.syn > slang_shader_syn.h

slang_pp_version_syn.h: syn_to_c slang_pp_version.syn
	./syn_to_c slang_pp_version.syn > slang_pp_version_syn.h

#
# builtin library sources
#

builtin_110: slang_common_builtin_gc.h slang_core_gc.h slang_fragment_builtin_gc.h slang_vertex_builtin_gc.h

builtin_120: slang_120_core_gc.h slang_builtin_120_common_gc.h slang_builtin_120_fragment_gc.h


slang_120_core_gc.h: gc_to_bin slang_120_core.gc
	./gc_to_bin 1 slang_120_core.gc slang_120_core_gc.h

slang_builtin_120_common_gc.h: gc_to_bin slang_builtin_120_common.gc
	./gc_to_bin 1 slang_builtin_120_common.gc slang_builtin_120_common_gc.h

slang_builtin_120_fragment_gc.h: gc_to_bin slang_builtin_120_fragment.gc
	./gc_to_bin 1 slang_builtin_120_fragment.gc slang_builtin_120_fragment_gc.h

slang_common_builtin_gc.h: gc_to_bin slang_common_builtin.gc
	./gc_to_bin 1 slang_common_builtin.gc slang_common_builtin_gc.h

slang_core_gc.h: gc_to_bin slang_core.gc
	./gc_to_bin 1 slang_core.gc slang_core_gc.h

slang_fragment_builtin_gc.h: gc_to_bin slang_fragment_builtin.gc
	./gc_to_bin 1 slang_fragment_builtin.gc slang_fragment_builtin_gc.h

slang_vertex_builtin_gc.h: gc_to_bin slang_vertex_builtin.gc
	./gc_to_bin 2 slang_vertex_builtin.gc slang_vertex_builtin_gc.h

