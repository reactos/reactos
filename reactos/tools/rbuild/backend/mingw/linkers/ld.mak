# -exclude-all-symbols disables autoexporting all symbols *if none were found* (either in a DEF file or using __declspec(dllexport)
LDFLAG_DLL:=-shared -exclude-all-symbols
LDFLAG_DRIVER:=-shared --subsystem=native -exclude-all-symbols
LDFLAG_NOSTDLIB:=-nostartfiles -nostdlib
LDFLAG_CONSOLE:=--subsystem=console
LDFLAG_WINDOWS:=--subsystem=windows
LDFLAG_NATIVE:=--subsystem=native

LDFLAG_EXCLUDE_ALL_SYMBOLS=-exclude-all-symbols
DLLTOOL_FLAGS=--kill-at
ifeq ($(ARCH),amd64)
    DLLTOOL_FLAGS= --no-leading-underscore
endif

#~ #(module, objs, deps, ldflags, output, def, libs, entry, base)
#(module, objs, deps, ldflags, output, def, libs, entry, base, extralibs)
define RBUILD_LINK

ifneq ($(6),)
${call RBUILD_intermediate_dir,$(5)}$$(SEP)lib${call RBUILD_name,$(5)}.a: $(6) | ${call RBUILD_intermediate_path,$(5)}
	$$(ECHO_IMPLIB)
	$${dlltool} --def $(6) $(DLLTOOL_FLAGS) --output-lib=$$@

${call RBUILD_intermediate_dir,$(5)}$$(SEP)lib${call RBUILD_name,$(5)}.delayimp.a: $(6) | ${call RBUILD_intermediate_path,$(5)}
	$$(ECHO_IMPLIB)
	$${dlltool} --def $(6) $(DLLTOOL_FLAGS) --output-delaylib=$$@

${call RBUILD_intermediate_path_noext,$(5)}.exp: $(6) | ${call RBUILD_intermediate_path,$(5)}
	$$(ECHO_IMPLIB)
	$${dlltool} --def $(6) $(DLLTOOL_FLAGS) --output-exp=$$@

$(1)_CLEANFILES+=\
	${call RBUILD_intermediate_dir,$(5)}$$(SEP)lib$(notdir $(5)).a \
	${call RBUILD_intermediate_dir,$(5)}$$(SEP)lib$(notdir $(5)).delayimp.a \
	${call RBUILD_intermediate_path_noext,$(5)}.exp
endif

# TODO: refactor this out of here and into rules.mak
${call RBUILD_intermediate_dir,$(5)}$$(SEP)$(1)_objs.rsp: $(2) $$(if $(6),${call RBUILD_intermediate_path_noext,$(5)}.exp) $(3) | ${call RBUILD_intermediate_dir,$(5)}
	$$(ECHO_RSP)
	-@$${rm} $$@ 2>$$(NUL)
	$${cp} $$(NUL) $$@ >$$(NUL)
	$$(foreach obj,$(2) $$(if $(6),${call RBUILD_intermediate_path_noext,$(5)}.exp),$$(Q)echo $$(QUOTE)$$(subst \,\\,$$(obj))$$(QUOTE)>>$$@$$(NL))

$(1)_CLEANFILES+=${call RBUILD_intermediate_dir,$(5)}$$(SEP)$(1)_objs.rsp

$(5): ${call RBUILD_intermediate_dir,$(5)}$$(SEP)$(1)_objs.rsp $(7) $(3) $$(RSYM_TARGET) $$(PEFIXUP_TARGET) | ${call RBUILD_dir,$(5)}
	$$(ECHO_LD)
#~	$${ld} --entry=$(8) --image-base=$(9) @${call RBUILD_intermediate_dir,$(5)}$$(SEP)$(1)_objs.rsp $(7) ${call RBUILD_ldflags,$(1),$(4)} -o $$@
	$${ld} --entry=$(8) --image-base=$(9) @${call RBUILD_intermediate_dir,$(5)}$$(SEP)$(1)_objs.rsp --start-group $(10) $(7) --end-group ${call RBUILD_ldflags,$(1),$(4)} -o $$@
ifneq ($(or $(6),$$(MODULETYPE$$($(1)_TYPE)_KMODE)),)
	$$(ECHO_PEFIXUP)
	$$(Q)$$(PEFIXUP_TARGET) $$@ $(if $(6),-exports) $$(if $$(MODULETYPE$($(1)_TYPE)_KMODE),-sections)
endif
ifeq ($(ROS_BUILDMAP),full)
	$$(ECHO_OBJDUMP)
	$${objdump} -d -S $$@ > ${call RBUILD_output_path_noext,$(5)}.map
else
ifeq ($(ROS_BUILDMAP),yes)
	$$(ECHO_NM)
	$${nm} --numeric-sort $$@ > ${call RBUILD_output_path_noext,$(5)}.map
endif
endif
ifeq ($(ROS_BUILDNOSTRIP),yes)
	$$(ECHO_CP)
	$${cp} $(5) $(basename $(5)).nostrip$(suffix $(5)) 1>$(NUL)
endif
ifneq ($(ROS_GENERATE_RSYM),no)
	$$(ECHO_RSYM)
	$$(Q)$$(RSYM_TARGET) $$@ $$@
endif
ifeq ($(ROS_LEAN_AND_MEAN),yes)
	$$(ECHO_STRIP)
	$${strip} -s -x -X $$@
endif

ifneq ($(ROS_BUILDMAP),)
$(1)_CLEANFILES+=${call RBUILD_output_path_noext,$(5)}.map
endif

ifeq ($(ROS_BUILDNOSTRIP),yes)
$(1)_CLEANFILES+=$(basename $(5)).nostrip$(suffix $(5))
endif

endef

#~ #(module, def, deps, ldflags, libs, entry, base)
#~ RBUILD_LINK_RULE=${call RBUILD_LINK,$(1),$(value $(1)_OBJS),$(3),$(4),$(value $(1)_TARGET),$(2),$(5) $(value $(1)_LIBS) $(5),$(6),$(7)}
#(module, def, deps, ldflags, libs, entry, base, extralibs)
RBUILD_LINK_RULE=${call RBUILD_LINK,$(1),$(value $(1)_OBJS),$(3),$(4),$(value $(1)_TARGET),$(2),$(value $(1)_LIBS),$(6),$(7),$(5)}