#~ #(module, objs, deps, ldflags, output, libs, subsys, entry, base, falign, salign)
#~ define RBUILD_LINK

#~ $(5) ${call RBUILD_output_dir,$(5)}$$(SEP)$(basename $(notdir $(5))).map: $(2) $(3) $(6) | ${call RBUILD_dir,$(5)}
	#~ $$(ECHO_CC)
	#~ $${ld} -subsystem=$(7) -entry=$(8) -base=$(9) -file-alignment=$(10) -section-alignment=$(11) $(2) $(6) ${call RBUILD_ldflags,$(1),$(4)} -o $(5)
#~ ifeq ($(ROS_BUILDMAP),full)
	#~ $$(ECHO_OBJDUMP)
	#~ $${objdump} -d -S $$@ > ${call RBUILD_output_dir,$(5)}$$(SEP)$(basename $(notdir $(5))).map
#~ else
#~ ifeq ($(ROS_BUILDMAP),yes)
	#~ $$(ECHO_NM)
	#~ $${nm} --numeric-sort $$@ > ${call RBUILD_output_dir,$(5)}$$(SEP)$(basename $(notdir $(5))).map
#~ endif
#~ endif
#~ ifeq ($(ROS_BUILDNOSTRIP),yes)
	#~ $$(ECHO_CP)
	#~ $${cp} $(5) $(basename $(5)).nostrip$(suffix $(5)) 1>$(NUL)
#~ endif
#~ ifneq ($(ROS_GENERATE_RSYM),no)
	#~ $$(ECHO_RSYM)
	#~ $$(Q)$$(rsym_TARGET) $$@ $$@
#~ endif
#~ ifeq ($(ROS_LEAN_AND_MEAN),yes)
	#~ $$(ECHO_STRIP)
	#~ $${strip} -s -x -X $$@
#~ endif

#~ endef

#~ #(module, dependencies, ldflags, subsys, entry, base, falign, salign)
#~ RBUILD_LINK_RULE=${call RBUILD_LINK,$(1),$(1)_OBJS,$(2),$(3),$(1)_TARGET,$(1)_LIBS,$(4),$(5),$(6),$(7),$(8)}
