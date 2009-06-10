CFLAG_WERROR:=-Werror
CFLAG_CRTDLL:=-D_DLL -D__USE_CRTIMP

CXXFLAG_WERROR:=-Werror
CXXFLAG_CRTDLL:=-D_DLL -D__USE_CRTIMP

CPPFLAG_WERROR:=-Werror
CPPFLAG_UNICODE:=-DUNICODE -D_UNICODE

# FIXME: disabled until RosBE stops sucking
# BUILTIN_CPPFLAGS+= -nostdinc
BUILTIN_CFLAGS+= -fno-optimize-sibling-calls
BUILTIN_CXXFLAGS+= -fno-optimize-sibling-calls

#(module, source, dependencies, cflags, output)
define RBUILD_DEPENDS

$(5): $(2) $(3) | ${call RBUILD_dir,$(2)}
	$$(ECHO_DEPENDS)
	$${gcc} -xc -MF $$@ $(4) -M -MP -MT $$@ $$<

endef

#(module, source, dependencies, cflags, output)
define RBUILD_CXX_DEPENDS

$(5): $(2) $(3) | ${call RBUILD_dir,$(2)}
	$$(ECHO_DEPENDS)
	$${gpp} -MF $$@ $(4) -M -MP -MT $$@ $$<

endef

#(module, source, dependencies, cflags, output)
define RBUILD_CPP

$(5): $(2) $(3) | ${call RBUILD_dir,$(2)}
	$$(ECHO_CPP)
	$${gcc} -xc -E $(4) $$< > $$@

endef

#(module, source, dependencies, cflags, output)
define RBUILD_CXX_CPP

$(5): $(2) $(3) | ${call RBUILD_dir,$(2)}
	$$(ECHO_CPP)
	$${gpp} -E $(4) $$< > $$@

endef

#(module, source, dependencies, cflags, output)
define RBUILD_CC

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

${call RBUILD_DEPENDS,$(1),$(2),,${call RBUILD_cflags,$(1),$(4)},$(5).d}
-include $(5).d

$(5): $(2) $(5).d $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${gcc} -o $$@ ${call RBUILD_cflags,$(1),$(4)} -c $$<

else

$(5): $(2) $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${gcc} -o $$@ ${call RBUILD_cflags,$(1),$(4)} -c $$<

endif

endef

#(module, source, dependencies, cflags, output)
define RBUILD_CXX

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

${call RBUILD_CXX_DEPENDS,$(1),$(2),,${call RBUILD_cxxflags,$(1),$(4)},$(5).d}
-include $(5).d

$(5): $(2) $(5).d $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${gpp} -o $$@ ${call RBUILD_cxxflags,$(1),$(4)} -c $$<

else

$(5): $(2) $(3) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${gpp} -o $$@ ${call RBUILD_cxxflags,$(1),$(4)} -c $$<

endif

endef

#(module, source, dependencies, cflags)
RBUILD_CC_RULE=${call RBUILD_CC,$(1),$(2),$(3),$(4),${call RBUILD_intermediate_path_unique,$(1),$(2)}.o}
RBUILD_CXX_RULE=${call RBUILD_CXX,$(1),$(2),$(3),$(4),${call RBUILD_intermediate_path_unique,$(1),$(2)}.o}

#(module, source, dependencies, cflags)
define RBUILD_CC_PCH_RULE

$(2): $${$(1)_precondition}

ifeq ($$(ROS_BUILDDEPS),full)

${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch.d: $(2) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_DEPENDS)
	$${gcc} -MF $$@ ${call RBUILD_cflags,$(1),$(4)} -x c-header -M -MP -MT $$@ $$<

-include $$(intermediate_dir)$$(SEP).gch_$$(module_name)$$(SEP)$(notdir $(2)).gch.d

${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch: $(2) ${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch.d $(3) | ${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)
	$$(ECHO_PCH)
	$${gcc} -MF $$@ ${call RBUILD_cflags,$(1),$(4)} -x c-header -M -MP -MT $$@ $$<

else

${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch: $(2) $(3) | ${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)
	$$(ECHO_PCH)
	$${gcc} -MF $$@ ${call RBUILD_cflags,$(1),$(4)} -x c-header -M -MP -MT $$@ $$<

endif

endef

#(module, source, dependencies, cflags)
define RBUILD_CXX_PCH_RULE

$(2): $${$(1)_precondition}

ifeq ($$(ROS_BUILDDEPS),full)

${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch.d: $(2) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_DEPENDS)
	$${gpp} -MF $$@ ${call RBUILD_cxxflags,$(1),$(4)} -x c++-header -M -MP -MT $$@ $$<

-include $$(intermediate_dir)$$(SEP).gch_$$(module_name)$$(SEP)$(notdir $(2)).gch.d

${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch: $(2) ${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch.d $(3) | ${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)
	$$(ECHO_PCH)
	$${gpp} -MF $$@ ${call RBUILD_cxxflags,$(1),$(4)} -x c++-header -M -MP -MT $$@ $$<

else

${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)$$(SEP)$(notdir $(2)).gch: $(2) $(3) | ${call RBUILD_intermediate_dir,$(2)}$$(SEP).gch_$(1)
	$$(ECHO_PCH)
	$${gpp} -MF $$@ ${call RBUILD_cxxflags,$(1),$(4)} -x c++-header -M -MP -MT $$@ $$<

endif

endef

