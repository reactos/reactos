CFLAGS+= $(_CL_)
_CL_=

CINCLUDES:=$(INCLUDE) $(CINCLUDES)
INCLUDE=

CFLAG_WERROR:=/WX
CFLAG_CRTDLL:=/D_DLL /D__USE_CRTIMP /MD

CXXFLAG_WERROR:=/WX
CXXFLAG_CRTDLL:=/D_DLL /D__USE_CRTIMP /MD

CPPFLAG_WERROR:=/WX
CPPFLAG_UNICODE:=/DUNICODE /D_UNICODE

BUILTIN_CPPFLAGS+= /X

cl=$$(Q)$$(RBUILD_HELPER_TARGET) "RBUILD_CL_" "$(notdir $<<)" cl /nologo

#(module, source, dependencies, cflags, output)
#TODO
RBUILD_CL_DEPENDS=$$(error Full dependencies are not implemented for Microsoft C/C++ Compiler yet)
RBUILD_DEPENDS=${call RBUILD_CL_DEPENDS,$(1),$(2),$(3),$(4) /TC,$(5)}
RBUILD_CXX_DEPENDS=${call RBUILD_CL_DEPENDS,$(1),$(2),$(3),$(4) /TP,$(5)}

#(module, source, dependencies, cflags, output)
define RBUILD_CL_CPP

$(5): $(2) $(3) $$(RBUILD_HELPER_TARGET) | ${call RBUILD_dir,$(2)}
	$$(ECHO_CPP)
	$${cl} /E $(4) $$< > $$@

endef

RBUILD_CPP=${call RBUILD_CL_CPP,$(1),$(2),$(3),$(4) /TC,$(5)}
RBUILD_CXX_CPP=${call RBUILD_CL_CPP,$(1),$(2),$(3),$(4) /TPP,$(5)}

#(module, source, dependencies, cflags, output)
define RBUILD_CC

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

${call RBUILD_DEPENDS,$(1),$(2),,${call RBUILD_cflags,$(1),$(4)},$(5).d}
-include $(5).d

$(5): $(2) $(5).d $(3) $$(RBUILD_HELPER_TARGET) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${cl} /TC /Fo$$@ ${call RBUILD_cflags,$(1),$(4)} /c $$<

else

$(5): $(2) $(3) $$(RBUILD_HELPER_TARGET) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${cl} /TC /Fo$$@ ${call RBUILD_cflags,$(1),$(4)} /c $$<

endif

endef

#(module, source, dependencies, cflags, output)
define RBUILD_CXX

$(2): $${$(1)_precondition}

ifeq ($(ROS_BUILDDEPS),full)

${call RBUILD_CXX_DEPENDS,$(1),$(2),,${call RBUILD_cflags,$(1),$(4)},$(5).d}
-include $(5).d

$(5): $(2) $(5).d $(3) $$(RBUILD_HELPER_TARGET) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${cl} /TP /Fo$$@ ${call RBUILD_cxxflags,$(1),$(4)} /c $$<

else

$(5): $(2) $(3) $$(RBUILD_HELPER_TARGET) | ${call RBUILD_dir,$(5)}
	$$(ECHO_CC)
	$${cl} /TP /Fo$$@ ${call RBUILD_cxxflags,$(1),$(4)} /c $$<

endef

#(module, source, dependencies, cflags)
RBUILD_CC_RULE=${call RBUILD_CC,$(1),$(2),$(3),$(4),${call RBUILD_intermediate_path_unique,$(1),$(2)}.o}
RBUILD_CXX_RULE=${call RBUILD_CXX,$(1),$(2),$(3),$(4),${call RBUILD_intermediate_path_unique,$(1),$(2)}.o}

#(module, source, dependencies, cflags)
#TODO
RBUILD_CC_PCH_RULE=$$(error Precompiled headers are not implemented for Microsoft C/C++ Compiler yet)

#(module, source, dependencies, cflags)
#TODO
RBUILD_CXX_PCH_RULE=$$(error Precompiled headers are not implemented for Microsoft C/C++ Compiler yet)
