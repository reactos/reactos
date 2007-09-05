WINEBUILD_BASE = $(TOOLS_BASE)$(SEP)winebuild
WINEBUILD_BASE_ = $(WINEBUILD_BASE)$(SEP)
WINEBUILD_INT = $(INTERMEDIATE_)$(WINEBUILD_BASE)
WINEBUILD_INT_ = $(WINEBUILD_INT)$(SEP)
WINEBUILD_OUT = $(OUTPUT_)$(WINEBUILD_BASE)
WINEBUILD_OUT_ = $(WINEBUILD_OUT)$(SEP)

$(WINEBUILD_INT): | $(TOOLS_INT)
	$(ECHO_MKDIR)
	${mkdir} $@

ifneq ($(INTERMEDIATE),$(OUTPUT))
$(WINEBUILD_OUT): | $(TOOLS_OUT)
	$(ECHO_MKDIR)
	${mkdir} $@
endif

WINEBUILD_TARGET = \
	$(EXEPREFIX)$(WINEBUILD_OUT_)winebuild$(EXEPOSTFIX)

WINEBUILD_DEPENDS = $(BUILDNO_H)

WINEBUILD_SOURCES = $(addprefix $(WINEBUILD_BASE_), \
	import.c \
	main.c \
	parser.c \
	res16.c \
	res32.c \
	spec32.c \
	utils.c \
	mkstemps.c \
	)

WINEBUILD_OBJECTS = \
  $(addprefix $(INTERMEDIATE_), $(WINEBUILD_SOURCES:.c=.o))

WINEBUILD_HOST_CFLAGS = $(TOOLS_CFLAGS) -D__USE_W32API \
                        -Iinclude/reactos/wine -Iinclude -Iinclude/reactos -Iinclude/psdk \
                        -I$(INTERMEDIATE_)include

WINEBUILD_HOST_LFLAGS = $(TOOLS_LFLAGS)

.PHONY: winebuild
winebuild: $(WINEBUILD_TARGET)

$(WINEBUILD_TARGET): $(WINEBUILD_OBJECTS) | $(WINEBUILD_OUT)
	$(ECHO_LD)
	${host_gcc} $(WINEBUILD_OBJECTS) $(WINEBUILD_HOST_LFLAGS) -o $@

$(WINEBUILD_INT_)import.o: $(WINEBUILD_BASE_)import.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)main.o: $(WINEBUILD_BASE_)main.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)parser.o: $(WINEBUILD_BASE_)parser.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)res16.o: $(WINEBUILD_BASE_)res16.c $(WINEBUILD_DEPENDS | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)res32.o: $(WINEBUILD_BASE_)res32.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)spec32.o: $(WINEBUILD_BASE_)spec32.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)utils.o: $(WINEBUILD_BASE_)utils.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_INT_)mkstemps.o: $(WINEBUILD_BASE_)mkstemps.c $(WINEBUILD_DEPENDS) | $(WINEBUILD_INT)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

.PHONY: winebuild_clean
winebuild_clean:
	-@$(rm) $(WINEBUILD_TARGET) $(WINEBUILD_OBJECTS) 2>$(NUL)
clean: winebuild_clean
