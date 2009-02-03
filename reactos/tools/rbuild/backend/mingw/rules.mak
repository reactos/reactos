RBUILD_fullpath=${subst <<<,,${subst $(SEP)<<<,,${subst /,$(SEP),${subst \\,$(SEP),$(1)}}<<<}}

RBUILD_compress_prefix=${subst >>>,,${subst >>>$($(2)),$$($(2)),>>>$(1)}}
RBUILD_compress_prefixes=${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,${call RBUILD_compress_prefix,$(1),INTERMEDIATE},OUTPUT},CDOUTPUT},TEMPORARY},INSTALL}

RBUILD_strip_prefix=${subst >>>,,${subst >>>$($(2)),,>>>$(1)}}
RBUILD_strip_prefixes=${subst >>>,,${subst >>>$(SEP),,>>>${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,${call RBUILD_strip_prefix,$(1),INTERMEDIATE},OUTPUT},CDOUTPUT},TEMPORARY},INSTALL}}}

#(module, source)
RBUILD_intermediate_path_unique=${call RBUILD_intermediate_dir,$(2)}$(SEP)$(basename $(notdir $(2)))_$(1)

#(source)
RBUILD_intermediate_dir=${call RBUILD_fullpath,$(value INTERMEDIATE)$(SEP)$(dir ${call RBUILD_strip_prefixes,$(1)})}

#(module, source, dependencies, cflags)
define RBUILD_GCC_RULE

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o.d: $(2) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_DEPENDS)
	$${gcc} -MF $$@ $$($(1)_CFLAGS) $(4) -M -MP -MT $$@ $$<

-include ${call RBUILD_intermediate_path_unique,$(1),$(2)}.o.d

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o: $(2) ${call RBUILD_intermediate_path_unique,$(1),$(2)}.o.d $(3) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_CC)
	$${gcc} -o $$@ $$($(1)_CFLAGS) $(4) -c $$<

else

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o: $(2) $(3) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_CC)
	$${gcc} -o $$@ $$($(1)_CFLAGS) $(4) -c $$<

endif

endef

#(module, source, dependencies, cflags)
define RBUILD_GPP_RULE

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o.d: $(2) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_DEPENDS)
	$${gpp} -MF $$@ $$($(1)_CXXFLAGS) $(4) -M -MP -MT $$@ $$<

-include ${call RBUILD_intermediate_path_unique,$(1),$(2)}.o.d

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o: $(2) ${call RBUILD_intermediate_path_unique,$(1),$(2)}.o.d $(3) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_CC)
	$${gpp} -o $$@ $$($(1)_CXXFLAGS) $(4) -c $$<

else

${call RBUILD_intermediate_path_unique,$(1),$(2)}.o: $(2) $(3) | ${call RBUILD_intermediate_dir,$(2)}
	$$(ECHO_CC)
	$${gpp} -o $$@ $$($(1)_CXXFLAGS) $(4) -c $$<

endif

endef

# EOF
